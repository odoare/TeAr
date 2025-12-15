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

    addAndMakeVisible(chordMethodBox);
    // Manually populate the ComboBox *before* creating the attachment.
    if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("chordMethod")))
    {
        chordMethodBox.clear();
        chordMethodBox.addItemList(parameter->choices, 1);
    }
    // Now create the attachment, which will sync the selection with the parameter's current value.
    chordMethodAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "chordMethod", chordMethodBox);

    addAndMakeVisible(subdivisionLabel);
    subdivisionLabel.setText("Subdivision", juce::dontSendNotification);
    subdivisionLabel.attachToComponent(&subdivisionBox, true);

    addAndMakeVisible(subdivisionBox);
    // Manually populate the ComboBox *before* creating the attachment.
    if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("subdivision")))
    {
        subdivisionBox.clear();
        subdivisionBox.addItemList(parameter->choices, 1);
    }
    subdivisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "subdivision", subdivisionBox);

    addAndMakeVisible(scaleRootLabel);
    scaleRootLabel.setText("Scale Root", juce::dontSendNotification);
    scaleRootLabel.attachToComponent(&scaleRootBox, true);

    addAndMakeVisible(scaleRootBox);
    if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("scaleRoot")))
    {
        scaleRootBox.clear();
        scaleRootBox.addItemList(parameter->choices, 1);
    }
    scaleRootAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "scaleRoot", scaleRootBox);

    addAndMakeVisible(scaleTypeLabel);
    scaleTypeLabel.setText("Scale Type", juce::dontSendNotification);
    scaleTypeLabel.attachToComponent(&scaleTypeBox, true);

    addAndMakeVisible(scaleTypeBox);
    if (auto* parameter = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("scaleType")))
    {
        scaleTypeBox.clear();
        scaleTypeBox.addItemList(parameter->choices, 1);
    }
    scaleTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "scaleType", scaleTypeBox);

    addAndMakeVisible(followMidiInButton);
    followMidiInButton.setButtonText("Follow MIDI In");
    followMidiInAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "followMidiIn", followMidiInButton);

    // Start the timer to update the UI 30 times per second
    startTimerHz(30);


    setSize (600, 320); // Increased height for the larger text editor
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
    mainBox.items.add(juce::FlexItem(arpeggiatorEditor).withFlex(1.f).withMargin(10));
    mainBox.items.add(juce::FlexItem(hBox1).withFlex(0.15f).withMargin(juce::FlexItem::Margin( 0.f,10.f,0.f,10.f)));
    mainBox.items.add(juce::FlexItem(hBox2).withFlex(0.15f).withMargin(juce::FlexItem::Margin(0.f,10.f,0.f,10.f)));
    mainBox.performLayout(bounds);
}
