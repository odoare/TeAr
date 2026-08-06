# TeAr architecture and roadmap

State of the plugin as of version 0.4.0. This file exists to prime a session
(human or assistant) quickly: what the pieces are, which decisions were
deliberate, and what is planned next.

For the pattern language as a user sees it, read the tables in `readme.md`.
This document covers how it is implemented and why.

---

## What TeAr is

A text arpeggiator. Each arpeggiator holds a pattern written as a short string
(`1 2 3 . _ 5`), and the plugin turns held MIDI notes plus that string into a
stream of MIDI notes. It is an instrument (`aumu` / `IS_SYNTH TRUE`) that
produces MIDI rather than audio, because instrument-with-MIDI-out is the only
shape that works consistently across Logic, Live, Cubase, Bitwig, Reaper,
Studio One and FL Studio. Logic's dedicated MIDI FX slot would be a more
natural home, but plugins there are AU only.

Several arpeggiators run at once (up to `MAX_ARPS` = 16, four by default), each
with its own pattern, step duration, MIDI output channel and on/off state.

---

## Layout

```
CMakeLists.txt              project(TeAr VERSION 0.4.0) is the single version source
Source/
  PluginProcessor.{h,cpp}   APVTS, the arpeggiator vector, state, presets
  PluginEditor.{h,cpp}      the whole GUI
  Theme.h                   colours, geometry, the identity ramp
  ArpInstance.h             one arpeggiator's data + getArpColour()
  ArpeggiatorComponent.*    one arpeggiator's panel (text, ON, duration, channel)
  KeyboardComponent.*       the keyboard along the bottom
  ArpLookAndFeel.h          look-and-feel for the pattern text editor and combos
  popupWindow.*             the current (small) pattern generator callout
  ScaleComponent.*          unused, superseded by KeyboardComponent
  FxmeLogo.*                unused, superseded by fxme::TopBar
  libs/FxmeTools/           submodule, shared across all FX-Mechanics plugins
tools/check-macos-artifact.sh   verifies a downloaded macOS release from Linux
doc/architecture.md         this file
```

`ScaleComponent` and `FxmeLogo` are still compiled (they are listed in
`target_sources`) but nothing references them. They can be removed whenever
someone is confident enough to do it.

---

## Processor

`TeArAudioProcessor` holds three things that interact carefully.

**APVTS parameters.** `chordMethod`, `scaleRoot`, `scaleType`, `followMidiIn`,
and `arp0On` … `arp15On`. Only the on/off flags are per-arpeggiator, because
those are the only per-arpeggiator values worth automating.

**The arpeggiator vector.** `std::vector<ArpInstance> arps`, guarded by
`arpsLock`. Each entry carries the engine (`fxme::Arpeggiator`), the pattern
string, the MIDI channel, the step subdivision and a cached on/off. The count
is dynamic and the patterns are text, so these cannot be APVTS parameters.

**`fxme::PresetManager`.** Presets are just the APVTS state written to XML.

### The side-state problem, and how it is solved

A preset only ever carries `apvts.state`, so an arpeggiator living outside it
would not be saved, and a TeAr preset without patterns is useless. The
arpeggiators are therefore mirrored into an `"Arpeggiators"` child of
`apvts.state`:

- `buildArpsTree()` snapshots `arps` into a detached tree (takes `arpsLock`).
- `storeArpsInState()` refreshes that child on the live state. It runs on the
  message thread from every arpeggiator mutator, which is also what marks the
  preset dirty so the preset bar shows its asterisk after a pattern edit.
- `loadArpsFromTree()` rebuilds `arps` from such a tree.
- The manager's `onBeforeSave` and `onAfterLoad` hooks tie the two ends.

`getStateInformation()` does the same refresh against the deep copy that
`copyState()` returns, so it never mutates the tree the GUI is reading.

The on/off flags stay authoritative in the APVTS (they are the automatable
ones), so `buildArpsTree()` reads them from the parameter rather than from the
cached copy, and `syncArpOnStatesFromParameters()` pushes them back after a
load.

One consequence worth remembering: `removeArpeggiator()` has to shift the
`arpNOn` parameter values down, because those are indexed by slot while the
instances shift. Without that, deleting an arpeggiator in the middle leaves
every one after it wearing its neighbour's on/off state, which surfaces later
as the wrong arpeggiator being muted.

### Versioning and migration

Two separate numbers are written as properties on `apvts.state` (not as
attributes added at `getStateInformation()` time, because presets never pass
through it and would carry no version at all):

- `pluginVersion`, informational, the same string the top bar shows.
- `patternSyntax`, the integer migration actually tests. It moves only when
  the pattern language moves, which the plugin version does not.

Absent means the state predates versioning, so syntax 1. Currently
`currentPatternSyntax` is 2, and `migrateArpPatterns()` runs
`fxme::Arpeggiator::migratePatternV1toV2()` over every pattern on load.

