/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class TeArAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                  public juce::ChangeListener
{
public:
    TeArAudioProcessorEditor (TeArAudioProcessor&);
    ~TeArAudioProcessorEditor() override;

    //==============================================================================
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    TeArAudioProcessor& audioProcessor;

    juce::Label arpeggiatorLabel;
    
    juce::Label chordMethodLabel;
    juce::ComboBox chordMethodBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> chordMethodAttachment;

    juce::Label subdivisionLabel;
    juce::ComboBox subdivisionBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subdivisionAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TeArAudioProcessorEditor)
};
