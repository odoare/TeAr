/*
  ==============================================================================
 
    This file contains the basic framework code for a JUCE plugin processor.
 
  ==============================================================================
*/
 
#include "PluginProcessor.h"
#include "PluginEditor.h"
//#include <memory>

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
    , apvts(*this, nullptr, "Parameters", createParameters())
{
    for (int i = 0; i < 4; ++i)
    {
        arpeggiators.add(Arpeggiator());
        arpeggiatorOnStates.add(true); // Default to ON
        arpeggiatorMidiChannels.add(i + 1); // Default to channel i + 1
        apvts.addParameterListener("midiChannel" + juce::String(i + 1), this);
        apvts.addParameterListener("arpOn" + juce::String(i + 1), this);
        arpeggiatorPatterns.add("1 2 3");
        apvts.addParameterListener("subdivision" + juce::String(i + 1), this);
    }

    apvts.addParameterListener("chordMethod", this);
    apvts.addParameterListener("scaleRoot", this);
    apvts.addParameterListener("scaleType", this);
    apvts.addParameterListener("followMidiIn", this);
}

TeArAudioProcessor::~TeArAudioProcessor()
{
    for (int i = 0; i < 4; ++i)
        apvts.removeParameterListener("midiChannel" + juce::String(i + 1), this);
    for (int i = 0; i < 4; ++i)
        apvts.removeParameterListener("arpOn" + juce::String(i + 1), this);
    for (int i = 0; i < 4; ++i)
        apvts.removeParameterListener("subdivision" + juce::String(i + 1), this);
    apvts.removeParameterListener("chordMethod", this);
    apvts.removeParameterListener("scaleRoot", this);
    apvts.removeParameterListener("scaleType", this);
    apvts.removeParameterListener("followMidiIn", this);
}

//==============================================================================
const juce::String TeArAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

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

double TeArAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TeArAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int TeArAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TeArAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String TeArAudioProcessor::getProgramName (int index)
{
    return {};
}

void TeArAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void TeArAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (int i = 0; i < arpeggiators.size(); ++i)
    {
        arpeggiators.getReference(i).prepareToPlay(sampleRate);
        arpeggiators.getReference(i).setPattern(arpeggiatorPatterns[i]);
    }
}

void TeArAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TeArAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
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
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    bool transportJustStopped = false;

    // --- Get Host Transport Information ---
    if (auto* playHead = getPlayHead())
    {
        juce::AudioPlayHead::CurrentPositionInfo positionInfo;
        if (playHead->getCurrentPosition(positionInfo))
        {
            // Update tempo if it has changed
            if (positionInfo.bpm > 0.0 && positionInfo.bpm != lastKnownBPM)
            {
                lastKnownBPM = positionInfo.bpm;
                for (auto& arp : arpeggiators) arp.setTempo(lastKnownBPM);
            }

            // Sync the arpeggiator to the host's grid if playing
            // Only sync arpeggiators that are turned on
            if (positionInfo.isPlaying)
                for (auto& arp : arpeggiators) arp.syncToPlayHead(positionInfo);
            else if (wasPlaying)
                transportJustStopped = true;

            if (positionInfo.isPlaying && !wasPlaying) // Playback just started
            {
                // This block can be used for logic that needs to run only on the first block of playback.
            }
            wasPlaying = positionInfo.isPlaying;
        }
    }

    // --- Handle incoming MIDI notes to track held notes ---
    bool notesChanged = false;
    for (const auto metadata : midiMessages) // This is why we must not clear midiMessages at the start!
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            heldNotes.addIfNotAlreadyThere(msg.getNoteNumber());
            // Update the arpeggiator's velocity based on the incoming note's velocity, only for active arps.
            for (int i = 0; i < arpeggiators.size(); ++i)
                if (arpeggiatorOnStates[i])
                    arpeggiators.getReference(i).setGlobalVelocityFromMidi(msg.getVelocity());
            notesChanged = true;
        }
        else if (msg.isNoteOff())
        {
            heldNotes.removeFirstMatchingValue(msg.getNoteNumber());
            notesChanged = true;
        }
    }

    if (notesChanged)
    {
        MidiTools::Chord playedChord("");
        auto chordMethod = static_cast<int>(apvts.getRawParameterValue("chordMethod")->load());

        switch (chordMethod)
        {
            case 0: // Notes played
                playedChord.setDegreesByArray(heldNotes);
                break;
            case 1: // Chord played as is
                playedChord.setNotesByArray(heldNotes);
                break;
            case 2: // Single note
                if (!heldNotes.isEmpty())
                {
                    int lastNote = heldNotes.getLast();
                    int lastNoteSemitone = lastNote % 12;

                    auto followMidiIn = apvts.getRawParameterValue("followMidiIn")->load();
                    auto scaleTypeIndex = static_cast<int>(apvts.getRawParameterValue("scaleType")->load());
                    auto scaleType = static_cast<MidiTools::Scale::Type>(scaleTypeIndex);

                    if (followMidiIn)
                    {
                        // The incoming note sets the root of the scale.
                        // We update the parameter, which will also update the UI.
                        apvts.getParameter("scaleRoot")->setValueNotifyingHost(lastNoteSemitone / 11.0f);

                        MidiTools::Scale currentScale(lastNoteSemitone, scaleType);
                        // Set the arpeggiator's base octave from the played note, only for active arps.
                        for (int i = 0; i < arpeggiators.size(); ++i)
                            if (arpeggiatorOnStates[i])
                                arpeggiators.getReference(i).setBaseOctaveFromNote(lastNote);
                        // The chord is built from the root of this new scale.
                        playedChord = MidiTools::Chord::fromScaleAndDegree(currentScale, 0);
                    }
                    else
                    {
                        // Use the fixed scale from the UI to find the degree of the played note.
                        auto rootNoteIndex = static_cast<int>(apvts.getRawParameterValue("scaleRoot")->load());
                        MidiTools::Scale currentScale(rootNoteIndex, scaleType);
                        const auto& scaleNotes = currentScale.getNotes();
                        int degree = scaleNotes.indexOf(lastNoteSemitone);

                        if (degree != -1) // If the note is in the scale
                        {
                            for (int i = 0; i < arpeggiators.size(); ++i)
                                if (arpeggiatorOnStates[i])
                                    arpeggiators.getReference(i).setBaseOctaveFromNote(lastNote);
                            playedChord = MidiTools::Chord::fromScaleAndDegree(currentScale, degree);
                        }
                        else // Note is not in scale, find nearest below
                        {
                            int nearestDegree = -1;
                            for (int i = 1; i < 12; ++i)
                            {
                                int semitoneToTest = (lastNoteSemitone - i + 12) % 12;
                                int foundDegree = scaleNotes.indexOf(semitoneToTest);
                                if (foundDegree != -1)
                                {
                                    nearestDegree = foundDegree;
                                    break;
                                }
                            }
                            for (int i = 0; i < arpeggiators.size(); ++i)
                                if (arpeggiatorOnStates[i])
                                    arpeggiators.getReference(i).setBaseOctaveFromNote(lastNote);
                            playedChord = MidiTools::Chord::fromScaleAndDegree(currentScale, nearestDegree != -1 ? nearestDegree : 0);
                        }
                    }
                }
                break;
        }
        // Set chord for all active arpeggiators
        for (int i = 0; i < arpeggiators.size(); ++i)
            if (arpeggiatorOnStates[i]) arpeggiators.getReference(i).setChord(playedChord);
        // std::cout << "Notes: " ;
        // for (int note : heldNotes)
        //     std::cout << note << " ";
        // std::cout << std::endl;
        // std::cout << "Chord: " << playedChord.getName() << std::endl;
    }

    // We clear the incoming buffer and fill it with arpeggiator output
    midiMessages.clear();

    // If the user just released the last key, send a note off.
    if (notesChanged && heldNotes.isEmpty())
    {
        for (int i = 0; i < arpeggiators.size(); ++i)
            if (arpeggiatorOnStates[i])
                midiMessages.addEvents(arpeggiators.getReference(i).turnOff(arpeggiatorMidiChannels[i]), 0, -1, 0);
    }
    // If the transport just stopped, also send a note off.
    // This is placed after the clear() call to ensure the message is not erased.
    if (transportJustStopped)
    {
        for (int i = 0; i < arpeggiators.size(); ++i)
            if (arpeggiatorOnStates[i])
                midiMessages.addEvents(arpeggiators.getReference(i).reset(arpeggiatorMidiChannels[i]), 0, -1, 0);
    }


    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels. Alternatively,
    // you can process the samples with the channels interleaved by keeping the same state.
    if (!heldNotes.isEmpty())
        for (int i = 0; i < arpeggiators.size(); ++i)
            if (arpeggiatorOnStates[i])
                midiMessages.addEvents(arpeggiators.getReference(i).processBlock(buffer.getNumSamples(), arpeggiatorMidiChannels[i]), 0, -1, 0);
}

