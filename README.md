# Oregana

Oregana is a physically modelled church organ: 51 stops on three manuals and
pedal, 5,130 pipes, each one simulated as a pipe taking wind. A flue pipe is a
jet of air driving an air column and a reed pipe a brass tongue beating under
a resonator; the wind comes from bellows that sag and bounce, the third
manual stands in a swell box, and a church answers. It plays from one soft
stopped flute to full organ with 32' reeds, and it can be as well kept or as
worn as you like: controls set how far the pipes have drifted out of tune, how
unevenly they are voiced, how unsteady the wind is, and how many pipes are
dead, pallets hang or channels leak. The machine around the pipes is there
too: stops that slide and can be left half drawn, couplers with lost motion, a
blower that can be switched off under a held chord, or a calcant treading the
bellows. This repository holds the JUCE-free C++17
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

### 0.5.0 — 2026-11-13 — pipes that hear each other, divisions in their places

- **Neighbouring flue pipes hear each other.** A flue hears the nearest
  speaking flues of its own key up to two rows away on the chest. Two that
  are within about 3 cents pull into one pitch and stop beating (a ripple
  of 0.1 to 0.3 dB where it was 5 dB); further apart they beat as before.
  Ranks meant to beat (the Voix celeste) and the reeds stay out of it.
- **Each division sounds from where it stands**: the Positiv before and
  below the Hauptwerk, the Récit above and behind it, the Pedal behind. The
  church reflects each from its own place, and a rank further back on its
  chest is heard a third of a millisecond later for each row.
- **More of the machine is heard** on Action and Wind Noise: a coupled key
  knocks again at each chest it pulls, a pedal key lands lower and longer,
  the swell's shutters knock at the ends of their travel, the tremulant's
  valve thumps once a beat, a slider rubs while it travels, and the blower
  is heard starting and running down.
- **About half the CPU**: the Tutti demo's worst block went from 37 % of a
  core to 20 %, the Plenum's from 14 % to 8 %. The pipes and the church's
  delay lines are stepped four to an instruction and give the samples they
  gave one at a time. Pipes ringing out after their keys are let go no
  longer cost double on a host that does not flush denormal numbers: half a
  second after sixteen keys of full organ, 7 % of a core where it was 15 %.
  A released pipe is let go once it has rung down some 105 dB under its own
  speech, where it rang 20 dB further (owner, 2026-11-13: past hearing): a
  quarter to a half fewer pipes ring on in a release. A pipe struck again
  after that starts from rest, so a render differs from 0.4.0's in how
  near-unison pipes beat, not in their levels. A reed is judged by what its
  resonator gives the room, not by the wave inside it: the lowest Trompete
  is let go 2.3 s after its key where it was kept 7.6 s, its sound by then
  116 dB down.
- **The flue pipes breathe as recorded ones do.** A pipe's wind noise is now
  strongest about its first harmonics and falls towards the top, where it
  was even: 9 to 17 dB more noise between a pipe's harmonics, as two
  recorded organs have it
  ([Ilmenau](Docs/research/benchmark-ilmenau-2026-11-13.md),
  [Bureå](Docs/research/benchmark-burea-2026-11-13.md)). Soft flutes and
  single principals are airier; a full chorus changes little.
- **A held pipe is no longer perfectly steady.** Each flue's own wind wavers
  a little, as its jet does: a held note wanders some 0.7 cents and 0.09 dB,
  where a recorded principal or flute wanders 0.8 cents and 0.14 dB, and no
  two pipes wander together. A reed's tongue is pushed about by the eddies
  in its gap, as Respira's reeds are, so its pitch walks: 0.4 to 0.8 cents
  on the Trompete 8', where a recorded Trumpet 8' walks 0.8 to 0.9. Both
  follow the Wind control and are gone at 0
  ([Docs/research/known-gaps-2026-11-13.md](Docs/research/known-gaps-2026-11-13.md),
  which also holds the research on every gap below).
