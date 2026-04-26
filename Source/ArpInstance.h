#pragma once
#include <JuceHeader.h>
#include "libs/cppMusicTools/Arpeggiator.h"

// Color palette — first four match the original hardcoded colors
inline juce::Colour getArpColour (int index)
{
    static const juce::Colour palette[] = {
        juce::Colours::lime,
        juce::Colours::cyan,
        juce::Colours::magenta,
        juce::Colours::yellow,
        juce::Colours::orange,
        juce::Colour (0xff88ccff),   // light blue
        juce::Colour (0xffff88cc),   // pink
        juce::Colour (0xff88ffcc),   // mint
        juce::Colour (0xffffcc88),   // peach
        juce::Colour (0xffcc88ff),   // lavender
        juce::Colour (0xffccff88),   // yellow-green
        juce::Colour (0xffff8888),   // coral
    };
    static const int n = (int) (sizeof (palette) / sizeof (palette[0]));
    return palette[((index % n) + n) % n];
}

struct ArpInstance
{
    Arpeggiator engine;
    juce::String pattern    { "1 2 3" };
    bool         onState    { true };
    int          midiChannel{ 1 };
    int          subdivision{ 4 };      // index 4 = 1/16 (default)
};
