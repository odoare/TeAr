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

    /** True the first time it is called on this processor, false ever after.
        The editor uses it to show the splash once. The flag has to live here
        rather than on the editor, because the editor is destroyed and rebuilt
        every time the window is closed and reopened. Not serialised: a
        reloaded session is a new run and gets its splash. */
    bool claimSplash() noexcept
    {
        const bool first = ! splashClaimed;
        splashClaimed = true;
        return first;
    }

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

    // --- Versioning ----------------------------------------------------------
    // Two separate numbers, on purpose. "pluginVersion" is informational (the
    // same string the top bar shows, from project(TeAr VERSION ...)), while
    // "patternSyntax" is what migration actually tests: it moves only when the
    // pattern language changes, which the plugin version does not.
    //
    // They live as properties on apvts.state rather than as attributes added
    // at getStateInformation() time, because presets are written straight from
    // apvts.copyState() and would otherwise carry no version at all.

    /** Syntax the patterns in this build are written in. 1 = decimal velocity
        on a level*16 scale; 2 = hexadecimal values (velocity 1-F, degrees
        1-F). */
    static constexpr int currentPatternSyntax = 2;

    static const juce::Identifier pluginVersionProperty;
    static const juce::Identifier patternSyntaxProperty;

    /** Stamps both version properties onto `state`. */
    static void writeVersionTo (juce::ValueTree& state);

    /** Brings every pattern in `arps` up to currentPatternSyntax, given the
        syntax version the state was written with. Absent (0) means v1: the
        state predates versioning entirely. */
    void migrateArpPatterns (int fromSyntax);

    double sampleRate   = 0.0;
    double lastKnownBPM = 120.0;
    bool   wasPlaying   = false;

    // Pending scaleRoot update from processBlock; applied on the message thread via AsyncUpdater.
    std::atomic<int> pendingScaleRootSemitone { -1 };

    juce::Array<int> heldNotes;

    bool splashClaimed = false;   // runtime only, never saved

    // Scratch buffers reused by processBlock to avoid heap allocations on the audio thread.
    // Pre-warmed in prepareToPlay so their internal arrays have allocated capacity for the
    // largest scale/chord we will encounter.
    fxme::MidiTools::Chord scratchPlayedChord { "" };
    fxme::MidiTools::Chord scratchRootChord   { "" };
    fxme::MidiTools::Scale scratchScale       { 0, fxme::MidiTools::Scale::Type::Major };

    void applyChordToAllArps (const fxme::MidiTools::Chord& chord);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TeArAudioProcessor)
};
