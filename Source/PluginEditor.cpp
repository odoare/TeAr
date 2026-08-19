#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    /** std::vector indexes with an unsigned type, while every index in this file
        is an int that a caller has already bounds-checked. Converting in one
        named place keeps the call sites readable and keeps the build free of
        the sign-conversion warnings that used to drown out everything else.

        The assertion is free in a release build and catches a negative index in
        a debug one, which a bare (size_t) cast at each site would not. */
    inline std::size_t asIndex (int i) noexcept
    {
        jassert (i >= 0);
        return (std::size_t) i;
    }
}

//==============================================================================
TeArAudioProcessorEditor::TeArAudioProcessorEditor (TeArAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    audioProcessor.addChangeListener (this);

    auto& apvts = audioProcessor.getAPVTS();
    const auto neutral = juce::Colours::white;

    // A drop-down is a window of its own and never sees the combo that opened
    // it, so the menu takes its highlight from the look-and-feel rather than
    // from the box. One call here tints every menu this editor opens; without
    // it they stay neutral grey against a cyan panel.
    fxmeLAF.setAccentColour (Theme::accent);

    // A TooltipWindow parented to nullptr lives on the desktop, so it is drawn
    // by the default look-and-feel rather than by this editor's unless it is
    // pointed at ours explicitly.
    tooltipWindow.setLookAndFeel (&fxmeLAF);

    // --- FX-Mechanics top bar: logo, name, preset strip, preset browser toggle ---
    addAndMakeVisible (topBar);
    topBar.setAccentColour (Theme::accent);
    topBar.setBackgroundColour (Theme::background);

    // Centred in whatever space is left between the blurb and the preset
    // controls. The bar gives the blurb priority and shrinks the artwork into
    // the remaining gap, so it never collides with either.
    topBar.setDecoration (juce::ImageCache::getFromMemory (BinaryData::icon_png,
                                                           BinaryData::icon_pngSize));

    // Clicking either the company logo or the icon reopens the splash.
    topBar.onLogoClicked = [this] { splash.show(); };

    // Hidden until shown; show() brings it to the front itself, so it does not
    // matter that later children are added above it.
    splash.setImage (juce::ImageCache::getFromMemory (BinaryData::Splash_png,
                                                      BinaryData::Splash_pngSize));
    addChildComponent (splash);

    presetBar.setAccentColour (Theme::accent);

    presetsButton.setTooltip ("Browse presets");
    presetsButton.setMouseClickGrabsKeyboardFocus (false);
    presetsButton.setColour (juce::TextButton::buttonColourId,
                             juce::Colours::black.withAlpha (0.25f));
    presetsButton.setColour (juce::TextButton::textColourOffId,
                             Theme::accent.brighter (0.3f));
    presetsButton.onClick = [this] {
        setPresetPanelVisible (! presetOverlay.isVisible());
    };

    topBar.setRightControls (&presetBar, Theme::presetBarWidth,
                             &presetsButton, Theme::presetToggleWidth);

    addChildComponent (presetOverlay);
    presetOverlay.browser.setAccentColour (Theme::accent);

    // --- Toolbar buttons ---
    addAndMakeVisible (addArpButton);
    styleMomentaryButton (addArpButton, Theme::accent);
    addArpButton.setButtonText ("+");
    addArpButton.setTooltip ("Add an arpeggiator");
    addArpButton.onClick = [this] {
        audioProcessor.addArpeggiator();
        selectedArpIndex = audioProcessor.getNumArpeggiators() - 1;
        rebuildArpUI();
    };

    addAndMakeVisible (removeArpButton);
    styleMomentaryButton (removeArpButton, Theme::accent);
    removeArpButton.setButtonText ("-");
    removeArpButton.setTooltip ("Remove the selected arpeggiator");
    removeArpButton.onClick = [this] {
        if (audioProcessor.getNumArpeggiators() <= 1) return;
        audioProcessor.removeArpeggiator (selectedArpIndex);
        selectedArpIndex = juce::jlimit (0, audioProcessor.getNumArpeggiators() - 1, selectedArpIndex);
        rebuildArpUI();
    };

    addAndMakeVisible (patternGenButton);
    styleMomentaryButton (patternGenButton, Theme::accent);
    patternGenButton.setButtonText ("?");
    patternGenButton.setTooltip ("Generate a pattern for the selected arpeggiator");
    patternGenButton.onClick = [this] {
        if (!juce::isPositiveAndBelow (selectedArpIndex, (int) arpComponents.size())) return;

        auto colour = getArpColour (selectedArpIndex);

        // The two generators are static on fxme::Arpeggiator and read no
        // engine state, so these lambdas capture nothing. They previously held
        // a reference to arps[selectedArpIndex].engine, obtained without the
        // lock and then stored in the popup, which outlives this click: a
        // preset or session load clearing `arps` would have left it dangling.
        auto makeEuclidian = [] (int hits, int steps, int rotation) {
            return fxme::Arpeggiator::makeEuclidianPattern (hits, steps, rotation);
        };
        auto makeRandom = [] () {
            return fxme::Arpeggiator::makeRandomPattern();
        };
        auto onOk = [this] (juce::String pattern) {
            audioProcessor.setArpeggiatorPattern (selectedArpIndex, pattern);
        };

        auto* content = new ArpPatternPopup (makeEuclidian, makeRandom, onOk, colour);
        juce::CallOutBox::launchAsynchronously (
            std::unique_ptr<juce::Component> (content),
            patternGenButton.getScreenBounds(), this);
    };

    // --- Help ---
    addAndMakeVisible (infoButton);
    infoButton.setTooltip ("Pattern language and shortcuts");
    infoButton.setColours ({ Theme::accent,                       // round button fill
                             Theme::text,                         // glyph and body text
                             Theme::background,                   // callout panel
                             Theme::accent.withAlpha (0.35f) });  // panel hairline
    infoButton.setInfo ("TeAr pattern language",
        "Each step is one command. Spaces are ignored, so \"1 2 3\" and \"123\"\n"
        "are the same pattern.\n"
        "\n"
        "NOTES   degrees are uppercase hex, 1-F = 1st to 15th\n"
        "  1-F   play that degree of the chord or scale\n"
        "  0     play the current degree, used to close a modifier as in vF0\n"
        "  _     sustain the previous note\n"
        "  .     rest\n"
        "  +     next degree            -   previous degree\n"
        "  ?     random note            =   repeat the last degree\n"
        "\n"
        "PITCH   prefix, this step only\n"
        "  #     up a semitone          b   down a semitone\n"
        "\n"
        "VELOCITY   level 1-F over the MIDI range: 1 = 8, 8 = 68, F = 127\n"
        "  vN v+ v- v?    this note only\n"
        "  VN V+ V- V?    from here on, until the next V\n"
        "  The relative forms saturate rather than wrap.\n"
        "\n"
        "OCTAVE   N is 0-7\n"
        "  oN o+ o-       this note only\n"
        "  ON O+ O-       from here on\n"
        "\n"
        "PROBABILITY   N is 1-9, the step plays N x 10% of the time\n"
        "  pN X           X on success, otherwise a rest\n"
        "  pN X:Y         Y plays instead on failure\n"
        "  pN (a b):(c d) gates a whole group, fallback group optional\n"
        "\n"
        "BLOCKS\n"
        "  ( ... )        octave and velocity are restored at the closing bracket\n"
        "  \" ... \"        Scale mode only: anchor these steps to the Scale Root\n"
        "                 rather than to the note being played\n"
        "\n"
        "EDITOR\n"
        "  Return         commit the pattern\n"
        "  Shift+Return   new line inside the pattern\n"
        "  +  -           add or remove an arpeggiator\n"
        "  ?              generate a Euclidean or random pattern\n"
        "  a tab number   select it; click it again to switch it on or off\n"
        "\n"
        "The full reference is in the readme, and typeset in\n"
        "doc/TeAr-language-reference.pdf.");

    // --- Global controls ---
    addAndMakeVisible (chordMethodLabel);
    chordMethodLabel.setText ("Method", juce::dontSendNotification);
    chordMethodLabel.attachToComponent (&chordMethodBox, true);
    chordMethodLabel.setLookAndFeel (&arpLAF);
    chordMethodLabel.setColour (juce::Label::textColourId, neutral);

    addAndMakeVisible (chordMethodBox);
    chordMethodBox.setLookAndFeel (&fxmeLAF);
    chordMethodBox.setColour (juce::ComboBox::textColourId,    neutral);
    chordMethodBox.setColour (juce::ComboBox::outlineColourId, neutral);
    chordMethodBox.setColour (juce::ComboBox::arrowColourId,   neutral);
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("chordMethod")))
        { chordMethodBox.clear(); chordMethodBox.addItemList (param->choices, 1); }
    chordMethodAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, "chordMethod", chordMethodBox);
    chordMethodBox.onChange = [this] { updateScaleDisplay(); };

    addAndMakeVisible (scaleRootLabel);
    scaleRootLabel.setText ("Scale Root", juce::dontSendNotification);
    scaleRootLabel.attachToComponent (&scaleRootBox, true);
    scaleRootLabel.setLookAndFeel (&arpLAF);
    scaleRootLabel.setColour (juce::Label::textColourId, neutral);

    addAndMakeVisible (scaleRootBox);
    scaleRootBox.setLookAndFeel (&fxmeLAF);
    scaleRootBox.setColour (juce::ComboBox::textColourId,    neutral);
    scaleRootBox.setColour (juce::ComboBox::outlineColourId, neutral);
    scaleRootBox.setColour (juce::ComboBox::arrowColourId,   neutral);
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("scaleRoot")))
        { scaleRootBox.clear(); scaleRootBox.addItemList (param->choices, 1); }
    scaleRootAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, "scaleRoot", scaleRootBox);
    scaleRootBox.onChange = [this] { updateScaleDisplay(); };

    addAndMakeVisible (scaleTypeLabel);
    scaleTypeLabel.setText ("Scale Type", juce::dontSendNotification);
    scaleTypeLabel.attachToComponent (&scaleTypeBox, true);
    scaleTypeLabel.setLookAndFeel (&arpLAF);
    scaleTypeLabel.setColour (juce::Label::textColourId, neutral);

    addAndMakeVisible (scaleTypeBox);
    scaleTypeBox.setLookAndFeel (&fxmeLAF);
    scaleTypeBox.setColour (juce::ComboBox::textColourId,    neutral);
    scaleTypeBox.setColour (juce::ComboBox::outlineColourId, neutral);
    scaleTypeBox.setColour (juce::ComboBox::arrowColourId,   neutral);
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("scaleType")))
        { scaleTypeBox.clear(); scaleTypeBox.addItemList (param->choices, 1); }
    scaleTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, "scaleType", scaleTypeBox);
    scaleTypeBox.onChange = [this] { updateScaleDisplay(); };

    addAndMakeVisible (followMidiInButton);
    // fxme::FxmeButton is a plain Component, so it is not a TooltipClient; the
    // inner ToggleButton it exposes is, and it fills the whole bounds, so the
    // tooltip is found under the mouse either way.
    followMidiInButton.button.setTooltip ("Take the scale root from the note being played");
    // Forwards to the inner ToggleButton, so it picks up the house tick and the
    // hover and disabled states that go with it.
    followMidiInButton.setLookAndFeel (&fxmeLAF);

    addAndMakeVisible (keyboardComponent);

    // Last of the permanent children, so the glow stacks over the header and
    // the main UI alike; the preset overlay brings itself to the front.
    addAndMakeVisible (glowLine);

    rebuildArpUI();

    startTimerHz (60);
    updateScaleDisplay();
    updateControlEnablement();

    setSize (820, 480 + Theme::topBarHeight + 6);

    // First window of this run only: reopening the editor must not replay the
    // splash, which is why the flag lives on the processor.
    if (audioProcessor.claimSplash())
        splash.show();
}

