#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    /** std::vector indexes with an unsigned type, while every index in this file
        is an int that a caller has already bounds-checked. Converting in one
        named place keeps the call sites readable and keeps the build free of
        the sign-conversion warnings that used to drown out everything else.

        The assertion is free in a release build and catches a negative index in
        a debug one, which a bare (size_t) cast at each site would not. */
    inline std::size_t asIndex (int i) noexcept
    {
        jassert (i >= 0);
        return (std::size_t) i;
    }
}

//==============================================================================
TeArAudioProcessor::TeArAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
     #endif
    , apvts (*this, nullptr, "Parameters", createParameters())
    , presetManager (apvts,
                     fxme::PresetManager::getDefaultUserPresetDirectory ("TeAr"),
                     BinaryData::namedResourceList,
                     BinaryData::namedResourceListSize,
                     BinaryData::getNamedResource)
{
    // Default: 4 arpeggiators (matches v1 behavior)
    for (int i = 0; i < 4; ++i)
    {
        ArpInstance arp;
        arp.midiChannel = i + 1;
        arps.push_back (std::move (arp));
    }

    apvts.addParameterListener ("chordMethod", this);
    for (int i = 0; i < MAX_ARPS; ++i)
        apvts.addParameterListener ("arp" + juce::String (i) + "On", this);

    int chordMethod = (int) apvts.getRawParameterValue ("chordMethod")->load();
    for (auto& arp : arps)
        arp.engine.setChordMethod (chordMethod);

    // A preset only carries apvts.state, so the arpeggiators have to be folded
    // in on the way out and rebuilt on the way back in.
    presetManager.onBeforeSave = [this]
    {
        storeArpsInState();
        writeVersionTo (apvts.state);
    };
    presetManager.onAfterLoad  = [this]
    {
        // Read the preset's syntax version before anything rewrites it.
        const int syntax = (int) apvts.state.getProperty (patternSyntaxProperty, 0);

        auto tree = apvts.state.getChildWithName ("Arpeggiators");
        if (tree.isValid())
            loadArpsFromTree (tree);

        migrateArpPatterns (syntax);
        syncArpOnStatesFromParameters();
        sendChangeMessage();
    };
    // Deliberately not seeding the state here: writing to apvts.state is what
    // marks a preset dirty, and a freshly opened plugin has not been edited.
    // getStateInformation() and onBeforeSave both build the tree on demand.
}

TeArAudioProcessor::~TeArAudioProcessor()
{
    apvts.removeParameterListener ("chordMethod", this);
    for (int i = 0; i < MAX_ARPS; ++i)
        apvts.removeParameterListener ("arp" + juce::String (i) + "On", this);
}

//==============================================================================
const juce::String TeArAudioProcessor::getName() const  { return JucePlugin_Name; }

bool TeArAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TeArAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TeArAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TeArAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int  TeArAudioProcessor::getNumPrograms()                              { return 1; }
int  TeArAudioProcessor::getCurrentProgram()                           { return 0; }
void TeArAudioProcessor::setCurrentProgram (int)                       {}
const juce::String TeArAudioProcessor::getProgramName (int)            { return {}; }
void TeArAudioProcessor::changeProgramName (int, const juce::String&)  {}

//==============================================================================
void TeArAudioProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    // auval / Logic may call this with zero values during validation.
    if (sr <= 0 || samplesPerBlock <= 0)
        return;

    sampleRate = sr;

    juce::ScopedLock lock (arpsLock);

    // Pre-allocate held-notes storage so addIfNotAlreadyThere never reallocates.
    heldNotes.ensureStorageAllocated (128);

    // Pre-warm scratch chord/scale buffers. We use the largest scale (8-note octatonic)
    // and the largest possible held-notes count so that the internal juce::Arrays grow to
    // the maximum capacity we will ever need. Subsequent in-place updates from processBlock
    // then reuse this storage without allocating.
    scratchScale.reset (0, fxme::MidiTools::Scale::Type::OctatonicHalfWhole);
    scratchPlayedChord.ensureCapacity (16, 128);
    scratchRootChord  .ensureCapacity (16, 128);
    scratchPlayedChord.setFromScaleAndDegree (scratchScale, 0);
    scratchRootChord  .setFromScaleAndDegree (scratchScale, 0);

    for (auto& arp : arps)
    {
        arp.engine.prepareToPlay (sampleRate);
        arp.engine.setPattern (arp.pattern);
        // Pre-warm each engine's internal chord storage to match the scratch buffers.
        arp.engine.setChord     (scratchPlayedChord);
        arp.engine.setRootChord (scratchRootChord);
    }
}

void TeArAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TeArAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif
    return true;
  #endif
}
#endif

void TeArAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ScopedTryLock lock (arpsLock);
    if (! lock.isLocked())
    {
        midiMessages.clear();
        return;
    }

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    bool transportJustStopped = false;

    // --- Host transport ---
    if (auto* ph = getPlayHead())
    {
        juce::AudioPlayHead::CurrentPositionInfo positionInfo;
        JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")
        if (ph->getCurrentPosition (positionInfo))
        JUCE_END_IGNORE_WARNINGS_GCC_LIKE
        {
            if (positionInfo.bpm > 0.0)
            {
                lastKnownBPM = positionInfo.bpm;
                for (auto& arp : arps) arp.engine.setTempo (lastKnownBPM);
            }

            if (positionInfo.isPlaying)
                for (auto& arp : arps) arp.engine.syncToPlayHead (positionInfo);
            else if (wasPlaying)
                transportJustStopped = true;

            wasPlaying = positionInfo.isPlaying;
        }
    }

    // --- Track held notes ---
    bool notesChanged = false;
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            heldNotes.addIfNotAlreadyThere (msg.getNoteNumber());
            for (auto& arp : arps)
                if (arp.onState)
                    arp.engine.setPlayedVelocityFromMidi (msg.getVelocity());
            notesChanged = true;
        }
        else if (msg.isNoteOff())
        {
            heldNotes.removeFirstMatchingValue (msg.getNoteNumber());
            notesChanged = true;
        }
    }

    if (notesChanged)
    {
        scratchPlayedChord.reset();
        auto chordMethod = (int) apvts.getRawParameterValue ("chordMethod")->load();

        switch (chordMethod)
        {
            case 0:
                scratchPlayedChord.setDegreesByArray (heldNotes);
                break;
            case 1:
                scratchPlayedChord.setNotesByArray (heldNotes);
                break;
            case 2:
                if (!heldNotes.isEmpty())
                {
                    int lastNote = heldNotes.getLast();
                    int lastNoteSemitone = lastNote % 12;
                    auto followMidiIn   = apvts.getRawParameterValue ("followMidiIn")->load();
                    auto scaleTypeIndex = (int) apvts.getRawParameterValue ("scaleType")->load();
                    auto scaleType      = static_cast<fxme::MidiTools::Scale::Type> (scaleTypeIndex);

                    scratchRootChord.reset();

                    if (followMidiIn > 0.5f)
                    {
                        pendingScaleRootSemitone.store (lastNoteSemitone);
                        triggerAsyncUpdate();
                        scratchScale.reset (lastNoteSemitone, scaleType);
                        for (auto& arp : arps)
                            if (arp.onState) arp.engine.setBaseOctaveFromNote (lastNote);
                        scratchPlayedChord.setFromScaleAndDegree (scratchScale, 0);
                        // With followMidiIn the pressed note IS the root, so rootChord == playedChord
                        scratchRootChord.setFromScaleAndDegree (scratchScale, 0);
                    }
                    else
                    {
                        auto rootNoteIndex = (int) apvts.getRawParameterValue ("scaleRoot")->load();
                        scratchScale.reset (rootNoteIndex, scaleType);
                        // rootChord is always degree-0 of the fixed scale
                        scratchRootChord.setFromScaleAndDegree (scratchScale, 0);

                        const auto& scaleNotes = scratchScale.getNotes();
                        int degree = scaleNotes.indexOf (lastNoteSemitone);

                        if (degree != -1)
                        {
                            for (auto& arp : arps)
                                if (arp.onState) arp.engine.setBaseOctaveFromNote (lastNote);
                            scratchPlayedChord.setFromScaleAndDegree (scratchScale, degree);
                        }
                        else
                        {
                            int nearestDegree = -1;
                            for (int i = 1; i < 12; ++i)
                            {
                                int semitoneToTest = (lastNoteSemitone - i + 12) % 12;
                                int foundDegree = scaleNotes.indexOf (semitoneToTest);
                                if (foundDegree != -1) { nearestDegree = foundDegree; break; }
                            }
                            for (auto& arp : arps)
                                if (arp.onState) arp.engine.setBaseOctaveFromNote (lastNote);
                            scratchPlayedChord.setFromScaleAndDegree (scratchScale,
                                          nearestDegree != -1 ? nearestDegree : 0);
                        }
                    }

                    for (auto& arp : arps)
                        if (arp.onState) arp.engine.setRootChord (scratchRootChord);
                }
                break;
        }

        for (auto& arp : arps)
            if (arp.onState) arp.engine.setChord (scratchPlayedChord);
    }

    midiMessages.clear();

    if (notesChanged && heldNotes.isEmpty())
    {
        for (auto& arp : arps)
            if (arp.onState)
                midiMessages.addEvents (arp.engine.turnOff (arp.midiChannel), 0, -1, 0);
    }

    if (transportJustStopped)
    {
        for (auto& arp : arps)
            if (arp.onState)
                midiMessages.addEvents (arp.engine.reset (arp.midiChannel), 0, -1, 0);
    }

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (!heldNotes.isEmpty())
        for (auto& arp : arps)
            if (arp.onState)
                midiMessages.addEvents (arp.engine.processBlock (buffer.getNumSamples(), arp.midiChannel), 0, -1, 0);
}