Syntax 2 (new in 0.4.0) made degrees and velocity levels uppercase hexadecimal.
Only velocity changed meaning, so only velocity is converted. Migration only
fires where TeAr controls the load path: a pattern pasted in by hand is read
with the new meaning, which is why the change is called out in `readme.md`.

---

## Editor

Top to bottom: `fxme::TopBar` (logo, name, blurb, version, preset strip and
the triangle preset button parked at its right end), a glowing identity line
straddling the bar's bottom edge, a row of global controls, the arpeggiator
tab row, the selected arpeggiator's panel, and the keyboard along the bottom.

The preset browser (`fxme::PresetComponent`) is an overlay covering the whole
working area, wrapped in an opaque backdrop because it paints only a
translucent panel of its own.

`GlowLine` is a component rather than a `paint()` call because half the glow
falls inside the top bar, and only a sibling stacked above it can bleed over a
child. Anything added to the editor later must be added before it, or bring it
back to the front.

### The two ways to turn an arpeggiator on and off

The tab buttons select. Clicking the tab that is already selected also toggles
that arpeggiator, and the ON switch in the panel below does the same thing
explicitly.

The tabs deliberately do **not** use a radio group and do not self-toggle.
JUCE turns the other buttons of a radio group off with click notification, so
selecting a different arpeggiator would fire `onClick` on the previously
selected one and flip it behind the user's back. The lit state is set
explicitly in `updateTabAppearance()` instead, which leaves `onClick` meaning
"this button was actually clicked".

A tab shows two independent things on two channels: a lit body means selected,
and saturation means playing. An arpeggiator that is on but not selected gets
its number in the full accent, a muted one falls back to near grey.

Each tab also blinks when its arpeggiator fires, polled at 60 Hz from
`fxme::Arpeggiator::getNoteOnCount()` (a monotonic counter, so a change since
the last frame means it fired). The blink is an outline rather than a tint of
the body: at the rates a sequencer runs, a body that lights up reads as
flicker and buries the button's own colour.

### Identity

Green, cyan, magenta. `Theme::tearAt(t)` is the single source of the ramp, used
both for the glow line under the top bar and as the first three entries of
`getArpColour()`, so the line and the tabs are visibly the same colour code.
The chrome accent is cyan, the middle of the ramp.

---

## What comes from FxmeTools

`TopBar`, `PresetManager`, `PresetComponent`, `PresetBarComponent`,
`AccentToggle`, `TextEntryFocusFixer`, `FxmeLookAndFeel`, and the engine
itself (`midi/Arpeggiator.h`, `midi/MidiTools.h`).

The submodule is shared with every other FX-Mechanics plugin, so changes there
are changes to a shared library: keep the public API backward compatible, and
commit the submodule before bumping the pointer in the parent.

`Arpeggiator.h` carries the unit tests that matter most here
(`libs/FxmeTools/tests/test_arpeggiator.cpp`, 36 cases). Run them with
`Source/libs/FxmeTools/tests/run_tests.sh`, or configure with
`-DTEAR_BUILD_TESTS=ON` and `ctest -R ArpeggiatorTests`. Note that
`build_tests/` may still hold a stale `cppMusicToolsTests` binary from an
earlier layout; the current one is `FxmeToolsTests`.

---

## Known issues

- `getStepForPatternIndex()` counts `"` as a step while `numSteps()` and
  `getPatternIndexForStep()` do not. This makes the playing-step highlight
  drift in patterns using root-relative blocks. Left as it was rather than
  changed under a syntax migration.
- `ScaleComponent` and `FxmeLogo` are dead code that still compiles.
- `ArpeggiatorComponent::ArpTextEditor` keeps its own asynchronous focus grab,
  which is now redundant next to the editor's `fxme::TextEntryFocusFixer`.
  Harmless, but only one of them needs to exist.

---

## Release

`project(TeAr VERSION x.y.z)` in `CMakeLists.txt` is the only place the version
is set. The manual-run fallback in `.github/workflows/release.yml` duplicates
it and drifts, so check it when bumping.

Before tagging: run the release workflow through `workflow_dispatch`, let the
verify step pass, then download the macOS zip and check it with
`tools/check-macos-artifact.sh`. The artifact is what users get, the build tree
is not.

The macOS settings that must not be disturbed: the `if(APPLE)` block sits
before `project()` (CMake reads the Apple toolchain settings there, and after
it the deployment target is ignored outright), the target is 10.13 rather than
11.0 (11.0 silently excludes Catalina), and both are also passed as `-D` on the
CI configure line so file ordering cannot defeat them. The verify step must be
able to fail: a step that only prints `lipo -info` is how an arm64-only
"universal" release ships.

The plugin is not notarised, so a downloaded bundle is quarantined and the DAW
skips it silently. That is what the `xattr -dr com.apple.quarantine` section in
`readme.md` is for.