void TeArAudioProcessorEditor::styleMomentaryButton (fxme::AccentToggle& b, juce::Colour accent)
{
    // AccentToggle latches by default; the toolbar buttons are push buttons.
    b.setClickingTogglesState (false);
    b.setAccent (accent, accent.brighter (0.4f), Theme::buttonBody);
}

void TeArAudioProcessorEditor::setPresetPanelVisible (bool shouldBeVisible)
{
    presetOverlay.setVisible (shouldBeVisible);
    presetsButton.setButtonText (juce::String::fromUTF8 (shouldBeVisible
                                                             ? "\xe2\x96\xb4"    // up triangle
                                                             : "\xe2\x96\xbe")); // down triangle
    if (shouldBeVisible)
        presetOverlay.toFront (false);
    glowLine.toFront (false);   // the line belongs to the header, above both
    resized();
}

TeArAudioProcessorEditor::~TeArAudioProcessorEditor()
{
    tooltipWindow.setLookAndFeel (nullptr);
    followMidiInButton.setLookAndFeel (nullptr);
    chordMethodLabel.setLookAndFeel (nullptr);
    chordMethodBox.setLookAndFeel (nullptr);
    scaleRootLabel.setLookAndFeel (nullptr);
    scaleRootBox.setLookAndFeel (nullptr);
    scaleTypeLabel.setLookAndFeel (nullptr);
    scaleTypeBox.setLookAndFeel (nullptr);
    audioProcessor.removeChangeListener (this);
    stopTimer();
}

