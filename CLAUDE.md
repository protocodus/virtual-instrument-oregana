# Working conventions

## Architecture

This is Oregana's canonical repository: it owns the DSP engine in
`Source/DSP/`, a physically modelled church organ of 51 stops on three
manuals and pedal. It is a first pass (README, Known gaps): there is no JUCE
plug-in, no CI and no release yet. When a plug-in is added it wraps the same
engine, with plug-in parameter IDs taken from the control ids in
`Source/DSP/OreganaControls.h`. A Reason Rack Extension would live in the
private `protocodus/reason-rack-extensions` repository as a thin adapter that
mounts this repository as the pinned submodule `Examples/Oregana/Instrument`
and compiles `Source/DSP/OreganaEngine.cpp` directly. DSP and playing-model
changes land here first; the Rack pin then advances. Reason property mapping,
MIDI/CV, routing, host latency, patches and panels stay in the adapter.

Repository layout, naming, CI, versions and the `Docs/` taxonomy follow the
Protocodus portfolio standard kept in the project, not in this repository;
`python3 Tools/check_repo_standard.py <checkout>` in the Rack repository
reports what this first pass does not yet meet (the plug-in, CI, the
parameter matrix and the generated README parts).

- The DSP version is the product version: `project(Oregana VERSION X.Y.Z)` in
  `CMakeLists.txt`. Any DSP or behaviour change bumps it, and versions only go
  up; a Rack device takes the same `X.Y.Z` plus its own stage suffix.
- `Source/DSP/` is JUCE-free and C++17 (the Rack toolchain); the
  `OreganaDSPCxx17` target proves it with the Rack's `-fno-builtin`. No
  allocation, file I/O, locks or exceptions after construction, and no libm
  call on the per-sample path that `-fno-builtin` turns into a real call: use
  `DspMath.h`.
- Front ends share `Source/DSP/OreganaControls.h` (the controls, their ids,
  ranges and normalised mapping) and `Source/DSP/OreganaPresets.h` (the
  factory registrations). Never rename or reuse a control or stop id, and
  append new controls at the end: saved sessions and Rack patches recall by
  them.
- A front end with one keyboard or a rate other than 48 kHz plays through
  `Source/DSP/OreganaInstrument.h` (the keyboard's manual and Pedal keys, the
  mod wheel and expression as the swell pedal, the resampler).
  Playing behaviour belongs there or in the engine, never in one front end.
- When a plug-in is added, its `BUNDLE_ID` and CLAP id are
  `cz.protocodus.oregana`, and they and its plug-in codes never change once a
  build reaches users: hosts recall sessions by them.
- Never add Reason/Jukebox SDK material, secrets, recordings or unreleased
  commercial material: the product repositories are public.

## Documentation layout

`README.md` is the single product document. It opens with the heading and a
one-paragraph intro, the editor screenshot, **Audio demos**, a concise
**Version history** ledger and **Downloads**; after a `---` separator come
**How it works**, **Known gaps**, **Build** and **Licensing**. Long per-version
narratives go to `Docs/release/` and are linked.

Everything else in `Docs/` has one home, and nothing sits loose there except
`decisions.md`, the by-ear decision log described below:

| Folder | Holds |
| --- | --- |
| `audio/` | rendered demos, per-fix previews, committed listening tests |
| `screenshots/` | editor images |
| `guide/` | user, install and preset guides, parameter meaning |
| `research/` | modelling research, references, design notes, experiments |
| `validation/` | audits, test results, host validation |
| `benchmarks/` | CPU and performance measurements |
| `plans/` | improvement plans, requirements, open work |
| `release/` | release checklists, readiness, release notes, asset provenance |

Name documents in lowercase kebab-case, `<topic>-YYYY-MM-DD.md` for dated
work. Moving a document is a rename only: keep its content and date, and update
every link to it in the same commit. Legal files (`LICENSE`,
`THIRD_PARTY_NOTICES.md`, `PRIVACY.md`) stay at the root; upstream licence
texts live in `ThirdParty/`.