- **Fixed: a repeated note and a very short one no longer click.** A flue's
  key struck again within 20 ms, before its foot had emptied (a fast
  repetition, or a sequencer's note-off and note-on a tick apart), began its
  puff of wind with a step, a click as large as the pipe's whole sound; a
  note under 10 ms ended its puff with the same step. The puff now begins
  from nothing and dies with the foot (`RepeatedKeysDoNotClick`).
- **Fixed: a quiet patch chosen under a held chord no longer swells first.**
  A quiet registration's Volume, up to 10 dB above a loud one's, arrived in
  20 ms while the loud stops took 0.15 s to go in, so Tutti to Voix celeste
  peaked 6 dB over Tutti for a moment. A rising Volume now takes about 0.2 s
  (2 dB over, `QuietRegistrationDoesNotLiftTheLoud`); a falling one is as
  quick as before.
- **Brighter trumpets.** The Trompetes, the Posaunes, the Trompette and the
  Clairon pass three times the wind under their open tongues, so the flow is
  cut only in the short time a tongue is nearly shut, as in a real reed. The
  eighth harmonic of the Trompete 8's lowest octave rose by 19 dB. This came
  from the first comparison with a recorded organ
  ([Docs/research/benchmark-burea-2026-11-13.md](Docs/research/benchmark-burea-2026-11-13.md)),
  which also found a weakness of the principals that voicing cannot mend
  (Known gaps). Every pipe was regulated again and fourteen presets' Volume
  trims moved by 1 dB.
- **The mod wheel is the swell pedal** for a front end with one keyboard
  (owner, 2026-11-13: loud at 0). At rest the swell box is open; turning the
  wheel up closes it, so only the Récit grows quieter. The crescendo roller,
  which the wheel turned in 0.4.0, is the Crescendo control alone.
- **A seat across the nave.** Seat, a new control at the end of the list,
  moves the listener from the axis towards either side wall, up to 5.5 m.
  The organ is then heard up to 16 degrees to one side, the far ear 8
  samples later and a quarter weaker, and the wall the listener sits by
  answers 1.0 m behind the organ's own sound where it answered 4.6 m behind.
  On the axis, as loaded, nothing is different, and the seat costs nothing
  that can be measured.
- **The pitch wheel warms and cools the air** (owner, 2026-11-13), by up to
  10 °C on top of Air Temperature. Flues follow while they sound, up to
  29.5 cents either way, gliding without a step; reeds, whose tongues set
  their pitch, stay, so the organ goes out of tune with its reeds as a real
  one does on a warm day. With the wheel at rest nothing is different and
  nothing costs more; while it is turned, full organ costs about a third
  more.
- **A preset's building follows its size** (owner, 2026-11-13: the smaller
  the organ, the less reverberation). Room, Church Size, Decay, Decay Bass
  and Width are set from the number of stops a preset draws: three stops
  stand in a chapel (1.3 s), fifteen in the church as it loads (4.5 s),
  twenty-two and more in a cathedral (8 s). A single stop no longer rings
  as long as full organ, the Tutti rings longer than it did, and Cathedral
  Fonds, fifteen stops, lost its long nave. The Volume trims were read
  again.
- **Wind Pressure and Width no longer click when thrown.** A jump of either
  under held notes, as automation or a patch change makes, was taken at
  once and clicked; both now cross their range in 50 ms. Every control was
  thrown and swept under held notes and no other one clicked.
- A session sounds a little different: locked pipes are steadier and the
  early reflections changed. No control was added or renamed.

### 0.4.0 — 2026-11-12 — twenty organs

- **Twenty organ presets** after the 17 registrations: five chapel organs
  (Positive, Italian, Village, Romantic, Derelict), ten church organs
  (Baroque, Saxon, Flutes, French, Italian, Romantic, Hymn, Reeds, Evensong,
  Winter) and five cathedral organs (Plenum, Symphony, Reeds, Fonds, Nave).
  Each draws only the stops such an instrument would have and sets its
  building, wind, tuning and state with them: a positive on treadled bellows
  in meantone at 415 Hz in a small dry room, a cold parish church whose reeds
  no longer agree with its flues, a full organ heard from far down a nave
  that rings for ten seconds. The values are typical of such organs, not
  measurements of one.
