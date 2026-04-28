#pragma once
#include <JuceHeader.h>

class KeyboardComponent : public juce::Component
{
public:
    KeyboardComponent (int numOctaves = 7, int startOctave = 1);
    ~KeyboardComponent() override;

    void paint (juce::Graphics& g) override;

    // scaleNotes  → gray-dimmed fill on non-scale keys
    // rootNote    → white outline
    // currentNotes→ arp-colour fill on playing output notes
    // inputNotes  → red outline on pressed MIDI-input notes
    void updateScale (const juce::Array<int>&      newScaleNotes,
                      int                           newRootNote,
                      const juce::Array<juce::var>& newCurrentNotes,
                      const juce::Array<int>&        newInputNotes = {});

private:
    int numOctaves;
    int startOctave;
    int startMidi;

    juce::Array<int>       scaleNotes;
    int                    rootNote { -1 };
    juce::Array<juce::var> currentNotes;
    juce::Array<int>       inputNotes;

    juce::Colour getColourForArp (int arpIndex) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyboardComponent)
};