Research the code depends on — a specific value, a manual page, a measurement,
a derivation — belongs in a comment **beside the code that uses it**, not only
in a document that drifts away from it. A measurement protocol belongs at the
head of the tool that enforces it.

These README parts are written by the build, never by hand:

- The demo table between `<!-- audio-demos-begin -->` and
  `<!-- audio-demos-end -->`, rendered from one
  `Docs/audio/midi/<NN>-<preset-slug>.mid` per curated preset demo (at most 8,
  each preset showing a different side of the instrument, or every preset of a
  bank of 8 or fewer; change a source only on purpose, in its own commit) and
  committed by the main build as `Docs/audio/<NN>-<preset-slug>.wav`, 8–12 s
  of 48 kHz 24-bit audio.
- The block between `<!-- build-download-begin -->` and
  `<!-- build-download-end -->`, rewritten by the main build, which also
  commits the README screenshot under `Docs/screenshots/`.

Version history is a ledger of what changed about the product — what it
sounds like, what it exposes, what a session must know. Pure refactors,
deduplications and test-coverage additions belong in git history, not in it.

Per-fix audio previews are **one-time review evidence**: render them while the
change is under review, then leave them frozen. Do not re-render them when
later engine changes make them stale.

## A–Z listening tests

This is a physical model with no owned calibration captures, so there are
decisions the measurements cannot close. When a change is audible and the
physics admits more than one defensible option, **render the options as WAVs,
letter them, and let the user decide the direction by ear.** A listening test is
a legitimate way to choose between candidates. It is not a way to fit a number —
see the limit below.

**When to run one.** Whenever any of these is true:

- two or more formulations are each derivable and the model cannot say which
  describes the instrument (which radiation law a body obeys, which of two
  reference geometries a term should be measured against);
- a change is audible and the open question is whether it is an improvement
  rather than whether it is correct;
- a calibrated constant has to be re-pinned and more than one pinning is
  defensible;
- the user asks for one.

Do not run one to decide something a measurement already decides. If a number
settles it, quote the number.

**How to build one.**

- **A is always the shipping engine**, unchanged. B, C, … Z are the candidates.
  So "A against B" is always "what it does now against what it would do".
- **Letters only in the filenames.** `A.wav`, `B.wav`, … carry no hint of the
  mechanism, so the listener is not primed by the label. Write the key down at
  the same time, in a separate `key.md` the user can read after deciding, and
  say in the handover that the key exists and is unread-by-design.
- **Change one thing.** Identical MIDI, seed, sample rate, block size, length
  and controls across every letter; only the mechanism under test differs.
- **Level-match, and say how.** Match on whole-file RMS unless level is itself
  what is being judged, and state the measure and the trims applied. Louder
  reads as better, and an unmatched set tests loudness.
- **Give the same material to every letter**, and pick material that exposes the
  thing under test — a stroke that drives the mechanism hard, not a general
  demo. Keep it short enough to A/B repeatedly.
- Render through the shipping signal path, never through an offline
  approximation of it.

**Where they live.** Listening tests are decision artefacts, not review
evidence. Render them to a working directory and send them to the user; do not
commit them by default — a 26-file set per decision would bloat the repository.
Commit a set only when the user asks, under
`Docs/audio/listening-tests/<date>-<topic>/` with its `key.md`, and freeze it
on merge exactly like the per-fix previews above.

**What gets committed is the verdict.** Record in `Docs/decisions.md` what was
being decided, what each letter was, which letter the user chose, and what that
choice licensed. A direction chosen by ear is recorded as chosen by ear — never
written up as though a measurement had settled it.

**The limit.** A listening test chooses between candidates that are each already
defensible on their own physics. It does not license drawing a constant, fitting
a curve, or keeping a formulation the measurements reject. If every candidate
needs an invented number, the answer is still that none of them ships.

## Demo songs

