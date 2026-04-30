#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    sampleRate = sr;

    juce::ScopedLock lock (arpsLock);
    for (auto& arp : arps)
    {
        arp.engine.prepareToPlay (sampleRate);
        arp.engine.setPattern (arp.pattern);
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
    juce::ScopedLock lock (arpsLock);

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
                    arp.engine.setGlobalVelocityFromMidi (msg.getVelocity());
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
        MidiTools::Chord playedChord ("");
        auto chordMethod = (int) apvts.getRawParameterValue ("chordMethod")->load();

        switch (chordMethod)
        {
            case 0:
                playedChord.setDegreesByArray (heldNotes);
                break;
            case 1:
                playedChord.setNotesByArray (heldNotes);
                break;
            case 2:
                if (!heldNotes.isEmpty())
                {
                    int lastNote = heldNotes.getLast();
                    int lastNoteSemitone = lastNote % 12;
                    auto followMidiIn   = apvts.getRawParameterValue ("followMidiIn")->load();
                    auto scaleTypeIndex = (int) apvts.getRawParameterValue ("scaleType")->load();
                    auto scaleType      = static_cast<MidiTools::Scale::Type> (scaleTypeIndex);

                    MidiTools::Chord rootChordForArps ("");

                    if (followMidiIn > 0.5f)
                    {
                        apvts.getParameter ("scaleRoot")->setValueNotifyingHost (lastNoteSemitone / 11.0f);
                        MidiTools::Scale currentScale (lastNoteSemitone, scaleType);
                        for (auto& arp : arps)
                            if (arp.onState) arp.engine.setBaseOctaveFromNote (lastNote);
                        playedChord = MidiTools::Chord::fromScaleAndDegree (currentScale, 0);
                        // With followMidiIn the pressed note IS the root, so rootChord == playedChord
                        rootChordForArps = playedChord;
                    }
                    else
                    {
                        auto rootNoteIndex = (int) apvts.getRawParameterValue ("scaleRoot")->load();
                        MidiTools::Scale currentScale (rootNoteIndex, scaleType);
                        // rootChord is always degree-0 of the fixed scale
                        rootChordForArps = MidiTools::Chord::fromScaleAndDegree (currentScale, 0);

                        const auto& scaleNotes = currentScale.getNotes();
                        int degree = scaleNotes.indexOf (lastNoteSemitone);

                        if (degree != -1)
                        {
                            for (auto& arp : arps)
                                if (arp.onState) arp.engine.setBaseOctaveFromNote (lastNote);
                            playedChord = MidiTools::Chord::fromScaleAndDegree (currentScale, degree);
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
                            playedChord = MidiTools::Chord::fromScaleAndDegree (currentScale,
                                          nearestDegree != -1 ? nearestDegree : 0);
                        }
                    }

                    for (auto& arp : arps)
                        if (arp.onState) arp.engine.setRootChord (rootChordForArps);
                }
                break;
        }

        for (auto& arp : arps)
            if (arp.onState) arp.engine.setChord (playedChord);
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
void TeArAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ScopedLock lock (arpsLock);

    auto apvtsState = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (apvtsState.createXml());

    // New format: store arps as child element
    auto* arpsXml = xml->createNewChildElement ("Arpeggiators");
    arpsXml->setAttribute ("count", (int) arps.size());
    for (int i = 0; i < (int) arps.size(); ++i)
    {
        auto* arpXml = arpsXml->createNewChildElement ("Arp");
        arpXml->setAttribute ("index",       i);
        arpXml->setAttribute ("pattern",     arps[i].pattern);
        arpXml->setAttribute ("on",          arps[i].onState ? 1 : 0);
        arpXml->setAttribute ("midiChannel", arps[i].midiChannel);
        arpXml->setAttribute ("subdivision", arps[i].subdivision);
    }

    copyXmlToBinary (*xml, destData);
}

void TeArAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState == nullptr) return;

    if (xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));

    std::vector<bool> loadedOnStates;

    {
        juce::ScopedLock lock (arpsLock);

        if (auto* arpsXml = xmlState->getChildByName ("Arpeggiators"))
        {
            // --- New format ---
            arps.clear();
            int count = arpsXml->getIntAttribute ("count", 1);
            count = juce::jlimit (1, 64, count);

            for (auto* arpXml : arpsXml->getChildIterator())
            {
                if (!arpXml->hasTagName ("Arp")) continue;
                ArpInstance arp;
                arp.pattern     = arpXml->getStringAttribute ("pattern", "1 2 3");
                arp.onState     = arpXml->getIntAttribute    ("on",          1) != 0;
                arp.midiChannel = arpXml->getIntAttribute    ("midiChannel", 1);
                arp.subdivision = arpXml->getIntAttribute    ("subdivision", 4);

                arp.engine.prepareToPlay (sampleRate);
                arp.engine.setPattern    (arp.pattern);
                arp.engine.setSubdivision(arp.subdivision);
                arp.engine.setTempo      (lastKnownBPM);
                arp.engine.setChordMethod((int) apvts.getRawParameterValue ("chordMethod")->load());
                arps.push_back (std::move (arp));
            }

            // Safety: ensure at least one arp
            if (arps.empty())
                arps.push_back (ArpInstance{});
        }
        else
        {
            // --- Legacy format (v1: 4 fixed arps, params stored as APVTS PARAMs + attributes) ---
            auto getParam = [&] (const juce::String& paramId, float def) -> float
            {
                for (auto* child : xmlState->getChildIterator())
                    if (child->hasTagName ("PARAM") && child->getStringAttribute ("id") == paramId)
                        return (float) child->getDoubleAttribute ("value", def);
                return def;
            };

            arps.clear();
            for (int i = 0; i < 4; ++i)
            {
                ArpInstance arp;
                arp.pattern     = xmlState->getStringAttribute ("arpeggiatorPattern" + juce::String (i), "1 2 3");
                arp.onState     = xmlState->getBoolAttribute   ("arpOn"             + juce::String (i), true);
                arp.midiChannel = (int) getParam ("midiChannel"  + juce::String (i + 1), (float) (i + 1));
                arp.subdivision = (int) getParam ("subdivision"  + juce::String (i + 1), 4.0f);

                arp.engine.prepareToPlay (sampleRate);
                arp.engine.setPattern    (arp.pattern);
                arp.engine.setSubdivision(arp.subdivision);
                arp.engine.setTempo      (lastKnownBPM);
                arp.engine.setChordMethod((int) apvts.getRawParameterValue ("chordMethod")->load());
                arps.push_back (std::move (arp));
            }
        }

        for (auto& arp : arps)
            loadedOnStates.push_back (arp.onState);
    }

    // Sync APVTS bool params to match XML-loaded on/off states. This ensures
    // the async parameterChanged callbacks that apvts.replaceState() scheduled
    // will fire with the correct values and not overwrite what we loaded from XML.
    for (int i = 0; i < (int) loadedOnStates.size() && i < MAX_ARPS; ++i)
        if (auto* p = dynamic_cast<juce::AudioParameterBool*> (
                apvts.getParameter ("arp" + juce::String (i) + "On")))
            p->setValueNotifyingHost (loadedOnStates[i] ? 1.0f : 0.0f);

    sendChangeMessage();
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
    sendChangeMessage();
}