- Nothing about the engine's sound changes; the registrations are as they
  were.

### 0.3.0 — 2026-11-12 — one keyboard, any sample rate

- **A playing layer for front ends** (`OreganaInstrument.h`): one keyboard
  plays the manual the new **Keyboard** control names, and the Pedal as the
  new **Pedal Keys** control says: off, the lowest key held (as loaded), or
  every key below c. The mod wheel is the crescendo roller and expression the
  swell pedal.
- **Any host rate from 44.1 to 192 kHz.** The organ still runs at 48 kHz;
  off that rate its output is resampled, which adds 32 engine samples of
  latency.
- **Fixed:** a pipe that spoke before a new Tuning, Voicing, Pitch,
  Temperament or Temperature had reached it lost its key's touch and was
  counted into its channel's load at its old size.

### 0.2.0 — 2026-11-12 — the organ as a machine, a new church, 88 keys

- **The keyboards are a piano's 88 keys** (A0 to C8) and the Pedal runs from
  the same A to g1; every key sounds at its own pitch. The organ grew from
  3,618 to 5,130 pipes. Keys and `PipeHz` now take MIDI note numbers.
- **The crescendo roller is smooth.** It turns to where it is set in up to
  0.8 s, each stop it adds slides on, and a roller resting on a stage's edge
  moves nothing.
- **Stops slide and can stay half drawn.** A stop control is continuous: a
  knob takes 0.1 to 0.2 s of travel, each its own, and a slider left part way
  starves its pipes, which sound flat and soft.
- **Couplers have lost motion**, by Action: a coupled manual speaks a few
  milliseconds late, each key by its own amount and later for a slow key.
  The divisions stand at their own depths in the case, the Recit last.
- **Wind Supply**: the blower can be switched off (the reservoir runs down
  and the pipes go flat and die) and on again, or a calcant treads the
  bellows, with a dip and a thud at each changeover. **Wind Pressure** runs
  the organ from 0.6 to 1.5 times the pressure it was voiced on.
- **A new church**: 33 early reflections and a field of 32 delay lines, with
  Church Size, Decay Bass and Decay Treble beside Decay (now 1 to 12 s),
  Room and Width. All of them can be turned while the organ sounds.
- The Blower control is now called Wind Noise. A session's saved control ids
  are unchanged, and five controls are added at the end.
- Every pipe's own share of each imperfection is new, so 0.1.0's renders do
  not repeat; the registrations' Volume trims were read again.

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

Couplers: II/I, III/I, III/II, I/P, II/P, III/P. The 51 stops draw 69 ranks.

**The keys.** An organ's manuals have 61 keys (C to c4) and its Pedal 32 (C
to g1). Here every manual has a piano's 88 keys, A0 to C8 (MIDI 21 to 108),
and the Pedal runs from the same A up to g1 (MIDI 21 to 67), so any key a
player tries first answers at its own pitch. The keys outside C to c4 are no
organ's: they are the same ranks carried on by their builders' rules. Below
C the pipes go on growing, down to a 32' pipe and no further, so a 16' stop
has no pipe below the piano's C1 and a 32' Pedal stop none below C. Above c4
a rank goes on until its pipes would be too small to make (flues above
8.4 kHz, reeds above 2.1 kHz) and then repeats an octave lower, as a mixture
does.

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
jet's turbulence is the wind noise, strongest about the first harmonics and
falling 6 dB an octave above them (calibrated against two recorded organs),
and the puff of the wind's arrival is the chiff. Principals, open, tapered, stopped and chimney flutes, Quintadena,
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
stop drawn gives each pipe a little less. The pressure of a division has a
slow unrest of 0.3 %, and each flue's own wind wavers by 1.5 % under 8 Hz,
its own and no other pipe's: that figure is set by a recorded organ's held
notes (`Tools/benchmark_steadiness.py`). The eddies in a reed's gap push its
tongue by a quarter of the boot's push, which walks its pitch as a recorded
trumpet's walks. The Positiv and the Recit have a
tremulant each. These constants are voiced to written accounts of such
winding, not measured on an organ.

