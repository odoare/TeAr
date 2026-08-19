# TeAr — FX-Mechanics compliance audit

Revisions:

- 2026-08-18, first pass. Project commit `f4cb950` ("Tag 0.4.2"), FxmeTools
  pinned at `03a1b11`, twelve commits behind `origin/main`.
- 2026-08-18, revised after FxmeTools was synced to `origin/main`. S1 is done,
  R1 / R3 / R8 are unblocked, H3 is narrowed, H8 is new.
- 2026-08-18, S2 fixed in `Source/libs/FxmeTools/FxmeTools/midi/Arpeggiator.h`,
  with the engine's test suite run (60 cases, 452 assertions, all passing) and
  the allocation measured away. Committed as FxmeTools `a3de21f` and parent
  `2ef7142`; the submodule is pushed, the parent commit is not.
- 2026-08-18, S3 fixed in `Source/PluginProcessor.cpp`, plus a dead binding
  removed from `Source/PluginEditor.cpp`. All five translation units then
  compiled individually with the plugin's own flags: zero errors. That closed
  the build-warning coverage caveat and added R9, R10 and H9. Committed as
  `831d07d`.
- 2026-08-18, S4 fixed in `Source/PluginProcessor.{h,cpp}` and its one call
  site. Sweeping the rest of the API for the same defect class added S7.
- 2026-08-18, S5 fixed in
  `Source/libs/FxmeTools/FxmeTools/components/TextEntryFocusFixer.h`, verified
  with a runtime probe over a real juce::CallOutBox. Committed as FxmeTools plus
  parent `516db93`.
- 2026-08-18, S6 fixed in `.github/workflows/ci.yml`. Committed as `991d8f5`.
- 2026-08-18, S7 fixed across `midi/Arpeggiator.h` (both generators made
  static) and `Source/PluginEditor.cpp`. **Every silent bug, S1 to S7, is now
  closed.**
  Committed as FxmeTools `ce4b540` plus parent `e5d71de`.
- 2026-08-18, H7 fixed in both workflows and committed.
- 2026-08-18, R1, R2 and R3 done: the five combo boxes and both accent calls,
  verified with a rendering probe. Committed as `c5d6663`.
- 2026-08-18, R5 and R9 done. TeAr's own sources are now warning-free apart
  from R10. Committed as `99f46bb`.
- 2026-08-19, R8 done: the scale controls grey out when the mode ignores them,
  verified by rendering. Committed as `9bd4dee`.
- 2026-08-19, R10 done. **TeAr's own sources now compile with no warnings at
  all**, down from 45. Committed as `f3d0b09`.
- 2026-08-19, R4, R6 and R7 done in one pass: a TooltipWindow, the Follow MIDI
  In toggle moved to fxme::FxmeButton, and an InfoButton carrying the pattern
  language. Committed as `4fa40c4`. **The whole retrofit set, R1 to R10, is
  closed.**
- 2026-08-19, H1 done: the dead cppMusicTools submodule removed, with
  doc/development.md updated. Committed as `777587f`.
- 2026-08-19, H2 done: KeyboardComponent moved into FxmeTools as
  fxme::ScaleKeyboardComponent, verified pixel-identical against the original.
  Spans both repos.
- 2026-08-19, H3 done: ArpLookAndFeel is down to its two text-editor overrides,
  the labels having moved to the house look-and-feel.
- 2026-08-19, H4, H5, H6 and H8 done, and H9 in part: the popup restyled, two
  stale version strings corrected, the backdrop moved to
  fxme::paintComponentBackground, and four real defects cleared from FxmeTools
  headers. Clearing those turned up H10, which is new and deliberately not
  fixed. **Every S and R item is closed, and H1 to H9 with it**; H9's warning
  sweep and H10 are what remain. Uncommitted, and spans both repos.

Audited against JUCE 8.0.12 in `../JUCE`, at project commit `f4cb950`
("Tag 0.4.2").

