/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
void TeArAudioProcessorEditor::ArpLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor& editor)
{
    g.setColour (juce::Colours::darkblue.darker(2.f));
    g.fillRoundedRectangle (juce::Rectangle<int>(0,0, width, height).toFloat(), 10.0f);
    g.setColour (editor.findColour(juce::TextEditor::outlineColourId));
    g.drawRoundedRectangle (juce::Rectangle<int>(0, 0, width, height).toFloat(), 10.0f,2.f);

}

void TeArAudioProcessorEditor::ArpLookAndFeel::drawTextEditorOutline (juce::Graphics& /*g*/, int /*width*/, int /*height*/, juce::TextEditor& /*editor*/)
{
    // We don't want an outline, so this is empty.
}

void TeArAudioProcessorEditor::ArpLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour (juce::Label::backgroundColourId));

    if (! label.isBeingEdited())
    {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        const juce::Font font (getLabelFont (label));

        g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (alpha));
        g.setFont (font);

        auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());

        g.drawText (label.getText(), textArea, label.getJustificationType(), true);

        g.setColour (label.findColour (juce::Label::outlineColourId).withMultipliedAlpha (alpha));
    }
    else if (label.isEnabled())
    {
        g.setColour (label.findColour (juce::Label::outlineColourId));
    }

    g.drawRect (label.getLocalBounds());
}

void TeArAudioProcessorEditor::ArpLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box)
{
    auto cornerSize = box.getHeight() * 0.2f;
    juce::Path p;
    p.addRoundedRectangle(0, 0, width, height, cornerSize);

    g.setColour(juce::Colours::darkblue.darker(2.f));
    g.fillPath(p);

    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.strokePath(p, juce::PathStrokeType(2.0f));

    auto arrowZone = juce::Rectangle<int> (width - 30, 0, 20, height);
    juce::Path arrow;
    arrow.addTriangle(arrowZone.getX() + 5, arrowZone.getCentreY() - 3, arrowZone.getRight() - 5, arrowZone.getCentreY() - 3, arrowZone.getCentreX(), arrowZone.getCentreY() + 4);
    g.fillPath(arrow);
}

juce::Font TeArAudioProcessorEditor::ArpLookAndFeel::getLabelFont (juce::Label& /*label*/)
{
    return { 15.0f };
}

