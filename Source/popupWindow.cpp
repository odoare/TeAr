/*
  ==============================================================================

    popupWindow.cpp
    Created: 23 Jan 2026
    Author:  Olivier Doaré

  ==============================================================================
*/

#include "popupWindow.h"
#include <FxmeTools/lookandfeels/PanelBackground.h>
#include "Theme.h"

ArpPatternPopup::ArpPatternPopup(std::function<juce::String(int, int, int)> makeEuclidian,
                                 std::function<juce::String()> makeRandom,
                                 std::function<void(juce::String)> onOk,
                                 juce::Colour color)
    : makeEuclidianCallback(makeEuclidian),
      makeRandomCallback(makeRandom),
      onOkCallback(onOk),
      mainColor(color)
{
    // Menus and labels take the arpeggiator's own colour, as its panel does.
    laf.setAccentColour(mainColor);

    addAndMakeVisible(patternDisplay);
    patternDisplay.setLookAndFeel(&laf);
    patternDisplay.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    patternDisplay.setColour(juce::Label::outlineColourId, mainColor.withAlpha(0.f));
    patternDisplay.setColour(juce::Label::textColourId, mainColor);
    patternDisplay.setJustificationType(juce::Justification::centred);
    patternDisplay.setText("", juce::dontSendNotification);

    addAndMakeVisible(hitsLabel);
    hitsLabel.setText("Hits:", juce::dontSendNotification);
    hitsLabel.setLookAndFeel(&laf);
    hitsLabel.setColour(juce::Label::textColourId, mainColor);
    hitsLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(hitsEditor);
    hitsEditor.setText("3");
    hitsEditor.setInputRestrictions(2, "0123456789");
    hitsEditor.setLookAndFeel(&textLaf);
    hitsEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    hitsEditor.setColour(juce::TextEditor::textColourId, mainColor);
    hitsEditor.setColour(juce::TextEditor::outlineColourId, mainColor);
    hitsEditor.setColour(juce::CaretComponent::caretColourId, mainColor.brighter());

    addAndMakeVisible(stepsLabel);
    stepsLabel.setText("Steps:", juce::dontSendNotification);
    stepsLabel.setLookAndFeel(&laf);
    stepsLabel.setColour(juce::Label::textColourId, mainColor);
    stepsLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(stepsEditor);
    stepsEditor.setText("8");
    stepsEditor.setInputRestrictions(2, "0123456789");
    stepsEditor.setLookAndFeel(&textLaf);
    stepsEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    stepsEditor.setColour(juce::TextEditor::textColourId, mainColor);
    stepsEditor.setColour(juce::TextEditor::outlineColourId, mainColor);
    stepsEditor.setColour(juce::CaretComponent::caretColourId, mainColor.brighter());

    addAndMakeVisible(rotateLabel);
    rotateLabel.setText("Rot:", juce::dontSendNotification);
    rotateLabel.setLookAndFeel(&laf);
    rotateLabel.setColour(juce::Label::textColourId, mainColor);
    rotateLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(rotateEditor);
    rotateEditor.setText("0");
    rotateEditor.setInputRestrictions(3, "-0123456789");
    rotateEditor.setLookAndFeel(&textLaf);
    rotateEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    rotateEditor.setColour(juce::TextEditor::textColourId, mainColor);
    rotateEditor.setColour(juce::TextEditor::outlineColourId, mainColor);
    rotateEditor.setColour(juce::CaretComponent::caretColourId, mainColor.brighter());

    addAndMakeVisible(randomizeBtn);
    randomizeBtn.setButtonText("Randomize");
    randomizeBtn.setClickingTogglesState(false);
    randomizeBtn.setAccent(mainColor, mainColor.brighter(0.4f), Theme::buttonBody);
    randomizeBtn.onClick = [this] {
        if (makeRandomCallback)
            patternDisplay.setText(makeRandomCallback(), juce::dontSendNotification);
    };

    addAndMakeVisible(euclidBtn);
    euclidBtn.setButtonText("Make Euclidean");
    euclidBtn.setClickingTogglesState(false);
    euclidBtn.setAccent(mainColor, mainColor.brighter(0.4f), Theme::buttonBody);
    euclidBtn.onClick = [this] {
        if (makeEuclidianCallback)
        {
            int hits = hitsEditor.getText().getIntValue();
            int steps = stepsEditor.getText().getIntValue();
            int rotation = rotateEditor.getText().getIntValue();
            patternDisplay.setText(makeEuclidianCallback(hits, steps, rotation), juce::dontSendNotification);
        }
    };

    addAndMakeVisible(okBtn);
    okBtn.setButtonText("OK");
    okBtn.setClickingTogglesState(false);
    okBtn.setAccent(mainColor, mainColor.brighter(0.4f), Theme::buttonBody);
    okBtn.onClick = [this] {
        if (onOkCallback)
            onOkCallback(patternDisplay.getText());
        
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
            box->dismiss();
    };

    addAndMakeVisible(cancelBtn);
    cancelBtn.setButtonText("Cancel");
    cancelBtn.setClickingTogglesState(false);
    cancelBtn.setAccent(mainColor, mainColor.brighter(0.4f), Theme::buttonBody);
    cancelBtn.onClick = [this] {
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
            box->dismiss();
    };

    setSize(360, 200);
}

ArpPatternPopup::~ArpPatternPopup()
{
    // Both look-and-feels are members and would be destroyed after these
    // widgets anyway, but releasing them explicitly matches the rest of the
    // project and survives anyone reordering the members later.
    patternDisplay.setLookAndFeel(nullptr);
    hitsLabel     .setLookAndFeel(nullptr);
    stepsLabel    .setLookAndFeel(nullptr);
    rotateLabel   .setLookAndFeel(nullptr);
    hitsEditor    .setLookAndFeel(nullptr);
    stepsEditor   .setLookAndFeel(nullptr);
    rotateEditor  .setLookAndFeel(nullptr);
}

void ArpPatternPopup::paint(juce::Graphics& g)
{
    fxme::paintComponentBackground(g, getLocalBounds().toFloat(), mainColor);
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
    rotateLabel.setBounds(euclidRow.removeFromLeft(40));
    rotateEditor.setBounds(euclidRow.removeFromLeft(40));
    euclidRow.removeFromLeft(10);
    euclidBtn.setBounds(euclidRow);

    // Bottom: OK / Cancel
    auto buttonRow = area.removeFromBottom(30);
    okBtn.setBounds(buttonRow.removeFromLeft(buttonRow.getWidth() / 2).reduced(2, 0));
    cancelBtn.setBounds(buttonRow.reduced(2, 0));
}