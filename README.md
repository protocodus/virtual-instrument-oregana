# Oregana

Oregana is a physically modelled church organ: 51 stops on three manuals and
pedal, 3,618 pipes, each one simulated as a pipe taking wind. A flue pipe is a
jet of air driving an air column and a reed pipe a brass tongue beating under
a resonator; the wind comes from bellows that sag and bounce, the third
manual stands in a swell box, and a church answers. It plays from one soft
stopped flute to full organ with 32' reeds, and it can be as well kept or as
worn as you like: controls set how far the pipes have drifted out of tune, how
unevenly they are voiced, how unsteady the wind is, and how many pipes are
dead, pallets hang or channels leak. This repository holds the JUCE-free C++17
engine, its tools and its tests. It is a first pass: there is no plug-in and
no release yet.

## Audio demos

No audio is committed yet. Eight phrases, one for each of eight registrations
from a single stop to full organ, are committed in `Docs/audio/midi/` and
written by `Tools/write_preset_midi.py`:

| # | Registration | Phrase |
| --- | --- | --- |
| 1 | Principal 8 | a hymn phrase in four parts on the one open 8' rank |
| 2 | Gedackt 8 | a pastorale on the softest stop, a stopped flute |
| 5 | Voix celeste | slow chords on two string ranks tuned apart, under the swell pedal |
| 7 | Cornet | a solo on the five-rank Cornet over a flute accompaniment |
| 9 | Krummhorn | a tenor solo on a short-resonator reed with the tremulant |
| 12 | Trompete | a trumpet tune on a chorus reed over the principals |
| 14 | Plenum | the opening of a praeludium on the principal chorus and mixtures |
| 17 | Tutti | the first flourish and chord of Bach's Toccata in D minor, BWV 565 |

`build-dsp/RenderOreganaPresetDemos Docs/audio/midi <out-dir>` renders them,
12 s each. `python3 Tools/render_showcase.py build-dsp <out-dir>` renders the
longer listening set to a directory outside the checkout: the Toccata's
opening on full organ, a hymn under the crescendo roller from one stop to all
of them, and one phrase with the imperfection controls at none, as loaded
and plenty.

## Version history

### 0.1.0 — 2026-11-12 — first pass: the engine, 51 stops and the demos

- The flue and reed pipe models, the stop list, the wind, the couplers,
  tremulants, swell box and crescendo roller, the church, and the
  imperfection controls. 17 factory registrations and eight demo phrases.

## Downloads

There is no build to download yet.

---

## How it works

**The organ.** The core is a German baroque organ of the kind Bach played,
after the plan of Arp Schnitger's instruments with colours from Gottfried
Silbermann and Zacharias Hildebrandt; the third manual is an enclosed Recit
after Aristide Cavaille-Coll, with an Italian Voce umana.
[The research](Docs/research/organ-registers-2026-11-12.md) has those
organs' stop lists and sources. It is no one organ's specification, and the
scales and voicing are typical of each kind of stop, not measurements of any
instrument.

| Division | Stops |
| --- | --- |
| Hauptwerk (I) | Principal 16', Octave 8', Viola da Gamba 8', Rohrfloete 8', Octave 4', Spitzfloete 4', Quinte 2 2/3', Superoctave 2', Cornet V (from c1), Mixtur V, Scharff III, Trompete 16', Trompete 8' |
| Positiv (II) | Gedackt 8', Quintadena 8', Unda maris 8', Principal 4', Rohrfloete 4', Nasat 2 2/3', Octave 2', Terz 1 3/5', Quinte 1 1/3', Siffloete 1', Cimbel III, Dulcian 16', Krummhorn 8', Vox humana 8', Tremulant |
| Recit (III), enclosed | Principal 8', Voce umana 8', Flute harmonique 8', Gambe 8', Voix celeste 8', Flute octaviante 4', Octavin 2', Plein jeu IV, Basson 16', Trompette 8', Basson-Hautbois 8', Clairon 4', Tremulant |
| Pedal | Untersatz 32', Principal 16', Subbass 16', Octave 8', Gedackt 8', Octave 4', Nachthorn 2', Mixtur IV, Contraposaune 32', Posaune 16', Trompete 8', Cornet 2' |