//==============================================================================
void TeArAudioProcessorEditor::rebuildArpUI()
{
    // Remove old per-arp widgets
    for (auto& btn  : tabButtons)    removeChildComponent (btn.get());
    for (auto& comp : arpComponents) removeChildComponent (comp.get());
    tabButtons.clear();
    arpComponents.clear();
    lastStepIndices.clear();
    lastNoteOnCounts.clear();
    blinkLevels.clear();

    int numArps = audioProcessor.getNumArpeggiators();
    if (numArps == 0) { resized(); return; }
    selectedArpIndex = juce::jlimit (0, numArps - 1, selectedArpIndex);

    for (int i = 0; i < numArps; ++i)
    {
        // Selection button: selects, and clicking the one already selected
        // toggles that arpeggiator on or off — the same shortcut as before,
        // now alongside the explicit ON switch in the panel below.
        //
        // Deliberately not a radio group and not self-toggling: JUCE turns the
        // other buttons of a group off *with click notification*, so selecting
        // a different arpeggiator would fire onClick on the previously
        // selected one and flip it on or off behind the user's back. The lit
        // state is set explicitly in updateTabAppearance instead, which leaves
        // onClick meaning "this button was actually clicked".
        auto btn = std::make_unique<fxme::AccentToggle>();
        btn->setButtonText (juce::String (i + 1));
        btn->setTooltip ("Select arpeggiator " + juce::String (i + 1)
                         + " (click again to turn it on or off)");
        btn->setClickingTogglesState (false);
        btn->setToggleState (i == selectedArpIndex, juce::dontSendNotification);
        btn->onClick = [this, i] {
            if (selectedArpIndex == i)
                audioProcessor.setArpeggiatorOn (i, ! audioProcessor.isArpeggiatorOn (i));
            selectArp (i);
        };
        addAndMakeVisible (*btn);
        tabButtons.push_back (std::move (btn));

        // Arp component
        auto comp = std::make_unique<ArpeggiatorComponent>();
        comp->setArpColour (getArpColour (i));
        comp->setText (audioProcessor.getArpeggiatorPattern (i), false);
        comp->setSubdivisionIndex (audioProcessor.getArpeggiatorSubdivision (i));
        comp->setMidiChannel (audioProcessor.getArpeggiatorMidiChannel (i));
        comp->setOnState (audioProcessor.isArpeggiatorOn (i));

        comp->onPatternChanged = [this, i] (const juce::String& pat) {
            audioProcessor.setArpeggiatorPattern (i, pat);
        };
        comp->onSubdivisionChanged = [this, i] (int subdivIdx) {
            audioProcessor.setArpeggiatorSubdivision (i, subdivIdx);
        };
        comp->onMidiChannelChanged = [this, i] (int channel) {
            audioProcessor.setArpeggiatorMidiChannel (i, channel);
        };
        comp->onOnOffChanged = [this, i] (bool on) {
            audioProcessor.setArpeggiatorOn (i, on);
        };

        addChildComponent (*comp);   // invisible by default
        arpComponents.push_back (std::move (comp));

        lastStepIndices.add (-1);
        lastNoteOnCounts.push_back (audioProcessor.getArpeggiatorNoteOnCount (i));
        blinkLevels.push_back (0.0f);
    }

    // Make selected one visible
    arpComponents[asIndex (selectedArpIndex)]->setVisible (true);

    updateTabAppearance();
    if (presetOverlay.isVisible())
        presetOverlay.toFront (false);   // the new children were added above it
    glowLine.toFront (false);
    resized();
}

