#pragma once

#include <JuceHeader.h>
#include <FxmeTools/presets/PresetManager.h>
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
    const fxme::Arpeggiator& getArpeggiator (int index) const;
    bool             areNotesHeld() const;
    juce::Array<int> getHeldNotes() const;

    /** Monotonic note-on count for one arpeggiator; a change between two GUI
        polls means it fired. Returns 0 for an out-of-range index. */
    juce::uint32     getArpeggiatorNoteOnCount (int index) const;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    fxme::PresetManager& getPresetManager() noexcept { return presetManager; }

    static constexpr int MAX_ARPS = 16;

private:
    mutable juce::CriticalSection arpsLock;
    std::vector<ArpInstance>      arps;

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    fxme::PresetManager presetManager;

    // --- Arpeggiator side state ---------------------------------------------
    // The arpeggiators are dynamic (variable count, text patterns), so they
    // cannot be APVTS parameters and live in `arps` instead. They are mirrored
    // into an "Arpeggiators" child of apvts.state so that presets, which only
    // ever carry the APVTS state, capture them too.

    /** Snapshot of `arps` as a detached tree. Takes arpsLock; safe to call
        from the thread that serialises state. */
    juce::ValueTree buildArpsTree() const;

    /** Message thread: refresh the "Arpeggiators" child of the live
        apvts.state. Also what marks the preset dirty after an edit. */
    void storeArpsInState();

    /** Rebuilds `arps` from a tree produced by buildArpsTree(). */
    void loadArpsFromTree (const juce::ValueTree& tree);

    /** Reads each arpeggiator's on/off from its APVTS parameter, which stays
        authoritative (it is the automatable one). */
    void syncArpOnStatesFromParameters();

    void loadLegacyArps (const juce::XmlElement& xmlState);

    double sampleRate   = 0.0;
    double lastKnownBPM = 120.0;
    bool   wasPlaying   = false;

    // Pending scaleRoot update from processBlock; applied on the message thread via AsyncUpdater.
    std::atomic<int> pendingScaleRootSemitone { -1 };

    juce::Array<int> heldNotes;

    // Scratch buffers reused by processBlock to avoid heap allocations on the audio thread.
    // Pre-warmed in prepareToPlay so their internal arrays have allocated capacity for the
    // largest scale/chord we will encounter.
    fxme::MidiTools::Chord scratchPlayedChord { "" };
    fxme::MidiTools::Chord scratchRootChord   { "" };
    fxme::MidiTools::Scale scratchScale       { 0, fxme::MidiTools::Scale::Type::Major };

    void applyChordToAllArps (const fxme::MidiTools::Chord& chord);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TeArAudioProcessor)
};