TeArAudioProcessorEditor::TeArAudioProcessorEditor (TeArAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    audioProcessor.addChangeListener(this);

    lastStepIndices.insertMultiple(0, -1, 4);

    auto& apvts = audioProcessor.getAPVTS();

    const auto neutralColour = juce::Colours::white;

    for (int i = 0; i < 4; ++i)
    {
        juce::Colour arpColour = juce::Colours::lime; // Default for Arp 1
        if (i == 1) arpColour = juce::Colours::cyan;
        else if (i == 2) arpColour = juce::Colours::magenta;
        else if (i == 3) arpColour = juce::Colours::yellow;
        auto* editor = new ArpeggiatorTextEditor();
        arpeggiatorEditors.add(editor);
        addAndMakeVisible(editor);
        editor->setLookAndFeel(&arpLookAndFeel);
        editor->setMultiLine(true);
        editor->setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 24.0f, juce::Font::plain));

        // Set the colors for the current arpeggiator
        editor->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        editor->setColour(juce::TextEditor::textColourId, arpColour);
        editor->setColour(juce::CaretComponent::caretColourId, arpColour.brighter());
        editor->setColour(juce::TextEditor::highlightColourId, arpColour);
        editor->setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::black);
        editor->setColour(juce::TextEditor::outlineColourId, arpColour); // For the LookAndFeel

        editor->setText(audioProcessor.getArpeggiatorPattern(i), false);

        auto validateAndSetPattern = [this, i, editor] {
            audioProcessor.setArpeggiatorPattern(i, editor->getText());
        };

        editor->onReturnKey = [this, validateAndSetPattern] {
            validateAndSetPattern();
            giveAwayKeyboardFocus();
        };
        editor->onFocusLost = validateAndSetPattern;
    }
    

    // --- Global Controls (Neutral Color) ---
    addAndMakeVisible(chordMethodLabel);
    chordMethodLabel.setText("Chord Method", juce::dontSendNotification);
    chordMethodLabel.attachToComponent(&chordMethodBox, true);
    chordMethodLabel.setLookAndFeel(&arpLookAndFeel);
    chordMethodLabel.setColour(juce::Label::textColourId, neutralColour);
    
    addAndMakeVisible(chordMethodBox);
    // Manually populate the ComboBox *before* creating the attachment.
    chordMethodBox.setLookAndFeel(&arpLookAndFeel);
    chordMethodBox.setColour(juce::ComboBox::textColourId, neutralColour);
    chordMethodBox.setColour(juce::ComboBox::outlineColourId, neutralColour);
    chordMethodBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);

    if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("chordMethod")))
    {
        chordMethodBox.clear();
        chordMethodBox.addItemList(parameter->choices, 1);
    }
    // Now create the attachment, which will sync the selection with the parameter's current value.
    chordMethodAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "chordMethod", chordMethodBox);
    chordMethodBox.onChange = [this] { updateScaleDisplay(); };

    // --- Per-Arpeggiator Controls (Individual Colors) ---
    for (int i = 0; i < 4; ++i)
    {
        juce::Colour arpColour = juce::Colours::lime;
        if (i == 1) arpColour = juce::Colours::cyan;
        else if (i == 2) arpColour = juce::Colours::magenta;
        else if (i == 3) arpColour = juce::Colours::yellow;
        auto* label = new juce::Label();
        // subdivisionLabels.add(label);
        // addAndMakeVisible(label);
        // label->setText("Sub " + juce::String(i + 1), juce::dontSendNotification);
        // label->setLookAndFeel(&arpLookAndFeel);
        // label->setColour(juce::Label::textColourId, arpColour);

        auto* box = new juce::ComboBox();
        subdivisionBoxes.add(box);
        addAndMakeVisible(box);
        // label->attachToComponent(box, true);
        box->setLookAndFeel(&arpLookAndFeel);
        box->setColour(juce::ComboBox::textColourId, arpColour);
        box->setColour(juce::ComboBox::outlineColourId, arpColour);
        box->setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);

        auto paramID = "subdivision" + juce::String(i + 1);
        if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(paramID)))
        {
            box->clear();
            box->addItemList(parameter->choices, 1);
        }
        subdivisionAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, paramID, *box));
    }
    
    for (int i = 0; i < 4; ++i)
    {
        juce::Colour arpColour = juce::Colours::lime;
        if (i == 1) arpColour = juce::Colours::cyan;
        else if (i == 2) arpColour = juce::Colours::magenta;
        else if (i == 3) arpColour = juce::Colours::yellow;
        auto* button = new juce::ToggleButton();
        arpeggiatorOnButtons.add(button);
        addAndMakeVisible(button);
        button->setLookAndFeel(&fxmeLookAndFeel);
        button->setButtonText(""); // Text is handled by the label now
        button->setColour(juce::ToggleButton::tickColourId, arpColour);
        button->setToggleState(audioProcessor.isArpeggiatorOn(i), juce::dontSendNotification);

        auto paramID = "arpOn" + juce::String(i + 1);
        arpeggiatorOnAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, paramID, *button));
    }

    for (int i = 0; i < 4; ++i)
    {
        juce::Colour arpColour = juce::Colours::lime;
        if (i == 1) arpColour = juce::Colours::cyan;
        else if (i == 2) arpColour = juce::Colours::magenta;
        else if (i == 3) arpColour = juce::Colours::yellow;

        auto* label = new juce::Label();
        midiChannelLabels.add(label);
        addAndMakeVisible(label);
        label->setText("Ch ", juce::dontSendNotification);
        label->setLookAndFeel(&arpLookAndFeel);
        label->setColour(juce::Label::textColourId, arpColour);

        auto* box = new juce::ComboBox();
        midiChannelBoxes.add(box);
        addAndMakeVisible(box);
        label->attachToComponent(box, true);
        box->setLookAndFeel(&arpLookAndFeel);
        box->setColour(juce::ComboBox::textColourId, arpColour);
        box->setColour(juce::ComboBox::outlineColourId, arpColour);
        box->setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        for (int ch = 1; ch <= 16; ++ch) box->addItem(juce::String(ch), ch);
        midiChannelAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "midiChannel" + juce::String(i + 1), *box));
    }

    addAndMakeVisible(scaleRootLabel);
    scaleRootLabel.setText("Scale Root", juce::dontSendNotification);
    scaleRootLabel.attachToComponent(&scaleRootBox, true);
    scaleRootLabel.setLookAndFeel(&arpLookAndFeel);
    scaleRootLabel.setColour(juce::Label::textColourId, neutralColour);

    addAndMakeVisible(scaleRootBox);
    scaleRootBox.setLookAndFeel(&arpLookAndFeel);
    scaleRootBox.setColour(juce::ComboBox::textColourId, neutralColour);
    scaleRootBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    scaleRootBox.setColour(juce::ComboBox::outlineColourId, neutralColour);

    if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("scaleRoot")))
    {
        scaleRootBox.clear();
        scaleRootBox.addItemList(parameter->choices, 1);
    }
    scaleRootAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "scaleRoot", scaleRootBox);
    scaleRootBox.onChange = [this] { updateScaleDisplay(); };

    addAndMakeVisible(scaleTypeLabel);
    scaleTypeLabel.setText("Scale Type", juce::dontSendNotification);
    scaleTypeLabel.attachToComponent(&scaleTypeBox, true);
    scaleTypeLabel.setLookAndFeel(&arpLookAndFeel);
    scaleTypeLabel.setColour(juce::Label::textColourId, neutralColour);

    addAndMakeVisible(scaleTypeBox);
    scaleTypeBox.setLookAndFeel(&arpLookAndFeel);
    scaleTypeBox.setColour(juce::ComboBox::textColourId, neutralColour);
    scaleTypeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    scaleTypeBox.setColour(juce::ComboBox::outlineColourId, neutralColour);

    if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("scaleType")))
    {
        scaleTypeBox.clear();
        scaleTypeBox.addItemList(parameter->choices, 1);
    }
    scaleTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "scaleType", scaleTypeBox);
    scaleTypeBox.onChange = [this] { updateScaleDisplay(); };

    addAndMakeVisible(followMidiInButton);
    followMidiInButton.setButtonText("Follow MIDI In");
    followMidiInButton.setColour(juce::ToggleButton::textColourId, neutralColour);
    followMidiInAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "followMidiIn", followMidiInButton);

    addAndMakeVisible(scaleComponent);
    addAndMakeVisible(logo);

    // Start the timer to update the UI 30 times per second
    startTimerHz(60);

    // Initial update of the scale component to show the default scale
    updateScaleDisplay();


    setSize (960, 400); // Increased size for multiple arps
}

