/*
  ==============================================================================

    ScaleComponent.cpp
    Created: 15 Dec 2025 5:32:30pm
    Author:  doare

  ==============================================================================
*/

#include "ScaleComponent.h"

ScaleComponent::ScaleComponent()
{
  setOpaque(false);
}

ScaleComponent::~ScaleComponent()
{
}

void ScaleComponent::paint(juce::Graphics &g)
{
    // Background
    // g.fillAll(juce::Colours::darkblue.darker(2.f));
    g.setColour (juce::Colours::darkblue.darker(2.f));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f);
    g.setColour (juce::Colours::white);
    g.drawRoundedRectangle (getLocalBounds().toFloat(), 10.0f, 2.f);

    const int numNotes = 12;
    const float width = (float)getWidth();
    const float height = (float)getHeight();
    const float noteWidth = width / numNotes;

    static const juce::String noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    const juce::Colour tickColour = juce::Colours::grey;
    const juce::Colour scaleTickColour = juce::Colours::white;
    const juce::Colour rootHighlightColour = juce::Colours::white;

    for (int i = 0; i < numNotes; ++i)
    {
        const float x = i * noteWidth;
        // The scaleNotes array can contain values > 11 to represent octave wrapping.
        // We need to check if any note in the scale corresponds to the current semitone (i).
        const bool isNoteInScale = std::any_of(scaleNotes.begin(), scaleNotes.end(),
                                               [i](int note) { return (note % 12) == i; });

        const bool isRootNote = (i == (rootNote % 12));

        // --- Draw Highlights ---
        // Draw root note first, so current notes can be drawn on top if they overlap.
        if (isRootNote)
        {
            g.setColour(rootHighlightColour.withAlpha(0.5f));
            g.drawRoundedRectangle(x+20.f, 5.0f, noteWidth-40.f, height-10.f,10.0f,2.f);
        }

        // Draw highlights for all currently playing notes
        for (const auto& noteInfo : currentNotes)
        {
            if (auto* obj = noteInfo.getDynamicObject())
            {
                int note = obj->getProperty("note");
                int arpIndex = obj->getProperty("arpIndex");

                if ((note % 12) == i)
                {
                    g.setColour(getColourForArp(arpIndex).withAlpha(0.9f));
                    g.fillRoundedRectangle(x+20.f, 5.0f, noteWidth - 40.f, height - 10.f, 10.0f);
                }
            }
        }
        // --- Draw Ticks ---
        float tickHeight = height * 0.3f;
        float tickThickness = 1.0f;

        if (isNoteInScale)
        {
            g.setColour(scaleTickColour);
            tickHeight = height * 0.3f; // Make ticks for notes in the scale longer
            tickThickness = 4.f;
        }
        else
        {
            g.setColour(tickColour);
        }
        
        // Use fillRect to draw a line with a specific thickness.
        float tickX = (x + noteWidth * 0.5f) - (tickThickness * 0.5f);
        g.fillRect(tickX, height - tickHeight, tickThickness, tickHeight);

        // --- Draw Note Names ---
        g.setColour(isNoteInScale ? scaleTickColour : tickColour);
        g.setFont(15.0f);
        g.drawText(noteNames[i], x, height - 30.0f, noteWidth, 12.0f, juce::Justification::centred);
    }
}

void ScaleComponent::updateScale(const juce::Array<int> &newScaleNotes, int newRootNote, const juce::Array<juce::var>& newCurrentNotes)
{
  scaleNotes = newScaleNotes;
  rootNote = newRootNote;
  currentNotes = newCurrentNotes;
  repaint();
}

juce::Colour ScaleComponent::getColourForArp(int arpIndex) const
{
    switch (arpIndex)
    {
        case 0: return juce::Colours::lime;
        case 1: return juce::Colours::cyan;
        case 2: return juce::Colours::magenta;
        case 3: return juce::Colours::yellow;
        default: return juce::Colours::green;
    }
}
