/*
  ==============================================================================

    Theme.h

    Colours and geometry for the TeAr editor, in one place.

    The identity is a green -> cyan -> magenta ramp (tearAt), used both as a
    gradient — the glowing line under the top bar — and as the first three
    per-arpeggiator accents. The arpeggiator palette itself stays in
    ArpInstance.h (getArpColour) because the processor side refers to it too;
    its first three entries are the ramp's stops.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/components/AccentToggle.h>

namespace Theme
{
    // The TeAr ramp. Deliberately literals rather than cross-references:
    // inline variables in a header must not depend on each other's
    // initialisation order.
    inline const juce::Colour tearGreen   { 0xff00ff00 };
    inline const juce::Colour tearCyan    { 0xff00ffff };
    inline const juce::Colour tearMagenta { 0xffff00ff };

    /** The ramp at `t`: 0 = green, 0.5 = cyan, 1 = magenta. The single source
        of the colour code — anything spreading across the identity samples
        this. */
    inline juce::Colour tearAt (float t) noexcept
    {
        t = juce::jlimit (0.0f, 1.0f, t);
        return t < 0.5f ? tearGreen.interpolatedWith (tearCyan, t * 2.0f)
                        : tearCyan.interpolatedWith (tearMagenta, (t - 0.5f) * 2.0f);
    }

    /** The identity line under the top bar: a crisp ramp with a soft glow
        bleeding above and below it.

        The glow is a stack of ever-taller, ever-fainter copies of the same
        ramp. On the dark backdrop that reads as light spilling off the line
        rather than as a drawn halo, and it costs a handful of gradient fills,
        so it needs no blur buffer. */
    inline void paintTearLine (juce::Graphics& g, juce::Rectangle<float> line,
                               float glowHeight = 16.0f)
    {
        auto fillRamp = [&g] (juce::Rectangle<float> b, float alpha)
        {
            juce::ColourGradient grad (tearGreen  .withMultipliedAlpha (alpha), b.getTopLeft(),
                                       tearMagenta.withMultipliedAlpha (alpha), b.getTopRight(),
                                       false);
            grad.addColour (0.5, tearCyan.withMultipliedAlpha (alpha));
            g.setGradientFill (grad);
            g.fillRect (b);
        };

        // Halo first, widest and faintest, so the crisp line lands on top of
        // it; the layers accumulate towards the centre, which is the falloff.
        constexpr int layers = 5;
        for (int i = layers; i >= 1; --i)
        {
            const float t = (float) i / (float) layers;   // 1 = outermost
            fillRamp (line.expanded (0.0f, glowHeight * t), 0.10f * (1.0f - t) + 0.025f);
        }

        fillRamp (line, 1.0f);
    }

    // House palette, matching fxme::TopBar's defaults except for the accent,
    // which is the middle of the ramp so the chrome belongs to the identity.
    inline const juce::Colour background   { 0xff14101a };
    inline const juce::Colour accent       { 0xff00ffff };   // = tearCyan
    inline const juce::Colour text         { 0xffd8d8e0 };
    inline const juce::Colour dimText      { 0xff9a9aa8 };
    inline const juce::Colour buttonBody   { 0xff2b2b2b };

    // Geometry
    inline constexpr int topBarHeight      = 54;
    inline constexpr int presetBarWidth    = 190;
    inline constexpr int presetToggleWidth = 28;
    inline constexpr int tabRowHeight      = 32;
    inline constexpr int tabWidth          = 34;
    inline constexpr int onOffWidth        = 34;
    inline constexpr int keyboardHeight    = 60;

    /** How long a selection button's outline stays lit after its arpeggiator
        fires. One frame at 60 Hz is ~16 ms, so this decay leaves a visible
        blink at any sensible step rate without smearing into the next one. */
    inline constexpr float blinkDecayPerSecond = 7.0f;

    /** The standard FX-Mechanics latching button, in an arpeggiator's accent.
        Used for the ON switch, whose toggle state *is* the on/off: a dimmed
        label there means "muted". */
    inline void accentToggle (fxme::AccentToggle& b, juce::Colour accentColour)
    {
        b.setAccent (accentColour, accentColour.withMultipliedSaturation (0.4f), buttonBody);
        b.setFlashColour (juce::Colours::white);
    }

    /** An arpeggiator selection tab. Two independent things to show, so they
        use two channels: the toggle state is *selected* (lit body), and the
        arpeggiator's own on/off is the saturation.

        A tab that is playing but not selected therefore gets its number in the
        full accent — the same colour the panel and the keyboard use for that
        arpeggiator — so it is unmistakably distinct from a muted one, which
        falls back to near-grey. */
    inline void arpTabToggle (fxme::AccentToggle& b, juce::Colour col, bool arpIsOn)
    {
        b.setAccent (arpIsOn ? col : col.withSaturation (0.2f).darker (0.4f),   // selected body
                     arpIsOn ? col : col.withSaturation (0.15f).darker (0.25f), // unselected text
                     buttonBody);
        b.setFlashColour (juce::Colours::white);
    }
}
