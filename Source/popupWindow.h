/*
  ==============================================================================

    popupWindow.h
    Created: 23 Jan 2026
    Author:  Olivier Doaré

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/components/AccentToggle.h>
#include <FxmeTools/lookandfeels/FxmeLookAndFeel.h>
#include "ArpLookAndFeel.h"

class ArpPatternPopup : public juce::Component
{
public:
    ArpPatternPopup(std::function<juce::String(int, int, int)> makeEuclidian,
                    std::function<juce::String()> makeRandom,
                    std::function<void(juce::String)> onOk,
                    juce::Colour color);
    ~ArpPatternPopup() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Declared before the widgets that point at them, so they outlive them.
    // The same pairing the arpeggiator panel uses: the house look-and-feel for
    // the labels and menus, ArpLookAndFeel for the rounded dark box behind a
    // text field, which the house one does not draw.
    fxme::FxmeLookAndFeel laf;
    ArpLookAndFeel        textLaf;

    juce::Label patternDisplay;
    juce::Label hitsLabel, stepsLabel, rotateLabel;
    juce::TextEditor hitsEditor, stepsEditor, rotateEditor;

    // fxme::AccentToggle rather than juce::TextButton, matching the toolbar
    // buttons these sit under. They latch by default, so each one turns that
    // off in the constructor.
    fxme::AccentToggle randomizeBtn, euclidBtn, okBtn, cancelBtn;

    std::function<juce::String(int, int, int)> makeEuclidianCallback;
    std::function<juce::String()> makeRandomCallback;
    std::function<void(juce::String)> onOkCallback;
    juce::Colour mainColor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArpPatternPopup)
};