void TeArAudioProcessorEditor::selectArp (int index)
{
    if (!juce::isPositiveAndBelow (index, (int) arpComponents.size())) return;
    if (juce::isPositiveAndBelow (selectedArpIndex, (int) arpComponents.size()))
        arpComponents[asIndex (selectedArpIndex)]->setVisible (false);
    selectedArpIndex = index;
    arpComponents[asIndex (selectedArpIndex)]->setVisible (true);
    updateTabAppearance();
}

void TeArAudioProcessorEditor::updateTabAppearance()
{
    int numArps = (int) tabButtons.size();
    for (int i = 0; i < numArps; ++i)
    {
        const auto col  = getArpColour (i);
        const bool isOn = audioProcessor.isArpeggiatorOn (i);

        Theme::arpTabToggle (*tabButtons[asIndex (i)], col, isOn);
        tabButtons[asIndex (i)]->setToggleState (i == selectedArpIndex, juce::dontSendNotification);

        if (juce::isPositiveAndBelow (i, (int) arpComponents.size()))
            arpComponents[asIndex (i)]->setOnState (isOn);
    }

    removeArpButton.setEnabled (numArps > 1);
}

//==============================================================================
void TeArAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    int processorCount = audioProcessor.getNumArpeggiators();
    if (processorCount != (int) arpComponents.size())
    {
        rebuildArpUI();
    }
    else
    {
        // Structure unchanged — just refresh patterns and tab appearance
        for (int i = 0; i < (int) arpComponents.size(); ++i)
            arpComponents[asIndex (i)]->setText (audioProcessor.getArpeggiatorPattern (i), false);
        updateTabAppearance();
    }
}

