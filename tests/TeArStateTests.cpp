// The processor's pattern syntaxes across presets, saved projects and edits
// (doc/architecture.md, "Versioning and migration"): an old pattern keeps
// syntax 2 and plays as it always did; an edited or new one is syntax 3.

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "../Source/PluginProcessor.h"

#include <set>

namespace
{
    juce::MemoryBlock binaryOf (const juce::XmlElement& xml)
    {
        juce::MemoryBlock block;
        juce::AudioProcessor::copyXmlToBinary (xml, block);
        return block;
    }

    void load (TeArAudioProcessor& p, const juce::XmlElement& xml)
    {
        const auto block = binaryOf (xml);
        p.setStateInformation (block.getData(), (int) block.getSize());
    }

    std::unique_ptr<juce::XmlElement> saved (TeArAudioProcessor& p)
    {
        juce::MemoryBlock block;
        p.getStateInformation (block);
        return juce::AudioProcessor::getXmlFromBinary (block.getData(), (int) block.getSize());
    }

    juce::File presets() { return juce::File (TEAR_SOURCE_DIR).getChildFile ("Source/presets"); }
}

TEST_CASE ("every factory preset loads in syntax 2, its patterns untouched, and saves and reloads so")
{
    const auto files = presets().findChildFiles (juce::File::findFiles, false, "*.xml");
    REQUIRE (files.size() >= 7);
    for (const auto& f : files)
    {
        CAPTURE (f.getFileName());
        auto xml = juce::XmlDocument::parse (f);
        REQUIRE (xml != nullptr);
        TeArAudioProcessor p;
        p.prepareToPlay (48000.0, 512);
        load (p, *xml);
        const auto* arps = xml->getChildByName ("Arpeggiators");
        REQUIRE (arps != nullptr);
        int i = 0;
        for (const auto* arp : arps->getChildWithTagNameIterator ("Arp"))
        {
            CHECK (p.getArpeggiatorSyntax (i) == 2);
            CHECK (p.getArpeggiatorPattern (i) == arp->getStringAttribute ("pattern"));
            CHECK (p.getArpeggiatorError (i).isEmpty());   // syntax 2 reports nothing
            ++i;
        }
        CHECK (p.getNumArpeggiators() == i);

        // Saved: the state says syntax 3, each arpeggiator its own (2 here); reloaded: the same.
        auto state = saved (p);
        REQUIRE (state != nullptr);
        CHECK (state->getIntAttribute ("patternSyntax") == 3);
        TeArAudioProcessor q;
        q.prepareToPlay (48000.0, 512);
        load (q, *state);
        for (int k = 0; k < i; ++k)
        {
            CHECK (q.getArpeggiatorSyntax (k) == 2);
            CHECK (q.getArpeggiatorPattern (k) == p.getArpeggiatorPattern (k));
        }
    }
}

TEST_CASE ("an edited pattern becomes syntax 3 and stays so; an unchanged one keeps syntax 2; a new arpeggiator is syntax 3")
{
    auto xml = juce::XmlDocument::parse (presets().getChildFile ("OctaveClimb.xml"));
    REQUIRE (xml != nullptr);
    TeArAudioProcessor p;
    p.prepareToPlay (48000.0, 512);
    load (p, *xml);
    REQUIRE (p.getArpeggiatorSyntax (0) == 2);

    p.setArpeggiatorPattern (0, p.getArpeggiatorPattern (0));   // a field that lost focus, nothing changed
    CHECK (p.getArpeggiatorSyntax (0) == 2);

    p.setArpeggiatorPattern (0, "{135} _ (m32 1 2 3 4)");
    CHECK (p.getArpeggiatorSyntax (0) == 3);
    CHECK (p.getArpeggiatorError (0).isEmpty());

    p.addArpeggiator();
    CHECK (p.getArpeggiatorSyntax (p.getNumArpeggiators() - 1) == 3);

    auto state = saved (p);
    TeArAudioProcessor q;
    q.prepareToPlay (48000.0, 512);
    load (q, *state);
    CHECK (q.getArpeggiatorSyntax (0) == 3);
    CHECK (q.getArpeggiatorPattern (0) == "{135} _ (m32 1 2 3 4)");
    CHECK (q.getArpeggiatorSyntax (q.getNumArpeggiators() - 1) == 3);
}

TEST_CASE ("an invalid syntax-3 pattern says why, and the arpeggiator keeps its last valid one")
{
    TeArAudioProcessor p;
    p.prepareToPlay (48000.0, 512);
    p.setArpeggiatorPattern (0, "1 3 5");
    REQUIRE (p.getArpeggiatorError (0).isEmpty());
    p.setArpeggiatorPattern (0, "1 w 5");
    CHECK (p.getArpeggiatorError (0).contains ("unknown character"));
    CHECK (p.getArpeggiator (0).getProgram() != nullptr);
    CHECK (p.getArpeggiator (0).getProgram()->text() == "1 3 5");   // still playing
}