**Keys and stops** (`OreganaEngine.cpp`). MIDI channels 1 to 4 play the
Hauptwerk, Positiv, Recit and Pedal. Velocity is how fast the key opens the
pallet, which a tracker action passes on to the pipes' speech: it changes
the attack, never the level, and there is no sustaining pedal. CC11 is the
swell pedal, whose shutters take 8 dB of the Recit's bass and 24 dB of its
treble when shut. Each pipe is heard from where it stands: the keys
alternate between the two sides of each chest, the Pedal in the towers, the
Positiv at the front of the case, the Hauptwerk half a metre behind it, the
Pedal a metre and a half and the Recit's box three metres, so each pipe's
sound arrives 6 to 20 ms after its pallet opens and the Recit about 8 ms
after the Positiv. All 5,130 pipes exist all the time, and only those
speaking or ringing cost anything.

**The machine.** What a sampled organ cannot do, because here the pipes are
driven by the wind they are given:

- *The crescendo roller* (the Crescendo control; CC1 on the engine's own
  MIDI map) adds 45 stops and couplers in order to whatever is drawn. The
  roller turns to where the control is at a roller's speed, the whole way in
  0.8 s; each stop it reaches slides on like any other, and a stage that has
  come on stays until the control is 0.4 of a stage back below it. Thrown from one flute to full organ, the level
  rises by 18 dB with no step larger than 2.2 dB in 25 ms.
- *Stops slide.* A stop is a slider under its rank with a hole for each
  pipe. A knob takes 0.1 to 0.2 s to travel, each knob its own time, and the
  pipes get more wind as the holes line up. A stop control anywhere between
  in and drawn leaves the slider there: at half, the Octave 8' gets 0.63 of
  its wind and sounds 26 cents flat and 5 dB down, as a half-drawn stop
  does. Under about 0.45 its flues get too little to speak and only hiss.
- *Couplers have lost motion.* With Action at 0 a coupled manual's pallets
  open with the key. A worn action takes up its slack first: 7 to 21 ms at
  an ordinary stroke with Action at 1, each key and coupler its own share
  (7 ms on c1), longer for a slow key and shorter for a fast one, and a
  quarter of that as loaded.
- *The wind supply.* With the blower running the reservoir is kept full.
  Switched off, the organ plays on what is in store, a second and a half of
  full organ or some ten seconds of one stop; then the pressure falls, the
  held flues go flat, weaken and fall silent at a little over half the
  pressure, and the reeds hold on longest. Switched on, the blower
  comes up in about a second and the held pipes speak again. On Bellows a
  calcant treads instead: the pressure droops a little as each bellows
  sinks, and the next takes over with a dip of 3 % and a thud (a full chord
  empties one about every 7 s). He treads only while the organ is played,
  and there is no blower hum.
- *Wind Pressure* runs the whole organ from 0.6 to 1.5 times the pressure
  it was voiced on. On 0.6 the Octave 8' is 29 cents flat and 5 dB down; on
  1.5 it is 24 cents sharp and louder. A little under 0.6 the flues stop
  speaking, which is where the range ends.

- *Pipes hear each other.* A flue pipe's mouth takes in the sound of its
  neighbours. Each flue hears the nearest speaking flue of its own key up to
  two rows before and behind it on the chest, at a strength that falls with
  the distance and grows with the neighbour's width. Two pipes within about
  3 cents of each other pull into one pitch, as close ranks on a real chest
  do: an Octave 8' and a Gamba 8' 2 cents apart ripple by under 1 dB, and
  4 cents apart they beat by more than 3 dB (`PipesHearEachOther`). The
  Voix celeste, the reeds and pipes without wind neither hear nor are heard.
  The strength is an estimate from the pipes' sizes and spacing, not a
  measured coupling.