TeArAudioProcessorEditor::~TeArAudioProcessorEditor()
{
    for (auto* editor : arpeggiatorEditors) editor->setLookAndFeel(nullptr);
    for (auto* button : arpeggiatorOnButtons) button->setLookAndFeel(nullptr); // Toggle buttons don't use LookAndFeel for basic drawing, but good practice
    for (auto* label : midiChannelLabels) label->setLookAndFeel(nullptr);
    for (auto* box : midiChannelBoxes) box->setLookAndFeel(nullptr);
    for (auto* label : subdivisionLabels) label->setLookAndFeel(nullptr);
    for (auto* box : subdivisionBoxes) box->setLookAndFeel(nullptr);
    audioProcessor.removeChangeListener(this);
    stopTimer();
}

void TeArAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    // When the processor tells us something changed, update our manual controls.
    for (int i = 0; i < arpeggiatorEditors.size(); ++i)
        arpeggiatorEditors[i]->setText(audioProcessor.getArpeggiatorPattern(i), false);
}

void TeArAudioProcessorEditor::timerCallback()
{
    const bool notesAreHeld = audioProcessor.areNotesHeld();

    if (notesAreHeld)
    {
        // The chord is the same for all arps, so we can get it from the first one.
        const auto& chord = audioProcessor.getArpeggiator(0).getChord();
        auto chordMethod = static_cast<int>(audioProcessor.getAPVTS().getRawParameterValue("chordMethod")->load());
        const auto& notes = (chordMethod == 1) ? chord.getRawNotes() : chord.getDegrees();

        // Collect all currently playing notes from active arpeggiators
        juce::Array<juce::var> currentNotes;
        for (int i = 0; i < 4; ++i)
        {
            if (audioProcessor.isArpeggiatorOn(i))
            {
                int note = audioProcessor.getArpeggiator(i).getLastPlayedNote();
                if (note != -1)
                {
                    auto* noteInfo = new juce::DynamicObject();
                    noteInfo->setProperty("note", note);
                    noteInfo->setProperty("arpIndex", i);
                    currentNotes.add(juce::var(noteInfo));
                }
            }
        }

        if (!notes.isEmpty())
            scaleComponent.updateScale(notes, notes.getFirst(), currentNotes);
    }
    else
    {
        updateScaleDisplay();
    }

    // Update step highlights for each editor
    if (notesAreHeld) // Only highlight if notes are held
    {
        for (int i = 0; i < 4; ++i)
        {
            auto* editor = arpeggiatorEditors[i];

            if (!editor->hasKeyboardFocus(true) && audioProcessor.isArpeggiatorOn(i))
            {
                int currentStep = audioProcessor.getArpeggiatorCurrentStep(i);

                if (currentStep != lastStepIndices[i])
                {
                    lastStepIndices.set(i, currentStep);

                    const auto& arp = audioProcessor.getArpeggiator(i);
                    const auto& pattern = audioProcessor.getArpeggiatorPattern(i);
                    int stepStart = arp.getPatternIndexForStep(currentStep);
                    int stepEnd = arp.getPatternIndexForStep(currentStep + 1);

                    if (stepEnd <= stepStart)
                        stepEnd = pattern.length();

                    editor->setHighlightedRegion({ stepStart, stepEnd });
                }
            }
            else if (lastStepIndices[i] != -1 || !audioProcessor.isArpeggiatorOn(i)) // Clear highlight if not on or not playing
            {
                editor->setHighlightedRegion({});
                lastStepIndices.set(i, -1);
            }
        }
    }
    else // If no notes are held, clear all highlights
    {
        for (int i = 0; i < 4; ++i)
        {
            if (lastStepIndices[i] != -1)
            {
                arpeggiatorEditors[i]->setHighlightedRegion({});
                lastStepIndices.set(i, -1);
            }
        }
    }
}