//==============================================================================
bool TeArAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TeArAudioProcessor::createEditor()
{
    return new TeArAudioProcessorEditor (*this);
}

//==============================================================================
void TeArAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Get the state from APVTS
    auto apvtsState = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (apvtsState.createXml());

    // Manually add our string parameters to the XML
    for (int i = 0; i < arpeggiatorPatterns.size(); ++i)
        xml->setAttribute("arpeggiatorPattern" + juce::String(i), arpeggiatorPatterns[i]);
    for (int i = 0; i < arpeggiatorOnStates.size(); ++i)
        xml->setAttribute("arpOn" + juce::String(i), arpeggiatorOnStates[i]);

    // Convert the XML to binary and store it.
    copyXmlToBinary(*xml, destData);
}

void TeArAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
 
    if (xmlState != nullptr)
    {
        // Restore the APVTS state
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));

        // Sync cached MIDI channels and subdivisions from APVTS
        for (int i = 0; i < arpeggiatorMidiChannels.size(); ++i)
        {
            if (auto* p = apvts.getRawParameterValue("midiChannel" + juce::String(i + 1)))
                arpeggiatorMidiChannels.set(i, static_cast<int>(*p));

            if (auto* p = apvts.getRawParameterValue("subdivision" + juce::String(i + 1)))
                arpeggiators.getReference(i).setSubdivision(static_cast<int>(*p));
        }

        // Manually restore our string parameters from the same XML
        for (int i = 0; i < arpeggiatorPatterns.size(); ++i)
        {
            juce::String attributeName = "arpeggiatorPattern" + juce::String(i);
            if (xmlState->hasAttribute(attributeName))
            {
                arpeggiatorPatterns.set(i, xmlState->getStringAttribute(attributeName, "0 1 2"));
                arpeggiators.getReference(i).setPattern(arpeggiatorPatterns[i]);
            }
        }
        for (int i = 0; i < arpeggiatorOnStates.size(); ++i)
        {
            juce::String attributeName = "arpOn" + juce::String(i);
            if (xmlState->hasAttribute(attributeName))
            {
                arpeggiatorOnStates.set(i, xmlState->getBoolAttribute(attributeName, true));
            }
        }
        // Notify listeners (like the editor) that our manual state has changed.
        sendChangeMessage();
    }
}

void TeArAudioProcessor::setArpeggiatorPattern(int index, const juce::String& pattern)
{
    if (juce::isPositiveAndBelow(index, arpeggiatorPatterns.size()))
    {
        arpeggiatorPatterns.set(index, pattern);
        arpeggiators.getReference(index).setPattern(pattern);

        // If the pattern of an arp changed and the DAW is not playing, sync it to another running arp.
        // This ensures that when playback is stopped, all arps remain rhythmically aligned.
        if (!wasPlaying && arpeggiatorOnStates[index])
        {
            double masterSamplesUntilNext = -1.0;
            // Find a running arpeggiator to use as the master clock
            for (int i = 0; i < arpeggiators.size(); ++i)
            {
                if (i != index && arpeggiatorOnStates[i])
                {
                    masterSamplesUntilNext = arpeggiators[i].getSamplesUntilNextNote();
                    break;
                }
            }

            // If we found a master, sync the arpeggiator whose pattern just changed.
            if (masterSamplesUntilNext >= 0.0)
                arpeggiators.getReference(index).setSamplesUntilNextNote(masterSamplesUntilNext);
        }

        // Notify the editor that the pattern has changed so it can update the text box.
        sendChangeMessage();
    }
}

const juce::String& TeArAudioProcessor::getArpeggiatorPattern(int index) const
{
    if (juce::isPositiveAndBelow(index, arpeggiatorPatterns.size()))
        return arpeggiatorPatterns.getReference(index);
    
    static const juce::String emptyString;
    return emptyString;
}

void TeArAudioProcessor::randomizeArpeggiator(int index)
{
    if (juce::isPositiveAndBelow(index, arpeggiators.size()))
    {
        arpeggiators.getReference(index).randomize();
        // Update the stored pattern string to match the new random pattern
        arpeggiatorPatterns.set(index, arpeggiators.getReference(index).getPattern());
        sendChangeMessage(); // Notify the editor to update the text box
    }
}

