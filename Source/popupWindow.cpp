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
                                 std::function<void(juce::String)> onOk,
                                 juce::Colour color)
    : makeEuclidianCallback(makeEuclidian),
      makeRandomCallback(makeRandom),
      onOkCallback(onOk),
      mainColor(color)
{
    addAndMakeVisible(patternDisplay);
    patternDisplay.setColour(juce::Label::backgroundColourId, juce::Colours::darkblue.darker(2.f));
    patternDisplay.setColour(juce::Label::outlineColourId, mainColor.withAlpha(0.f));
    patternDisplay.setColour(juce::Label::textColourId, mainColor);
    patternDisplay.setJustificationType(juce::Justification::centred);
    patternDisplay.setText("", juce::dontSendNotification);

    addAndMakeVisible(hitsLabel);
    hitsLabel.setText("Hits:", juce::dontSendNotification);
    hitsLabel.setColour(juce::Label::textColourId, mainColor);
    hitsLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(hitsEditor);
    hitsEditor.setText("3");
    hitsEditor.setInputRestrictions(2, "0123456789");
    hitsEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::darkblue.darker(2.f));
    hitsEditor.setColour(juce::TextEditor::textColourId, mainColor);

    addAndMakeVisible(stepsLabel);
    stepsLabel.setText("Steps:", juce::dontSendNotification);
    stepsLabel.setColour(juce::Label::textColourId, mainColor);
    stepsLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(stepsEditor);
    stepsEditor.setText("8");
    stepsEditor.setInputRestrictions(2, "0123456789");
    stepsEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::darkblue.darker(2.f));
    stepsEditor.setColour(juce::TextEditor::textColourId, mainColor);

    addAndMakeVisible(randomizeBtn);
    randomizeBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    randomizeBtn.setColour(juce::TextButton::textColourOffId, mainColor);
    randomizeBtn.setColour(juce::TextButton::textColourOnId, mainColor.brighter());
    randomizeBtn.onClick = [this] {
        if (makeRandomCallback)
            patternDisplay.setText(makeRandomCallback(), juce::dontSendNotification);
    };

    addAndMakeVisible(euclidBtn);
    euclidBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    euclidBtn.setColour(juce::TextButton::textColourOffId, mainColor);
    euclidBtn.setColour(juce::TextButton::textColourOnId, mainColor.brighter());
    euclidBtn.onClick = [this] {
        if (makeEuclidianCallback)
        {
            int hits = hitsEditor.getText().getIntValue();
            int steps = stepsEditor.getText().getIntValue();
            patternDisplay.setText(makeEuclidianCallback(hits, steps), juce::dontSendNotification);
        }
    };

    addAndMakeVisible(okBtn);
    okBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    okBtn.setColour(juce::TextButton::textColourOffId, mainColor);
    okBtn.setColour(juce::TextButton::textColourOnId, mainColor.brighter());
    okBtn.onClick = [this] {
        if (onOkCallback)
            onOkCallback(patternDisplay.getText());
        
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
            box->dismiss();
    };

    addAndMakeVisible(cancelBtn);
    cancelBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    cancelBtn.setColour(juce::TextButton::textColourOffId, mainColor);
    cancelBtn.setColour(juce::TextButton::textColourOnId, mainColor.brighter());
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
    g.fillAll(juce::Colours::darkblue.darker(2.f)); // Background for the popup content
    g.setColour(mainColor);
    g.drawRect(getLocalBounds(), 1);
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