//==============================================================================
bool TeArAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* TeArAudioProcessor::createEditor()
{
    return new TeArAudioProcessorEditor (*this);
}

//==============================================================================
const juce::Identifier TeArAudioProcessor::pluginVersionProperty  { "pluginVersion" };
const juce::Identifier TeArAudioProcessor::patternSyntaxProperty  { "patternSyntax" };

void TeArAudioProcessor::writeVersionTo (juce::ValueTree& state)
{
    state.setProperty (pluginVersionProperty, JucePlugin_VersionString, nullptr);
    state.setProperty (patternSyntaxProperty, currentPatternSyntax,     nullptr);
}

void TeArAudioProcessor::migrateArpPatterns (int fromSyntax)
{
    if (fromSyntax >= currentPatternSyntax)
        return;

    juce::ScopedLock lock (arpsLock);

    for (auto& arp : arps)
    {
        arp.pattern = fxme::Arpeggiator::migratePatternV1toV2 (arp.pattern);
        arp.engine.setPattern (arp.pattern);
    }
}

juce::ValueTree TeArAudioProcessor::buildArpsTree() const
{
    juce::ScopedLock lock (arpsLock);

    juce::ValueTree tree ("Arpeggiators");
    tree.setProperty ("count", (int) arps.size(), nullptr);

    for (int i = 0; i < (int) arps.size(); ++i)
    {
        juce::ValueTree arpTree ("Arp");
        arpTree.setProperty ("index",   i,               nullptr);
        arpTree.setProperty ("pattern", arps[asIndex (i)].pattern, nullptr);

        // The on/off state is an automatable parameter, so the parameter wins
        // over the cached copy in the instance.
        auto* onParam = juce::isPositiveAndBelow (i, MAX_ARPS)
                            ? apvts.getRawParameterValue ("arp" + juce::String (i) + "On")
                            : nullptr;
        arpTree.setProperty ("on", onParam ? (*onParam > 0.5f ? 1 : 0)
                                           : (arps[asIndex (i)].onState ? 1 : 0), nullptr);
        arpTree.setProperty ("midiChannel", arps[asIndex (i)].midiChannel, nullptr);
        arpTree.setProperty ("subdivision", arps[asIndex (i)].subdivision, nullptr);
        tree.appendChild (arpTree, nullptr);
    }

    return tree;
}

void TeArAudioProcessor::storeArpsInState()
{
    auto existing = apvts.state.getChildWithName ("Arpeggiators");
    if (existing.isValid())
        apvts.state.removeChild (existing, nullptr);
    apvts.state.appendChild (buildArpsTree(), nullptr);
}

void TeArAudioProcessor::loadArpsFromTree (const juce::ValueTree& tree)
{
    juce::ScopedLock lock (arpsLock);

    arps.clear();
    const int chordMethod = (int) apvts.getRawParameterValue ("chordMethod")->load();

    for (const auto& arpTree : tree)
    {
        if (! arpTree.hasType ("Arp"))
            continue;
        if ((int) arps.size() >= MAX_ARPS)
            break;

        ArpInstance arp;
        arp.pattern     = arpTree.getProperty ("pattern",     "1 2 3").toString();
        arp.onState     = (int) arpTree.getProperty ("on",          1) != 0;
        arp.midiChannel = juce::jlimit (1, 16, (int) arpTree.getProperty ("midiChannel", 1));
        arp.subdivision = (int) arpTree.getProperty ("subdivision", 4);

        arp.engine.prepareToPlay  (sampleRate);
        arp.engine.setPattern     (arp.pattern);
        arp.engine.setSubdivision (arp.subdivision);
        arp.engine.setTempo       (lastKnownBPM);
        arp.engine.setChordMethod (chordMethod);
        arps.push_back (std::move (arp));
    }

    // Safety: ensure at least one arp
    if (arps.empty())
        arps.push_back (ArpInstance{});
}

