# TeAr architecture and roadmap

State of the plugin as of version 0.4.0. This file exists to prime a session
(human or assistant) quickly: what the pieces are, which decisions were
deliberate, and what is planned next.

For the pattern language as a user sees it, read the tables in `readme.md`, or
`doc/TeAr-language-reference.tex` for the full typeset reference (build it with
`pdflatex TeAr-language-reference.tex`, twice, for the table of contents; the
last page is a one-page card meant for printing). This document covers how the
language is implemented and why.

Keep the LaTeX reference in step with the parser when the language changes: it
documents syntax version 2, and the version is on its title page.

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
  ArpLookAndFeel.h          look-and-feel for the pattern text editor
  popupWindow.*             the current (small) pattern generator callout
  libs/FxmeTools/           submodule, shared across all FX-Mechanics plugins
tools/check-macos-artifact.sh   verifies a downloaded macOS release from Linux
doc/architecture.md         this file
```

The keyboard strip along the bottom is not in this list any more: it moved into
FxmeTools on 2026-08-19 as `fxme::ScaleKeyboardComponent`, since nothing in it
was specific to TeAr once the arpeggiator palette was replaced by a colour
callback. The editor sets that callback to `getArpColour` and otherwise uses it
as it did before.

CMake is the only build system. The Projucer file was deleted in 0.4.0: it had
not been part of the build for some time, still declared version 0.2, and
referenced files that no longer existed.

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
Factory presets live in `Source/presets/*.xml` and are globbed into the binary
data alongside the images; the manager keeps every embedded `*_xml` resource
whose root tag is `Parameters` and ignores the rest.

Two things a hand-written factory preset must get right. It needs
`patternSyntax="2"` on the root, or the migration will treat it as a
pre-0.4.0 file and rewrite its velocity digits. And its `arpNOn` parameters
must agree with the `on` attributes in the `Arpeggiators` child, because
`syncArpOnStatesFromParameters()` runs after the tree is read and the
parameters win.

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

Top to bottom: `fxme::TopBar` (logo, name, blurb, the teardrop icon centred in
the gap, version, preset strip and the triangle preset button parked at its
right end), a glowing identity line
straddling the bar's bottom edge, a row of global controls, the arpeggiator
tab row, the selected arpeggiator's panel, and the keyboard along the bottom.

The preset browser (`fxme::PresetComponent`) is an overlay covering the whole
working area, wrapped in an opaque backdrop because it paints only a
translucent panel of its own.

A `fxme::SplashOverlay` shows `Source/assets/Splash.png` once per plugin
instance, and again whenever the top bar logo is clicked. The once-per-run flag
(`TeArAudioProcessor::claimSplash()`) lives on the processor rather than the
editor, because the editor is destroyed and rebuilt every time the window is
closed and reopened. It is not serialised, so a reloaded session counts as a
new run. The overlay brings itself to the front when shown, so it does not
matter that it is added before most of the GUI.

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

## Step counting

`numSteps()`, `getPatternIndexForStep()` and `getStepForPatternIndex()` are the
same walk over the pattern asked three different questions, so they share a
single private `advanceOneToken()`. They used to carry a copy of the walk each,
and the copies had drifted: one counted `"` as a musical step while the others
treated it as a block marker, so in any pattern using a root-relative block the
playing-step highlight landed on the wrong characters, further out of place
with every `"` passed. Fixed in 0.4.0.

Anything added to the language has to be taught to `advanceOneToken()` and to
`isStepCommand()`, and nowhere else. The round-trip property
(`getStepForPatternIndex(getPatternIndexForStep(s)) == s` for every step) is
asserted in the tests across a spread of patterns, and it is what catches this
class of drift.

## Transport sync

`syncToPlayHead()` runs on every block while the host transport is playing,
whether or not any note is held. It does two things, and until 0.4.1 it only
did the first.

The step grid: it sets `samplesUntilNextNote` to the distance from the start of
the block to the next step boundary in the host timeline, recomputed from the
host position each block rather than accumulated, so step onsets cannot drift.
Positions within a millionth of a step of a boundary are snapped onto it, since
hosts do not report exact binary fractions and a position a hair short of a
boundary must not be read as a whole step still to go.

The pattern phase: it also places `pos` so that pattern step *n* falls on song
step *n* modulo the pattern length. Without this the pattern kept the grid but
was free to sit at any rotation against the bar, and which rotation you got
depended on how many steps had been consumed since the plugin loaded. Because
`setChord()` does not reset `pos` and the engine's `processBlock()` only runs
while notes are held, the rotation left over from one chord carried into the
next and never corrected itself. Reported from Bitwig as patterns starting an
eighth late and not repeating on the half note.

The correction is applied only when the current step differs from the one the
song asks for. Reassigning `pos` unconditionally would re-enter any `(` or `"`
that opens the step, pushing a duplicate scope entry every block. When it does
fire it clears the scope and probability stacks and returns the global octave
and velocity to their defaults, because a jump abandons whatever blocks were
open.

Patterns whose length varies are excluded, via `patternHasFixedLength()`
cached at `setPattern()` time. A probability group runs a different number of
steps depending on the roll, so there is no single length to take a modulo
against, and forcing `pos` each block would also yank the parser out of a
fallback group it was midway through. Those patterns keep the grid and are left
unanchored. Making them anchor correctly needs the rolls moved to the start of
each pass, which is the pre-roll design below.

## Known issues

Patterns containing probability groups (`pN (…):(…)`, or `pN (…)` with no
fallback) are not phase-anchored to the bar. See the section above and the
pre-roll design below.

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

## Prerequisite for per-sequence visual feedback

Showing each sequence's length and its current position needs an engine change
first, because today neither is knowable.

What happens now: a probability roll is evaluated lazily, at the moment
playback reaches the `p`. When the success and fallback branches have different
lengths (`p5 1:(1 2 3)` plays one step or three), the loop's length is decided
part way through the loop and differs from one pass to the next.

Two consequences, both characterised in the tests:

- `numSteps()` measures the success branch only, always, because the walk
  steps over `:` and its fallback as a single token. For `p0 1:(1 2 3)` it
  reports 1 while the loop plays 3. It is a property of the string, and for a
  variable-length pattern there is no single right answer it could give.
- `getCurrentStepIndex()` is derived from the character position by that same
  walk, so every step inside a fallback group reports the same index. The
  playing-step highlight already freezes there today.

`reset(positionInfo)` also divides by `ppqDuration()` to place the pattern in
the song grid when the transport starts mid-song, so it lands at the wrong
offset for these patterns. (`syncToPlayHead()` computes the duration but never
uses it, so ongoing sync is unaffected.)

The fix is the one Olivier proposed: resolve a loop's rolls at loop start
rather than as playback meets them. At the moment `pos` wraps to 0, walk the
pattern once deciding every `p` outcome, record the decisions, and let the
parser consume them instead of rolling. The loop then has a known length and a
meaningful step index before its first note sounds, which is exactly what a
position display needs.

Two things to get right when implementing it:

- The resolving walk has to agree with the playback parser about which
  branches are entered, including `p` nested inside a group (how many rolls a
  loop needs depends on which branches are taken, so it is a walk of the
  decision tree, not a scan for `p` in the string). Two walks that must agree
  is the same shape as the bug fixed in 0.4.0, so they should share code the
  way `advanceOneToken()` is shared.
- `?` (random degree) can stay lazy. It changes pitch, not length.

Note that the fallback group syntax is parentheses. Braces are not part of the
language: `p5 1:{123}` is read as a single-character fallback `{` followed by
three ordinary steps, and measures as four steps.

## Related open question

`v+`, `v-`, `V+` and `V-` work as of 0.4.0. If the generator starts emitting
velocity shaping, decide whether to use the relative commands (compact, and
they drift if the pattern length changes) or absolute levels (verbose, and
exactly reproducible).
