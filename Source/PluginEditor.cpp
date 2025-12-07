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
    // This lambda is called when the user finishes editing the label text
    auto onTextChange = [this]
    {
        audioProcessor.setArpeggiatorPattern (arpeggiatorLabel.getText());
    };

    arpeggiatorLabel.setEditable(true);
    arpeggiatorLabel.setJustificationType(juce::Justification::centred);
    arpeggiatorLabel.onEditorShow = [this] { arpeggiatorLabel.getCurrentTextEditor()->setInputRestrictions(0); }; // Allow any characters
    arpeggiatorLabel.onTextChange = onTextChange;
    addAndMakeVisible(arpeggiatorLabel);

    arpeggiatorLabel.setText (audioProcessor.getArpeggiatorPattern(), juce::dontSendNotification);

    addAndMakeVisible(chordMethodLabel);
    chordMethodLabel.setText("Chord Method", juce::dontSendNotification);
    chordMethodLabel.attachToComponent(&chordMethodBox, true);

    addAndMakeVisible(chordMethodBox);
    chordMethodBox.addItem("Notes played", 1);
    chordMethodBox.setSelectedId(1);

    setSize (400, 300);
}

TeArAudioProcessorEditor::~TeArAudioProcessorEditor()
{
}

//==============================================================================
void TeArAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void TeArAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    
    auto topArea = bounds.removeFromTop(40);
    chordMethodBox.setBounds(topArea.withLeft(100));

    arpeggiatorLabel.setBounds(bounds);
}
