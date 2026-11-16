# Plan: MPE in TeAr

> 2026-11-09. Written for a separate working session. Status: not started.
> Everything the engine needs is in FxmeTools since the shared pattern player work of 2026-11-09 (FxmeTools `doc/api-changes.md`). This plan is only TeAr's side.

## Goal

An **MPE option, off by default**. On:
- each note TeAr plays goes out on a channel of its own (2 to 16, the lower zone), with its own pitch bend;
- the pattern language's links work as real glides: `3~5` is one note whose bend moves to 5, `3^5` one attack with a jump.

Off: nothing changes for anyone, except that `~` and `^` now overlap the notes (below).

**Why:** per-note pitch needs per-note channels. It's the only way two arpeggiators can glide independently, or a chord can glide voice by voice (`{135}~{246}`).

## What FxmeTools now gives

- `fxme::Arpeggiator::setMpeOut (fxme::MpeOut* shared)`: the engine plays through a shared `MpeOut`. It's `nullptr` by default: plain notes on the channel given to `processBlock()`, exactly as before. The golden recordings (`core/tests/ArpeggiatorGolden.h`) are unchanged.
- `fxme::MpeOut` (`midi/Mpe.h`): the channels, the bends, and the zone announced once (the MPE configuration message, RPN 0 = ±48). `setMpe (bool)` turns it on and off; `reset()` announces the zone again before the next note.
- `ArpMidiEvent` grew `kind` (`Note`, `PitchBend`, `Controller`) and `value`. Without MPE every event is a `Note`.
- **Links in syntax 3**, MPE or not: `3~5` and `3^5`. Without MPE, the next note starts before this one ends, which a mono synth slurs.

## Changes in TeAr

1. **Pin.** `cmake/Dependencies.cmake`: FxmeTools to a commit that has the above. It is currently at `6441f62`, which is older than the links too.
   - The prefix form `~3` of syntax 3 is gone: it is now an error ("needs a sound before it"). Check factory presets and the language reference for it. Syntax-2 presets are not concerned (`~` and `^` stay ignored characters there).

2. **The processor** (`PluginProcessor`):
   - One `fxme::MpeOut mpeOut` member, shared by **all** the arpeggiators of the instance. This matters: they write to one MIDI output, so with one `MpeOut` each they would fight over the same channels. Off at construction (`mpeOut.setMpe (false)`).
   - A parameter `mpe`, an `AudioParameterBool`, default **false**. Saved with the state; old sessions and presets read it as off.
   - Wherever an `ArpInstance` is created or added: `arp.engine.setMpeOut (&mpeOut)`. `mpeOut` is a member of the processor, so the pointer stays valid when the arp vector grows (the vector's `ArpInstance`s may move; the `MpeOut` doesn't). Check `ArpInstance` copies keep the pointer.
   - In `processBlock`, when the parameter changed: `turnOff()` every arpeggiator (their note-offs go out on the channels they started on), **then** `mpeOut.setMpe (on)`. The change must happen between blocks' notes, or a held note's note-off is lost.
   - `addArpEvents` handles the kinds. **This is required:** today anything that isn't `noteOn` is written as a note-off.

     ```cpp
     switch (e.kind)
     {
         case fxme::ArpMidiEvent::Kind::PitchBend:  midi.addEvent (juce::MidiMessage::pitchWheel (e.channel, e.value + 8192), e.sample); break;
         case fxme::ArpMidiEvent::Kind::Controller: midi.addEvent (juce::MidiMessage::controllerEvent (e.channel, e.note, e.value), e.sample); break;
         case fxme::ArpMidiEvent::Kind::Note:       /* as today */ break;
     }
     ```
   - `mpeOut.reset()` in `prepareToPlay` and when the transport starts, so a synth loaded later hears the zone.
   - **Sample offsets:** a glide's points fall inside the block, and `juce::MidiBuffer` keeps them in time order.

3. **The window:**
   - An **MPE** toggle (`fxme::FxmeButton` on the parameter) in the header, with a tooltip saying the receiving synth must be in MPE mode, with a lower zone.
   - With MPE on, each arpeggiator's **Ch** box does nothing. Disable it (`setEnabled (false)`, polled in the timer as the house style says, so automation and presets are caught).

4. **Docs:**
   - `readme.md` and `doc/TeAr-language-reference.tex`: links `~` and `^` (syntax 3), and MPE. The pattern side is in Otomatotamoto's `doc/pattern-language.md` §9.2:
     - the sign goes **between** two steps;
     - the glide starts at the step written just before it, runs over the holds after it, and arrives on the next note, which is not attacked;
     - `3___~__4`: 3 for three steps, a glide over three;
     - a rest in between cancels it.
   - `doc/architecture.md`: the shared `MpeOut`, and why one.

## To test

No unit test can replace a real synth here; the engine's side is tested in FxmeTools (`CoreArpeggiatorTests`, `CoreMpeTests`).

1. **State:** old project → MPE off, same MIDI as before. Save with MPE on, reload: still on.
2. **MPE on, into an MPE synth** (Surge XT in MPE mode, Vital, or Camshaft after its own plan; a MIDI monitor first):
   - the configuration message and RPN 0 come once, before the first note;
   - each note is on its own channel, 2–16;
   - `1~5 . .` at 1/4 is one note-on, a stream of bends, one note-off;
   - `1^3` is one note-on and one jump;
   - `{135}~{246}` glides voice by voice;
   - two arpeggiators at once never share a channel while their notes are held.
3. **Toggle MPE while notes play:** no stuck notes.
4. **MPE off:** `1~5` overlaps (a mono synth slurs); the channel boxes work.
5. Fifteen notes is the zone's limit: a sixteenth shares the oldest channel. It's rare with a text arpeggiator; check it isn't a surprise on long releases.

## Open questions for that session

- Should the **Ch** box mean something with MPE on (several zones, or an upper zone)? Proposed: no; one lower zone, keep it simple.
- Should TeAr's **incoming** MPE keyboard be read (per-note bend from the player's hands)? Not in this plan.
