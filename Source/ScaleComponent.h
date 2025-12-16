/*
  ==============================================================================

    ScaleComponent.h
    Created: 15 Dec 2025 5:32:30pm
    Author:  doare

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "libs/cppMusicTools/MidiTools.h"

class ScaleComponent : public juce::Component
{
public:
    ScaleComponent();

    ~ScaleComponent() override;

    void paint(juce::Graphics& g) override;

    void updateScale(const juce::Array<int> &newScaleNotes, int newRootNote, const juce::Array<juce::var>& newCurrentNotes);
    
private:
    juce::Array<int> scaleNotes;
    int rootNote = -1;
    juce::Array<juce::var> currentNotes; // Array of objects: { note: 60, arpIndex: 0 }

    juce::Colour getColourForArp(int arpIndex) const;
};