void TeArAudioProcessor::removeArpeggiator (int index)
{
    {
        juce::ScopedLock lock (arpsLock);
        if ((int) arps.size() <= 1) return;
        if (!juce::isPositiveAndBelow (index, (int) arps.size())) return;
        arps.erase (arps.begin() + index);
    }
    sendChangeMessage();
}

//==============================================================================
// Per-arp accessors

void TeArAudioProcessor::setArpeggiatorPattern (int index, const juce::String& pattern)
{
    {
        juce::ScopedLock lock (arpsLock);
        if (!juce::isPositiveAndBelow (index, (int) arps.size())) return;
        arps[index].pattern = pattern;
        arps[index].engine.setPattern (pattern);

        if (!wasPlaying && arps[index].onState)
        {
            double master = -1.0;
            for (int i = 0; i < (int) arps.size(); ++i)
                if (i != index && arps[i].onState) { master = arps[i].engine.getSamplesUntilNextNote(); break; }
            if (master >= 0.0)
                arps[index].engine.setSamplesUntilNextNote (master);
        }
    }
    sendChangeMessage();
}

const juce::String& TeArAudioProcessor::getArpeggiatorPattern (int index) const
{
    juce::ScopedLock lock (arpsLock);
    if (juce::isPositiveAndBelow (index, (int) arps.size()))
        return arps[index].pattern;
    static const juce::String empty;
    return empty;
}

void TeArAudioProcessor::randomizeArpeggiator (int index)
{
    {
        juce::ScopedLock lock (arpsLock);
        if (!juce::isPositiveAndBelow (index, (int) arps.size())) return;
        arps[index].engine.randomize();
        arps[index].pattern = arps[index].engine.getPattern();
    }
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
        return arps[index].onState;
    return false;
}

void TeArAudioProcessor::setArpeggiatorMidiChannel (int index, int channel)
{
    juce::ScopedLock lock (arpsLock);
    if (juce::isPositiveAndBelow (index, (int) arps.size()))
        arps[index].midiChannel = juce::jlimit (1, 16, channel);
}

int TeArAudioProcessor::getArpeggiatorMidiChannel (int index) const
{
    juce::ScopedLock lock (arpsLock);
    if (juce::isPositiveAndBelow (index, (int) arps.size()))
        return arps[index].midiChannel;
    return 1;
}

void TeArAudioProcessor::setArpeggiatorSubdivision (int index, int subdivIndex)
{
    juce::ScopedLock lock (arpsLock);
    if (juce::isPositiveAndBelow (index, (int) arps.size()))
    {
        arps[index].subdivision = subdivIndex;
        arps[index].engine.setSubdivision (subdivIndex);
    }
}

int TeArAudioProcessor::getArpeggiatorSubdivision (int index) const
{
    juce::ScopedLock lock (arpsLock);
    if (juce::isPositiveAndBelow (index, (int) arps.size()))
        return arps[index].subdivision;
    return 4;
}

int TeArAudioProcessor::getArpeggiatorCurrentStep (int index) const
{
    // No lock: acceptable minor race for visual feedback only
    if (juce::isPositiveAndBelow (index, (int) arps.size()))
        return arps[index].engine.getCurrentStepIndex();
    return 0;
}

const Arpeggiator& TeArAudioProcessor::getArpeggiator (int index) const
{
    // No lock: acceptable minor race for visual feedback only
    return arps[index].engine;
}

bool TeArAudioProcessor::areNotesHeld() const
{
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
                arps[i].onState = on;
                if (on && !wasPlaying)
                {
                    double master = -1.0;
                    for (int j = 0; j < (int) arps.size(); ++j)
                        if (j != i && arps[j].onState) { master = arps[j].engine.getSamplesUntilNextNote(); break; }
                    if (master >= 0.0)
                        arps[i].engine.setSamplesUntilNextNote (master);
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
        "scaleType", "Scale Type", MidiTools::Scale::getScaleTypeNames(), 0));

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