void TeArAudioProcessorEditor::updateScaleDisplay()
{
    auto& apvts = audioProcessor.getAPVTS();
    auto chordMethod = static_cast<int>(apvts.getRawParameterValue("chordMethod")->load());

    // When not playing, only show the scale if the method is "Single note".
    if (chordMethod == 2) // "Single note"
    {
        auto scaleRoot = static_cast<int>(apvts.getRawParameterValue("scaleRoot")->load());
        auto scaleType = static_cast<MidiTools::Scale::Type>(static_cast<int>(apvts.getRawParameterValue("scaleType")->load()));

        currentDisplayScale = MidiTools::Scale(scaleRoot, scaleType);

        // Update the component with the scale notes, but with -1 for root and current note
        // to indicate that no note is currently playing. The root note should be shown.
        scaleComponent.updateScale(currentDisplayScale.getNotes(), currentDisplayScale.getRootNote(), {});
    }
    else // For "Notes played" or "Chord played as is", clear the display when not playing.
        scaleComponent.updateScale({}, -1, {});

    lastPlayedArpNote = -1; // Reset the last played note tracker
}
//==============================================================================
void TeArAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto diagonale = (getLocalBounds().getTopLeft() - getLocalBounds().getBottomRight()).toFloat();
    auto length = diagonale.getDistanceFromOrigin();
    auto perpendicular = diagonale.rotatedAboutOrigin (juce::degreesToRadians (270.0f)) / length;
    auto height = float (getWidth() * getHeight()) / length;
    auto bluegreengrey = juce::Colour::fromFloatRGBA (0.15f, 0.15f, 0.25f, 1.0f);
    juce::ColourGradient grad (bluegreengrey.darker().darker().darker(), perpendicular * height,
                           bluegreengrey, perpendicular * -height, false);
    g.setGradientFill(grad);
    g.fillAll();
}

void TeArAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    juce::FlexBox mainBox, editorBox, controlsBox, subdivisionRowBox, channelRowBox;
    mainBox.flexDirection = juce::FlexBox::Direction::column;
    editorBox.flexDirection = juce::FlexBox::Direction::row;
    controlsBox.flexDirection = juce::FlexBox::Direction::row;
    subdivisionRowBox.flexDirection = juce::FlexBox::Direction::row;
    channelRowBox.flexDirection = juce::FlexBox::Direction::row;

    for (int i = 0; i < 4; ++i)
    {
        int leftMargin = 5;
        int rightMargin = 5;
        if (i==0) leftMargin = 0;
        if (i==3) rightMargin = 0;
        editorBox.items.add(juce::FlexItem(*arpeggiatorEditors[i]).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, rightMargin, 0, leftMargin)));
    }

    for (int i = 0; i < 4; ++i)
    {
        // Add the On/Off button to the left of the subdivision label
        subdivisionRowBox.items.add(juce::FlexItem(*arpeggiatorOnButtons[i]).withFlex(0.15f).withMargin(juce::FlexItem::Margin(0.f, 5.f, 0.f, 0.f)));
        //subdivisionRowBox.items.add(juce::FlexItem(*subdivisionLabels[i]).withFlex(0.25f));
        subdivisionRowBox.items.add(juce::FlexItem(*subdivisionBoxes[i]).withFlex(0.5f).withMargin(juce::FlexItem::Margin(0.f, 5.f, 0.f, 0.f)));
        subdivisionRowBox.items.add(juce::FlexItem(*midiChannelLabels[i]).withFlex(0.25f).withMargin(juce::FlexItem::Margin(0.f, 5.f, 0.f, 0.f)));
        subdivisionRowBox.items.add(juce::FlexItem(*midiChannelBoxes[i]).withFlex(0.3f).withMargin(juce::FlexItem::Margin(0.f, 10.f, 0.f, 0.f)));
    }
    // Add some space between the subdivision groups
    for (int i = 0; i < 3; ++i) {
        subdivisionRowBox.items.insert(i * 5 + 4, juce::FlexItem().withFlex(0.1f));
    }

    controlsBox.items.add(juce::FlexItem(logo).withFlex(0.2f));
    controlsBox.items.add(juce::FlexItem(chordMethodLabel).withFlex(0.5f));
    controlsBox.items.add(juce::FlexItem(chordMethodBox).withFlex(1.0f));
    controlsBox.items.add(juce::FlexItem(scaleRootLabel).withFlex(0.5f).withMargin(juce::FlexItem::Margin(0.f, 0.f, 0.f, 10.f)));
    controlsBox.items.add(juce::FlexItem(scaleRootBox).withFlex(0.5f));
    controlsBox.items.add(juce::FlexItem(followMidiInButton).withFlex(0.6f).withMargin(juce::FlexItem::Margin(0.f, 5.f, 0.f, 5.f)));
    controlsBox.items.add(juce::FlexItem(scaleTypeLabel).withFlex(0.5f));
    controlsBox.items.add(juce::FlexItem(scaleTypeBox).withFlex(1.0f));

    mainBox.items.add(juce::FlexItem(controlsBox).withFlex(0.12f).withMargin(juce::FlexItem::Margin(0.f, 10.f, 0.f, 10.f)));
    mainBox.items.add(juce::FlexItem(scaleComponent).withFlex(0.17f).withMargin(juce::FlexItem::Margin(5.f, 10.f, 0.f, 10.f)));
    mainBox.items.add(juce::FlexItem(editorBox).withFlex(1.0f).withMargin(10));
    mainBox.items.add(juce::FlexItem(subdivisionRowBox).withFlex(0.12f).withMargin(juce::FlexItem::Margin(0.f, 10.f, 0.f, 10.f)));
    mainBox.performLayout(bounds);
}