FxmeTools submodule: now at `a3de21f` ("No more heap allocation in audio
thread"), which is `origin/main` exactly and is pushed. That is the S2 fix,
sitting on top of `4b22e3c` ("SphereView"), which was itself a fast-forward from
the original `03a1b11` pin, so nothing was rewritten anywhere.

Parent repo: `2ef7142` ("Sync to FxmeTools (no more heap allocation in the audio
buffer)") carries the bumped pointer. It is committed but **not yet pushed**.

Of every FxmeTools header TeAr uses (`TopBar.h`, `PresetBarComponent.h`,
`PresetComponent.h`, `SplashOverlay.h`, `TextEntryFocusFixer.h`,
`AccentToggle.h`, `FxmeButton.h`, `PresetManager.h`, `midi/Arpeggiator.h`,
`midi/MidiTools.h`, `lookandfeels/FxmeLookAndFeel.h`), the bump changed
`FxmeLookAndFeel.h` and nothing else (`git diff --stat 03a1b11 HEAD` over that
set). So there is no API break to absorb, and the two FxmeTools findings that
are not about the look-and-feel (S2 and S5) are untouched by the sync: those two
files are byte-identical across the bump.

The audit itself is **static**: the plugin was never built, and every finding
was checked against the actual JUCE and FxmeTools sources in this working tree
rather than asserted from the house checklist, with the places where the generic
rule does not apply recorded in "Already correct" below.

Findings have since been acted on. S1, S2 and S3 are fixed. S1 and S2 are
committed and the submodule is pushed; S3 and this file are still uncommitted.

The fixes were built and tested, unlike the audit: the `FxmeToolsTests` console
target (Debug, no LTO), an allocation probe, and a per-file compile of all five
of TeAr's translation units with the plugin's own flags. The plugin itself has
still never been linked here, so nothing below reports on a running build.

Ids are stable: cite them as S1, R3, H2 in later sessions. Each item is marked
**safe to apply** (mechanical, no behaviour change) or **decision** (changes
behaviour, product surface, or a shared library).

---

## Silent bugs

- [x] ~~**S1 — the FxmeTools pin predates the entire 2026-08 look-and-feel
      upgrade.**~~ **Done on 2026-08-18.** The submodule is now at `4b22e3c`,
      matching `origin/main`. `FxmeLookAndFeel.h` went from 360 lines to 867 and
      now carries `setAccentColour()` / `getAccentColour()`, `forState()` and
      `forHover()`, `drawLabel`, `drawTooltip`, `drawComboBox` with
      `getComboBoxFont` and `positionComboBoxText`, and
      `drawPopupMenuBackground` / `drawPopupMenuItem` / `getPopupMenuFont`.

      R1, R3 and R8 are unblocked by this. Note one thing the upgrade does *not*
      cover, which narrows H3: the new look-and-feel overrides neither
      `fillTextEditorBackground` / `drawTextEditorOutline` nor `getLabelFont`.

      Committed as parent `2ef7142`, which also carries S2. Not yet pushed.

- [x] **S2 — `fxme::Arpeggiator` allocates on the audio thread on every emitted
      event.** Fixed, verified, and committed as FxmeTools `a3de21f` ("No more
      heap allocation in audio thread"), which is pushed. The parent's pointer
      followed in `2ef7142`.

      What was wrong: `processBlock`, `getNext`, `reset` and `turnOff` each
      constructed a fresh `juce::MidiBuffer` local and returned it by value
      ([Arpeggiator.h:241](../Source/libs/FxmeTools/FxmeTools/midi/Arpeggiator.h#L241),
      [:274](../Source/libs/FxmeTools/FxmeTools/midi/Arpeggiator.h#L274),
      [:1087](../Source/libs/FxmeTools/FxmeTools/midi/Arpeggiator.h#L1087),
      [:1126](../Source/libs/FxmeTools/FxmeTools/midi/Arpeggiator.h#L1126)).
      `MidiBuffer` stores its events in a `juce::Array<uint8>`, which does not
      allocate while empty but does on the first `addEvent`. So a block in which
      any arpeggiator fires a note allocates and frees on the audio thread, once
      per step in `getNext` and again in `processBlock`, multiplied by the number
      of arpeggiators that are on.

      Call sites in this plugin:
      [PluginProcessor.cpp:307](../Source/PluginProcessor.cpp#L307),
      [:314](../Source/PluginProcessor.cpp#L314),
      [:323](../Source/PluginProcessor.cpp#L323).

      What was done: the engine now owns two persistent buffers, `outMidi`
      (shared by `processBlock`, `reset` and `turnOff`, which are never in
      flight at the same time) and `stepMidi` (`getNext` only, so `processBlock`
      merging
      a step into its own buffer is not a self-aliasing `addEvents`). Each
      method clears its buffer on entry and returns it by `const` reference,
      and `prepareToPlay` reserves 2 kB and 64 bytes respectively.

      The mechanism was verified rather than assumed: `MidiBuffer::clear()` is
      `data.clearQuick()` (`juce_MidiBuffer.cpp:122`), which reaches
      `ArrayBase::clear()` (`juce_ArrayBase.h:254`), and that sets `numUsed = 0`
      without touching `numAllocated` or `elements`. `ensureAllocatedSize` only
      ever grows (`juce_ArrayBase.h:240`). So after the reserve, the
      clear-and-refill cycle allocates nothing.

      Returning `const juce::MidiBuffer&` rather than adding out-parameter
      overloads was chosen so that no call site anywhere has to change:
      `midiMessages.addEvents (arp.processBlock (n, ch), 0, -1, 0)` binds to the
      reference, and `auto buf = arp.processBlock (n)` (which is what
      `tests/test_arpeggiator.cpp:54` does) still deduces `juce::MidiBuffer` and
      takes a copy, exactly as before. Both shapes were compiled to confirm it.
      The one new obligation on callers is documented in a block comment above
      `processBlock`: the reference is valid only until the next call to any of
      the three. TeAr consumes each one immediately
      ([PluginProcessor.cpp:307](../Source/PluginProcessor.cpp#L307),
      [:314](../Source/PluginProcessor.cpp#L314),
      [:323](../Source/PluginProcessor.cpp#L323)) and gives every arpeggiator
      its own engine, so it is unaffected.

      Verified on 2026-08-18, two ways.

      Behaviour: the engine's own suite was configured, built (Debug, `-j2`) and
      run. 60 test cases, 452 assertions, all passing, `ctest` green. The suite
      covers the arpeggiator and the modulation kernels.

      The allocation itself: a probe built against the current header and
      against the pre-change one, driving the engine with the test suite's own
      timing (1000 Hz, 150 BPM, 1/16, so a 100-sample block is one step) over
      5000 blocks plus 500 `turnOff` / `reset` rounds. Both produce an identical
      11001 MIDI events. The pre-change engine makes 20000 allocations across
      the 5000 blocks, exactly four per block, and 21501 across all three
      methods. The current one makes none.

      Getting that measurement right took two attempts, which is worth
      recording: an `operator new` counter reports zero for both versions,
      because `juce::Array` holds its elements in a `juce::HeapBlock` and that
      uses `std::malloc` directly (`juce_HeapBlock.h:126`). The numbers above
      come from wrapping `malloc` / `calloc` / `realloc` at link time instead.
      A probe with no before/after control would have "confirmed" the fix
      while measuring nothing at all.

- [x] **S3 — `areNotesHeld()` reads `heldNotes` with no lock.** Fixed and
      compiled in the working tree; the commit is still to come.
      [PluginProcessor.cpp:692](../Source/PluginProcessor.cpp#L692) reads
      `heldNotes.isEmpty()` from the editor's 60 Hz timer
      ([PluginEditor.cpp:335](../Source/PluginEditor.cpp#L335)) while
      `processBlock` mutates the same array under `arpsLock`
      ([PluginProcessor.cpp:211](../Source/PluginProcessor.cpp#L211),
      [:219](../Source/PluginProcessor.cpp#L219)). Every other accessor on this
      class takes the lock; this one is the exception, and it is not marked as a
      deliberate one the way `getArpeggiatorCurrentStep` and `getArpeggiator`
      are.

      Fixed on 2026-08-18 by adding `juce::ScopedLock lock (arpsLock);`, as in
      the neighbouring `getHeldNotes()`. `arpsLock` is declared `mutable`
      ([PluginProcessor.h:93](../Source/PluginProcessor.h#L93)), so it works in
      a `const` method. Blocking the message thread on it is safe, because
      `processBlock` takes the lock with `ScopedTryLock` and gives up rather
      than waiting. A lock rather than an atomic mirror of the flag, so that
      `heldNotes` keeps a single source of truth; the editor already takes this
      lock a few dozen times per frame through its other accessors, so one more
      very short acquisition is noise.

      Every access to `heldNotes` was then re-checked: the ones at
      [PluginProcessor.cpp:211-320](../Source/PluginProcessor.cpp#L211) are all
      inside `processBlock` under its try-lock, line 123 is in `prepareToPlay`
      under a lock, and `getHeldNotes()` locks. With this change the member is
      fully covered.

      One extra site was removed while verifying, which is beyond what S3 as
      written asked for and is called out here so it can be dropped if
      unwanted. `PluginEditor.cpp:365` bound
      `const auto& chord = audioProcessor.getArpeggiator (0).getChord();` and
      never used it: the compiler reports it as an unused variable, and it was
      also one of the two deliberately unlocked `getArpeggiator()` reads, so a
      dead binding was opening a race window against `loadArpsFromTree`'s
      `arps.clear()` for nothing. Deleting the line removes both.

      Verified by compiling `PluginProcessor.cpp` and `PluginEditor.cpp` with
      the plugin's own flags: no errors, and the unused-variable warning is
      gone.

      **safe to apply**

- [x] **S4 — `getArpeggiatorPattern()` returns a reference that outlives its
      lock.** Fixed and compiled in the working tree; the commit is still to
      come. [PluginProcessor.cpp:596](../Source/PluginProcessor.cpp#L596)
      returns `const juce::String&` into `arps[index].pattern`, and the
      `ScopedLock` is released as the function returns. The editor holds that
      reference across several statements at
      [PluginEditor.cpp:415](../Source/PluginEditor.cpp#L415). `arps` is cleared
      and rebuilt by `loadArpsFromTree`
      ([PluginProcessor.cpp:398](../Source/PluginProcessor.cpp#L398)), reached
      from `setStateInformation`, which a host may call on a thread of its
      choosing. The referent is then destroyed while the editor is reading it.

      Fixed on 2026-08-18 by returning `juce::String` by value, in both the
      declaration ([PluginProcessor.h:53](../Source/PluginProcessor.h#L53)) and
      the definition, with the `static const juce::String empty` fallback
      becoming a plain `return {}`.

      Two things were checked rather than assumed. The copy is made before the
      `ScopedLock` is released, because the return object is initialised while
      the function's locals are still alive. And it is cheap and safe to hand
      out: `juce::String` is copy-on-write over a `std::atomic<int> refCount`
      (`juce_String.cpp:65`), so the return costs an atomic increment rather
      than a character copy, and the returned string keeps the buffer alive on
      its own if another thread reassigns the original afterwards.

      The one caller that held the result across statements
      ([PluginEditor.cpp:414](../Source/PluginEditor.cpp#L414)) was changed from
      `const auto&` to `const auto`. Binding a `const&` to the temporary would
      still have been correct through lifetime extension, but it reads as the
      very hazard this finding is about. The other two callers pass the result
      straight into `setText`, where a temporary binds fine.

      Compiles clean, with no new warnings.

      **safe to apply**

- [x] **S5 — `TextEntryFocusFixer` suspends itself over the pattern generator
      popup, which is not a desktop window.** Fixed and verified in the working
      tree; the commit is still to come. The popup is launched with the
      editor as parent
      ([PluginEditor.cpp:94](../Source/PluginEditor.cpp#L94)), and
      `CallOutBox`'s constructor does `parent->addChildComponent (this)` in that
      case rather than `addToDesktop`
      (`juce_CallOutBox.cpp:42-47`), so it shares the editor's peer.
      `launchAsynchronously` still puts it in a modal state, and
      `modalWouldBlockComponent` (`juce_ComponentHelpers.h:249`) only exempts a
      modal that is a *parent* of the component being tested, so the editor
      counts as blocked. The fixer's `blockedByModal()` therefore returns true
      and both `grabPeerFocus()` and `timerCallback()` bail out for as long as
      the popup is open
      ([TextEntryFocusFixer.h:168](../Source/libs/FxmeTools/FxmeTools/components/TextEntryFocusFixer.h#L168),
      [:193](../Source/libs/FxmeTools/FxmeTools/components/TextEntryFocusFixer.h#L193)).

      The suspension exists for a good reason, spelled out in the class comment:
      a modal on its own *temporary desktop window* is dismissed by Windows the
      moment the window loses focus, so grabbing focus back would close it. That
      reasoning does not hold for a parented `CallOutBox`, which has no window of
      its own. The result is that the Hits / Steps / Rot fields
      ([popupWindow.cpp:34](../Source/popupWindow.cpp#L34),
      [:45](../Source/popupWindow.cpp#L45),
      [:56](../Source/popupWindow.cpp#L56)) get no focus enforcement on Linux,
      which is exactly the case the fixer was written for.

      Fixed on 2026-08-18 in FxmeTools. `blockedByModal()` now looks at which
      modal is actually in front and returns false when it is the root itself
      or a descendant of it, falling through to the old
      `isCurrentlyBlockedByAnotherModalComponent()` otherwise. The class comment
      was rewritten to say why the two cases differ, since the original comment
      correctly justified the suspension for desktop popups and that reasoning
      is worth keeping.

      Verified at runtime rather than by reading, with a probe that launches a
      real `juce::CallOutBox` against a real parent inside a
      `ScopedJuceInitialiser_GUI`, and evaluates both the old and the new
      predicate against it. Twelve checks, all passing:

      - a parented callout is a descendant of the root, and is not on the
        desktop (so it shares the root's peer, which is the whole premise)
      - the old predicate returned "blocked" for it, which is the bug
      - the new predicate returns "not blocked"
      - a modal outside the root still blocks under both predicates, so
        desktop popups keep the behaviour the original comment describes
      - with nothing modal, both predicates agree on "not blocked"

      Note what is *not* covered: the actual focus behaviour on Linux needs a
      hosted plugin window and a DAW, and `FxmeToolsTests` does not link
      `juce_gui_basics`, so the suite says nothing about this file. What the
      probe establishes is that the predicate now classifies the three cases
      correctly. `PluginEditor.cpp` recompiles clean against the change.

      **decision** (a behaviour change in a shared component; every plugin that
      owns a fixer and a parented callout is affected, and all of them get the
      enforcement switched back on inside such popups).

- [x] **S6 — CI never runs by itself.** Fixed in the working tree; the commit
      is still to come.

      `ci.yml` was `on: workflow_dispatch:` only, so no push and no pull request
      was ever built or verified. The macOS universal-binary check was present
      and could fail correctly, but only when someone remembered to press the
      button.

      The cost objection recorded in the first pass was wrong, and the check is
      worth writing down: `odoare/TeAr` is a **public** repository (confirmed
      with `gh repo view`), so GitHub-hosted Actions minutes are free, macOS
      runners included. There was no per-run cost to weigh against turning this
      on.

      Fixed on 2026-08-18 by adding `push` and `pull_request` on `main`, keeping
      `workflow_dispatch`, and adding a concurrency group so a superseded commit
      does not tie up a universal macOS build. Two details were checked rather
      than assumed:

      - `branches: [main]` does not match tag pushes, so this does not
        double-build a release; `release.yml` still owns tags.
      - `paths-ignore` (for `**.md`, `doc/**`, `LICENSE`, `.gitignore`) *skips*
        a workflow rather than passing it, which would leave a required status
        check pending forever. `main` has no branch protection (confirmed via
        the API), so that is safe here, and the file carries a comment saying to
        drop the filters if required checks are ever turned on.

      The concurrency group is deliberately the plain `ci-${{ github.ref }}`
      rather than an expression that exempts manual runs. The clever version
      would have been a single mistyped expression away from breaking the
      workflow at the exact moment it was first being relied on.

      Verified by parsing the file with PyYAML: the three triggers are present,
      the concurrency block is well formed, and all four jobs survive the edit
      (34 insertions, no deletions). Neither `actionlint` nor `yamllint` is
      installed here, so this is a YAML-level check, not a GitHub-schema one.

      **decision**

- [x] **S7 — the pattern generator captures an engine reference into lambdas
      that outlive the click.** Fixed and verified. Committed as FxmeTools
      `ce4b540` ("Pattern generators are now static") plus parent `e5d71de`. Found while sweeping for S4's defect class in
      the rest of the API. `getArpeggiator (int)`
      ([PluginProcessor.h:69](../Source/PluginProcessor.h#L69)) is the one
      accessor still returning a reference into `arps`, and it takes no lock at
      all (it is commented as a deliberate race for visual feedback, which holds
      for the two call sites that read a single int and return).

      The third call site is different.
      [PluginEditor.cpp:80](../Source/PluginEditor.cpp#L80) binds
      `const auto& arp` and then captures it by reference into two lambdas
      ([:83](../Source/PluginEditor.cpp#L83),
      [:86](../Source/PluginEditor.cpp#L86)) which are copied into
      `ArpPatternPopup` and invoked whenever the user later presses Randomize or
      Make Euclidean. In between, anything that reallocates or clears `arps`
      (`addArpeggiator`'s `push_back`, `removeArpeggiator`'s `erase`, a preset
      or session load's `clear`) leaves the capture dangling or pointing at a
      different arpeggiator. The `CallOutBox` is modal, so the "+" and "-"
      buttons are out of reach while it is open, but a host-driven
      `setStateInformation` is not blocked by JUCE modality.

      It also reaches the engine through
      `const_cast<fxme::Arpeggiator&> (arp)`, which would be an unlocked
      mutation of an object the audio thread is using.

      Both concerns turn out to be latent rather than live, and the report says
      so because it changes the priority: `makeEuclidianPattern` and
      `makeRandomPattern` read and write **no member state at all**. They are
      pure functions of their arguments (plus the global RNG), so the dangling
      `this` is never dereferenced and the `const_cast` mutates nothing. It is
      formally undefined behaviour that currently cannot misbehave, and it
      becomes a real bug the first time either method touches a member.

      Fixed on 2026-08-18, and it was as small as expected. `makeEuclidianPattern`
      and `makeRandomPattern` are now `static` in FxmeTools, so the editor's two
      lambdas capture nothing at all and call
      `fxme::Arpeggiator::makeEuclidianPattern (hits, steps, rotation)` on the
      type. The `const_cast`, the reference capture and the `getArpeggiator`
      call all leave this path together.

      Making them static is itself the proof of the claim that they touch no
      member state: had either done so, it would not compile. Both do.

      Verified three ways beyond that:

      - the FxmeTools suite still passes unchanged, 60 cases and 452
        assertions. It is worth being honest that this proves little here,
        because the suite never calls either generator;
      - so a separate probe exercises all the call shapes an existing plugin
        might use: through a non-const instance (`arp.makeRandomPattern()`),
        through a const reference, on the type, and through the old
        `const_cast<fxme::Arpeggiator&> (...)` wrapper. All compile, and
        `makeEuclidianPattern (3, 8, 0)` returns `1 . . 1 . . 1 .` identically
        through every one of them, so no other plugin in the family breaks and
        the behaviour is unchanged;
      - `PluginEditor.cpp` and `PluginProcessor.cpp` recompile with no errors.

      What this does *not* do is retire `getArpeggiator (int)`. Its other two
      call sites ([PluginEditor.cpp:369](../Source/PluginEditor.cpp#L369),
      [:406](../Source/PluginEditor.cpp#L406)) still read an int and return
      immediately, which is the deliberate unlocked race the accessor was
      documented for. Only the one site that stored the reference is gone.

      **decision** (the change is in the shared library, though making a member
      function `static` keeps existing `arp.makeRandomPattern()` calls
      compiling, which was confirmed rather than assumed)

---

## Retrofit

- [x] **R1 — five combo boxes are drawn by the local `ArpLookAndFeel`, and
      their drop-down menus by `LookAndFeel_V4`.** Fixed and verified in the
      working tree; the commit is still to come. Each box does get a
      `setLookAndFeel()` call, so the usual "combo silently on V4" finding does
      not apply, but `ArpLookAndFeel`
      ([ArpLookAndFeel.h](../Source/ArpLookAndFeel.h)) overrides `drawComboBox`
      and not `drawPopupMenuBackground` / `drawPopupMenuItem`, so the menu that
      opens falls back to the stock V4 panel, which does not match the dark TeAr
      chrome.

      Unblocked by S1: `FxmeLookAndFeel` now draws combos (`drawComboBox`, with
      `getComboBoxFont` and `positionComboBoxText` so the text respects the new
      arrow zone) and their menus (`drawPopupMenuBackground`,
      `drawPopupMenuItem`, `getPopupMenuFont`, with the tick in the accent). The
      change is `setLookAndFeel (&arpLAF)` becomes `setLookAndFeel (&fxmeLAF)`
      at each site, which also resolves R2. Sites:
      - [x] [PluginEditor.cpp:117](../Source/PluginEditor.cpp#L117) `chordMethodBox`
      - [x] [PluginEditor.cpp:134](../Source/PluginEditor.cpp#L134) `scaleRootBox`
      - [x] [PluginEditor.cpp:151](../Source/PluginEditor.cpp#L151) `scaleTypeBox`
      - [x] [ArpeggiatorComponent.cpp:34](../Source/ArpeggiatorComponent.cpp#L34) `subdivisionBox`
      - [x] [ArpeggiatorComponent.cpp:47](../Source/ArpeggiatorComponent.cpp#L47) `midiChannelBox`

      Done on 2026-08-18. Each of the five now takes `&fxmeLAF`. The labels and
      the pattern text editor deliberately stay on `ArpLookAndFeel`, for the
      reason recorded under H3: the new look-and-feel covers neither the
      text-editor background nor `getLabelFont`, so moving those would be a
      separate change with a font regression attached.

      `ArpeggiatorComponent` gained its own `fxme::FxmeLookAndFeel` member
      rather than sharing the editor's. That is what lets each arpeggiator's
      drop-down highlight in its own colour, since a menu is its own window and
      can only read the accent from the look-and-feel that opened it. Both
      classes declare their look-and-feels before the widgets pointing at them,
      so reverse-declaration-order destruction still leaves no dangling
      pointer; both destructors also null them explicitly, as before.

      One colour change came with it: the `backgroundColourId =
      transparentBlack` override was dropped from all five, so the body is
      filled as a rounded pill in `FxmeLookAndFeel`'s dark default instead of
      being invisible. `arrowColourId` is now set to each control's own colour,
      so the chevron matches rather than falling back to a generic white.

      **decision** (this replaces a working custom look with a different one, so
      it is a visual change rather than a repair).

- [x] **R2 — `fxme::FxmeLookAndFeel fxmeLAF;` is declared and never used.**
      Resolved by R1, which is what now uses it.
      [PluginEditor.h:75](../Source/PluginEditor.h#L75) is the only mention of
      it in the whole project; nothing calls `setLookAndFeel (&fxmeLAF)`. Either
      it becomes the look-and-feel for R1 and R3, or it should go.

      **safe to apply** (was: remove it, if R1 were declined; R1 was taken
      instead)

- [x] **R3 — no `setAccentColour()` anywhere on a `FxmeLookAndFeel`.** Fixed
      and verified in the working tree; the commit is still to come. The three
      `setAccentColour` calls in the project are on `fxme::TopBar`,
      `fxme::PresetBarComponent` and `fxme::PresetComponent`
      ([PluginEditor.cpp:15](../Source/PluginEditor.cpp#L15),
      [:33](../Source/PluginEditor.cpp#L33),
      [:49](../Source/PluginEditor.cpp#L49)), which are those components' own
      methods, not the look-and-feel's. `Theme::accent` is already the right
      value to pass.

      Done on 2026-08-18, in two places rather than one. The editor calls
      `fxmeLAF.setAccentColour (Theme::accent)` in its constructor, and each
      `ArpeggiatorComponent` calls it from `setArpColour` with that
      arpeggiator's own colour, so a drop-down highlights in the colour of the
      arpeggiator it belongs to.

      R1 and R3 were verified together with a rendering probe, because both are
      claims about drawing that a compile cannot check. It renders the same
      `juce::ComboBox` twice, once configured the way the code was before
      (stock `LookAndFeel_V4` plus the transparent-body override) and once the
      way it is now, and compares the pixels. Six checks, all passing: the two
      renderings differ at all, so the look-and-feel genuinely changed hands;
      the old centre pixel is `00000000`, a transparent body; the new one is
      `FF1E1E1E`, opaque, which is `FxmeLookAndFeel`'s pill; and
      `setAccentColour` does land on `PopupMenu::highlightedBackgroundColourId`,
      which is the part of R3 that would otherwise fail silently by leaving
      menus grey.

      That probe segfaulted on its first run, and the cause is worth recording
      because it invalidates a shortcut used earlier in this audit: it was
      linking against object files from the plugin's own build, which were
      compiled with a different set of JUCE config defines than the probe
      itself. The struct layouts disagreed and `ComboBox::setSelectedId`
      corrupted memory. Rebuilding the JUCE modules with the probe's own flags
      fixed it and the crash disappeared. Any runtime probe of this kind has to
      compile its dependencies consistently, or its results mean nothing.

      **safe to apply**

- [x] **R4 — six `setTooltip()` calls and no `juce::TooltipWindow`, so no
      tooltip is ever visible.** Fixed in the working tree; the commit is still
      to come.

      Done on 2026-08-19: the editor now owns
      `juce::TooltipWindow tooltipWindow { nullptr, 700 }`, pointed at `fxmeLAF`
      in the constructor and released in the destructor. Parented to `nullptr`
      it lives on the desktop, so without that call it would be drawn by the
      default look-and-feel rather than the panel's, which is the whole reason
      `FxmeLookAndFeel::drawTooltip` exists.

      Where it sits among the members does not matter: `TooltipWindow`'s
      constructor calls `setAlwaysOnTop (true)` (`juce_TooltipWindow.cpp:42`),
      so this is not the z-order trap it looks like.

      Eight tooltips are now reachable rather than six: the original six, plus
      the new ones on the info button and on Follow MIDI In.

      **decision** — taken: tooltips now appear on eight controls that never
      showed one.

- [x] **R5 — three deprecated `juce::Font` constructor calls.** Fixed and
      verified in the working tree; the commit is still to come. All three go
      through `Font (float, int)` or `Font (const String&, float, int)`, both
      marked `[[deprecated]]` in JUCE 8.0.12
      (`juce_Font.h:76`, `:88`).
      - [x] [ArpeggiatorComponent.cpp:13](../Source/ArpeggiatorComponent.cpp#L13)
            `juce::Font (getDefaultMonospacedFontName(), 24.0f, juce::Font::plain)`
            becomes
            `juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 24.0f, juce::Font::plain))`.
      - [x] [KeyboardComponent.cpp:108](../Source/KeyboardComponent.cpp#L108)
            `g.setFont (juce::Font (9.f))` becomes `g.setFont (9.0f)`, which is
            a genuine non-deprecated `Graphics` overload.
      - [x] [ArpLookAndFeel.h:54](../Source/ArpLookAndFeel.h#L54)
            `return {15.0f};` is the implicit conversion the `juce::Font (` grep
            misses: it constructs the deprecated `Font (float)`. It becomes
            `return juce::Font (juce::FontOptions (15.0f));`. Moot if H3 removes
            the class.

      Done on 2026-08-18, all three, and the compiler confirms it: the three
      `-Wdeprecated-declarations` warnings are gone from a full recompile (the
      `ArpLookAndFeel.h` one had counted three times, once per translation unit
      that includes it).

      **One caveat that stops this being purely mechanical, contrary to how it
      was first classified.** The deprecated constructors do not simply forward
      to `FontOptions`: they route through `legacyArgs()` (`juce_Font.cpp:400`),
      which applies `.withMetricsKind (TypefaceMetricsKind::legacy)`, whereas
      `FontOptions` defaults to `portable` (`juce_FontOptions.h:247`). JUCE
      describes legacy metrics as ones that "may differ for the same font file
      when running on different platforms", and portable as the consistent
      choice it recommends for new work.

      Measured rather than assumed, for both fonts, comparing the deprecated
      constructor against portable and against an explicitly-legacy
      `FontOptions`: height, ascent, descent and the rendered width of a sample
      pattern string come out identical to three decimal places across all
      three, at 24 pt monospaced and at 15 pt. On this platform the change is
      exactly neutral.

      What that cannot cover is macOS and Windows, which are precisely where
      JUCE warns the two metric kinds may diverge. Portable was kept rather than
      pinning legacy, because it is the direction JUCE recommends and it makes
      the three platforms agree with each other; pinning
      `.withMetricsKind (juce::TypefaceMetricsKind::legacy)` on those two call
      sites is a one-line change if the old rendering matters more.

      **safe to apply on Linux, measured; worth a glance on macOS or Windows**

- [x] **R6 — `followMidiInButton` is a bare `juce::ToggleButton` plus a
      hand-held `ButtonAttachment`** where the house control is
      `fxme::FxmeButton`. Fixed in the working tree; the commit is still to
      come.

      Done on 2026-08-19. The member is now
      `fxme::FxmeButton followMidiInButton { audioProcessor.getAPVTS(),
      "followMidiIn", "Follow MIDI In", juce::Colours::white }`, which builds
      its own attachment, so the separate `followMidiInAttachment` member is
      gone and there is no longer a second thing to keep in step. It is given
      `fxmeLAF`, which is what this buys in practice: the house tick, hover and
      disabled drawing, the last of which R8 depends on.

      Two details the compiler and the code turned up:

      - `fxme::FxmeButton` is a plain `juce::Component`, not a
        `SettableTooltipClient`, so `setTooltip` goes to the inner
        `juce::ToggleButton` it exposes. That button fills the whole bounds, so
        the tooltip is still found under the mouse.
      - the parameter id it binds to was checked against `createParameters`
        rather than assumed: `"followMidiIn"` is declared at
        [PluginProcessor.cpp:786](../Source/PluginProcessor.cpp#L786). A
        mismatch would have asserted at construction rather than failed
        quietly, but it is one grep.

      R8 still works through it: `setEnabled` on the wrapper makes the inner
      button's `isEnabled()` false, since that is false whenever any ancestor is
      disabled.

      **safe to apply** — taken.

- [x] **R7 — no `fxme::InfoButton`.** Fixed in the working tree; the commit is
      still to come.

      Done on 2026-08-19. The editor now carries one, at the right-hand end of
      the tab row so that adding arpeggiators eats into the middle rather than
      pushing it off the edge. It is themed from `Theme` (accent for the disc,
      the house text colour, the panel background, an accent hairline) and
      carries a written-out reference to the pattern language.

      The help text was taken from the readme's syntax tables rather than
      composed from memory, and covers notes, pitch, velocity, octave,
      probability in both its single-step and group forms, the two block
      commands, and the editor's own shortcuts (Return commits, Shift+Return
      inserts a newline, clicking the selected tab toggles that arpeggiator). It
      ends by pointing at the readme and the typeset PDF, which stay the full
      reference.

      Worth noting how this composes with S5: `InfoButton::clicked()` launches
      its callout with a `nullptr` parent, so it is a temporary *desktop*
      window, and `blockedByModal()` therefore still suspends the focus fixer
      while it is open. That is the case the original suspension was written
      for, and S5 deliberately exempted only modals that are descendants of the
      editor, so the two changes agree rather than fight.

      Verified by rendering, since `setColours` failing to reach the paint path
      would compile cleanly and simply look wrong: the button draws, the themed
      image differs from the default-palette one, the disc carries
      `Theme::accent` darkened the way `paintButton` darkens it, and the default
      palette does not contain that colour. One check in that probe failed at
      first and was wrong rather than the code: it sampled the centre pixel,
      which is the "i" glyph, and `Theme::text` happens to be exactly
      `InfoButton`'s default text colour, so the two palettes genuinely agree
      there. Comparing the whole image instead is the right test.

      **decision** — taken.

- [x] **R8 — three controls are superseded by another control and never grey
      out.** Fixed and verified in the working tree; the commit is still to come. `scaleRootBox`, `scaleTypeBox` and `followMidiInButton` only do
      anything when `chordMethod` is 2 ("Single note"): that is the only branch
      of `processBlock` that reads them
      ([PluginProcessor.cpp:237-293](../Source/PluginProcessor.cpp#L237)), and
      the only branch that draws a scale
      ([PluginEditor.cpp:435](../Source/PluginEditor.cpp#L435)). On top of that,
      `scaleRootBox` is superseded a second time when `followMidiIn` is on,
      because the played note then overwrites the parameter
      ([PluginProcessor.cpp:250](../Source/PluginProcessor.cpp#L250),
      [:509](../Source/PluginProcessor.cpp#L509)). None of them ever calls
      `setEnabled()`.

      The editor already runs a 60 Hz timer
      ([PluginEditor.cpp:165](../Source/PluginEditor.cpp#L165)), so the poll the
      checklist asks for costs nothing extra and host automation is picked up
      for free.

      Done on 2026-08-19. A new `updateControlEnablement()` is called from the
      existing 60 Hz timer, and once from the constructor so the first paint is
      already correct. It is a poll rather than an `onChange` hook, so a
      parameter moved by host automation greys the controls too, which is the
      whole reason the checklist asks for a timer here.
      `Component::setEnabled` early-returns when nothing changed
      (`juce_Component.cpp:2860`), so polling costs nothing.

      The rule was re-derived from the processor rather than taken from this
      report's first pass, and it held: `scaleType`, `scaleRoot` and
      `followMidiIn` are read only inside `case 2` of `processBlock`'s switch
      ("Single note"), and within that branch `scaleRoot` is read only when
      `followMidiIn` is off, because when it is on the processor *writes* the
      parameter from the played note instead. So `scaleType` and
      `followMidiIn` follow the mode, and `scaleRoot` needs both conditions.
      The two attached labels are set alongside their boxes:
      `attachToComponent` makes them siblings rather than children, so
      disabling a box does not reach its label.

      **A correction to what the first pass recorded here.** It said
      `LookAndFeel_V4::drawComboBox` "ignores `isEnabled()` entirely". That is
      wrong: it fades the arrow, and only the arrow, on the last line of the
      method (`juce_LookAndFeel_V4.cpp:947`). The earlier reading looked at the
      top of the function and stopped too soon. The claim that actually
      mattered is unaffected, and is now measured rather than read.

      Verified by rendering the same combo box enabled and disabled under each
      look-and-feel and comparing every pixel:

      - `FxmeLookAndFeel` (now): differs, so R8 has something to show
      - `ArpLookAndFeel` (what TeAr's combos used before R1): **identical**, so
        the finding was real and the dependency on R1 was real
      - stock `LookAndFeel_V4`: differs, by that faded arrow alone
      - a `juce::Label` on `ArpLookAndFeel`: differs, so the attached labels do
        grey with their boxes

      `followMidiInButton` is a plain `juce::ToggleButton` on the default
      look-and-feel, whose `drawToggleButton` passes `isEnabled()` into
      `drawTickBox`, so it greyed either way.

      **safe to apply** (needed R1 first, which has landed)

- [x] **R9 — three `-Wshadow` warnings where a local shadows the constructor's
      parameter.** Fixed in the working tree; the commit is still to come. The editor's constructor takes `TeArAudioProcessor& p`, and
      three `if (auto* p = dynamic_cast<...>)` lines inside it reuse the name:
      [PluginEditor.cpp:111](../Source/PluginEditor.cpp#L111),
      [:128](../Source/PluginEditor.cpp#L128),
      [:145](../Source/PluginEditor.cpp#L145). Harmless as written, since the
      parameter is not wanted there, but it is three warnings on every build.

      Done on 2026-08-18: the three locals are now `param`, and the compiler
      confirms all three warnings are gone.

      **safe to apply**

- [x] **R10 — about forty `-Wsign-conversion` warnings from indexing a
      `std::vector` with an `int`.** Fixed and verified in the working tree; the
      commit is still to come.

      Every `arps[index]`, `arpComponents[i]` and `tabButtons[i]` where the
      index was an `int` produced one, across
      [PluginProcessor.cpp](../Source/PluginProcessor.cpp) and
      [PluginEditor.cpp](../Source/PluginEditor.cpp): 36 occurrences over 33
      lines, which was the entire remaining warning output of the project.

      Individually harmless, since each index is bounds-checked with
      `juce::isPositiveAndBelow` first, so none can actually be negative. The
      cost was signal-to-noise: this one class was effectively all the build
      output, and it is what hid the dead `chord` binding that S3 removed.

      Done on 2026-08-19 with a file-local helper in each of the two
      translation units rather than a cast at every site:

          inline std::size_t asIndex (int i) noexcept
          {
              jassert (i >= 0);
              return (std::size_t) i;
          }

      Preferred over a bare `(size_t)` at 36 sites because it names the intent
      once, and because the assertion is free in a release build while catching
      a negative index in a debug one, which a plain cast never would.
      Duplicated in the two files rather than given a shared header, since a
      three-line conversion does not justify new coupling between them.

      The rewrite was applied by pattern rather than by hand, then checked:
      every index in both files is a simple identifier (`i`, `j`, `index`,
      `selectedArpIndex`), the two sites that already carried an explicit
      `(size_t)` cast were left alone, nothing was double-wrapped, and the one
      textual match inside the S7 explanatory comment in `PluginEditor.cpp` was
      correctly not touched.

      Verified with a full recompile of all five translation units: 5 of 5
      built, 0 errors, and **0 warnings in TeAr's own sources**, down from 45 at
      the start of this audit and 36 after R5 and R9. Everything the build still
      reports now comes from FxmeTools headers, which is H9.

      One process note, since it nearly produced a false result: the first
      attempt at that sweep wrote an empty log, because a `cd` failed and
      short-circuited the `rm` that was meant to force the rebuild, so
      "0 warnings" was being counted from no output at all. The numbers above
      come from a log confirmed to hold 1123 lines and five compiler
      invocations.

      **safe to apply**

---

## House style

- [x] **H1 — `Source/libs/cppMusicTools` is a dead submodule.** Removed in the
      working tree; the commit is still to come.

      It held an older vendored copy of the same arpeggiator engine
      (`Arpeggiator.h`, `MidiTools.h`) that now lives in FxmeTools, was
      referenced by nothing in `Source/`, `CMakeLists.txt` or the workflows, and
      was already documented as unused. It still cost every clone a
      `--recurse-submodules` fetch and left two copies of the engine in the tree
      for a future session to confuse.

      Re-verified before removing rather than trusting the first pass: the only
      build-file mentions of it were inside the submodule itself (its own
      `tests/CMakeLists.txt`, which nothing adds), and TeAr's root
      `CMakeLists.txt` only ever calls `add_subdirectory` on `../JUCE` and on
      `Source/libs/FxmeTools/tests`.

      Nothing is lost. `odoare/cppMusicTools` is a public repository in its own
      right, last pushed 2026-05-03, so the history stays reachable; this only
      stops TeAr carrying a second copy.

      Removed with the full three-step sequence, since `git rm` alone leaves the
      internal clone behind: `git submodule deinit -f`, then `git rm -f`, then
      deleting `.git/modules/Source/libs/cppMusicTools`. Afterwards
      `.gitmodules` lists only FxmeTools, `git config` has no stale
      `submodule.*` entry, and the FxmeTools submodule is untouched at
      `ce4b540` with its working tree present. The changes are staged but not
      committed.

      Verified by configuring from scratch in a throwaway directory: the
      Release configure succeeds with **zero** files in the generated build
      system mentioning `cppMusicTools` against ten mentioning
      `Source/libs/FxmeTools`, the `-DTEAR_BUILD_TESTS=ON` configure succeeds
      too, and rebuilding and running the suite on the existing tree still
      passes.

      `doc/development.md` was updated alongside: the layout table is down to
      one submodule, with a note saying where the old one went and how to tidy
      a clone made before today, which will otherwise keep an empty directory
      and a stale `.git/config` entry.

      **decision** — taken. Anyone with an existing clone needs
      `git submodule sync` and a `git submodule update --init`, or simply to
      delete the leftover directory.

- [x] **H2 — `KeyboardComponent` is generic and belongs in FxmeTools.** Moved
      in both working trees; the commits are still to come.

      Done on 2026-08-19 as
      `FxmeTools/components/ScaleKeyboardComponent.h`, header-only, added to the
      `FxmeTools.h` umbrella. `TeAr/Source/KeyboardComponent.{h,cpp}` are
      deleted and dropped from the `CMakeLists.txt` source list.

      Two things were redesigned on the way, and they are worth calling out
      because they go beyond "move the file":

      - the `getArpColour` dependency became a
        `std::function<juce::Colour (int voice)>` the host sets, which is what
        the finding asked for. The editor now assigns it once in its
        constructor. Nothing in the component knows what a voice is.
      - the playing-note list was `juce::Array<juce::var>` holding
        `DynamicObject`s with "note" and "arpIndex" properties. That is a poor
        thing to freeze into a shared library's public API, and it allocated a
        `DynamicObject` per sounding note per frame at 60 Hz. It is now a plain
        `struct PlayingNote { int midiNote; int voice; }`. This was not in the
        finding as written; it is here because publishing the `var` shape would
        have standardised it for every other plugin that picks the component up.

      A `Colours` struct was also added, since a keyboard with red and white
      compiled into it is only half reusable. Every default reproduces exactly
      what TeAr drew before, so it changes nothing here.

      Verified by rendering rather than by reading, which is the real risk when
      a `paint()` is rewritten during a move: the old component was recovered
      from `HEAD` and both were drawn over the same seven states at 780 by 60
      (empty, scale and root only, one voice, four voices across white and
      black keys, held input notes, everything at once, and notes playing with
      no scale). **Every pixel matches in all seven.** The palette used for the
      old one was extracted verbatim from `ArpInstance.h` so the comparison is
      not colour-shifted.

      Also: 4 of 4 translation units rebuild after the reconfigure, no errors,
      no warnings from TeAr's sources and none from the new header.
      `doc/architecture.md` had the file in its layout block and now records
      where it went.

      **decision** — taken. It adds public API to a shared library, so the
      FxmeTools commit should go first and be pushed before the parent pointer
      moves.

- [x] **H3 — `ArpLookAndFeel` shrinks to the text-editor overrides once R1
      lands, but does not disappear.** Done in the working tree; the commit is
      still to come.

      It had five overrides. `drawComboBox` was already dead once R1 moved the
      five combo boxes to `FxmeLookAndFeel`. `drawLabel` and `getLabelFont` died
      on 2026-08-19 when the four labels followed (the three in the editor and
      the `Ch` label in each arpeggiator panel). What is left is
      `fillTextEditorBackground` and `drawTextEditorOutline`, which
      `FxmeLookAndFeel` does not provide and which are what give the pattern
      field its rounded dark box.

      The class is now applied to exactly one widget, so the editor's own
      `arpLAF` member and its include went with it; only
      `ArpeggiatorComponent` still holds one. The file carries a header comment
      saying what it is down to and that it can go entirely if those two
      overrides ever move upstream.

      Moving the labels needed the hardcoded 15 pt to be set on each label,
      because `FxmeLookAndFeel` does not override `getLabelFont` and so
      inherits V4's, which just returns `label.getFont()`. That matters twice
      over: `Label::componentMovedOrResized` also *measures* with
      `getLabelFont`, so an attached label's width comes from it too.

      Whether that renders the same was measured rather than assumed, by
      drawing each label under both look-and-feels and comparing pixels. Four
      of six cases are identical. The two that are not are worth recording:

      - **disabled**: 360 pixels differ. `ArpLookAndFeel::drawLabel` fades a
        disabled label to alpha 0.5; `FxmeLookAndFeel` fades to 0.4
        (`textAlpha`). This one is live rather than hypothetical, because R8
        disables `scaleRootLabel` and `scaleTypeLabel`. It was taken anyway, and
        is the point of the change: the label now greys by the same rule as the
        combo box beside it instead of by its own.
      - **cramped**: 439 pixels differ at a deliberately too-narrow 45 px,
        because `drawFittedText` shrinks to fit where `drawText` clips. This
        cannot occur here: `Label::componentMovedOrResized` sizes an attached
        label to its own text width (`juce_Label.cpp:177`), capped only by the
        space to the left of its owner, which is ample for all four.

      4 of 4 translation units rebuild, no errors, no warnings.

      **decision** — taken, with the disabled-label fade changing from 0.5 to
      0.4 as described above.

- [x] **H4 — the pattern generator popup is the one part of the GUI outside the
      house style.** Fixed in the working tree; the commit is still to come.

      Done on 2026-08-19. Its four `juce::TextButton`s are now
      `fxme::AccentToggle` with `setClickingTogglesState (false)`, matching the
      toolbar buttons they sit under. The popup owns the same pair of
      look-and-feels the arpeggiator panel does: `fxme::FxmeLookAndFeel` for the
      labels, with `setAccentColour` set to the arpeggiator's own colour, and
      `ArpLookAndFeel` for the three number fields, so they get the same rounded
      dark box as the pattern editor. The backdrop is
      `fxme::paintComponentBackground`, as the editor's now is under H8.

      Every hardcoded `juce::Colours::darkblue.darker(2.f)` is gone; the colour
      the popup is handed drives all of it.

      **decision** — taken: this changes a dialog users already know.

- [x] **H5 — `doc/architecture.md` is two patch versions stale.** Fixed in the
      working tree; the commit is still to come.

      Two lines said 0.4.0 where they meant the current version: the opening
      "State of the plugin as of" and the `project(TeAr VERSION ...)` line in
      the layout block. Both now say 0.4.2.

      The file's other nine mentions of 0.4.0 were deliberately left alone: they
      are historical statements ("new in 0.4.0", "fixed in 0.4.0", "deferred
      past 0.4.0") that are still true. A blanket replace would have corrupted
      them, which is the only reason this needed care at all.

      **safe to apply** — taken.

- [x] **H6 — the release workflow's manual-run version fallback is hardcoded to
      0.4.0.** Fixed in the working tree; the commit is still to come.

      Now 0.4.2, with the comment extended to say what it actually affects (the
      `pkgbuild` identifier on a `workflow_dispatch` run, never a tagged
      release) so the next person can judge whether the drift matters.

      **safe to apply** — taken.

- [x] **H7 — both workflows still fetch and install `odoare/FxmeJuceTools`,
      which this project no longer uses.** Fixed, verified and committed. Three steps in each of the eight jobs
      check the repo out into `_fxme_tmp` and move `module/FxmeJuceTools` into
      `JUCE/usermodules/`
      ([ci.yml:29-38](../.github/workflows/ci.yml#L29),
      [release.yml:43-53](../.github/workflows/release.yml#L43), and the same
      block in each other job). The build gets FxmeTools from the
      `Source/libs/FxmeTools` submodule via
      [CMakeLists.txt:27](../CMakeLists.txt#L27) instead, and nothing references
      the usermodule.

      Verified rather than assumed: `github.com/odoare/FxmeJuceTools` and its
      `module/FxmeJuceTools` directory both still exist, so the `mv` succeeds and
      CI is not red. This is dead weight (an extra clone per job), not a
      breakage.

      S6 raised the stakes, which is why this was taken next rather than left as
      a tidy-up. While CI only ran on demand, an unnecessary clone of an
      unrelated repository cost nothing anyone noticed. Once `ci.yml` ran on
      every push and pull request, those steps executed four times per push, and
      the `mv` was an unguarded dependency on a repository this project no
      longer builds against: if `FxmeJuceTools` were ever renamed, deleted or
      made private, every CI run would go red for a reason unrelated to the
      code.

      Fixed on 2026-08-18 by deleting all sixteen steps (a Checkout and an
      Install in each of four jobs, in each of two workflows): 93 deletions, no
      insertions.

      The dead-ness was established rather than assumed, and one piece of
      evidence initially pointed the other way, which is worth recording. The
      local `build/` tree does contain a
      `usermodules/FxmeJuceTools/FxmeJuceTools.cpp.o`, and `../JUCE/usermodules`
      does carry a `FxmeJuceTools` symlink, so at a glance the module looked
      live. It is not: that object is dated 2026-05-25, months before the move
      to the submodule, and the current `build.make`, `link.txt` and
      `CMakeCache.txt` contain no reference to it at all.

      The conclusive check was a fresh out-of-tree `cmake` configure (a
      configure, not a build). It succeeds, and the generated build system it
      produces contains **zero** files mentioning `FxmeJuceTools` against **ten**
      mentioning `Source/libs/FxmeTools`. Since a CI runner starts from a clean
      JUCE checkout with no `usermodules` directory at all, nothing was relying
      on those steps.

      Both files were then re-parsed: every job and trigger survives, the
      "Verify universal binaries" step is untouched in both, and no `_fxme_tmp`
      or `usermodules` residue remains.

      **safe to apply**

- [x] **H8 — the editor hand-rolls a backdrop that FxmeTools now provides.**
      Fixed in the working tree; the commit is still to come.

      `TeArAudioProcessorEditor::paint` is now a single call to
      `fxme::paintComponentBackground (g, getLocalBounds().toFloat(),
      Theme::accent)`, replacing the ten lines that built the same
      corner-to-corner diagonal gradient by hand. `Theme::panelTint`, which was
      the literal that construction duplicated, had no remaining reader and was
      removed with it.

      As recorded when this was raised: it is not a pure substitution. The old
      code used a fixed blue-grey; the house version interpolates the accent 5
      to 14 percent into near-black, so the backdrop becomes near-black with a
      hint of cyan rather than blue-grey. That is the point of the shared
      version (a family of differently tinted plugins reading as one product),
      but it is the most visible single change in this pass.

      **decision** — taken: the plugin's backdrop colour changes.

- [x] **H9 — FxmeTools headers TeAr does not use are warning on every build.**
      Partly fixed in the working tree; the rest is itemised below rather than
      swept. The commit is still to come.

      Fixed, because each is a real defect rather than noise:

      - `components/FxmeMeters.h` — the `-Wreorder` pair on
        `FxmeHorizontalMeter` and `FxmeVerticalMeter`. Both constructors listed
        `valueColour` before `backgroundColour` while the members are declared
        the other way round, so the written order was not the order they are
        actually initialised in. Benign today (neither depends on the other) and
        exactly the footgun the warning exists for. The initialiser lists now
        match the declarations, with a comment saying why the order matters.
      - `midi/Arpeggiator.h` — the constructor parameter `baseOctave` shadowed
        the member of the same name that it does not set. Renamed
        `initialOctave`.
      - `midi/Arpeggiator.h` — `reset()` and `turnOff()` take a `midiChannel`
        they never read, because the note-off goes out on
        `lastPlayedMidiChannel` instead. Kept in the signature for source
        compatibility and marked with `juce::ignoreUnused`, with a comment
        saying which channel is really used.
      - `components/SequencerRubber.h` — `const double pps = pixelsPerStep();`
        in `paint()` was dead; the loop under it uses `stepToX (s)`. Removed.
        The same pattern as the dead binding S3 removed, and worth checking
        rather than silencing: a computed value nobody reads is sometimes a
        half-finished edit.

      That takes the FxmeTools warning count from 204 lines to 156. The engine
      test suite still passes unchanged, 60 cases and 452 assertions.

      **Deliberately not fixed**, because each needs a judgement about intent in
      shared code this audit has not read, and a blanket sweep of a library four
      other plugins link against is how regressions get introduced:

      - `midi/NeoRiemannGrid.h` (60), `dsp/VuMeter.h` (12), `midi/Scale.h` (4),
        `midi/GridTransform.h` (4) — `-Wsign-conversion`, the same class R10
        fixed in TeAr, but across code whose bounds guarantees are unaudited
      - `image/ImageAdjustments.h` (36), `dsp/Ambisonics.h` (12),
        `components/WaveformDisplay.h` (4), `components/SequencerRubber.h` (4)
        — `-Wfloat-equal`. At least the `ImageAdjustments::operator==` case
        looks deliberate: exact equality is what a settings comparison wants,
        and an epsilon there would be the bug. These want a targeted pragma
        rather than a code change.
      - `midi/Scale.h` (8), `midi/GridTransform.h` (8) — `-Wswitch-enum` for a
        `Count` sentinel not listed in a switch. Mechanical, but it needs the
        enums read first.

      **decision** — the fixed part is taken; the remainder stays open.

- [ ] **H10 — `fxme::FxmeSlider`'s four-argument constructor accepts a colour
      and never applies it.** New, found while clearing H9's warnings.

      [FxmeSlider.h:25-33](../Source/libs/FxmeTools/FxmeTools/components/FxmeSlider.h#L25)
      takes `(apvts, paramID, labelText, const juce::Colour& colour)`, then sets
      the text box style, the name and the attachment, and never reads `colour`
      at all. The compiler says so as `-Wunused-parameter`.

      This is not warning noise. `FxmeLookAndFeel::drawRotarySlider` reads
      `rotarySliderFillColourId`, `thumbColourId`,
      `rotarySliderOutlineColourId` and `trackColourId`, so a caller passing a
      colour to this constructor is reasonably expecting the knob to come out in
      it, and instead gets the defaults. Any plugin using this overload has a
      silently uncoloured knob.

      **Deliberately left alone rather than fixed or silenced.** Fixing it means
      choosing which of those four colour ids the argument should drive, which
      is a design decision for the shared library; whatever mapping is picked
      will change the appearance of every plugin currently using this
      constructor, none of which is TeAr (it has no sliders at all), so nothing
      here can be looked at to check the result. Silencing it with
      `ignoreUnused` would be worse than leaving it: it would record a bug as
      intentional.

      Fix: decide the mapping, apply it, and look at one plugin that uses the
      overload. Probably `rotarySliderFillColourId` and `thumbColourId` from the
      accent, going by what the house style says about accent on the arc and
      pointer, but that wants confirming against a real knob.

      **decision** (shared library, and it changes how existing plugins look)


---

## Already correct

Areas checked and found clean, so that silence here means "not a problem"
rather than "not looked at".

**The macOS universal-binary trap.** The `if(APPLE)` block sits at
[CMakeLists.txt:9-11](../CMakeLists.txt#L9), before `project()` at line 17, with
the deployment target at 10.13 rather than 11.0, and a comment explaining why.
Both settings are also passed as `-D` on the CI configure line
([release.yml:194-197](../.github/workflows/release.yml#L194)), so file ordering
cannot lose them.

**The AU MIDI trap does not apply.** The target sets `IS_SYNTH TRUE` with
`NEEDS_MIDI_INPUT TRUE` and no explicit `AU_MAIN_TYPE`. JUCE's default chain
(`JUCEUtils.cmake:1815-1823`) picks `kAudioUnitType_MusicDevice` for a synth
before it ever reaches the `kAudioUnitType_Effect` branch, so this is `aumu`,
which receives MIDI. The choice of instrument-with-MIDI-out over `aumi` is
deliberate and documented in both [readme.md:230](../readme.md#L230) and
[doc/architecture.md](architecture.md). Run `/au-logic-audit` for the rest of
the matrix.

**The verify step can fail.** Both workflows' "Verify universal binaries" steps
run under `set -euo pipefail`, accumulate a `status` variable across both
bundles, emit `::error::` annotations, and end in `exit $status`
([ci.yml:150-169](../.github/workflows/ci.yml#L150),
[release.yml:204-223](../.github/workflows/release.yml#L204)). A missing slice
fails the job rather than printing `lipo -info` and moving on.

**README coverage.** [readme.md:205](../readme.md#L205) has the macOS
installation section with the `xattr -dr com.apple.quarantine` lines for both
the `.vst3` and the `.component`, the zip variant, and the "if it still does not
appear, that is a real bug" line. [readme.md:228-242](../readme.md#L228) has the
per-host MIDI routing section covering Logic, GarageBand, Live, Bitwig, Cubase,
Studio One and Reaper.

**Registration completeness.** Single-plugin repo with one `juce_add_plugin`
target, present in the root CMakeLists, in both workflows, and in the README, so
the multi-plugin omission this check exists for cannot occur here.

**State and versioning.** `getStateInformation` / `setStateInformation` are both
implemented, and the version handling is better than the checklist asks for:
two properties (`pluginVersion` for information, `patternSyntax` for the
migration that actually tests it) written onto `apvts.state` rather than as
attributes at serialisation time, precisely so that presets, which carry only
`apvts.copyState()`, are versioned too
([PluginProcessor.cpp:123-147 in the header](../Source/PluginProcessor.h#L123),
[PluginProcessor.cpp:338](../Source/PluginProcessor.cpp#L338)). Absence is
interpreted as syntax 1 and migrated, and there is a legacy loader for
pre-versioning states
([PluginProcessor.cpp:436](../Source/PluginProcessor.cpp#L436)).

**Preset system.** `fxme::PresetManager` is constructed against the BinaryData
resource list, with `onBeforeSave` / `onAfterLoad` folding the dynamic
arpeggiator vector into and out of the tree
([PluginProcessor.cpp:17-58](../Source/PluginProcessor.cpp#L17)). The editor has
both house preset components, a `fxme::PresetBarComponent` in the top bar and a
full `fxme::PresetComponent` in an overlay
([PluginEditor.h:81-83](../Source/PluginEditor.h#L81)). Seven factory presets
ship in `Source/presets/`.

**`fxme::EmbeddedAudio` does not apply.** The plugin loads no audio and
references no file paths; its binary data is three images and the preset XML.

**Controls.** There is not one slider or knob in TeAr, so the `fxme::FxmeSlider`
items (bare `juce::Slider` plus TextBox plus Label, `setShowLabel`, `setName`,
accent on the arc rather than the disc, `drawFromCentre` versus
`setCentralValue`) have nothing to apply to. `grep -rn 'juce::Slider\|
setTextBoxStyle\|drawFromCentre' Source/` returns nothing outside the submodules.
The latching buttons are already `fxme::AccentToggle`, styled through
[Theme.h:101](../Source/Theme.h#L101) and
[:115](../Source/Theme.h#L115).

**`TextEntryFocusFixer` is present and correctly placed.**
[PluginEditor.h:123](../Source/PluginEditor.h#L123) declares exactly one, last
among the members so it walks children already in place, and the multiline
Return handling is deliberately left to the pattern field itself
([ArpeggiatorComponent.h:9-33](../Source/ArpeggiatorComponent.h#L9)) because the
fixer leaves Return alone on multiline editors. The only gap is S5, which is the
fixer's own modal suspension, not a wiring mistake here.

**`setEnabled` is honoured where it is used.**
[PluginEditor.cpp:313](../Source/PluginEditor.cpp#L313) disables
`removeArpButton` when only one arpeggiator is left, and
`fxme::AccentToggle::paintButton` self-paints with an explicit `isEnabled()`
branch
([AccentToggle.h:95-102](../Source/libs/FxmeTools/FxmeTools/components/AccentToggle.h#L95)),
so that one was visible even before the S1 sync. R8 is about the controls that
never call it.

**Look-and-feel lifetime.** No dangling `LookAndFeel` pointers. Both
`ArpLookAndFeel` members are declared before the widgets that use them
([PluginEditor.h:74](../Source/PluginEditor.h#L74),
[ArpeggiatorComponent.h:63](../Source/ArpeggiatorComponent.h#L63)), so they
outlive them under reverse-declaration-order destruction, and both destructors
null the pointers explicitly anyway
([PluginEditor.cpp:195-205](../Source/PluginEditor.cpp#L195),
[ArpeggiatorComponent.cpp:58-64](../Source/ArpeggiatorComponent.cpp#L58)).

**Parameter reads in `processBlock` are atomic and allocation-free.** The
generic rule flags reading parameters by ID on the audio thread, but it does not
bite here: `AudioProcessorValueTreeState::adapterTable` is a
`std::map<StringRef, ..., StringRefLessThan>`
(`juce_AudioProcessorValueTreeState.h:662`), so `getRawParameterValue (StringRef)`
does a comparison-tree walk over borrowed pointers with no `juce::String`
constructed and nothing allocated, and it returns a `std::atomic<float>*` that
is then `load()`ed. Caching the pointers in the constructor would shave a few
string compares per block, but this is not a realtime-safety finding.

**The rest of `processBlock` is disciplined.** `ScopedTryLock` with a clean
bail-out rather than a blocking lock
([PluginProcessor.cpp:169-174](../Source/PluginProcessor.cpp#L169)),
`getPlayHead()` guarded before dereference
([:182](../Source/PluginProcessor.cpp#L182)), `heldNotes` pre-allocated to 128 in
`prepareToPlay` so `addIfNotAlreadyThere` cannot reallocate
([:123](../Source/PluginProcessor.cpp#L123)), the scratch chord and scale buffers
pre-warmed to the largest scale for the same reason
([:125-133](../Source/PluginProcessor.cpp#L125)), no file or console I/O, and the
audio-thread-to-message-thread hop done properly through an `std::atomic<int>`
plus `AsyncUpdater` rather than by touching the APVTS directly
([:250-251](../Source/PluginProcessor.cpp#L250),
[:507](../Source/PluginProcessor.cpp#L507)). `triggerAsyncUpdate` was checked
rather than assumed: its message object is allocated once in the constructor and
guarded by a compare-and-set, so a repeated call does not allocate
(`juce_AsyncUpdater.cpp:74-84`). S2 is the one allocation left.

**Editor-side threading.** The two unlocked accessors that remain,
`getArpeggiatorCurrentStep` and `getArpeggiator`
([PluginProcessor.cpp:670](../Source/PluginProcessor.cpp#L670),
[:678](../Source/PluginProcessor.cpp#L678)), are both commented as deliberate
races accepted for visual feedback, and both are called with indices the editor
has already bounds-checked.

**Documentation.** Beyond the version drift in H5, `doc/architecture.md` and
`doc/development.md` are unusually complete: layout, deliberate decisions, the
`-j2` LTO rule, the `file(GLOB)` reconfigure trap for new presets, and the
dead-submodule note that made H1 easy to confirm.

---

## Coverage caveats

What a static audit cannot see here.

- [x] ~~Paste the warnings from the first build after the S1 sync.~~ Done on
      2026-08-18, and better than a paste: all five of TeAr's translation units
      were compiled individually with the plugin's own flags (`make -f
      CMakeFiles/TeAr.dir/build.make <file>.cpp.o`, no link, no LTO). **Zero
      errors**, which also confirms the S1 bump introduces no API break in
      practice and not merely on a header diff.

      The warnings confirmed R5 exactly, at all three predicted sites including
      the implicit one that grep cannot see
      ([ArpeggiatorComponent.cpp:13](../Source/ArpeggiatorComponent.cpp#L13),
      [KeyboardComponent.cpp:108](../Source/KeyboardComponent.cpp#L108),
      [ArpLookAndFeel.h:54](../Source/ArpLookAndFeel.h#L54)), and turned up
      three things the static pass had missed, now recorded as R9, R10 and H9.
      One of them, a dead binding at `PluginEditor.cpp:365`, was removed as part
      of S3.
- No build, no `auval`, no host test was run, so nothing here says whether the
  plugin currently validates or loads.
- The AU and Logic matrix beyond `AU_MAIN_TYPE` was not covered; run
  `/au-logic-audit` for that.
- FxmeTools was audited only where TeAr reaches into it (the arpeggiator, the
  look-and-feel, the focus fixer, `AccentToggle`, `FxmeButton`). The rest of the
  submodule is out of scope for a TeAr audit.

---

## Commit plan

Apart from the FxmeTools sync itself, nothing below has been done. The submodule
goes first in every case, then the bumped pointer in the parent, because a
parent commit pointing at an unpushed submodule commit is a broken clone for
everyone else. `4b22e3c` is already on `origin/main`, so the pointer commit is
safe to make now.

- [x] ~~S1 is a fast-forward, not a commit.~~ Done, and committed.
- [x] ~~Commit and push S2 in the submodule, then the parent pointer.~~ Done:
      FxmeTools `a3de21f`, pushed; parent `2ef7142`, committed.
- [ ] **Push the parent.** `2ef7142` is committed but still local. The submodule
      commit it points at is already on `origin`, so this is safe to push now.
- [ ] **FxmeTools submodule** (`Source/libs/FxmeTools`), on top of `a3de21f`:
  - [x] ~~Run the engine's own test suite before committing S2.~~ Done: 60
        cases, 452 assertions, all passing, and the allocation measured away
        against the pre-change header. Reproduce with
        `Source/libs/FxmeTools/tests/run_tests.sh`, which configures
        `build_tests/` in Debug with `TEAR_BUILD_TESTS=ON` and runs the
        `ArpeggiatorTests` case. Note the script's build line is `--parallel`
        with no cap; the run here used `-j2` instead, out of habit, though this
        target is Debug and links no LTO.
  - [x] ~~S2 (`MidiBuffer` allocation in `midi/Arpeggiator.h`).~~ Committed as
        `a3de21f`: 73 insertions, 18 deletions, one file. Source-compatible with
        every existing call site.
  - [x] ~~S5 (modal test in `components/TextEntryFocusFixer.h`).~~ Done in the
        working tree, still to commit. Unlike S2 this does change behaviour for
        every plugin that owns a fixer and a parented callout, so it is worth
        a line in the FxmeTools commit message.
  - [ ] H9 (the warning cleanup in `FxmeMeters.h` and friends) if taken.
  - [x] ~~S7's FxmeTools half: make `makeEuclidianPattern` and
        `makeRandomPattern` static in `midi/Arpeggiator.h`.~~ Done.
  - [ ] Push FxmeTools before bumping the parent pointer again.
- [ ] **TeAr parent repo**, in one commit or a few:
  - [x] ~~the bumped `Source/libs/FxmeTools` pointer.~~ Done as `2ef7142`. The
        507 new lines of shared header were checked separately: all five
        translation units compile with zero errors against it.
  - [x] ~~S3 in `Source/PluginProcessor.cpp`~~, together with the dead-binding
        removal in `Source/PluginEditor.cpp`. Both compile clean; done in the
        working tree, still to commit.
  - [x] ~~S4 in `Source/PluginProcessor.cpp`~~ and `Source/PluginProcessor.h`,
        with the one call site in `Source/PluginEditor.cpp`. Compiles clean;
        done in the working tree, still to commit.
  - [x] ~~S7 in `Source/PluginEditor.cpp`~~ Done, together with its FxmeTools
        half. Submodule first, as always.
  - [x] ~~R9, R10 in `Source/PluginEditor.{h,cpp}` and
        `Source/PluginProcessor.cpp`~~ Done. R10 is uncommitted.
  - [x] ~~R1, R2, R3 in `Source/PluginEditor.cpp` and
        `Source/ArpeggiatorComponent.{h,cpp}`~~ Done in the working tree, still
        to commit. A visual change, so worth a look in a DAW before pushing.
  - [x] ~~R8 in the same files, now unblocked by R1~~ Done in the working
        tree, still to commit. Visible behaviour, so worth a look in a DAW.
  - [x] ~~R4 in `Source/PluginEditor.h`~~, with R6 and R7 in
        `Source/PluginEditor.{h,cpp}`. Done in the working tree, still to
        commit. All three are visible changes: tooltips start appearing, the
        Follow MIDI In tick is redrawn, and a help button joins the toolbar.
  - [x] ~~R5 in `Source/ArpeggiatorComponent.cpp`,
        `Source/KeyboardComponent.cpp`, `Source/ArpLookAndFeel.h`~~ and R9 in
        `Source/PluginEditor.cpp`. Done in the working tree, still to commit.
        With these two, TeAr's own sources carry no warnings at all except
        R10's sign-conversions.
  - [ ] H4 in `Source/popupWindow.cpp`, H8 in `Source/PluginEditor.cpp` (and
        `Source/Theme.h` for the now-unused `panelTint`)
  - [ ] H5 in `doc/architecture.md`, H6 in `.github/workflows/release.yml`
  - [x] ~~H7 in both workflows~~ Done in the working tree, still to commit.
        Pairs naturally with the S6 commit, or with H6, which is in one of the
        same files.
    - [x] ~~S6 in `.github/workflows/ci.yml`~~ Done in the working tree, still to
        commit. See H7 below: with CI now running on every push, the stale
        `FxmeJuceTools` steps run four times per push instead of only on demand.
  - [x] ~~H1 (`git rm` the `cppMusicTools` submodule)~~ Done and staged, with
        `doc/development.md` updated alongside. Still to commit.
  - [x] ~~H2 (moving `KeyboardComponent` out) is a separate commit spanning
        both repos~~ Done. FxmeTools gains
        `components/ScaleKeyboardComponent.h` and an umbrella include; TeAr
        loses two files, a CMakeLists entry, and gains the colour callback.
        Submodule first, then the parent pointer.
  - [ ] H9 is FxmeTools-only, so it belongs in the submodule list above rather
        than here
  - [ ] this audit file, once it has been read (it is deliberately left
        unstaged)
- [ ] **Build, on your side, one target at a time.** Release links with LTO, so
      never `-j$(nproc)`:

      cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
      cmake --build build --target TeAr_VST3 -j2

      Reconfigure is only needed if a file was added or removed (H2 and H3 both
      change the source list in `CMakeLists.txt`). Then copy the `.vst3` into the
      VST3 folder and make the DAW rescan, since the build does not install and
      the host caches the module.