None of these figures is measured on an organ. The times, the slack and the
supply's sizes are estimates from written accounts of such actions and
winding; the pitch and level figures are what the pipe models then do.

**The church** (`Nave.h`). First 33 reflections by image sources, up to the
third order, of an organ on a west gallery in a nave 14 m wide and 18 m
high, heard 18 m away; one from the side reaches the far ear later and
weaker. Then a reverberant field of 32 delay lines mixed by a Hadamard
matrix. The player has six controls, and all of them can be turned while the
organ sounds:

| Control | What it does | As loaded |
| --- | --- | --- |
| Room | the listener, from at the pipes to far down the nave, at one loudness | 0.6 |
| Church Size | every path's length against the church above: the reflections' spacing and the field's density, from half to twice | 1 |
| Decay | the time the field takes to fall 60 dB in the middle of the range, 1 to 12 s | 4.5 s |
| Decay Bass | the decay below 250 Hz against that, from half to twice | 1.3 |
| Decay Treble | the decay at the very top against it, 0.1 to 0.6: the air's share | 0.2, which leaves 2.7 s at 4 kHz |
| Width | from one field for both ears to the organ's breadth and two fields a head apart | 0.8 |
| Seat | the listener across the nave, from 1.5 m from the left wall, over the axis, to 1.5 m from the right | on the axis |

The divisions stand apart in it. The Hauptwerk is the organ's place above;
the Positiv stands half a metre before it and 2 m lower, the Récit 2.5 m
higher and as far behind as the wall allows, the Pedal a metre behind, and
each has its own 33 reflections. They share the reverberant field. On the
chest a rank two rows back is heard 0.7 ms after the front one. The places
are typical of such a case, not a measured organ's.

A new size or width fades in over 50 ms from the sound already in the
church, so nothing clicks and the tail's pitch does not bend. It is a voiced
church of the size such an organ stands in, not a model of a building.

**One keyboard.** The engine has a keyboard for each division, on MIDI
channels 1 to 4. A front end with one stream of notes plays through
`OreganaInstrument.h`: every key plays the manual Keyboard names, and Pedal
Keys gives the Pedal nothing, the lowest key held up to g1 (an automatic
pedal, as loaded), or every key below c, which then plays no manual. A key
moves with either control while it is held. The mod wheel is the swell pedal:
at rest the box is open, and turning it up closes it, as lowering expression
does, both on top of the Swell control. The crescendo roller is the Crescendo
control. The pitch wheel (MIDI pitch bend, `SetBend`) is the air's warmth:
each flue's loop is read between its samples, a cubic through the four
nearest, and made up to 1.7 % longer or shorter a sample at a time, its jet's
transit with it. Fourteen of the 4,177 flues, all above 3.9 kHz, have loops
too short for that and do not bend. At 48 kHz the engine's
samples go out untouched; at any other rate from 44.1 to 192 kHz they pass a
64-tap windowed sinc, flat to 18 kHz at 44.1 kHz and to 20 kHz above 48 kHz
with its images 80 dB down, by design and not yet measured.

**Imperfections.** An organ is thousands of hand-made pipes in an unheated
building. Each of these controls turns one such thing from none to plenty,
and each pipe keeps its own share of it whatever is played:

