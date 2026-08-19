# Development snippets

Copy-paste commands for building, testing, and releasing TeAr. Nothing here is
clever; it exists so the exact invocations do not have to be remembered.

Every path is relative to the repository root (`~/src/TeAr`).

## Repository layout

JUCE is not vendored. It is expected as a sibling directory, `../JUCE`, at tag
`8.0.12` (the version the CI workflow checks out).

One git submodule lives under `Source/libs`:

| Path | Purpose |
| --- | --- |
| `Source/libs/FxmeTools` | The FX-Mechanics shared JUCE module. Holds the arpeggiator engine and the shared GUI components, and is the one that gets edited. |

There used to be a second, `Source/libs/cppMusicTools`, an older vendored copy
of the same engine that took no part in the build. It was removed on
2026-08-19. Nothing is lost by that: it remains its own repository at
https://github.com/odoare/cppMusicTools, and the engine it held now lives in
FxmeTools under `FxmeTools/midi/`.

A clone made before that date keeps an empty `Source/libs/cppMusicTools`
directory and a stale entry in `.git/config`. `git submodule sync` followed by
a `git submodule update --init` tidies it, or simply delete the directory.

After cloning, or after a pull that moved a submodule pointer:

```bash
git submodule update --init --recursive
```

Cloning fresh:

```bash
git clone --recurse-submodules https://github.com/odoare/TeAr
```

## Building the plugin

