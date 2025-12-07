/*
  ==============================================================================
 
    This file contains the basic framework code for a JUCE plugin processor.
 
  ==============================================================================
*/
 
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
{
}

TeArAudioProcessor::~TeArAudioProcessor()
{
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
    arpeggiator.setPattern(arpeggiatorPattern);
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

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep

    // --- Handle incoming MIDI notes to track held notes ---
    bool notesChanged = false;
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            heldNotes.addIfNotAlreadyThere(msg.getNoteNumber());
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
        playedChord.setDegreesByArray(heldNotes);
        arpeggiator.setChord(playedChord);
        std::cout << "Notes: " ;
        for (int note : heldNotes)
            std::cout << note << " ";
        std::cout << std::endl;
        std::cout << "Chord: " << playedChord.getName() << std::endl;
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

    // We clear the incoming buffer and fill it with arpeggiator output
    midiMessages.clear();

    // Only run the arpeggiator if notes are being held
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
    // Create an XML element to hold our state
    auto state = std::make_unique<juce::XmlElement> ("TeArState");
    state->setAttribute ("arpeggiatorPattern", arpeggiatorPattern);

    // Convert the XML to binary and store it
    copyXmlToBinary (*state, destData);
}

void TeArAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Get the XML from the binary data
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
    {
        // Check that it's our XML tag
        if (xmlState->hasTagName ("TeArState"))
        {
            // Restore the value
            arpeggiatorPattern = xmlState->getStringAttribute ("arpeggiatorPattern", "C E G");
        }
    }
}

void TeArAudioProcessor::setArpeggiatorPattern (const juce::String& pattern)
{
    arpeggiatorPattern = pattern;
    arpeggiator.setPattern(arpeggiatorPattern);
}

const juce::String& TeArAudioProcessor::getArpeggiatorPattern() const
{
    return arpeggiatorPattern;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TeArAudioProcessor();
}