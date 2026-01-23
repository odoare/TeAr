/*
  ==============================================================================

    popupWindow.h
    Created: 23 Jan 2026
    Author:  Olivier Doaré

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ArpPatternPopup : public juce::Component
{
public:
    ArpPatternPopup(std::function<juce::String(int, int)> makeEuclidian,
                    std::function<juce::String()> makeRandom,
                    std::function<void(juce::String)> onOk,
                    juce::Colour color);
    ~ArpPatternPopup() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Label patternDisplay;
    juce::Label hitsLabel, stepsLabel;
    juce::TextEditor hitsEditor, stepsEditor;
    juce::TextButton randomizeBtn{ "Randomize" };
    juce::TextButton euclidBtn{ "Make Euclidean" };
    juce::TextButton okBtn{ "OK" };
    juce::TextButton cancelBtn{ "Cancel" };

    std::function<juce::String(int, int)> makeEuclidianCallback;
    std::function<juce::String()> makeRandomCallback;
    std::function<void(juce::String)> onOkCallback;
    juce::Colour mainColor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArpPatternPopup)
};