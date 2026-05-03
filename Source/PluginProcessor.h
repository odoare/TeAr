#pragma once

#include <JuceHeader.h>
#include "ArpInstance.h"

class TeArAudioProcessor : public juce::AudioProcessor
                         , public juce::ChangeBroadcaster
                         , public juce::AudioProcessorValueTreeState::Listener
                         , private juce::AsyncUpdater
{
public:
    TeArAudioProcessor();
    ~TeArAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

    // --- Arpeggiator management (UI thread) ---
    int  getNumArpeggiators() const;
    void addArpeggiator();
    void removeArpeggiator (int index);

    // --- Per-arp accessors ---
    void setArpeggiatorPattern (int index, const juce::String& pattern);
    const juce::String& getArpeggiatorPattern (int index) const;
    void randomizeArpeggiator (int index);

    void setArpeggiatorOn (int index, bool on);
    bool isArpeggiatorOn (int index) const;

    void setArpeggiatorMidiChannel (int index, int channel);
    int  getArpeggiatorMidiChannel (int index) const;

    void setArpeggiatorSubdivision (int index, int subdivIndex);
    int  getArpeggiatorSubdivision (int index) const;

    // --- UI helpers (read-only, called from message thread) ---
    int              getArpeggiatorCurrentStep (int index) const;
    const Arpeggiator& getArpeggiator (int index) const;
    bool             areNotesHeld() const;
    juce::Array<int> getHeldNotes() const;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    static constexpr int MAX_ARPS = 16;

private:
    mutable juce::CriticalSection arpsLock;
    std::vector<ArpInstance>      arps;

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    double sampleRate   = 0.0;
    double lastKnownBPM = 120.0;
    bool   wasPlaying   = false;

    // Pending scaleRoot update from processBlock; applied on the message thread via AsyncUpdater.
    std::atomic<int> pendingScaleRootSemitone { -1 };

    juce::Array<int> heldNotes;

    void applyChordToAllArps (const MidiTools::Chord& chord);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TeArAudioProcessor)
};
