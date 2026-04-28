#include "KeyboardComponent.h"
#include "ArpInstance.h"

static const int kWhiteToSem[7]  = { 0, 2, 4, 5, 7, 9, 11 };
static const int kBlackSems[5]        = { 1, 3,  6,  8, 10 };
static const int kBlackCenterWhite[5] = { 1, 2,  4,  5,  6 };

//==============================================================================
KeyboardComponent::KeyboardComponent (int numOctaves_, int startOctave_)
    : numOctaves  (numOctaves_),
      startOctave (startOctave_),
      startMidi   (12 + startOctave_ * 12)
{
    setOpaque (false);
}

KeyboardComponent::~KeyboardComponent() {}

//==============================================================================
void KeyboardComponent::paint (juce::Graphics& g)
{
    const int   numWhiteKeys = numOctaves * 7;
    const float w   = (float) getWidth();
    const float h   = (float) getHeight();
    const float wkW = w / (float) numWhiteKeys;
    const float bkW = wkW * 0.62f;
    const float bkH = h * 0.62f;
    const float gap = 0.8f;

    // Component background
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.f);

    const bool hasScale = !scaleNotes.isEmpty();

    // Helpers -----------------------------------------------------------------
    auto isInScale = [&] (int midiNote) -> bool
    {
        const int semi = midiNote % 12;
        return std::any_of (scaleNotes.begin(), scaleNotes.end(),
                            [semi] (int n) { return (n % 12) == semi; });
    };

    auto isRoot = [&] (int midiNote) -> bool
    {
        return rootNote >= 0 && (midiNote % 12) == (rootNote % 12);
    };

    auto isInputNote = [&] (int midiNote) -> bool
    {
        return inputNotes.contains (midiNote);
    };

    auto playColour = [&] (int midiNote) -> juce::Colour
    {
        for (const auto& v : currentNotes)
            if (auto* obj = v.getDynamicObject())
                if ((int) obj->getProperty ("note") == midiNote)
                    return getColourForArp ((int) obj->getProperty ("arpIndex"));
        return juce::Colours::transparentBlack;
    };

    // --- White keys ----------------------------------------------------------
    for (int wk = 0; wk < numWhiteKeys; ++wk)
    {
        const int   oct      = wk / 7;
        const int   wIdx     = wk % 7;
        const int   midiNote = startMidi + oct * 12 + kWhiteToSem[wIdx];
        const float x        = wk * wkW;
        const juce::Rectangle<float> rect (x + gap, 1.f, wkW - gap * 2.f, h - 2.f);

        // Inverted: scale notes get a very light gray, non-scale notes are full white
        if (hasScale && isInScale (midiNote))
            g.setColour (juce::Colour (0xFFAAAAAA));
        else
            g.setColour (juce::Colours::white.withAlpha (0.92f));
        g.fillRoundedRectangle (rect, 2.f);

        // Playing-note arp fill
        const auto pc = playColour (midiNote);
        if (pc.getAlpha() > 0)
        {
            g.setColour (pc.withAlpha (0.75f));
            g.fillRoundedRectangle (rect, 2.f);
        }

        // Red outline for pressed MIDI input notes
        if (isInputNote (midiNote))
        {
            g.setColour (juce::Colours::red.brighter (0.2f));
            g.drawRoundedRectangle (rect.reduced (2.5f), 2.f, 2.f);
        }

        // Small red dot for root note
        if (isRoot (midiNote))
        {
            const float dotR = juce::jmin (4.f, wkW * 0.25f);
            g.setColour (juce::Colours::red.brighter (0.2f));
            g.fillEllipse (rect.getCentreX() - dotR,
                           rect.getBottom() - dotR * 2.f - 2.f,
                           dotR * 2.f, dotR * 2.f);
        }

        // Octave label on each C key
        if (wIdx == 0)
        {
            g.setColour (juce::Colours::darkgrey.darker());
            g.setFont (juce::Font (9.f));
            g.drawText ("C" + juce::String (startOctave + oct),
                        x, h - 14.f, wkW, 12.f, juce::Justification::centred);
        }
    }

    // --- Black keys ----------------------------------------------------------
    for (int oct = 0; oct < numOctaves; ++oct)
    {
        for (int bi = 0; bi < 5; ++bi)
        {
            const int   midiNote = startMidi + oct * 12 + kBlackSems[bi];
            const float cx       = (float) (oct * 7 + kBlackCenterWhite[bi]) * wkW;
            const juce::Rectangle<float> rect (cx - bkW * 0.5f, 1.f, bkW, bkH);

            // Inverted: scale notes get a very dark near-black, non-scale notes normal black
            if (hasScale && isInScale (midiNote))
                g.setColour (juce::Colour (0xFF555555));
            else
                g.setColour (juce::Colours::black.withAlpha (0.88f));
            g.fillRoundedRectangle (rect, 2.f);

            // Playing-note arp fill
            const auto pc = playColour (midiNote);
            if (pc.getAlpha() > 0)
            {
                g.setColour (pc.withAlpha (0.85f));
                g.fillRoundedRectangle (rect, 2.f);
            }

            // // Subtle glossy highlight at the top
            // g.setColour (juce::Colours::white.withAlpha (0.08f));
            // g.fillRoundedRectangle (rect.withHeight (bkH * 0.35f), 2.f);

            // Red outline for pressed MIDI input notes
            if (isInputNote (midiNote))
            {
                g.setColour (juce::Colours::red.brighter (0.2f));
                g.drawRoundedRectangle (rect.reduced (1.5f), 2.f, 1.5f);
            }

            // Small red dot for root note
            if (isRoot (midiNote))
            {
                const float dotR = juce::jmin (3.f, bkW * 0.25f);
                g.setColour (juce::Colours::red.brighter (0.2f));
                g.fillEllipse (rect.getCentreX() - dotR,
                               rect.getBottom() - dotR * 2.f - 1.f,
                               dotR * 2.f, dotR * 2.f);
            }
        }
    }

    // Outer border
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.f, 1.f);
}

//==============================================================================
void KeyboardComponent::updateScale (const juce::Array<int>&      newScaleNotes,
                                     int                           newRootNote,
                                     const juce::Array<juce::var>& newCurrentNotes,
                                     const juce::Array<int>&        newInputNotes)
{
    scaleNotes   = newScaleNotes;
    rootNote     = newRootNote;
    currentNotes = newCurrentNotes;
    inputNotes   = newInputNotes;
    repaint();
}

juce::Colour KeyboardComponent::getColourForArp (int arpIndex) const
{
    return getArpColour (arpIndex);
}
