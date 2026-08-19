#pragma once
#include <JuceHeader.h>

class ArpLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor& editor) override
    {
        g.setColour (juce::Colours::darkblue.darker (2.f));
        g.fillRoundedRectangle (juce::Rectangle<int> (0, 0, width, height).toFloat(), 10.0f);
        g.setColour (editor.findColour (juce::TextEditor::outlineColourId));
        g.drawRoundedRectangle (juce::Rectangle<int> (0, 0, width, height).toFloat(), 10.0f, 2.f);
    }

    void drawTextEditorOutline (juce::Graphics&, int, int, juce::TextEditor&) override {}

    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        g.fillAll (label.findColour (juce::Label::backgroundColourId));
        if (!label.isBeingEdited())
        {
            auto alpha = label.isEnabled() ? 1.0f : 0.5f;
            g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (alpha));
            g.setFont (getLabelFont (label));
            g.drawText (label.getText(),
                        getLabelBorderSize (label).subtractedFrom (label.getLocalBounds()),
                        label.getJustificationType(), true);
            g.setColour (label.findColour (juce::Label::outlineColourId).withMultipliedAlpha (alpha));
        }
        else if (label.isEnabled())
        {
            g.setColour (label.findColour (juce::Label::outlineColourId));
        }
        g.drawRect (label.getLocalBounds());
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto cornerSize = box.getHeight() * 0.2f;
        juce::Path p;
        p.addRoundedRectangle (0, 0, width, height, cornerSize);
        g.setColour (juce::Colours::darkblue.darker (2.f));
        g.fillPath (p);
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.strokePath (p, juce::PathStrokeType (2.0f));
        auto arrowZone = juce::Rectangle<int> (width - 30, 0, 20, height);
        juce::Path arrow;
        arrow.addTriangle (arrowZone.getX() + 5,  arrowZone.getCentreY() - 3,
                           arrowZone.getRight() - 5, arrowZone.getCentreY() - 3,
                           arrowZone.getCentreX(), arrowZone.getCentreY() + 4);
        g.fillPath (arrow);
    }

    // Spelled out rather than `return {15.0f};`: the braced form goes through
    // the deprecated Font(float) constructor by implicit conversion, which a
    // grep for `juce::Font (` does not find.
    juce::Font getLabelFont (juce::Label&) override
    {
        return juce::Font (juce::FontOptions (15.0f));
    }
};