| Control | What it does | As loaded |
| --- | --- | --- |
| Tuning | every pipe off its pitch, flues by up to 10 cents and reeds by up to 20 | a quarter: a season after the tuner's visit |
| Temperature | flue pipes follow the air, 2.95 cents a degree, and reeds do not, so the reeds go out against the flues | 0: the 20 C the organ was tuned at |
| Voicing | every pipe off its rank's level (up to 3 dB), speech, chiff and wind noise | 0.3 |
| Wind | how far the reservoirs' sag, dip and bounce reach the pipes, with a slow unrest of the pressure, and how far each flue's own wind wavers and each reed's tongue is pushed about | 0.5: as modelled |
| Wind Noise | the blower's hum (lower while it starts or runs down), the chests' hiss and the bellows' thud | 0.2 |
| Action | how worn the action is: the knocks of keys, coupled pallets, pedals, stop knobs, the swell's shutters and the tremulant's valve, a slider's rub, and the couplers' lost motion | 0.25 |
| Dead Pipes | pipes that do not speak, up to one in sixteen | none |
| Ciphers | pallets that hang part open when their key is let go, up to one key in 25: the note whines on until its stops go in or all sound is cut | none |
| Runnings | wind leaking into the channel a whole tone away, whose pipes whisper along, up to one channel in twelve | none |

Pitch (a1 from 415 to 466 Hz) and Temperament (equal, Werckmeister III,
Kirnberger III, Vallotti, quarter-comma meantone) are the tuner's. The
blower's and action's noises are estimates, added at the output; they never
drive a pipe.

No two ranks of one key speak alike, even on a well-kept organ: on c1 as
loaded, eight ranks of three divisions are heard 6 to 19 ms after the key,
each at its own moment, between 1.5 cents flat and 1.1 sharp, and take 13
to 57 ms to speak (`RegistersSpeakApart` in the engine suite prints them).

**Output.** The engine runs at 48 kHz, the rate its pipes are tuned at, and
ends in the portfolio's shared peak limiter at -1.5 dBFS, which full organ
does not reach on the demos. The factory registrations carry a Volume trim so
that a single stop is not 18 dB below full organ when it is loaded. The organ
presets are trimmed the same way, and they do not limit the crescendo roller:
it still brings on the whole stop list, a chapel organ's too.

## Known gaps

- **No plug-in, no CI, no release.** This is the engine, its tools and
  tests; the generated README parts (screenshot, demo table, downloads) wait
  for a main build. The Rack Extension over it is development source in the
  Rack repository, not yet built with the SDK.
- **CPU is above the portfolio's 10 % target for large registrations.** One
  256-frame block takes at most 3 % of a core on a single stop, 8 % on the
  Plenum demo (198 pipes) and 20 % on the Tutti demo (468 pipes); sixteen
  keys of full organ take 36 %. Development container, GCC 13, Release, one
  thread. The plan is in
  [Docs/plans/cpu-optimisation-2026-11-12.md](Docs/plans/cpu-optimisation-2026-11-12.md).
- **The engine is 48 kHz only.** At another rate `OreganaInstrument.h`
  resamples it, which costs 32 engine samples of latency more.
- **The keys outside C to c4 are no organ's** (How it works, The keys). A
  16' manual stop drawn alone is silent on A0 to B0, and a 32' Pedal stop
  below C.
- **A stop drawn under a held key has no coupler lag**: the lost motion is
  taken up when a key goes down, not when a slider opens.
- **The bass decay is a little under what Decay Bass asks**: at 2 an 80 Hz
  tail lasts 1.9 times as long, at 0.5 it lasts 0.59 of it.
- **No parameter matrix and no parameter-reach test yet** (U4, U11). The
  engine suite checks the extremes of every control together, that each
  control at its other end changes the sound and that none clicks when
  thrown under held notes, and the fuzz target writes any value to any
  control. Pitch, Temperament, Tuning,
  Temperature and Voicing are the tuner's and voicer's work by design: a
  speaking pipe keeps what it was made with, and takes a change the next
  time it speaks.
- **The Vox humana follows the wind more than the other reeds**: 2.3 cents
  for each 1 % of pressure, so it wavers under a heavy registration.
- **Only the trumpet's unrest is checked.** A reed's pitch walks by a push
  set on one recorded Trumpet 8'. The same push walks the Krummhorn 1.5 to
  4.8 cents and the Vox humana 1.0 to 1.9, because their resonators hold
  their tongues less; no recording has checked those, and a reed's level
  wanders 0.03 dB where the recorded one's wanders 0.1 to 0.2.