TEST_CASE ("a syntax-1 state (before versioning) is migrated to syntax 2, then read so")
{
    // A pattern with the old decimal velocity: v8 was the top level, F since syntax 2.
    auto xml = juce::XmlDocument::parse (presets().getChildFile ("OctaveClimb.xml"));
    REQUIRE (xml != nullptr);
    xml->removeAttribute ("patternSyntax");
    auto* arp = xml->getChildByName ("Arpeggiators")->getChildByName ("Arp");
    arp->setAttribute ("pattern", "v8 1 2 3");
    TeArAudioProcessor p;
    p.prepareToPlay (48000.0, 512);
    load (p, *xml);
    CHECK (p.getArpeggiatorPattern (0) == "vF 1 2 3");
    CHECK (p.getArpeggiatorSyntax (0) == 2);
}

// ---------------------------------------------------------------------------
// MPE (doc/mpe-plan.md): off by default; on, a note per channel and links that glide
// ---------------------------------------------------------------------------

namespace
{
    struct Heard
    {
        int ons = 0, offs = 0, bends = 0, controllers = 0;
        std::set<int> noteChannels;
    };

    /** One arpeggiator playing `pattern` (the others off), a C held from the first block. */
    void playing (TeArAudioProcessor& p, const juce::String& pattern)
    {
        p.prepareToPlay (48000.0, 512);
        for (int i = 1; i < p.getNumArpeggiators(); ++i)
            p.setArpeggiatorOn (i, false);
        p.setArpeggiatorPattern (0, pattern);
    }

    void setMpe (TeArAudioProcessor& p, bool on)
    {
        p.getAPVTS().getParameter ("mpe")->setValueNotifyingHost (on ? 1.0f : 0.0f);
    }

    Heard run (TeArAudioProcessor& p, int blocks, bool pressFirst, bool releaseLast = false)
    {
        Heard h;
        juce::AudioBuffer<float> audio (2, 512);
        for (int b = 0; b < blocks; ++b)
        {
            juce::MidiBuffer midi;
            if (b == 0 && pressFirst)
                midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
            if (b == blocks - 1 && releaseLast)
                midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
            p.processBlock (audio, midi);
            for (const auto m : midi)
            {
                const auto msg = m.getMessage();
                if (msg.isNoteOn()) { ++h.ons; h.noteChannels.insert (msg.getChannel()); }
                else if (msg.isNoteOff()) ++h.offs;
                else if (msg.isPitchWheel()) ++h.bends;
                else if (msg.isController()) ++h.controllers;
            }
        }
        return h;
    }
}

TEST_CASE ("MPE is off by default; an old session reads it off; saved on, it reloads on; a preset leaves it alone")
{
    TeArAudioProcessor p;
    p.prepareToPlay (48000.0, 512);
    CHECK (p.getAPVTS().getRawParameterValue ("mpe")->load() < 0.5f);

    setMpe (p, true);
    auto state = saved (p);
    REQUIRE (state != nullptr);
    TeArAudioProcessor q;
    q.prepareToPlay (48000.0, 512);
    load (q, *state);
    CHECK (q.getAPVTS().getRawParameterValue ("mpe")->load() > 0.5f);

    // A session saved before the option: no `mpe` at all.
    for (auto* child : state->getChildWithTagNameIterator ("PARAM"))
        if (child->getStringAttribute ("id") == "mpe")
        {
            state->removeChildElement (child, true);
            break;
        }
    load (q, *state);
    CHECK (q.getAPVTS().getRawParameterValue ("mpe")->load() < 0.5f);

    // MPE describes the synth, not the patterns.
    setMpe (q, true);
    REQUIRE (q.getPresetManager().loadFactoryPreset (0));
    CHECK (q.getAPVTS().getRawParameterValue ("mpe")->load() > 0.5f);
}

TEST_CASE ("MPE off: links are ignored, `1~3` plays as `1 3`, plain notes on the arpeggiator's channel")
{
    for (const char* text : { "m4 1~3 . .", "m4 1^3 . ." })
    {
        CAPTURE (text);
        TeArAudioProcessor p;
        playing (p, text);
        const auto h = run (p, 100, true);   // 51200 samples: two quarter notes at 120 (24000 each)
        CHECK (h.ons == 2);
        CHECK (h.offs >= 1);
        CHECK (h.bends == 0);
        CHECK (h.controllers == 0);
        CHECK (h.noteChannels == std::set<int> { 1 });
    }
}

TEST_CASE ("MPE on: the zone announced, a glide one attack on a member channel and bends; switched off, nothing hangs")
{
    TeArAudioProcessor p;
    playing (p, "m4 1~3 . .");
    setMpe (p, true);
    auto h = run (p, 60, true);   // into the second quarter: the glide has moved
    CHECK (h.ons == 1);
    CHECK (h.controllers == 6 * 16);   // RPN 0 and the zone, once
    CHECK (h.bends > 10);
    REQUIRE (h.noteChannels.size() == 1);
    CHECK (*h.noteChannels.begin() >= 2);

    // Switched off while the note sounds: its note-off, then plain notes again.
    setMpe (p, false);
    juce::AudioBuffer<float> audio (2, 512);
    juce::MidiBuffer midi;
    p.processBlock (audio, midi);
    int offs = 0;
    for (const auto m : midi)
        if (m.getMessage().isNoteOff() && m.getMessage().getChannel() == *h.noteChannels.begin())
            ++offs;
    CHECK (offs >= 1);

    h = run (p, 100, false, true);
    CHECK (h.controllers == 0);
    CHECK ((h.noteChannels.empty() || h.noteChannels == std::set<int> { 1 }));
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juce;   // the processor's preset manager and state need it
    doctest::Context context (argc, argv);
    return context.run();
}
