#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TeArAudioProcessorEditor::TeArAudioProcessorEditor (TeArAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    audioProcessor.addChangeListener (this);

    auto& apvts = audioProcessor.getAPVTS();
    const auto neutral = juce::Colours::white;

    // --- Toolbar buttons ---
    addAndMakeVisible (addArpButton);
    addArpButton.setColour (juce::TextButton::buttonColourId,  juce::Colours::darkblue.darker (2.f));
    addArpButton.setColour (juce::TextButton::textColourOffId, neutral);
    addArpButton.onClick = [this] {
        audioProcessor.addArpeggiator();
        selectedArpIndex = audioProcessor.getNumArpeggiators() - 1;
        rebuildArpUI();
    };

    addAndMakeVisible (removeArpButton);
    removeArpButton.setColour (juce::TextButton::buttonColourId,  juce::Colours::darkblue.darker (2.f));
    removeArpButton.setColour (juce::TextButton::textColourOffId, neutral);
    removeArpButton.onClick = [this] {
        if (audioProcessor.getNumArpeggiators() <= 1) return;
        audioProcessor.removeArpeggiator (selectedArpIndex);
        selectedArpIndex = juce::jlimit (0, audioProcessor.getNumArpeggiators() - 1, selectedArpIndex);
        rebuildArpUI();
    };

    addAndMakeVisible (patternGenButton);
    patternGenButton.setColour (juce::TextButton::buttonColourId,  juce::Colours::darkblue.darker (2.f));
    patternGenButton.setColour (juce::TextButton::textColourOffId, neutral);
    patternGenButton.onClick = [this] {
        if (!juce::isPositiveAndBelow (selectedArpIndex, (int) arpComponents.size())) return;

        const auto& arp   = audioProcessor.getArpeggiator (selectedArpIndex);
        auto colour = getArpColour (selectedArpIndex);

        auto makeEuclidian = [&arp] (int hits, int steps, int rotation) {
            return const_cast<fxme::Arpeggiator&> (arp).makeEuclidianPattern (hits, steps, rotation);
        };
        auto makeRandom = [&arp] () {
            return const_cast<fxme::Arpeggiator&> (arp).makeRandomPattern();
        };
        auto onOk = [this] (juce::String pattern) {
            audioProcessor.setArpeggiatorPattern (selectedArpIndex, pattern);
        };

        auto* content = new ArpPatternPopup (makeEuclidian, makeRandom, onOk, colour);
        juce::CallOutBox::launchAsynchronously (
            std::unique_ptr<juce::Component> (content),
            patternGenButton.getScreenBounds(), this);
    };

    // --- Global controls ---
    addAndMakeVisible (chordMethodLabel);
    chordMethodLabel.setText ("Method", juce::dontSendNotification);
    chordMethodLabel.attachToComponent (&chordMethodBox, true);
    chordMethodLabel.setLookAndFeel (&arpLAF);
    chordMethodLabel.setColour (juce::Label::textColourId, neutral);

    addAndMakeVisible (chordMethodBox);
    chordMethodBox.setLookAndFeel (&arpLAF);
    chordMethodBox.setColour (juce::ComboBox::textColourId,       neutral);
    chordMethodBox.setColour (juce::ComboBox::outlineColourId,    neutral);
    chordMethodBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("chordMethod")))
        { chordMethodBox.clear(); chordMethodBox.addItemList (p->choices, 1); }
    chordMethodAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, "chordMethod", chordMethodBox);
    chordMethodBox.onChange = [this] { updateScaleDisplay(); };

    addAndMakeVisible (scaleRootLabel);
    scaleRootLabel.setText ("Scale Root", juce::dontSendNotification);
    scaleRootLabel.attachToComponent (&scaleRootBox, true);
    scaleRootLabel.setLookAndFeel (&arpLAF);
    scaleRootLabel.setColour (juce::Label::textColourId, neutral);

    addAndMakeVisible (scaleRootBox);
    scaleRootBox.setLookAndFeel (&arpLAF);
    scaleRootBox.setColour (juce::ComboBox::textColourId,       neutral);
    scaleRootBox.setColour (juce::ComboBox::outlineColourId,    neutral);
    scaleRootBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("scaleRoot")))
        { scaleRootBox.clear(); scaleRootBox.addItemList (p->choices, 1); }
    scaleRootAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, "scaleRoot", scaleRootBox);
    scaleRootBox.onChange = [this] { updateScaleDisplay(); };

    addAndMakeVisible (scaleTypeLabel);
    scaleTypeLabel.setText ("Scale Type", juce::dontSendNotification);
    scaleTypeLabel.attachToComponent (&scaleTypeBox, true);
    scaleTypeLabel.setLookAndFeel (&arpLAF);
    scaleTypeLabel.setColour (juce::Label::textColourId, neutral);

    addAndMakeVisible (scaleTypeBox);
    scaleTypeBox.setLookAndFeel (&arpLAF);
    scaleTypeBox.setColour (juce::ComboBox::textColourId,       neutral);
    scaleTypeBox.setColour (juce::ComboBox::outlineColourId,    neutral);
    scaleTypeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("scaleType")))
        { scaleTypeBox.clear(); scaleTypeBox.addItemList (p->choices, 1); }
    scaleTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, "scaleType", scaleTypeBox);
    scaleTypeBox.onChange = [this] { updateScaleDisplay(); };

    addAndMakeVisible (followMidiInButton);
    followMidiInButton.setButtonText ("Follow MIDI In");
    followMidiInButton.setColour (juce::ToggleButton::textColourId, neutral);
    followMidiInAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, "followMidiIn", followMidiInButton);

    addAndMakeVisible (keyboardComponent);
    addAndMakeVisible (logo);

    rebuildArpUI();

    startTimerHz (60);
    updateScaleDisplay();

    setSize (800, 480);
}

