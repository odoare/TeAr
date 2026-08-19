/*
  ==============================================================================

    ArpLookAndFeel.h

    What is left of TeAr's own look-and-feel: the rounded dark box behind the
    pattern text editor, and the suppression of the stock outline that would
    otherwise be drawn on top of it.

    Everything else this class used to do is now fxme::FxmeLookAndFeel's job.
    Its combo boxes, drop-down menus and labels all draw in the house style, so
    the local drawComboBox, drawLabel and getLabelFont overrides were removed
    once the controls moved across. FxmeLookAndFeel does not override
    fillTextEditorBackground or drawTextEditorOutline, which is the only reason
    this class still exists; if those two ever move upstream it can go
    altogether.

    Applied to one widget: ArpeggiatorComponent's pattern field.

  ==============================================================================
*/

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

    /** Deliberately empty: fillTextEditorBackground has already drawn the
        rounded outline, and the stock square one would sit across its corners. */
    void drawTextEditorOutline (juce::Graphics&, int, int, juce::TextEditor&) override {}
};
