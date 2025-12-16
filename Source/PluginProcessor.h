/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "libs/cppMusicTools/Arpeggiator.h"

//==============================================================================
/**
*/
class TeArAudioProcessor  : public juce::AudioProcessor
                          , public juce::ChangeBroadcaster
                          , public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    TeArAudioProcessor();
    ~TeArAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Getter and Setter for our custom string parameter
    void setArpeggiatorPattern (int index, const juce::String& pattern);
    const juce::String& getArpeggiatorPattern(int index) const;
    bool isArpeggiatorOn(int index) const;

    // Getter for the UI to know the current step
    int getArpeggiatorCurrentStep(int index) const;

    // Getter for the UI to access arpeggiator methods
    const Arpeggiator& getArpeggiator(int index) const;

    // Getter for the UI to know if notes are being held
    bool areNotesHeld() const;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
    juce::StringArray arpeggiatorPatterns;

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    double lastKnownBPM = 120.0;
    bool wasPlaying = false;
    juce::Array<bool> arpeggiatorOnStates;
    juce::Array<int> arpeggiatorMidiChannels;
    
    juce::Array<Arpeggiator> arpeggiators;
    juce::Array<int> heldNotes;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TeArAudioProcessor)
};