- **A reed's resonator does not follow the temperature.** In a real reed the
  warming resonator pulls the pitch up a little; here a reed does not move
  with Temperature at all. Letting the resonator's air warm was tried
  (2026-11-13) and taken out: the model's trumpets then moved 27 of the
  flues' 29.5 cents for 10 degrees, because here the resonator and not the
  tongue sets a reed's pitch, which is the reverse of a real reed.
- **The trumpets are still a little dull in the bass.** Against a recorded
  Trumpet 8' the eighth harmonic of the two lowest octaves is 8 dB weaker
  (it was up to 27 dB before 0.5.0), and no weaker above them
  ([benchmark](Docs/research/benchmark-burea-2026-11-13.md)). More flow
  would close the rest and makes the bass pipes slow to speak. The shallot
  has no resonance of its own here, and the Dulcian, Vox humana and Hautbois
  have been compared with no recording.
- **The Krummhorn has only its odd harmonics.** A recorded one has every
  harmonic strong; here the even ones are 25 dB down, because the resonator
  and not the tongue leads the pipe. A voicing led by the tongue matched the
  recording and went 3 to 5 cents flat or sharp for each 1 % of wind, so it
  was not kept. One published Krummhorn spectrum has its even harmonics
  suppressed as here (Pykett), so the recorded stop is one kind of
  Krummhorn and not every one.
- **The Posaune 16' has too much fundamental**: 10 to 20 dB against its
  harmonics in the bass octave, by a recorded Basun 16'. A resonator cut to
  two thirds or half of its length takes 8 to 14 dB off the fundamental on
  the bench; whether that organ's Basun is so built is not known, and a
  full-length Posaune is the German kind, so it was not changed.
- **The stopped pipes are too perfect.** A recorded Quintadena has its even
  harmonics 35 dB under the fundamental and these have them 70 dB under,
  with the fifth and seventh about 20 dB too strong in the bass; a recorded
  Rohrflöte has 15 to 25 dB more fifth harmonic in the middle of the compass.
- **A principal's second harmonic is about 10 dB too weak** against the same
  organ's Principal 8'. The jet swings symmetrically about a small offset;
  a larger offset stops the pipe speaking before it gives the harmonic.
  By Fletcher and Douglas (1980) such a jet's second harmonic can reach
  about 10 dB under its first and no more, which is where this one is;
  their own pipe radiated 3 dB more than that.
- **Real ranks are less even** than these: the notes of a recorded octave
  differ by 1.4 to 5 dB where these differ by under 1 dB as loaded (part of
  the recorded spread is the room).
- **The breath does not lead the tone.** A recorded pipe hisses for some
  100 ms before its tone begins; these are silent until it does. The cause
  is found: a pipe's foot fills here in 3 ms at 247 Hz, and with a foot
  that fills in 10 to 26 ms, as measured ones do (Castellengo, 2004), the
  model's breath leads its tone by 55 ms and the tone comes that much later.
  How late a note should speak is a choice for the ear (a listening set,
  not yet judged). And no
  short note has been compared with a recording: the one set with known key
  times holds only notes of 4 s
  ([benchmark](Docs/research/benchmark-ilmenau-2026-11-13.md)).
- **Nothing is fitted to an organ.** Scales, voicing, winding, the swell
  box, the church, the divisions' places, the pipes' hearing and the noises
  are typical values and estimates, stated as such beside the code. Two
  organs have been compared (the benchmarks above); the trumpets' flow and
  the flue pipes' noise spectrum were set by them, and nothing else.
- **The seat moves across the nave only.** Along it, Room mixes the organ
  and the church and the reflections stay those of a seat 18 m away. From a
  side seat the far ear has the organ later and weaker at every frequency
  alike: there is no head's shadow that grows with the pitch.
- **The new sound has not been listened to.** The pipes' hearing, the
  divisions' places, the mechanism's noises and the pipes' breath were added with A against B
  listening sets that no one has yet judged.

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
