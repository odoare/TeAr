/*
  ==============================================================================

    Theme.h

    Colours and geometry for the TeAr editor, in one place. The per-arpeggiator
    accents stay in ArpInstance.h (getArpColour) because the processor side
    refers to them too; everything else about how the GUI looks lives here.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <FxmeTools/components/AccentToggle.h>

namespace Theme
{
    // House palette, matching fxme::TopBar's defaults.
    inline const juce::Colour background   { 0xff14101a };
    inline const juce::Colour accent       { 0xffe0784a };
    inline const juce::Colour text         { 0xffd8d8e0 };
    inline const juce::Colour dimText      { 0xff9a9aa8 };
    inline const juce::Colour buttonBody   { 0xff2b2b2b };

    // The editor's own gradient, kept from the original design.
    inline const juce::Colour panelTint    { juce::Colour::fromFloatRGBA (0.15f, 0.15f, 0.25f, 1.0f) };

    // Geometry
    inline constexpr int topBarHeight      = 54;
    inline constexpr int presetBarWidth    = 190;
    inline constexpr int presetToggleWidth = 74;
    inline constexpr int tabRowHeight      = 32;
    inline constexpr int tabWidth          = 34;
    inline constexpr int onOffWidth        = 34;

    /** How long a selection button stays lit after its arpeggiator fires, and
        how the flash decays. One frame at 60 Hz is ~16 ms, so a 140 ms tail
        reads as a blink rather than a flicker at any sensible step rate. */
    inline constexpr float blinkDecayPerSecond = 7.0f;

    /** The standard FX-Mechanics latching button, in an arpeggiator's accent. */
    inline void accentToggle (fxme::AccentToggle& b, juce::Colour accentColour)
    {
        b.setAccent (accentColour, accentColour.withMultipliedSaturation (0.4f), buttonBody);
        b.setFlashColour (juce::Colours::white);
    }
}