TeArAudioProcessorEditor::~TeArAudioProcessorEditor()
{
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

    int numArps = audioProcessor.getNumArpeggiators();
    if (numArps == 0) { resized(); return; }
    selectedArpIndex = juce::jlimit (0, numArps - 1, selectedArpIndex);

    for (int i = 0; i < numArps; ++i)
    {
        // Tab button — selects on first click, toggles on/off on second click
        auto btn = std::make_unique<juce::TextButton> (juce::String (i + 1));
        btn->onClick = [this, i] {
            if (selectedArpIndex == i)
                audioProcessor.setArpeggiatorOn (i, !audioProcessor.isArpeggiatorOn (i));
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

        comp->onPatternChanged = [this, i] (const juce::String& pat) {
            audioProcessor.setArpeggiatorPattern (i, pat);
        };
        comp->onSubdivisionChanged = [this, i] (int subdivIdx) {
            audioProcessor.setArpeggiatorSubdivision (i, subdivIdx);
        };
        comp->onMidiChannelChanged = [this, i] (int channel) {
            audioProcessor.setArpeggiatorMidiChannel (i, channel);
        };

        addChildComponent (*comp);   // invisible by default
        arpComponents.push_back (std::move (comp));

        lastStepIndices.add (-1);
    }

    // Make selected one visible
    arpComponents[selectedArpIndex]->setVisible (true);

    updateTabAppearance();
    resized();
}

void TeArAudioProcessorEditor::selectArp (int index)
{
    if (!juce::isPositiveAndBelow (index, (int) arpComponents.size())) return;
    if (juce::isPositiveAndBelow (selectedArpIndex, (int) arpComponents.size()))
        arpComponents[selectedArpIndex]->setVisible (false);
    selectedArpIndex = index;
    arpComponents[selectedArpIndex]->setVisible (true);
    updateTabAppearance();
}

void TeArAudioProcessorEditor::updateTabAppearance()
{
    int numArps = (int) tabButtons.size();
    for (int i = 0; i < numArps; ++i)
    {
        auto col       = getArpColour (i);
        bool isOn      = audioProcessor.isArpeggiatorOn (i);
        bool isSelected= (i == selectedArpIndex);
        auto display   = isOn ? col : col.withSaturation (0.15f).darker (0.5f);

        if (isSelected)
        {
            tabButtons[i]->setColour (juce::TextButton::buttonColourId,  display.withAlpha (0.85f));
            tabButtons[i]->setColour (juce::TextButton::buttonOnColourId,display.withAlpha (0.85f));
            tabButtons[i]->setColour (juce::TextButton::textColourOffId, juce::Colours::black);
            tabButtons[i]->setColour (juce::TextButton::textColourOnId,  juce::Colours::black);
        }
        else
        {
            tabButtons[i]->setColour (juce::TextButton::buttonColourId,  display.withAlpha (0.15f));
            tabButtons[i]->setColour (juce::TextButton::buttonOnColourId,display.withAlpha (0.15f));
            tabButtons[i]->setColour (juce::TextButton::textColourOffId, display);
            tabButtons[i]->setColour (juce::TextButton::textColourOnId,  display);
        }
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
            arpComponents[i]->setText (audioProcessor.getArpeggiatorPattern (i), false);
        updateTabAppearance();
    }
}

void TeArAudioProcessorEditor::timerCallback()
{
    const bool notesAreHeld = audioProcessor.areNotesHeld();
    int numArps = audioProcessor.getNumArpeggiators();

    // --- Scale / keyboard display ---
    if (notesAreHeld && numArps > 0)
    {
        const auto& chord = audioProcessor.getArpeggiator (0).getChord();
        auto chordMethod  = (int) audioProcessor.getAPVTS().getRawParameterValue ("chordMethod")->load();

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
        auto* comp = arpComponents[selectedArpIndex].get();
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
                const auto& pattern = audioProcessor.getArpeggiatorPattern (selectedArpIndex);
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
    auto bounds = getLocalBounds().reduced (10);

    // --- Global controls row ---
    auto controlsRow = bounds.removeFromTop (35);
    {
        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::row;
        fb.items.add (juce::FlexItem (logo)              .withWidth (50.0f));
        fb.items.add (juce::FlexItem (chordMethodLabel)  .withFlex (0.5f));
        fb.items.add (juce::FlexItem (chordMethodBox)    .withFlex (1.0f));
        fb.items.add (juce::FlexItem (scaleRootLabel)    .withFlex (0.5f).withMargin ({ 0, 0, 0, 10 }));
        fb.items.add (juce::FlexItem (scaleRootBox)      .withFlex (0.5f));
        fb.items.add (juce::FlexItem (followMidiInButton).withFlex (0.6f).withMargin ({ 0, 5, 0, 5 }));
        fb.items.add (juce::FlexItem (scaleTypeLabel)    .withFlex (0.5f));
        fb.items.add (juce::FlexItem (scaleTypeBox)      .withFlex (1.0f));
        fb.performLayout (controlsRow.toFloat());
    }

    // --- Scale component ---
    bounds.removeFromTop (5);
    keyboardComponent.setBounds (bounds.removeFromTop (60));

    // --- Tab bar ---
    bounds.removeFromTop (5);
    auto tabRow = bounds.removeFromTop (32);

    addArpButton    .setBounds (tabRow.removeFromLeft (30)); tabRow.removeFromLeft (2);
    removeArpButton .setBounds (tabRow.removeFromLeft (30)); tabRow.removeFromLeft (2);
    patternGenButton.setBounds (tabRow.removeFromLeft (30)); tabRow.removeFromLeft (10);

    int numTabs = (int) tabButtons.size();
    if (numTabs > 0)
    {
        int tabW = juce::jmin (80, juce::jmax (30, tabRow.getWidth() / numTabs));
        for (auto& btn : tabButtons)
        {
            btn->setBounds (tabRow.removeFromLeft (tabW));
            tabRow.removeFromLeft (2);
        }
    }

    // --- Arp components (all share the same remaining area; only selected is visible) ---
    bounds.removeFromTop (5);
    for (auto& comp : arpComponents)
        comp->setBounds (bounds);
}