Couplers: II/I, III/I, III/II, I/P, II/P, III/P. The manuals have 61 keys
(C to c4) and the Pedal 32 (C to g1). The 51 stops draw 69 ranks.

**Flue pipes** (`FluePipe.h`). The air column is a travelling-wave loop: a
delay, the open end's reflection with its radiation loss, the mouth's end
correction (which puts a wide pipe's upper resonances sharp of its harmonics)
and the walls' loss. A stopped pipe's loop changes sign each pass, so it
sounds an octave below its length on odd harmonics. A chimney flute's stopper
carries a narrow tube joined at a scattering junction, and a harmonic flute is
a pipe of twice the length with a hole at its middle, blown to its octave.
The jet is deflected by the wave returning to the mouth, carries that
deflection across the cut-up in its transit time, which follows the wind, and
blows into or out of the pipe at the lip by a saturating law (Fletcher and
Rossing, *The Physics of Musical Instruments*, section 16.3). Nothing else
makes the tone: the pipe speaks when the wind lets the loop's gain pass one,
settles where the jet saturates and the lip sheds vortices, goes flat as the
wind sags, and rings down on its own resonances when the pallet closes. The
jet's turbulence is the wind noise and the puff of the wind's arrival is the
chiff. Principals, open, tapered, stopped and chimney flutes, Quintadena,
strings and harmonic flutes differ only in the dimensions and voicing a pipe
maker would choose (`Stops.h`).

**Reed pipes** (`ReedPipe.h`). The tongue is a sprung mass that the boot's
pressure pushes shut against the shallot, where it lands on a much stiffer
spring; its curve closes the opening gradually. Wind flows through the
opening by Bernoulli's law into the resonator, a travelling-wave loop closed
at the shallot, a cone or a cylinder; flow, tongue and resonator are solved
together every sample. The tongue sets the pitch and the tuner's correction is
its stiffness, as a tuning wire's is. Trumpets, the French Trompette, Basson
and Hautbois, the Posaunes, Krummhorn, Dulcian and Vox humana are different
tongues and resonators.

**Regulation** (`PipeRegulation.h`). As a voicer does, a generator sounds
every pipe on steady wind, measures its pitch and level and writes the tuning
and regulation each one needs. The pipes the engine suite samples are then
in tune within half a cent at any a1 from 415 to 466 Hz (the Vox humana
within 1.3), and each rank is even along its compass. The table is generated
and checked by a test.

**Wind** (`Wind.h`). Each division has its own reservoir, a weighted plate
over a chest: the pipes take wind by the square root of the pressure, the
trunk resists and the plate has to move before more wind comes. Full organ
sags the pressure by 2 to 3.5 %, a full chord's attack dips it by 6 to 9 %,
and it bounces back at 3.8 to 6.5 Hz, the Pedal's large reservoir slowest. Every
pipe's pitch and level follow, because the pipes are driven by that pressure
and nothing else. The pipes of one key share a pallet, so a key with every
stop drawn gives each pipe a little less. The Positiv and the Recit have a
tremulant each. These constants are voiced to written accounts of such
winding, not measured on an organ.

**Keys and stops** (`OreganaEngine.cpp`). MIDI channels 1 to 4 play the
Hauptwerk, Positiv, Recit and Pedal (notes 36 up). Velocity is how fast the
key opens the pallet, which a tracker action passes on to the pipes' speech:
it changes the attack, never the level, and there is no sustaining pedal. CC1
is a crescendo roller that adds 45 stops and couplers in order to whatever is
drawn; CC11 is the swell pedal, whose shutters take 8 dB of the Recit's bass
and 24 dB of its treble when shut. Each pipe is heard from where it stands:
the keys alternate between the two sides of each chest, the Pedal in the
towers, and each pipe's sound arrives up to 12 ms after its pallet opens,
by its row on the chest. All 3,618 pipes exist all the time, and only those
speaking or ringing cost anything.

**The church** (`Nave.h`). Seven early reflections by image sources of an
organ on a west gallery in a nave 14 m wide and 18 m high, heard 18 m away,
then a reverberant field of sixteen delay lines whose decay is the player's
(1.5 to 9 s, shorter towards the treble). Room moves the listener from the
pipes to far down the nave at one loudness. It is a voiced church of the size
such an organ stands in, not a model of a building.

**Imperfections.** An organ is thousands of hand-made pipes in an unheated
building. Each of these controls turns one such thing from none to plenty,
and each pipe keeps its own share of it whatever is played:

| Control | What it does | As loaded |
| --- | --- | --- |
| Tuning | every pipe off its pitch, flues by up to 10 cents and reeds by up to 20 | a quarter: a season after the tuner's visit |
| Temperature | flue pipes follow the air, 2.95 cents a degree, and reeds do not, so the reeds go out against the flues | 0: the 20 C the organ was tuned at |
| Voicing | every pipe off its rank's level (up to 3 dB), speech, chiff and wind noise | 0.3 |
| Wind | how far the reservoirs' sag, dip and bounce reach the pipes, with a slow unrest of the pressure | 0.5: as modelled |
| Blower | the blower's hum and the chests' hiss | 0.2 |
| Action | the knocks of keys, pallets and stop knobs | 0.25 |
| Dead Pipes | pipes that do not speak, up to one in sixteen | none |
| Ciphers | pallets that hang part open when their key is let go, up to one key in 25: the note whines on until its stops go in or all sound is cut | none |
| Runnings | wind leaking into the channel a whole tone away, whose pipes whisper along, up to one channel in twelve | none |

Pitch (a1 from 415 to 466 Hz) and Temperament (equal, Werckmeister III,
Kirnberger III, Vallotti, quarter-comma meantone) are the tuner's. The
blower's and action's noises are estimates, added at the output; they never
drive a pipe.

**Output.** The engine runs at 48 kHz, the rate its pipes are tuned at, and
ends in the portfolio's shared peak limiter at -1.5 dBFS, which full organ
does not reach on the demos. The factory registrations carry a Volume trim so
that a single stop is not 18 dB below full organ when it is loaded.

## Known gaps

- **No plug-in, no Rack Extension, no CI, no release.** This first pass is
  the engine, its tools and tests; the generated README parts (screenshot,
  demo table, downloads) wait for a main build.
- **CPU is above the portfolio's 10 % target for large registrations.** One
  256-frame block takes at most 2 % of a core on a single stop, 15 % on the
  Plenum demo (198 pipes) and 39 % on the Tutti demo (468 pipes); sixteen
  keys of full organ take 66 %. Development container, GCC 13, Release, one
  thread. The plan is in
  [Docs/plans/cpu-optimisation-2026-11-12.md](Docs/plans/cpu-optimisation-2026-11-12.md).
- **48 kHz only.** A front end at another rate has to convert.
- **No parameter matrix and no parameter-reach test yet** (U4, U11). The
  engine suite checks the extremes of every control together and the fuzz
  target writes any value to any control. Pitch, Temperament, Tuning,
  Temperature and Voicing are the tuner's and voicer's work by design: a
  speaking pipe keeps what it was made with, and takes a change the next
  time it speaks.
- **The Vox humana follows the wind more than the other reeds**: 2.3 cents
  for each 1 % of pressure, so it wavers under a heavy registration.
- **A reed's resonator does not follow the temperature.** In a real reed the
  warming resonator pulls the pitch up a little; here a reed does not move
  with Temperature at all.
- **Nothing is measured from an organ.** Scales, voicing, winding, the swell
  box, the church and the noises are typical values and estimates, stated as
  such beside the code. No comparison against recordings has been made.
- **The pipes do not hear each other.** Neighbouring pipes neither pull each
  other's pitch nor share a chest's resonance, and the church is one voiced
  room.

## Build

CMake 3.22 or newer and a C++20 compiler; the engine itself is C++17.

```sh
cmake -S . -B build-dsp -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-dsp --parallel 2
ctest --test-dir build-dsp --output-on-failure
```

`build-dsp/RenderOregana <registration> <phrase.mid> <out.wav> [id=value ...]`
plays a MIDI file through one registration with any control changed,
`RenderOregana --levels` reads each registration's level, and
`build-dsp/ProbePipe` sounds one pipe alone.

## Licensing

Oregana is source-available under the PolyForm Noncommercial License 1.0.0
with an additional permission for official builds; see [LICENSE](LICENSE) and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