void TeArAudioProcessorEditor::timerCallback()
{
    updateControlEnablement();

    const bool notesAreHeld = audioProcessor.areNotesHeld();
    int numArps = audioProcessor.getNumArpeggiators();

    // --- Note-on blink on the selection buttons ---
    // The engine only exposes a monotonic counter, so a change since the last
    // frame means "it fired", however many notes landed in between.
    {
        const float decay = Theme::blinkDecayPerSecond / 60.0f;
        const int   count = juce::jmin ((int) tabButtons.size(), (int) blinkLevels.size());

        for (int i = 0; i < count; ++i)
        {
            const auto noteOns = audioProcessor.getArpeggiatorNoteOnCount (i);
            if (noteOns != lastNoteOnCounts[(size_t) i])
            {
                lastNoteOnCounts[(size_t) i] = noteOns;
                blinkLevels[(size_t) i] = 1.0f;
            }
            else
            {
                blinkLevels[(size_t) i] = juce::jmax (0.0f, blinkLevels[(size_t) i] - decay);
            }

            tabButtons[(size_t) i]->setFlash (blinkLevels[(size_t) i]);
        }
    }

    // --- Scale / keyboard display ---
    if (notesAreHeld && numArps > 0)
    {
        auto chordMethod = (int) audioProcessor.getAPVTS().getRawParameterValue ("chordMethod")->load();

        // Actual held MIDI notes → red outline (works for all chord methods)
        juce::Array<int> inputNotes = audioProcessor.getHeldNotes();

        // Playing output notes → arp-colour fill
        juce::Array<juce::var> currentNotes;
        for (int i = 0; i < numArps; ++i)
        {
            if (audioProcessor.isArpeggiatorOn (i))
            {
                int note = audioProcessor.getArpeggiator (i).getLastPlayedNote();
                if (note != -1)
                {
                    auto* obj = new juce::DynamicObject();
                    obj->setProperty ("note",     note);
                    obj->setProperty ("arpIndex", i);
                    currentNotes.add (juce::var (obj));
                }
            }
        }

        // Scale notes → gray dimming of non-scale keys (scale mode only)
        if (chordMethod == 2)
            keyboardComponent.updateScale (currentDisplayScale.getNotes(),
                                           currentDisplayScale.getRootNote(),
                                           currentNotes, inputNotes);
        else
            keyboardComponent.updateScale ({}, -1, currentNotes, inputNotes);
    }
    else
    {
        updateScaleDisplay();
    }

    if (juce::isPositiveAndBelow (selectedArpIndex, (int) arpComponents.size()))
    {
        auto* comp = arpComponents[asIndex (selectedArpIndex)].get();
        bool canHighlight = notesAreHeld
                         && !comp->editorHasKeyboardFocus()
                         && audioProcessor.isArpeggiatorOn (selectedArpIndex);

        if (canHighlight)
        {
            int currentStep = audioProcessor.getArpeggiatorCurrentStep (selectedArpIndex);
            if (currentStep != lastStepIndices[selectedArpIndex])
            {
                lastStepIndices.set (selectedArpIndex, currentStep);
                const auto& arp     = audioProcessor.getArpeggiator (selectedArpIndex);
                const auto  pattern = audioProcessor.getArpeggiatorPattern (selectedArpIndex);
                int stepStart = arp.getPatternIndexForStep (currentStep);
                int stepEnd   = arp.getPatternIndexForStep (currentStep + 1);
                if (stepEnd <= stepStart) stepEnd = pattern.length();
                comp->setHighlightedRegion ({ stepStart, stepEnd });
            }
        }
        else if (lastStepIndices[selectedArpIndex] != -1)
        {
            comp->setHighlightedRegion ({});
            lastStepIndices.set (selectedArpIndex, -1);
        }
    }
}