Configure once per build directory:

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
```

Then build a single format rather than everything:

```bash
cmake --build build --target TeAr_VST3 -j2
```

Useful targets: `TeAr_VST3`, `TeAr_AU` (macOS only), `TeAr_All`,
`TeArBinaryData` (the images and factory presets blob).

Do not use `-j$(nproc)`. A Release build links with LTO
(`juce_recommended_lto_flags`), and running many LTO link jobs at once has
frozen this machine before. `-j2` is the working default, `-j4` the ceiling.

Editing existing sources needs no reconfigure. Adding or removing a file does:
the source list in `CMakeLists.txt` is explicit, and the factory presets are
collected by a plain `file(GLOB)`, which runs at configure time only. A new
`Source/presets/*.xml` will not appear in the plugin until:

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --target TeAr_VST3 -j2
```

### Installing the result

The build does not install anything. On Linux:

```bash
rm -rf ~/.vst3/TeAr.vst3
cp -r build/TeAr_artefacts/Release/VST3/TeAr.vst3 ~/.vst3/
```

The host caches the module, so it has to rescan or be restarted afterwards.
Bundle paths for the other formats:

```
build/TeAr_artefacts/Release/VST3/TeAr.vst3
build/TeAr_artefacts/Release/AU/TeAr.component        # macOS only
```

## Unit tests

The tests are opt-in, in their own build directory so the Debug settings do not
disturb the Release plugin build.

```bash
cmake -B build_tests -S . -DCMAKE_BUILD_TYPE=Debug -DTEAR_BUILD_TESTS=ON
cmake --build build_tests --target FxmeToolsTests -j2
build_tests/Source/libs/FxmeTools/tests/FxmeToolsTests_artefacts/Debug/FxmeToolsTests
```

The last line runs the doctest binary directly, which prints per-assertion
detail. Through CTest instead (one registered test, `ArpeggiatorTests`):

```bash
ctest --test-dir build_tests --output-on-failure
```

Running one case while working on it:

```bash
BIN=build_tests/Source/libs/FxmeTools/tests/FxmeToolsTests_artefacts/Debug/FxmeToolsTests
$BIN --list-test-cases
$BIN -tc="closing a block hands velocity back to the played note"
```

There is also `Source/libs/FxmeTools/tests/run_tests.sh`, which configures,
builds and runs in one go. Note that it builds with an uncapped `--parallel`;
prefer the explicit commands above.

A test that passes proves nothing until it has been seen to fail. When adding a
regression test, reintroduce the bug it targets, confirm that exact test fails,
then restore.

## Git with the submodule

Work in `Source/libs/FxmeTools` is a separate repository. The parent only
records which commit of it to use, so a change there always means two commits.

Seeing what changed on both sides:

```bash
git status --short                      # ' m ' on the submodule = its contents changed
                                        # ' M ' on the submodule = the recorded commit changed
git diff --submodule=diff               # parent diff, with submodule changes inlined
git -C Source/libs/FxmeTools status --short
git -C Source/libs/FxmeTools diff
```

Committing, submodule first:

```bash
git -C Source/libs/FxmeTools add -A
git -C Source/libs/FxmeTools commit -m "..."
git -C Source/libs/FxmeTools push
```

Then the parent, whose commit records the new pointer:

```bash
git add Source/libs/FxmeTools <other changed files>
git commit -m "..."
git push
```

Pushing the parent before the submodule leaves a pointer to a commit nobody
else can fetch, and CI (which checks out with `submodules: recursive`) fails.

Checking the pointer is not stale:

```bash
git submodule status                    # a leading '+' means the checkout differs
                                        # from the commit the parent records
```

## Releasing

The version has one source: the `project()` line in `CMakeLists.txt`.

```cmake
project(TeAr VERSION 0.4.0)
```

`juce_add_plugin` defaults the plugin version to `PROJECT_VERSION`, so the top
bar, the version stamped into saved states, and the pkg installer version all
follow that line. Bump it, commit, and only then tag.

```bash
git tag -a v0.4.0 -m "TeAr 0.4.0"
git push origin v0.4.0
```

Watch that push actually report the tag:

```
 * [new tag]         v0.4.0 -> v0.4.0
```

`Everything up-to-date` means the tag was already on the remote and no `push`
event was generated, so the workflow will not start. This is the usual reason a
release goes quiet.

Creating a tag through the GitHub web UI fires `create`, not `push`, which is
why `.github/workflows/release.yml` also listens for `release: [published]`:
publishing a release from the web UI covers that route. It does not listen for
`create`. A tag that already exists on the remote produces no event at all when
you try to push it again, which no trigger can rescue.

Checking from the command line:

```bash
git ls-remote --tags origin | grep v0.4.0        # is the tag on the remote?
gh run list --workflow=Release --limit 5         # did anything start?
gh run watch                                     # follow the latest run
```

To exercise the builds without publishing anything (the release job is skipped
unless the ref is a tag):

```bash
gh workflow run Release
```

Redoing a tag that was pushed wrong:

```bash
git push --delete origin v0.4.0
git tag -d v0.4.0
git tag -a v0.4.0 -m "TeAr 0.4.0"
git push origin v0.4.0
```

### Checking the macOS artifacts from Linux

The universal build is easy to get silently wrong (an arm64-only bundle passes
every other check). `tools/check-macos-artifact.sh` verifies both slices and
reports each slice's minimum macOS version, using `llvm-lipo` and `llvm-otool`
in place of Apple's tools.

```bash
sudo apt install llvm                            # once, for llvm-lipo
gh run download <run-id> -n macos-vst3
tools/check-macos-artifact.sh TeAr-VST3-macOS-universal.zip
```

It exits non-zero if a slice is missing, so it can be trusted in a pipeline.

## Documentation

The language reference is LaTeX. Two passes, because of the table of contents
and the cross-references:

```bash
cd doc
pdflatex -interaction=nonstopmode TeAr-language-reference.tex
pdflatex -interaction=nonstopmode TeAr-language-reference.tex
```

The `.tex` and the built `.pdf` are tracked; `.aux`, `.log`, `.out` and `.toc`
are ignored. Worth grepping the second pass for problems, since LaTeX reports
them without failing:

```bash
pdflatex -interaction=nonstopmode TeAr-language-reference.tex \
  | grep -Ei "error|overfull|underfull|Output written"
```

`doc/architecture.md` describes how the plugin is put together and what the
pattern generator work is meant to become.
