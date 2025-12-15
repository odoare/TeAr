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
    apvts.addParameterListener("subdivision", this);
    apvts.addParameterListener("chordMethod", this);
    apvts.addParameterListener("scaleRoot", this);
    apvts.addParameterListener("scaleType", this);
    apvts.addParameterListener("followMidiIn", this);
}

TeArAudioProcessor::~TeArAudioProcessor()
{
    apvts.removeParameterListener("subdivision", this);
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
    arpeggiator.prepareToPlay(sampleRate);
    arpeggiator.setPattern(arpeggiatorPattern); // Set initial pattern
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
                arpeggiator.setTempo(lastKnownBPM);
            }

            // Sync the arpeggiator to the host's grid if playing
            if (positionInfo.isPlaying)
                arpeggiator.syncToPlayHead(positionInfo);
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
            // Update the arpeggiator's velocity based on the incoming note's velocity.
            arpeggiator.setGlobalVelocityFromMidi(msg.getVelocity());
            notesChanged = true;
            std::cout << "Note on: " << msg.getNoteNumber() << std::endl;
        }
        else if (msg.isNoteOff())
        {
            heldNotes.removeFirstMatchingValue(msg.getNoteNumber());
            notesChanged = true;
            std::cout << "Note off: " << msg.getNoteNumber() << std::endl;            
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
                        // Set the arpeggiator's base octave from the played note.
                        arpeggiator.setBaseOctaveFromNote(lastNote);
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
                            arpeggiator.setBaseOctaveFromNote(lastNote);
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
                            arpeggiator.setBaseOctaveFromNote(lastNote);
                            playedChord = MidiTools::Chord::fromScaleAndDegree(currentScale, nearestDegree != -1 ? nearestDegree : 0);
                        }
                    }
                }
                break;
        }
        arpeggiator.setChord(playedChord);        
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
        midiMessages.addEvents(arpeggiator.turnOff(), 0, -1, 0);
    }
    // If the transport just stopped, also send a note off.
    // This is placed after the clear() call to ensure the message is not erased.
    if (transportJustStopped)
    {
        midiMessages.addEvents(arpeggiator.reset(), 0, -1, 0); // Reset without position info
    }


    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    if (!heldNotes.isEmpty())
        midiMessages.addEvents(arpeggiator.processBlock(buffer.getNumSamples()), 0, -1, 0);
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

    // Manually add our string parameter to the XML
    xml->setAttribute("arpeggiatorPattern", arpeggiatorPattern);

    // Convert the XML to binary and store it
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

        // Manually restore our string parameter from the same XML
        if (xmlState->hasAttribute("arpeggiatorPattern"))
        {
            // Restore the value
            arpeggiatorPattern = xmlState->getStringAttribute("arpeggiatorPattern", "0 1 2");
            arpeggiator.setPattern(arpeggiatorPattern);

            // Notify listeners (like the editor) that our manual state has changed.
            sendChangeMessage();
        }
    }
}

void TeArAudioProcessor::setArpeggiatorPattern(const juce::String& pattern)
{
    // The raw pattern from the UI (with newlines) is stored and passed directly to the engine.
    arpeggiatorPattern = pattern;
    arpeggiator.setPattern(arpeggiatorPattern);
}

const juce::String& TeArAudioProcessor::getArpeggiatorPattern() const { return arpeggiatorPattern; }

int TeArAudioProcessor::getArpeggiatorCurrentStep() const
{
    return arpeggiator.getCurrentStepIndex();
}

const Arpeggiator& TeArAudioProcessor::getArpeggiator() const
{
    return arpeggiator;
}

bool TeArAudioProcessor::areNotesHeld() const
{
    return !heldNotes.isEmpty();
}

void TeArAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "subdivision")
    {
        // Pass the integer index of the choice to the arpeggiator
        arpeggiator.setSubdivision(static_cast<int>(newValue));
    }
    else if (parameterID == "chordMethod")
    {
        // Pass the integer index of the choice to the arpeggiator
        arpeggiator.setChordMethod(static_cast<int>(newValue));
        // Reset arpeggiator to clear any old state
        arpeggiator.reset();
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
        0)); // Default to "Notes played"

    juce::StringArray subdivisions = { "1/4", "1/4T", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/32T", "1/64", "1/64T" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "subdivision",
        "Subdivision",
        subdivisions,
        4 // Default to 1/16
    ));

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