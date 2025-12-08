/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TeArAudioProcessorEditor::TeArAudioProcessorEditor (TeArAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    audioProcessor.addChangeListener(this);

    auto& apvts = audioProcessor.getAPVTS();

    // This lambda is called when the user finishes editing the label text
    auto onTextChange = [this] { audioProcessor.setArpeggiatorPattern(arpeggiatorLabel.getText()); };

    arpeggiatorLabel.setEditable(true);
    arpeggiatorLabel.setJustificationType(juce::Justification::centred);
    arpeggiatorLabel.onEditorShow = [this] { arpeggiatorLabel.getCurrentTextEditor()->setInputRestrictions(0); }; // Allow any characters
    arpeggiatorLabel.setFont(juce::Font(24.0f));
    arpeggiatorLabel.onTextChange = onTextChange;
    addAndMakeVisible(arpeggiatorLabel);
    arpeggiatorLabel.setText(audioProcessor.getArpeggiatorPattern(), juce::dontSendNotification);

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


    setSize (400, 300);
}

TeArAudioProcessorEditor::~TeArAudioProcessorEditor()
{
    audioProcessor.removeChangeListener(this);
}

void TeArAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    // When the processor tells us something changed, update our manual controls.
    arpeggiatorLabel.setText(audioProcessor.getArpeggiatorPattern(), juce::dontSendNotification);
}

//==============================================================================
void TeArAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void TeArAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(15);

    auto topRow = bounds.removeFromTop(40);
    auto leftColumn = topRow.removeFromLeft(topRow.getWidth() / 2).reduced(5, 0);
    auto rightColumn = topRow.reduced(5, 0);

    chordMethodBox.setBounds(leftColumn.withLeft(leftColumn.getX() + 90));
    subdivisionBox.setBounds(rightColumn.withLeft(rightColumn.getX() + 80));

    arpeggiatorLabel.setBounds(bounds);
}