void TeArAudioProcessor::syncArpOnStatesFromParameters()
{
    juce::ScopedLock lock (arpsLock);

    for (int i = 0; i < (int) arps.size() && i < MAX_ARPS; ++i)
        if (auto* val = apvts.getRawParameterValue ("arp" + juce::String (i) + "On"))
            arps[asIndex (i)].onState = (*val > 0.5f);
}

void TeArAudioProcessor::loadLegacyArps (const juce::XmlElement& xmlState)
{
    // v1: 4 fixed arps, params stored as APVTS PARAMs + attributes on the root.
    auto getParam = [&xmlState] (const juce::String& paramId, float def) -> float
    {
        for (auto* child : xmlState.getChildIterator())
            if (child->hasTagName ("PARAM") && child->getStringAttribute ("id") == paramId)
                return (float) child->getDoubleAttribute ("value", def);
        return def;
    };

    juce::ScopedLock lock (arpsLock);

    arps.clear();
    for (int i = 0; i < 4; ++i)
    {
        ArpInstance arp;
        arp.pattern     = xmlState.getStringAttribute ("arpeggiatorPattern" + juce::String (i), "1 2 3");
        arp.onState     = xmlState.getBoolAttribute   ("arpOn"             + juce::String (i), true);
        arp.midiChannel = (int) getParam ("midiChannel" + juce::String (i + 1), (float) (i + 1));
        arp.subdivision = (int) getParam ("subdivision" + juce::String (i + 1), 4.0f);

        arp.engine.prepareToPlay  (sampleRate);
        arp.engine.setPattern     (arp.pattern);
        arp.engine.setSubdivision (arp.subdivision);
        arp.engine.setTempo       (lastKnownBPM);
        arp.engine.setChordMethod ((int) apvts.getRawParameterValue ("chordMethod")->load());
        arps.push_back (std::move (arp));
    }
}

//==============================================================================
void TeArAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // copyState() is a deep copy, so the Arpeggiators child can be refreshed
    // here without touching the live tree the GUI is reading.
    auto state = apvts.copyState();

    auto existing = state.getChildWithName ("Arpeggiators");
    if (existing.isValid())
        state.removeChild (existing, nullptr);
    state.appendChild (buildArpsTree(), nullptr);
    writeVersionTo (state);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TeArAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState == nullptr) return;
    if (! xmlState->hasTagName (apvts.state.getType())) return;

    apvts.replaceState (juce::ValueTree::fromXml (*xmlState));

    // Absent means the state predates versioning, i.e. syntax 1.
    const int syntax = (int) apvts.state.getProperty (patternSyntaxProperty, 0);

    auto arpsTree = apvts.state.getChildWithName ("Arpeggiators");
    if (arpsTree.isValid())
        loadArpsFromTree (arpsTree);
    else
        loadLegacyArps (*xmlState);   // v1 by definition

    migrateArpPatterns (syntax);
    syncArpOnStatesFromParameters();
    sendChangeMessage();
}

//==============================================================================
void TeArAudioProcessor::handleAsyncUpdate()
{
    int semitone = pendingScaleRootSemitone.exchange (-1);
    if (semitone >= 0)
        apvts.getParameter ("scaleRoot")->setValueNotifyingHost (semitone / 11.0f);
}

//==============================================================================
// Arpeggiator management
int TeArAudioProcessor::getNumArpeggiators() const
{
    juce::ScopedLock lock (arpsLock);
    return (int) arps.size();
}

