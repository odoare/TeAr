/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
void TeArAudioProcessorEditor::ArpLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor& /*editor*/)
{
    g.setColour (juce::Colours::darkblue.darker(2.f));
    g.fillRoundedRectangle (juce::Rectangle<int>(0,0, width, height).toFloat(), 10.0f);
    g.setColour (juce::Colours::green);
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

    g.setColour(juce::Colours::green);
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

    auto& apvts = audioProcessor.getAPVTS();

    addAndMakeVisible(arpeggiatorEditor);
    arpeggiatorEditor.setLookAndFeel(&arpLookAndFeel);
    arpeggiatorEditor.setMultiLine(true);
    arpeggiatorEditor.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 24.0f, juce::Font::plain));
    arpeggiatorEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack); // Background is drawn by LookAndFeel
    arpeggiatorEditor.setColour(juce::TextEditor::textColourId, juce::Colours::lime);
    arpeggiatorEditor.setColour(juce::CaretComponent::caretColourId, juce::Colours::green);
    arpeggiatorEditor.setColour(juce::TextEditor::highlightColourId, juce::Colours::lime);
    arpeggiatorEditor.setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::black);
    arpeggiatorEditor.setText(audioProcessor.getArpeggiatorPattern(), false);

    // This lambda contains the logic to validate and update the pattern.
    auto validateAndSetPattern = [this] {
        // Send the raw text from the editor (with newlines) directly to the processor.
        audioProcessor.setArpeggiatorPattern(arpeggiatorEditor.getText());
    };

    // When Return is pressed, validate the pattern and give focus back to the main editor.
    arpeggiatorEditor.onReturnKey = [this, validateAndSetPattern] {
        validateAndSetPattern();
        giveAwayKeyboardFocus(); // Or `getRootComponent()->grabKeyboardFocus()`
    };
    // Also validate when the editor loses focus (e.g., user clicks away).
    arpeggiatorEditor.onFocusLost = validateAndSetPattern;

    addAndMakeVisible(chordMethodLabel);
    chordMethodLabel.setText("Chord Method", juce::dontSendNotification);
    chordMethodLabel.attachToComponent(&chordMethodBox, true);
    chordMethodLabel.setLookAndFeel(&arpLookAndFeel);
    chordMethodLabel.setColour(juce::Label::textColourId, juce::Colours::lime);

    addAndMakeVisible(chordMethodBox);
    // Manually populate the ComboBox *before* creating the attachment.
    chordMethodBox.setLookAndFeel(&arpLookAndFeel);
    chordMethodBox.setColour(juce::ComboBox::textColourId, juce::Colours::lime);
    chordMethodBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);

    if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("chordMethod")))
    {
        chordMethodBox.clear();
        chordMethodBox.addItemList(parameter->choices, 1);
    }
    // Now create the attachment, which will sync the selection with the parameter's current value.
    chordMethodAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "chordMethod", chordMethodBox);
    chordMethodBox.onChange = [this] { updateScaleDisplay(); };

    addAndMakeVisible(subdivisionLabel);
    subdivisionLabel.setText("Subdivision", juce::dontSendNotification);
    subdivisionLabel.attachToComponent(&subdivisionBox, true);
    subdivisionLabel.setLookAndFeel(&arpLookAndFeel);
    subdivisionLabel.setColour(juce::Label::textColourId, juce::Colours::lime);

    addAndMakeVisible(subdivisionBox);
    // Manually populate the ComboBox *before* creating the attachment.
    subdivisionBox.setLookAndFeel(&arpLookAndFeel);
    subdivisionBox.setColour(juce::ComboBox::textColourId, juce::Colours::lime);
    subdivisionBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);

    if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("subdivision")))
    {
        subdivisionBox.clear();
        subdivisionBox.addItemList(parameter->choices, 1);
    }
    subdivisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "subdivision", subdivisionBox);

    addAndMakeVisible(scaleRootLabel);
    scaleRootLabel.setText("Scale Root", juce::dontSendNotification);
    scaleRootLabel.attachToComponent(&scaleRootBox, true);
    scaleRootLabel.setLookAndFeel(&arpLookAndFeel);
    scaleRootLabel.setColour(juce::Label::textColourId, juce::Colours::lime);

    addAndMakeVisible(scaleRootBox);
    scaleRootBox.setLookAndFeel(&arpLookAndFeel);
    scaleRootBox.setColour(juce::ComboBox::textColourId, juce::Colours::lime);
    scaleRootBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);

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
    scaleTypeLabel.setColour(juce::Label::textColourId, juce::Colours::lime);

    addAndMakeVisible(scaleTypeBox);
    scaleTypeBox.setLookAndFeel(&arpLookAndFeel);
    scaleTypeBox.setColour(juce::ComboBox::textColourId, juce::Colours::lime);
    scaleTypeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);

    if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("scaleType")))
    {
        scaleTypeBox.clear();
        scaleTypeBox.addItemList(parameter->choices, 1);
    }
    scaleTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "scaleType", scaleTypeBox);
    scaleTypeBox.onChange = [this] { updateScaleDisplay(); };

    addAndMakeVisible(followMidiInButton);
    followMidiInButton.setButtonText("Follow MIDI In");
    followMidiInAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "followMidiIn", followMidiInButton);

    addAndMakeVisible(scaleComponent);

    // Start the timer to update the UI 30 times per second
    startTimerHz(30);

    // Initial update of the scale component to show the default scale
    updateScaleDisplay();


    setSize (600, 360); // Increased height for the scale component
}