The README demos play the committed MIDI in `Docs/audio/midi/` (More demos in
`more/`), written only by `Tools/write_preset_midi.py`; `<Name>.DemoMidiSources`
fails when the files and the script differ. Change a phrase in the script, on
purpose, and commit it with its regenerated `.mid` files on their own. Never
edit a `.mid` by hand or rewrite a phrase because the DSP changed. The demos
play each preset as a freshly loaded plug-in does, with no per-demo trims.
The WAVs and the README table come only from the main build. Use the
`portfolio-demo-songs` skill when choosing presets, writing phrases or changing
the renderer.

## Promotion and Shop material

Promo videos, Shorts, their music, stems and thumbnails are review
deliverables: render them to a dated folder outside the checkout, hand them
over, and never commit them. Their sound comes from the shipping engine
playing original MIDI, and every claim on screen or in the copy is one How it
works supports. Use the `portfolio-youtube-promo` skill for a 16:9 YouTube
promo and `portfolio-shorts` for YouTube Shorts and TikTok. Reason Shop copy
and images belong to the Rack repository's `rack-shop-content` skill, and
panel artwork to its `rack-panel-design` skill. Uploading or publishing any of
it is the owner's step.

## Testing

Tests follow the portfolio testing standard (U1–U13); the `portfolio-testing`
skill has the procedures. These hold in every session:

- Every CTest test is `<Name>.<Suite>` with exactly one tier label: `unit`,
  `audio`, `render`, `fuzz` or `perf`.
- A test is never skipped, disabled, loosened or given a longer timeout to
  get green. Diagnose a failure against the baseline first; when a fixture or
  limit was wrong, fix it and say why beside it, in the same commit.
- Checks run in Release builds: use the file's always-on helper, never a bare
  `assert()`. Seeds, rates and block sizes are fixed.
- No test outside the `perf` tier passes or fails on time, and `perf` never
  runs in a Debug or sanitized build. CPU figures are development-machine
  figures, never Reason or DAW CPU.
- A fixed fuzz crash's input goes in `Tests/Fixtures/fuzz/`, which
  `<Name>.DspFuzzReplay` replays in the `unit` tier.
- No parameter goes stale: `<Name>.ParameterReach` (U11) proves every change,
  from the editor or the host, reaches held and new notes after long play.
  Declare a parameter `NewNotes` or `Exempt` only where the design says so,
  with the reason beside it; a reach failure is a bug, not an exemption.
- At the default settings, which start at the lowest oversampling (U13), no
  block of a demo or the stress phrase should take more than 10% of one core
  on the CI runner (U12). `<Name>.CpuBudget` warns above that and never fails
  on it (owner, 2026-11-10); a product above it keeps a Known gap.
- A DSP change is done when the suites it touches, the `unit`, `audio` and
  `perf` tiers and `<Name>DSPCxx17` pass.

## Shared portfolio tools

`Source/DSP/OutputStage.h`, `Tools/PresetDemoRenderer.h`,
`Tools/update_readme_blocks.py`, `Tests/ParameterMatrix.h`,
`Tests/FuzzHarness.h` and `scripts/validate-plugins.sh`, and the Documentation
layout, A–Z listening tests, Demo songs, Promotion and Shop material and
Testing sections above, are byte-identical copies of the portfolio masters in
`reason-rack-extensions/Tools/portfolio/`. Change them there, never here.
Oregana's own parts are `Tools/RenderPresetDemos.cpp` (the registrations and
the CPU budget), `Tools/write_preset_midi.py` (the phrases in
`Docs/audio/midi/`) and `Tools/OreganaVoice.h` (the engine as the renderers
play it).

## Build and verify

```sh
cmake -S . -B build-dsp -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DOREGANA_REQUIRE_PYTHON_TESTS=ON
cmake --build build-dsp --parallel 2
ctest --test-dir build-dsp -L unit --output-on-failure
```