void TeArAudioProcessor::addArpeggiator()
{
    {
        juce::ScopedLock lock (arpsLock);
        int newIdx = (int) arps.size();
        ArpInstance arp;
        arp.midiChannel = juce::jmin (newIdx + 1, 16);
        if (juce::isPositiveAndBelow (newIdx, MAX_ARPS))
            if (auto* val = apvts.getRawParameterValue ("arp" + juce::String (newIdx) + "On"))
                arp.onState = (*val > 0.5f);
        arp.engine.prepareToPlay (sampleRate);
        arp.engine.setSubdivision (arp.subdivision);
        arp.engine.setTempo       (lastKnownBPM);
        arp.engine.setChordMethod ((int) apvts.getRawParameterValue ("chordMethod")->load());
        arp.engine.setPattern     (arp.pattern);
        arps.push_back (std::move (arp));
    }
    storeArpsInState();
    sendChangeMessage();
}

void TeArAudioProcessor::removeArpeggiator (int index)
{
    int remaining = 0;
    {
        juce::ScopedLock lock (arpsLock);
        if ((int) arps.size() <= 1) return;
        if (!juce::isPositiveAndBelow (index, (int) arps.size())) return;
        arps.erase (arps.begin() + index);
        remaining = (int) arps.size();
    }

    // The on/off parameters are indexed by slot, not by instance, so erasing
    // one in the middle would otherwise leave every arpeggiator after it
    // wearing its neighbour's on/off state. Shift them down to follow the
    // instances. Ascending order is safe: writing slot i never disturbs i + 1.
    for (int i = index; i < remaining && i + 1 < MAX_ARPS; ++i)
    {
        auto* source = apvts.getRawParameterValue ("arp" + juce::String (i + 1) + "On");
        if (auto* target = dynamic_cast<juce::AudioParameterBool*> (
                apvts.getParameter ("arp" + juce::String (i) + "On")))
            if (source != nullptr)
                target->setValueNotifyingHost (*source > 0.5f ? 1.0f : 0.0f);
    }

    syncArpOnStatesFromParameters();
    storeArpsInState();
    sendChangeMessage();
}

//==============================================================================
// Per-arp accessors

void TeArAudioProcessor::setArpeggiatorPattern (int index, const juce::String& pattern)
{
    {
        juce::ScopedLock lock (arpsLock);
        if (!juce::isPositiveAndBelow (index, (int) arps.size())) return;
        arps[asIndex (index)].pattern = pattern;
        arps[asIndex (index)].engine.setPattern (pattern);

        if (!wasPlaying && arps[asIndex (index)].onState)
        {
            double master = -1.0;
            for (int i = 0; i < (int) arps.size(); ++i)
                if (i != index && arps[asIndex (i)].onState) { master = arps[asIndex (i)].engine.getSamplesUntilNextNote(); break; }
            if (master >= 0.0)
                arps[asIndex (index)].engine.setSamplesUntilNextNote (master);
        }
    }
    storeArpsInState();
    sendChangeMessage();
}

juce::String TeArAudioProcessor::getArpeggiatorPattern (int index) const
{
    // By value, not by reference. A reference would outlive the lock that
    // guards it: setStateInformation may run on a host thread, and its
    // loadArpsFromTree clears `arps`, destroying the referent while the editor
    // is still reading it. The copy is made before the ScopedLock is released,
    // and it costs an atomic refcount bump rather than a character copy,
    // because juce::String is copy-on-write over a std::atomic<int> refCount.
    // That also keeps the buffer alive if another thread reassigns the
    // original afterwards.
    juce::ScopedLock lock (arpsLock);
    if (juce::isPositiveAndBelow (index, (int) arps.size()))
        return arps[asIndex (index)].pattern;
    return {};
}

void TeArAudioProcessor::randomizeArpeggiator (int index)
{
    {
        juce::ScopedLock lock (arpsLock);
        if (!juce::isPositiveAndBelow (index, (int) arps.size())) return;
        arps[asIndex (index)].engine.randomize();
        arps[asIndex (index)].pattern = arps[asIndex (index)].engine.getPattern();
    }
    storeArpsInState();
    sendChangeMessage();
}

void TeArAudioProcessor::setArpeggiatorOn (int index, bool on)
{
    if (!juce::isPositiveAndBelow (index, MAX_ARPS)) return;
    if (auto* p = dynamic_cast<juce::AudioParameterBool*> (
            apvts.getParameter ("arp" + juce::String (index) + "On")))
        p->setValueNotifyingHost (on ? 1.0f : 0.0f);
}

bool TeArAudioProcessor::isArpeggiatorOn (int index) const
{
    juce::ScopedLock lock (arpsLock);
    if (juce::isPositiveAndBelow (index, (int) arps.size()))
        return arps[asIndex (index)].onState;
    return false;
}

