/*
  ==============================================================================

    popupWindow.cpp
    Created: 23 Jan 2026
    Author:  Olivier Doaré

  ==============================================================================
*/

#include "popupWindow.h"

ArpPatternPopup::ArpPatternPopup(std::function<juce::String(int, int)> makeEuclidian,
                                 std::function<juce::String()> makeRandom,
                                 std::function<void(juce::String)> onOk)
    : makeEuclidianCallback(makeEuclidian),
      makeRandomCallback(makeRandom),
      onOkCallback(onOk)
{
    addAndMakeVisible(patternDisplay);
    patternDisplay.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.3f));
    patternDisplay.setColour(juce::Label::outlineColourId, juce::Colours::white.withAlpha(0.5f));
    patternDisplay.setJustificationType(juce::Justification::centred);
    patternDisplay.setText("Pattern Preview", juce::dontSendNotification);

    addAndMakeVisible(hitsLabel);
    hitsLabel.setText("Hits:", juce::dontSendNotification);
    hitsLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(hitsEditor);
    hitsEditor.setText("3");
    hitsEditor.setInputRestrictions(2, "0123456789");

    addAndMakeVisible(stepsLabel);
    stepsLabel.setText("Steps:", juce::dontSendNotification);
    stepsLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(stepsEditor);
    stepsEditor.setText("8");
    stepsEditor.setInputRestrictions(2, "0123456789");

    addAndMakeVisible(randomizeBtn);
    randomizeBtn.onClick = [this] {
        if (makeRandomCallback)
            patternDisplay.setText(makeRandomCallback(), juce::dontSendNotification);
    };

    addAndMakeVisible(euclidBtn);
    euclidBtn.onClick = [this] {
        if (makeEuclidianCallback)
        {
            int hits = hitsEditor.getText().getIntValue();
            int steps = stepsEditor.getText().getIntValue();
            patternDisplay.setText(makeEuclidianCallback(hits, steps), juce::dontSendNotification);
        }
    };

    addAndMakeVisible(okBtn);
    okBtn.onClick = [this] {
        if (onOkCallback)
            onOkCallback(patternDisplay.getText());
        
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
            box->dismiss();
    };

    addAndMakeVisible(cancelBtn);
    cancelBtn.onClick = [this] {
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
            box->dismiss();
    };

    setSize(300, 200);
}

ArpPatternPopup::~ArpPatternPopup()
{
}

void ArpPatternPopup::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey); // Background for the popup content
}

void ArpPatternPopup::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Top: Pattern Display
    patternDisplay.setBounds(area.removeFromTop(40));
    area.removeFromTop(10);

    // Middle: Controls
    auto controlsArea = area.removeFromTop(80);
    
    // Randomize Row
    auto randomRow = controlsArea.removeFromTop(30);
    randomizeBtn.setBounds(randomRow);

    controlsArea.removeFromTop(10);

    // Euclidean Row
    auto euclidRow = controlsArea.removeFromTop(30);
    hitsLabel.setBounds(euclidRow.removeFromLeft(40));
    hitsEditor.setBounds(euclidRow.removeFromLeft(40));
    stepsLabel.setBounds(euclidRow.removeFromLeft(50));
    stepsEditor.setBounds(euclidRow.removeFromLeft(40));
    euclidRow.removeFromLeft(10);
    euclidBtn.setBounds(euclidRow);

    // Bottom: OK / Cancel
    auto buttonRow = area.removeFromBottom(30);
    okBtn.setBounds(buttonRow.removeFromLeft(buttonRow.getWidth() / 2).reduced(2, 0));
    cancelBtn.setBounds(buttonRow.reduced(2, 0));
}