bool TeArAudioProcessor::isArpeggiatorOn(int index) const
{
    if (juce::isPositiveAndBelow(index, arpeggiatorOnStates.size()))
        return arpeggiatorOnStates[index];
    return false;
}
int TeArAudioProcessor::getArpeggiatorCurrentStep(int index) const
{
    if (juce::isPositiveAndBelow(index, arpeggiators.size()))
        return arpeggiators.getUnchecked(index).getCurrentStepIndex();
    return 0;
}

const Arpeggiator& TeArAudioProcessor::getArpeggiator(int index) const
{
    return arpeggiators.getReference(index);
}

bool TeArAudioProcessor::areNotesHeld() const
{
    return !heldNotes.isEmpty();
}

void TeArAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID.startsWith("arpOn"))
    {
        int arpIndex = parameterID.getLastCharacter() - '1';
        if (juce::isPositiveAndBelow(arpIndex, arpeggiatorOnStates.size()))
        {
            arpeggiatorOnStates.set(arpIndex, newValue > 0.5f);

            // If we just turned an arp ON and the DAW is not playing, sync it to another running arp.
            if (newValue > 0.5f && !wasPlaying)
            {
                double masterSamplesUntilNext = -1.0;
                // Find a running arpeggiator to use as the master clock
                for (int i = 0; i < arpeggiators.size(); ++i)
                {
                    if (i != arpIndex && arpeggiatorOnStates[i])
                    {
                        masterSamplesUntilNext = arpeggiators[i].getSamplesUntilNextNote();
                        break;
                    }
                }

                // If we found a master, sync the newly enabled arpeggiator to it.
                if (masterSamplesUntilNext >= 0.0)
                    arpeggiators.getReference(arpIndex).setSamplesUntilNextNote(masterSamplesUntilNext);
            }
        }
    }
    if (parameterID.startsWith("midiChannel"))
    {
        int arpIndex = parameterID.getLastCharacter() - '1';
        if (juce::isPositiveAndBelow(arpIndex, arpeggiatorMidiChannels.size()))
        {
            arpeggiatorMidiChannels.set(arpIndex, static_cast<int>(newValue));
        }
    }
    if (parameterID.startsWith("subdivision"))
    {
        int arpIndex = parameterID.getLastCharacter() - '1';
        if (juce::isPositiveAndBelow(arpIndex, arpeggiators.size()))
        {
            arpeggiators.getReference(arpIndex).setSubdivision(static_cast<int>(newValue));
        }
    }
    else if (parameterID == "chordMethod")
    {
        for (int i = 0; i < arpeggiators.size(); ++i)
        {
            auto& arp = arpeggiators.getReference(i);
            arp.setChordMethod(static_cast<int>(newValue)); // TODO: This should probably be passed to reset
            arp.reset();
        }
    }
    else if (parameterID == "scaleRoot")
    {
    }
    else if (parameterID == "scaleType")
    {
    }
    else if (parameterID == "followMidiIn")
    {
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout TeArAudioProcessor::createParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::StringArray chordMethods = { "Notes played", "Chord played as is", "Single note" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "chordMethod",
        "Chord Method",
        chordMethods,
        1)); // Default to "Chord played as is"

    for (int i = 0; i < 4; ++i)
    {
        layout.add(std::make_unique<juce::AudioParameterBool>(
            "arpOn" + juce::String(i + 1),
            "Arpeggiator " + juce::String(i + 1) + " On/Off",
            true // Default to ON
        ));
    }

    for (int i = 0; i < 4; ++i)
    {
        layout.add(std::make_unique<juce::AudioParameterInt>(
            "midiChannel" + juce::String(i + 1),
            "MIDI Channel " + juce::String(i + 1),
            1, 16, i + 1 // min, max, default (1, 2, 3, 4)
        ));
    }

    for (int i = 0; i < 4; ++i)
    {
        juce::StringArray subdivisions = { "1/4", "1/4T", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/32T", "1/64", "1/64T" };
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "subdivision" + juce::String(i + 1),
            "Subdivision " + juce::String(i + 1),
            subdivisions,
            4 // Default to 1/16
        ));
    }

    juce::StringArray scaleRoots = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "scaleRoot",
        "Scale Root",
        scaleRoots,
        0 // Default to C
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "scaleType",
        "Scale Type",
        MidiTools::Scale::getScaleTypeNames(),
        0 // Default to Major
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "followMidiIn",
        "Follow MIDI In",
        false // Default to false
    ));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TeArAudioProcessor();
}