void TeArAudioProcessor::setArpeggiatorMidiChannel (int index, int channel)
{
    {
        juce::ScopedLock lock (arpsLock);
        if (!juce::isPositiveAndBelow (index, (int) arps.size())) return;
        arps[asIndex (index)].midiChannel = juce::jlimit (1, 16, channel);
    }
    storeArpsInState();
}

int TeArAudioProcessor::getArpeggiatorMidiChannel (int index) const
{
    juce::ScopedLock lock (arpsLock);
    if (juce::isPositiveAndBelow (index, (int) arps.size()))
        return arps[asIndex (index)].midiChannel;
    return 1;
}

void TeArAudioProcessor::setArpeggiatorSubdivision (int index, int subdivIndex)
{
    {
        juce::ScopedLock lock (arpsLock);
        if (!juce::isPositiveAndBelow (index, (int) arps.size())) return;
        arps[asIndex (index)].subdivision = subdivIndex;
        arps[asIndex (index)].engine.setSubdivision (subdivIndex);
    }
    storeArpsInState();
}

int TeArAudioProcessor::getArpeggiatorSubdivision (int index) const
{
    juce::ScopedLock lock (arpsLock);
    if (juce::isPositiveAndBelow (index, (int) arps.size()))
        return arps[asIndex (index)].subdivision;
    return 4;
}

int TeArAudioProcessor::getArpeggiatorCurrentStep (int index) const
{
    // No lock: acceptable minor race for visual feedback only
    if (juce::isPositiveAndBelow (index, (int) arps.size()))
        return arps[asIndex (index)].engine.getCurrentStepIndex();
    return 0;
}

const fxme::Arpeggiator& TeArAudioProcessor::getArpeggiator (int index) const
{
    // No lock: acceptable minor race for visual feedback only
    return arps[asIndex (index)].engine;
}

juce::uint32 TeArAudioProcessor::getArpeggiatorNoteOnCount (int index) const
{
    juce::ScopedLock lock (arpsLock);
    if (juce::isPositiveAndBelow (index, (int) arps.size()))
        return arps[asIndex (index)].engine.getNoteOnCount();
    return 0;
}

bool TeArAudioProcessor::areNotesHeld() const
{
    // Locked like every other accessor here: processBlock mutates heldNotes
    // under arpsLock, and the editor polls this at 60 Hz, so reading it
    // unlocked was a plain data race. Blocking the message thread on it is
    // safe, because processBlock takes the lock with ScopedTryLock and gives
    // up rather than waiting.
    juce::ScopedLock lock (arpsLock);
    return !heldNotes.isEmpty();
}

juce::Array<int> TeArAudioProcessor::getHeldNotes() const
{
    juce::ScopedLock lock (arpsLock);
    return heldNotes;
}

//==============================================================================
void TeArAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "chordMethod")
    {
        juce::ScopedLock lock (arpsLock);
        for (auto& arp : arps)
        {
            arp.engine.setChordMethod ((int) newValue);
            arp.engine.reset();
        }
        return;
    }

    for (int i = 0; i < MAX_ARPS; ++i)
    {
        if (parameterID == "arp" + juce::String (i) + "On")
        {
            {
                juce::ScopedLock lock (arpsLock);
                if (!juce::isPositiveAndBelow (i, (int) arps.size())) return;
                bool on = (newValue > 0.5f);
                arps[asIndex (i)].onState = on;
                if (on && !wasPlaying)
                {
                    double master = -1.0;
                    for (int j = 0; j < (int) arps.size(); ++j)
                        if (j != i && arps[asIndex (j)].onState) { master = arps[asIndex (j)].engine.getSamplesUntilNextNote(); break; }
                    if (master >= 0.0)
                        arps[asIndex (i)].engine.setSamplesUntilNextNote (master);
                }
            }
            sendChangeMessage();
            return;
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout TeArAudioProcessor::createParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "chordMethod", "Chord Method",
        juce::StringArray { "Notes played", "Chord played as is", "Single note" }, 1));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "scaleRoot", "Scale Root",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "scaleType", "Scale Type", fxme::MidiTools::Scale::getScaleTypeNames(), 0));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        "followMidiIn", "Follow MIDI In", false));

    for (int i = 0; i < MAX_ARPS; ++i)
        layout.add (std::make_unique<juce::AudioParameterBool> (
            "arp" + juce::String (i) + "On",
            "Arp " + juce::String (i + 1) + " On",
            true));

    return layout;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TeArAudioProcessor();
}