Every test is `Oregana.<Suite>`: `Engine`, `Instrument`, `PipeRegulation`, `DspFuzzReplay`
and `DemoMidiSources` (`unit`), `AudioDemos` (`audio`), `DspFuzz` (`fuzz`,
with `-DOREGANA_BUILD_FUZZER=ON` and Clang) and `CpuBudget` (`perf`, Release
without sanitizers only). There is no `ParameterMatrix` or `ParameterReach`
yet (README, Known gaps). Select the suites that cover a change with `-R`,
build `OreganaDSPCxx17` for any `Source/DSP/` change, then run the `unit`,
`audio` and `perf` tiers before a substantive DSP change is called done.

## Oregana specifics

- **Every sound is a pipe taking wind.** A key opens a pallet and the drawn
  stops' pipes take their division's wind; a pipe's level, attack, pitch and
  release come only from its model under that pressure (`FluePipe.h`,
  `ReedPipe.h`). Never add an oscillator, a note envelope, a sample or a
  level that does not come from the wind.
- **The mechanism and the blower are the one exception**: the action's
  knocks and the wind's noise (`Mechanics.h`) are estimates added at the
  output. They never drive a pipe or the wind.
- `Source/DSP/PipeRegulation.h` is generated by
  `Tools/GeneratePipeRegulation.cpp`, which sounds every pipe and writes its
  tuning and level, and is checked by `Oregana.PipeRegulation`. Any change to
  `FluePipe.h`, `ReedPipe.h` or a rank's scale or voicing in `Stops.h`
  regenerates it in the same commit:
  `build-dsp/GeneratePipeRegulation --write Source/DSP/PipeRegulation.h`.
- The stop list is the owner's choice (2026-11-12): a German core with an
  enclosed third manual after Cavaille-Coll.
  `Docs/research/organ-registers-2026-11-12.md` has the historical organs and
  their sources. Scales, voicing, winding, the swell box, the church and the
  noises are typical values and estimates: say voiced or estimated, never
  measured.
- Each pipe's share of an imperfection is a hash of its place in the organ
  (`Unit`, `Spread` in `OreganaEngine.cpp`), never a random number drawn
  while playing: a pipe keeps its character and every render repeats.
- The pitch wheel is the air's warmth (`Engine::SetBend`, 10 C either way):
  speaking flues follow it by gliding their loops (`FluePipe::Bent`), reeds
  do not. At rest every pipe's bend is exactly 0 and it is stepped as it was
  made; `AirBendsTheFlues` holds pitch, level and the click-free glide.
- Tuning, Temperature, Voicing, Pitch and Temperament remake a pipe, so a
  speaking pipe keeps what it has and takes the change the next time it
  speaks; silent pipes are remade two a tick. Every other control reaches
  held notes.
- The engine must render the same samples however the host cuts its blocks
  (`BlockPartitionInvariance`): wind reaches each pipe on its own sample, in
  the order it is due, and the speaking pipes are added up in one list's
  order.
- Keys are MIDI note numbers everywhere (`KeyDown`, `PipeHz`): 21 to 108
  on the manuals, 21 to 67 on the Pedal. A pipe's place in the hashes is
  its rank and MIDI note (`PipePlace`, `KeyPlace`), so a wider compass
  would not move any pipe's character.
- A stop control is a slider's position, 0 to 1, and anything between is a
  half-drawn stop. Stops, couplers and the roller's stages come on through
  `Register` and the sliders, never at once; the couplers' lost motion is
  taken up only when a key goes down (`UpdateKey(key, true)`).
- The church's controls fade or glide (`Nave.h`): a new one must do the
  same, and `ChurchIsThePlayers` holds the decay in each band and the
  click-free turn.
- The factory registrations' Volume trims come from
  `build-dsp/RenderOregana --levels`; run it again after any change to the
  pipes' levels, the master level or the church.
- `python3 Tools/render_showcase.py build-dsp <out-dir>` renders the longer
  listening set (the Toccata's opening, the crescendo, the imperfection
  settings) to a directory outside the checkout; none of it is committed.
- The engine holds every pipe's delay lines and the church: keep it on the
  heap, and construct it off the audio thread (it designs 5,130 pipes).