void TeArAudioProcessorEditor::updateControlEnablement()
{
    auto& apvts = audioProcessor.getAPVTS();

    // The scale controls only do anything in "Single note" mode: that is the
    // one branch of processBlock's switch that reads scaleType, scaleRoot or
    // followMidiIn, and the one branch updateScaleDisplay draws a scale for.
    const bool scaleMode = (int) apvts.getRawParameterValue ("chordMethod")->load() == 2;

    // scaleRoot is superseded a second time by followMidiIn, which makes the
    // played note the root and writes the parameter rather than reading it.
    const bool followMidiIn = apvts.getRawParameterValue ("followMidiIn")->load() > 0.5f;
    const bool rootIsLive   = scaleMode && ! followMidiIn;

    // The labels are attached siblings, not children, so disabling a box does
    // not carry to its label; they have to be set alongside it.
    scaleTypeBox      .setEnabled (scaleMode);
    scaleTypeLabel    .setEnabled (scaleMode);
    followMidiInButton.setEnabled (scaleMode);
    scaleRootBox      .setEnabled (rootIsLive);
    scaleRootLabel    .setEnabled (rootIsLive);
}

void TeArAudioProcessorEditor::updateScaleDisplay()
{
    auto& apvts = audioProcessor.getAPVTS();
    auto chordMethod = (int) apvts.getRawParameterValue ("chordMethod")->load();

    if (chordMethod == 2)
    {
        auto scaleRoot = (int) apvts.getRawParameterValue ("scaleRoot")->load();
        auto scaleType = static_cast<fxme::MidiTools::Scale::Type> (
            (int) apvts.getRawParameterValue ("scaleType")->load());
        currentDisplayScale = fxme::MidiTools::Scale (scaleRoot, scaleType);
        keyboardComponent.updateScale (currentDisplayScale.getNotes(), currentDisplayScale.getRootNote(), {});
    }
    else
    {
        keyboardComponent.updateScale ({}, -1, {});
    }

    lastPlayedArpNote = -1;
}