---

## Working conventions

- Do not run plugin builds. Olivier compiles himself. A `cmake` configure is
  fine, and a syntax check against `build/compile_commands.json` is cheap. If a
  build is genuinely necessary, never `-j$(nproc)` (Release links with LTO):
  use `-j2` and one named target.
- Do not commit or push. When work spans both repositories, say precisely what
  to commit where, submodule first.
- Builds do not auto-install. The `.vst3` has to be copied and the DAW has to
  rescan, because it caches the module.
- Documentation is plain prose, emphasis used sparingly, parentheses preferred
  over dashes.

---

# Roadmap: a better pattern generator

Deferred past 0.4.0. This section is the design brief.

## Why the current one feels flat

`makeEuclidianPattern()` emits `1 . 1 . .`, so every onset is degree 1. It
generates rhythm and no pitch at all.

`makeRandomPattern()` samples each step independently from a fixed probability
table. Independent draws are white noise: no repetition, no phrase, no contour,
and nothing at step 9 relates to step 3. It also uses almost none of the
language, emitting no `( )`, no `p`, no `:` and no `"`. TeAr's syntax is far
richer than what its own generator writes.

So the goal is not better randomness. It is generating structure, and
generating text that uses the language.

## Ideas, roughly in order of payoff

**1. Pitch strategies for the Euclidean generator.** The rhythm is already
good, it just needs notes on the onsets. A choice of contour (run up, run down,
arch, pedal and move, alternate, random from chord) turns one generator into a
dozen. `E(5,8)` with an arch becomes `1 . 3 5 . 3 1 .` instead of
`1 . 1 1 . 1 1 .`. Cheap, and immediately musical.

**2. Motif and development.** Generate a cell of two to four steps, then build
the pattern by transforming it: repeat, transpose (`+` / `-`), retrograde,
invert, octave-displace by wrapping in `(O+ … )`, augment with `_`. Something
like `1 2 3 . 2 3 4 . (O+ 1 2 3) . 3 2 1 _`. This is the biggest single jump
from random to composed, because a listener hears the cell return, and it is
the only idea here that exercises the block syntax.

**3. Patterns that are themselves generative.** Emit `p` steps so the pattern
keeps evolving as it plays: `1 . p6 3:. 2 p8 (5 6):(4)`. One generated pattern
then never repeats exactly. This is unique to TeAr, since the probability lives
in the text where the user can read and edit it afterwards.

**4. Grammar and L-system rewriting.** Rules like `A → A B A`, `B → . A`
expanded a few generations give genuine self-similar long-range structure. It
fits the plugin's identity better than anything else here: TeAr is a text
arpeggiator, and L-systems are string rewriting. Two or three preset grammars
plus a depth control would produce patterns that no amount of per-step
randomness can.

**5. Multi-arpeggiator generation.** Generate a related set at once:
complementary Euclidean rotations that interlock, a hocket splitting one
cycle's onsets between arpeggiators, or call and response. No single
arpeggiator plugin can do this, and TeAr already has the architecture and the
per-arpeggiator MIDI channels to make it worth something.

**6. Mutate instead of reroll.** A vary button that makes a small edit to the
current pattern (swap two steps, nudge a degree, add a rest, wrap a run in
`(O+ … )`) with an amount control. Rerolling from scratch means never
converging on something you nearly liked. Pair it with a visible seed so a good
roll can be recovered.

**7. Real controls.** The dialog currently offers hits, steps and rotation for
Euclidean and nothing at all for random. Length, density, rest and sustain
probability, register spread, syncopation and chromaticism would make the
existing algorithms usable without changing them.

## Suggested order

1, 2 and 6 first: rhythm, melody and workflow, each independently useful.
Then 3, which is pure syntax generation with no new algorithm. Then 4 if TeAr
should have a generator no other arpeggiator has.

## Where the code goes

The algorithms are generic to any `fxme::Arpeggiator` user, so they belong in
FxmeTools as a `fxme::ArpPatternGenerator` (header-only, taking a seed and a
small options struct, returning a pattern string) next to `Arpeggiator.h`. The
dialog stays in TeAr's `Source/`.

`Arpeggiator::valueChar()` and `hexValue()` are already exposed for exactly
this: a generator emitting degrees or velocity levels above 9 needs them.

The current `ArpPatternPopup` is a 360x200 `CallOutBox` and will not hold a
strategy selector plus a handful of sliders. It wants to become a proper panel
with a preview and Regenerate / Apply, in the FX-Mechanics control style
(`fxme::FxmeSlider`, `fxme::AccentToggle`, one `fxme::TextEntryFocusFixer`).

## Related open question

`v+`, `v-`, `V+` and `V-` work as of 0.4.0. If the generator starts emitting
velocity shaping, decide whether to use the relative commands (compact, and
they drift if the pattern length changes) or absolute levels (verbose, and
exactly reproducible).