TeArAudioProcessorEditor::~TeArAudioProcessorEditor()
{
    audioProcessor.removeChangeListener(this);
    arpeggiatorEditor.setLookAndFeel(nullptr);
    stopTimer();
}

void TeArAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    // When the processor tells us something changed, update our manual controls.
    arpeggiatorEditor.setText(audioProcessor.getArpeggiatorPattern(), false);
}

void TeArAudioProcessorEditor::timerCallback()
{
    const auto& arpeggiator = audioProcessor.getArpeggiator();
    const auto& chord = arpeggiator.getChord();
    const int currentNote = arpeggiator.getLastPlayedNote();

    const bool notesAreHeld = audioProcessor.areNotesHeld();

    if (notesAreHeld)
    {
        // Arpeggiator is active (playing a note or on a rest)
        auto chordMethod = static_cast<int>(audioProcessor.getAPVTS().getRawParameterValue("chordMethod")->load());
        const auto& notes = (chordMethod == 1) ? chord.getRawNotes() : chord.getDegrees();

        if (!notes.isEmpty())
        {
            // currentNote will be -1 on a rest, which is what updateScale expects.
            scaleComponent.updateScale(notes, notes.getFirst(), currentNote);
        }
    }
    else
    {
        // Arpeggiator is fully stopped.
        updateScaleDisplay();
    }

    // Only show the highlight if the user is NOT editing the text.
    if (!arpeggiatorEditor.hasKeyboardFocus(true))
    {
        int currentStep = audioProcessor.getArpeggiatorCurrentStep();
    
        // Only update if the step has changed to avoid unnecessary redrawing
        if (currentStep != lastStepIndex)
        {
            lastStepIndex = currentStep;
    
            const auto pattern = audioProcessor.getArpeggiatorPattern();
            int stepStart = audioProcessor.getArpeggiator().getPatternIndexForStep(currentStep);
            int stepEnd = audioProcessor.getArpeggiator().getPatternIndexForStep(currentStep + 1);
    
            // If the next step is at index 0, it means we've looped, so highlight to the end.
            if (stepEnd <= stepStart)
                stepEnd = pattern.length();
    
            arpeggiatorEditor.setHighlightedRegion({ stepStart, stepEnd });
        }
    }
    else if (lastStepIndex != -1) // If we are editing, clear the highlight.
    {
        arpeggiatorEditor.setHighlightedRegion({});
        lastStepIndex = -1; // Reset to ensure highlight updates immediately when focus is lost.
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
        scaleComponent.updateScale(currentDisplayScale.getNotes(), currentDisplayScale.getRootNote(), -1);
    }
    else // For "Notes played" or "Chord played as is", clear the display when not playing.
        scaleComponent.updateScale({}, -1, -1);

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

    juce::FlexBox mainBox;
    mainBox.flexDirection = juce::FlexBox::Direction::column;
    juce::FlexBox hBox1, hBox2;
    hBox1.flexDirection = juce::FlexBox::Direction::row;
    hBox2.flexDirection = juce::FlexBox::Direction::row;

    hBox1.items.add(juce::FlexItem(subdivisionLabel).withFlex(0.8f));
    hBox1.items.add(juce::FlexItem(subdivisionBox).withFlex(.5f));
    hBox1.items.add(juce::FlexItem(chordMethodLabel).withFlex(1.f));
    hBox1.items.add(juce::FlexItem(chordMethodBox).withFlex(1.2f));
    hBox2.items.add(juce::FlexItem(scaleRootLabel).withFlex(0.8f));
    hBox2.items.add(juce::FlexItem(scaleRootBox).withFlex(.5f));
    hBox2.items.add(juce::FlexItem(followMidiInButton).withFlex(0.6f).withMargin(juce::FlexItem::Margin(0.f, 0.f, 0.f, 5.f)));
    hBox2.items.add(juce::FlexItem(scaleTypeLabel).withFlex(0.6f));
    hBox2.items.add(juce::FlexItem(scaleTypeBox).withFlex(1.f));
    mainBox.items.add(juce::FlexItem(scaleComponent).withFlex(0.2f).withMargin(juce::FlexItem::Margin(5.f, 10.f, 0.f, 10.f)));
    mainBox.items.add(juce::FlexItem(arpeggiatorEditor).withFlex(1.f).withMargin(10));
    mainBox.items.add(juce::FlexItem(hBox1).withFlex(0.12f).withMargin(juce::FlexItem::Margin( 0.f,10.f,0.f,10.f)));
    mainBox.items.add(juce::FlexItem(hBox2).withFlex(0.12f).withMargin(juce::FlexItem::Margin(0.f,10.f,0.f,10.f)));
    mainBox.performLayout(bounds);
}