//==============================================================================
void TeArAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto diagonale   = (getLocalBounds().getTopLeft() - getLocalBounds().getBottomRight()).toFloat();
    auto length      = diagonale.getDistanceFromOrigin();
    auto perpendicular = diagonale.rotatedAboutOrigin (juce::degreesToRadians (270.0f)) / length;
    auto height      = float (getWidth() * getHeight()) / length;
    auto bluegreengrey = juce::Colour::fromFloatRGBA (0.15f, 0.15f, 0.25f, 1.0f);
    juce::ColourGradient grad (bluegreengrey.darker().darker().darker(), perpendicular * height,
                               bluegreengrey, perpendicular * -height, false);
    g.setGradientFill (grad);
    g.fillAll();
}

void TeArAudioProcessorEditor::resized()
{
    splash.setBounds (getLocalBounds());

    auto full = getLocalBounds();
    topBar.setBounds (full.removeFromTop (Theme::topBarHeight));

    // Straddles the header's bottom edge so the glow bleeds into both.
    glowLine.setBounds (0, topBar.getBottom() - GlowLine::kHeight / 2,
                        getWidth(), GlowLine::kHeight);

    // The browser covers the whole working area when open.
    presetOverlay.setBounds (full);

    auto bounds = full.reduced (10);

    // --- Global controls row ---
    auto controlsRow = bounds.removeFromTop (35);
    {
        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::row;
        fb.items.add (juce::FlexItem (chordMethodLabel)  .withFlex (0.5f));
        fb.items.add (juce::FlexItem (chordMethodBox)    .withFlex (1.0f));
        fb.items.add (juce::FlexItem (scaleRootLabel)    .withFlex (0.5f).withMargin ({ 0, 0, 0, 10 }));
        fb.items.add (juce::FlexItem (scaleRootBox)      .withFlex (0.5f));
        fb.items.add (juce::FlexItem (followMidiInButton).withFlex (0.6f).withMargin ({ 0, 5, 0, 5 }));
        fb.items.add (juce::FlexItem (scaleTypeLabel)    .withFlex (0.5f));
        fb.items.add (juce::FlexItem (scaleTypeBox)      .withFlex (1.0f));
        fb.performLayout (controlsRow.toFloat());
    }

    // --- Keyboard, along the bottom ---
    keyboardComponent.setBounds (bounds.removeFromBottom (Theme::keyboardHeight));
    bounds.removeFromBottom (5);

    // --- Tab bar ---
    bounds.removeFromTop (5);
    auto tabRow = bounds.removeFromTop (Theme::tabRowHeight);

    addArpButton    .setBounds (tabRow.removeFromLeft (30)); tabRow.removeFromLeft (2);
    removeArpButton .setBounds (tabRow.removeFromLeft (30)); tabRow.removeFromLeft (2);
    patternGenButton.setBounds (tabRow.removeFromLeft (30)); tabRow.removeFromLeft (10);

    // Taken from the right before the tabs are laid out, so a growing number of
    // arpeggiators eats into the middle rather than pushing this off the edge.
    infoButton.setBounds (tabRow.removeFromRight (Theme::tabRowHeight).reduced (4));
    tabRow.removeFromRight (6);

    int numTabs = (int) tabButtons.size();
    if (numTabs > 0)
    {
        // Selection buttons are compact squares now that they no longer double
        // as the on/off control.
        int tabW = juce::jmin (Theme::tabWidth,
                               juce::jmax (24, (tabRow.getWidth() - 2 * numTabs) / numTabs));
        for (auto& btn : tabButtons)
        {
            btn->setBounds (tabRow.removeFromLeft (tabW).reduced (0, 2));
            tabRow.removeFromLeft (2);
        }
    }

    // --- Arp components (all share the same remaining area; only selected is visible) ---
    bounds.removeFromTop (5);
    for (auto& comp : arpComponents)
        comp->setBounds (bounds);
}
