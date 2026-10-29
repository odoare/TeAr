// The processor's pattern syntaxes across presets, saved projects and edits
// (doc/architecture.md, "Versioning and migration"): an old pattern keeps
// syntax 2 and plays as it always did; an edited or new one is syntax 3.

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "../Source/PluginProcessor.h"

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

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juce;   // the processor's preset manager and state need it
    doctest::Context context (argc, argv);
    return context.run();
}
