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
engine, its tools and its tests. Oregana ships as a Rack Extension for Reason,
which plays this engine; a plug-in front end is not built yet, and this
repository has no release.

## Audio demos

Eight registrations from a single stop to full organ, each playing a phrase
written for it (`Tools/write_preset_midi.py`, committed in
`Docs/audio/midi/`) through the shipping engine as the registration loads: a
hymn phrase in four parts on the one open 8' rank, a pastorale on the softest
stop, slow chords on two string ranks tuned apart under the swell pedal, a
solo on the five-rank Cornet over a flute, a tenor solo on a short-resonator
reed with the tremulant, a trumpet tune over the principals, the opening of a
praeludium on the principal chorus and mixtures, and the first flourish and
chord of Bach's Toccata in D minor, BWV 565, on the Tutti.

<!-- audio-demos-begin -->

| # | Preset | Demo | Length | Peak |
| --- | --- | --- | --- | --- |
| 1 | Principal 8 | [01-principal-8.wav](Docs/audio/01-principal-8.wav) | 12.0 s | -14.8 dBFS |
| 2 | Gedackt 8 | [02-gedackt-8.wav](Docs/audio/02-gedackt-8.wav) | 12.0 s | -17.9 dBFS |
| 5 | Voix celeste | [05-voix-celeste.wav](Docs/audio/05-voix-celeste.wav) | 12.0 s | -13.6 dBFS |
| 7 | Cornet | [07-cornet.wav](Docs/audio/07-cornet.wav) | 12.0 s | -16.3 dBFS |
| 9 | Krummhorn | [09-krummhorn.wav](Docs/audio/09-krummhorn.wav) | 12.0 s | -17.8 dBFS |
| 12 | Trompete | [12-trompete.wav](Docs/audio/12-trompete.wav) | 12.0 s | -12.3 dBFS |
| 14 | Plenum | [14-plenum.wav](Docs/audio/14-plenum.wav) | 12.0 s | -11.9 dBFS |
| 17 | Tutti | [17-tutti.wav](Docs/audio/17-tutti.wav) | 12.0 s | -8.4 dBFS |

<!-- audio-demos-end -->

`build-dsp/RenderOreganaPresetDemos Docs/audio/midi <out-dir>` renders them,
12 s of 48 kHz 24-bit stereo each, with the `demos.json` from which
`python3 Tools/update_readme_blocks.py demos --readme README.md --manifest
<out-dir>/demos.json` writes the table.
`python3 Tools/render_showcase.py build-dsp <out-dir>` renders the
longer listening set to a directory outside the checkout: the Toccata's
opening on full organ, a hymn under the crescendo roller from one stop to all
of them, and one phrase with the imperfection controls at none, as loaded
and plenty.

## Version history

### 0.14.0 — 2026-11-26 — the church as an effect (Churech)

- **Churech, the church for any sound.** `Churech.h` plays Oregana's own church (`Nave.h`, not a copy) as a stereo effect, which the Churech Rack Extension wraps: four buildings from a chapel to a cathedral, the sound's distance and the listener's seat, Width and the three decays, a pre-delay, low and high cuts after the church, a duck that draws the church down while the sound plays, Mix and Output. Off 48 kHz only the church's input and answer pass the shared resampler (`RateBridge.h`); the sound itself is never resampled or delayed. The organ sounds as it did.

### 0.13.0 — 2026-11-26 — a sound played in the church

- **An audio input into the church.** A front end can now play another stereo sound into the organ's church: it is heard from the Hauptwerk at the console and takes the Room, the seat, the Volume and the output stage with the organ, but not the divisions' outputs or meters. Off 48 kHz it is resampled into the engine through the organ's own resampler kernel, flat to the same band, 64 engine samples (1.3 ms) later.

### 0.12.5 — 2026-11-22 — bounded ciphers without leftover notes

- **Hanging pallets (`Ciphers`) settle shut on their own.** Sticking pallets now release within 2 to 5 s while other keys are played and shut within 0.75 s once no key is held across any division (or on the next tick if already hung that long), so no imperfection setting ever leaves an infinite note sounding after playing stops; pushing the stops in or sending CC120 still cuts hanging pipes immediately.

### 0.12.4 — 2026-11-21 — low-wind speech, harmonic flutes, reeds and wind-off tremulant

- **Every flue pipe now speaks from `Wind Pressure = 0.6` up.** Harmonic flutes (`Flute harmonique 8'`, `Flute octaviante 4'`, `Octavin 2'`) now split one period's wall loss evenly between both halves, use their open-top lowpass with fractional allpass delay instead of semitone-jumping the top pole, and size the jet against the junction's round-trip gain — restoring all 132 harmonic pipes at 0.6 wind and half-drawn stops, removing up to 7.3 dB note-to-note H4 steps and slow C4 attacks. Top-octave shedding caps at 2.0 instead of halving gentle flutes below threshold, `Tick()` compensates the jet delay-line linear-interpolation dip in real time, and `Stops.h` clamps family voicing interpolation to `[0, 1]` instead of extrapolating into collapse on 16' bass pipes (eliminating all 230 dead/stalling pipes across the organ at 0.6 wind).
- **Short-loop treble stopped flues follow pitch bend, and flues start click-free.** Stopped loops shorter than 5 samples (`Rohrfloete 4'` above 4.4 kHz, `Cornet V 8'` 3.95–4.19 kHz) now apply pitch bend through the allpass slope, and `FluePipe::Clear()` initializes `jetShare` to `JetShape(offset)` so sample 0 under held keys or couplers injects no step impulse.
- **Reed resonators, smooth boot opening and release backflow.** `ReedPipe::Design()` evaluates the resonator loop at its resonance rather than the note frequency (fixing `+73.8` cents mistuning and halved Q on treble `Vox humana 8'`), ramps smoothly across the second half of pallet opening instead of stepping 8x at `3 * attackSeconds`, and blocks negative resonator backflow from refilling an empty boot after key release.
- **Tremulant and `AllKeysUp`.** `Tremulant::Step()` now tracks supply head (slowing with `Wind Pressure` and stopping cleanly when `Wind Supply = Off` instead of beating on a dead organ), and `Engine::AllKeysUp()` resets held key velocities to neutral release touch.
- Regenerated `Source/DSP/PipeRegulation.h` (`0 pipes at fault`), updated the 11 factory presets whose `RenderOregana --levels` Volume trim moved by 1 dB, re-rendered the eight audio demos, and recorded the A/B set in [Docs/decisions.md](Docs/decisions.md).

### 0.12.3 — 2026-11-20 — Master tuning and repeated notes

Host master tuning now spans its full range at every Pitch setting. A low key below the pedal compass leaves the held bass sounding. Rapidly repeating a ringing pipe keeps its attack at the same sample across host block sizes, and a pipe preserves its neighbour’s delay history.

### 0.12.2 — 2026-11-20 — reconnecting division outputs

At 44.1, 88.2, 96 and 192 kHz, connecting a dry division output during playing
now starts with the same samples as a continuously connected output. Previously
the resampler could replay a short burst from an earlier note. The resampler
recovers its 64 input samples from the division delay line when a cable is
connected and processes only the main pair while all division outputs are
disconnected. Loading registrations and changing wind controls while playing
calculate each newly drawn pipe's wind load from its tuned diameter when the
pallet opens. The resonator takes its tuning when that pipe's wind arrives.
Audio reset releases all pipes at once; each pipe picks up its original
turbulence seed when next given wind. The church's feedback lines read
silence until the new performance has written them again.

### 0.12.1 — 2026-11-19 — gentler reeds, and in tune however their wind arrives

- **The cone reeds are voiced gentler.** Every Trumpet, Trompette, Posaune,
  Basson, Clairon and Hautbois takes 0.6 of 0.12.0's flow (the trumpet's 8'
  C 2.1, near 0.4.0's 2), chosen by ear: led by their tongues they had stood
  1.5 to 4 dB brighter above 2 kHz than 0.11.0 in a full chorus, and now
  stand level with it at 2 to 4 kHz and 1 to 2 dB brighter above 4 kHz
  ([Docs/decisions.md](Docs/decisions.md), 2026-11-19, reed brightness).
- **A reed's pitch no longer depends on how its wind arrived.** Its boot
  kept filling at its opening pallet's rate for as long as it spoke, which
  filtered what the tongue drew from it, so a reed whose stop was drawn under
  a held key sounded apart from one whose key went down after: 0.2 cents in
  0.12.0, 2 cents at the new flow. The boot now fills at the open pallet's
  rate once the pallet stands open, three of its time constants after it
  began to open. Every reed is retuned; the Trompete, Church Reeds, England
  Psalm and Germany Organo pleno take a Volume trim 1 dB lower.

### 0.12.0 — 2026-11-19 — reeds led by their tongues, the key's lift, a click removed

From a review of the branch `feat/0.10.0-physical-acoustics-velocity-wind`,
each idea measured against the recorded organs before it was taken
([Docs/decisions.md](Docs/decisions.md), 2026-11-19).

- **Every reed is led by its tongue.** The tongue swings just above the note
  and the wind's push on it is modest, the resonator is cut two halftones
  short and the tuning wire moves the tongue alone, and the tongue rolls
  onto a curved shallot face. A reed's pitch now falls a little with its
  wind, as a real one's does: for 1 % the Trompete 0.6 to 0.9 cents, the Vox
  humana 1.2 and the Hautbois 1.5, the Dulcian next to nothing (the trumpets
  moved 0.2 the other way, the Vox humana 2.3). So the wind's own unrest
  walks a reed, and the push of the gap's eddies is gone.
- **The reeds speak later, and nearer the recorded ones.** The Trompete 8'
  reaches half its level 149 ms after its wind on C, 66 on c1 and 34 on c3
  (it took 71, 20 and 7; a recorded Trumpet 8' takes 90 to 195 ms by its
  octave), the Posaune 16' 199 ms on C. A reed's boot fills through the
  same pallet as a flue's foot, so velocity reaches the reeds too.
- **The trumpets are brighter and the Krummhorn has its octave.** The
  Trompete's harmonics two to eight stand at −2 −4 +3 −1 −6 −8 −4 dB on C
  (−2 −5 −8 −11 −12 −10 −13 before; recorded +4 +1 +2 0 +1 +4 +3). The
  Krummhorn's octave is level with its first harmonic, where the cylinder
  held it 27 dB down and a recorded one has it 2 dB under to 10 over. This
  is brighter than the half-way flow chosen by ear in 0.8.0: the flow is
  the same, the tongue is not.
- **Note-off velocity is the key's lift.** A key let go slowly shuts its
  pallet slowly: the foot empties in up to half as long again, or half the
  time for a quick lift. A keyboard that sends none plays as before.
- **A click is gone.** A flue's jet was read one buffer's length off its
  place when its transit met the buffer's start to within a few millionths
  of a sample: one sample as loud as the pipe, once in a while under a
  wavering wind. 0.11.0's Cornet demo has one at 4.84 s.
- **A key's touch goes with its wind.** The same key struck or let go on
  another division before the wind arrived changed the first one's speech.
- The church and the mechanism cost a little less (the Tutti demo 0.080x
  of real time where it took 0.085x), their sound sample for sample the same.
- The Principal 8', Gedackt 8' and Voix celeste demos are the same files as
  0.11.0's; every registration with a reed sounds different. Trompete,
  Church French, England Trumpet Voluntary and England Psalm changed their
  Volume by 1 dB, and Church Reeds, Church Winter and Germany Organo pleno
  carry 1 dB.

### 0.11.0 — 2026-11-18 — pipes that breathe before they speak, a tremulant that spills wind, a measured swell box

From the rest of [the 2026-11-18 report](Docs/research/physical-gaps-2026-11-18.md)'s gaps.

- **A flue pipe breathes before it speaks.** Its foot fills as a measured
  chest's pallet let one fill at a "short" attack (Castellengo, 2004): the
  pallet's share of the filling is 20.4 ms where it was 1.2 ms, and a hard
  stroke halves it. The breath stands 7 to 12 dB under its steady level in
  the 40 ms before the tone, where there was none, and the tone comes later:
  the Octave 8's fundamental reaches −6 dB of its level 89 ms after the key
  on c1 and 212 ms on C, where it took 44 and 130 (velocity 80). The
  pallet's share counts only while the pallet opens: a filled foot follows
  its wind through its toe alone, and so does the foot of a stop drawn
  under a held key. Reeds are as they were.
- **The tremulant spills wind.** A motor on its division's trunk, driven by
  the wind, lets a pulse of wind escape once a beat, and the pipes take the
  winding's answer: the beat goes deepest where its rate meets the
  reservoir's bounce, and the motor slows as its wind falls, so a chord's
  attack pulls it about (after C. Pykett's account). It was a fixed dip on
  the pressure at exactly the Rate.
- **The swell box answers as measured wooden boxes do** (C. Pykett, several
  boxes averaged): shut, it takes hardly anything of the lowest notes, 3 dB
  at 160 Hz and 6 dB an octave more above that, some 35 dB at 8 kHz, and its
  louvres let most of the treble out in the first tenth of the pedal's
  travel. It took 8 dB below 500 Hz and 24 dB above, evenly with the pedal.
  A registration resting the pedal part open is brighter and louder for it.
- **Voicing loads at 0.6**, not 0.3: a Principal 8's notes now differ by
  2.4 to 2.7 dB over an octave, dry (1.2 to 1.3 at 0.3), where recorded
  octaves differ by 1.4 to 5 dB with their room. Chapel Italian and Chapel
  Village set their own (0.4 and 0.5) and now load more even than the
  default.
- The church was measured against two churches' impulse responses
  (`NaveResponse`, `Tools/room_measures.py`): set to Decay 2.2, Decay Bass
  0.75 and Decay Treble 0.35, it decays within 0.2 s of Ilmenau's in every
  octave from 63 Hz to 8 kHz. Its first echoes are too sparse (Known gaps).
- Every pipe was regulated again, and seventeen presets' Volume trims moved:
  Full Swell's and Chapel Romantic's by 4 dB, Church Evensong's by 3 and
  England Psalm's by 2, registrations that rest the swell pedal part open,
  and the rest by 1 (`RenderOregana --levels`).
- Four listening sets put 0.10.0 against these
  ([Docs/decisions.md](Docs/decisions.md)). The owner chose the breath's
  lead by ear; until the owner judges the other three, their candidates are
  taken as chosen.

### 0.10.0 — 2026-11-18 — flue pipes driven by their jets' change, voiced against recorded organs

From the ranked gaps of [the 2026-11-18 report](Docs/research/physical-gaps-2026-11-18.md).

- **A flue pipe is driven by the rate at which its jet's flow changes**, not
  by the flow itself (the jet-drive source of Verge, Fabre and Auvray): the
  jet's flow pushes the air in the mouth. It gives a principal the strong
  octave every recorded one has. The Octave 8's second harmonic now stands 3
  to 13 dB over its third from C to c3, where it stood 1 to 8 dB under it;
  three recorded organs have it 4 to 18 dB over.
- **Every flue family was voiced again** on the voicer's bench, harmonics two
  to five over five octaves against Bureå's and Ilmenau's pipes: a
  principal's second within 3 dB of theirs, its third within 4 and its fourth
  within 9, the twelfth of the Gedackt and the Quintadena within 4 and 5 dB.
  The Rohrflöte's chimney is longer and wider, to lift its seventeenth. The
  flutes and strings, which neither organ's recordings have, are voiced to
  estimates.
- **The lip sheds more**, which sets how fast a pipe speaks and how far it
  follows the wind: the Octave 8's C takes 126 ms from −20 to −6 dB, and
  the flue stops move 0.82 cents and 0.10 dB for 1 % of wind in the median,
  as recorded flues do (5 to 7 cents for each decibel).
- **The pitch wheel warms everything in a flue alike**: its loop, its jet's
  transit, its mouth's correction and the corners of its losses. The pipes
  now follow the bend within 0.7 cents; bending the delays alone had left a
  65 Hz principal 3.3 cents short of a 29.5-cent bend. At rest every pipe is
  exactly as it was made.
- **Each flue's own wind wavers by 1.25 %, not 1.5 %**: the new pipes follow
  their wind further, and their held notes wander 0.8 cents, as the recorded
  organ's do (`Tools/benchmark_steadiness.py`).
- The breath between the harmonics is 2 dB louder, nearer the recorded
  organs'.
- Every pipe was regulated again, and sixteen presets' Volume trims moved by
  1 dB (`RenderOregana --levels`).
- A listening set puts 0.9.0 against this ([Docs/decisions.md](Docs/decisions.md));
  until the owner judges it, the candidate is taken as chosen.

### 0.9.0 — 2026-11-14 — raw division outputs, a mixer for each division, every preset at 440 Hz

All three at the owner's request (2026-11-14).

- **Each division's own output is raw**: what its pipes give (the Récit's
  through its swell box), with neither its level nor the Volume on it. Its
  meter still reads it at its level and the Volume, as the main output hears
  it.
- **A division's level runs from −24 to +24 dB**, 0 dB in the middle and in
  every preset; it was −24 to +6.
- **Eight new controls, a Mute and a Solo for each division** (`mute_*`,
  `solo_*`): a muted division is out of the sound, and while any division is
  soloed only the soloed ones are in it. Like the level they glide, never
  click, and never reach a pipe, the wind or the division's own output.
- **Every preset loads at a1 = 440 Hz.** The sixteen that set a historical
  pitch (415 Hz for the Chapel Positive, Church French and three French
  registrations, 466 Hz for Church Saxon and the Germany and Saxony sets)
  keep their temperaments; Pitch still sets 415 to 466 Hz. Seven of them have a Volume trim 1 dB different from
  `RenderOregana --levels` at the new pitch.

### 0.8.0 — 2026-11-14 — similar stops set apart, trumpets half way, by ear

Every change here was chosen by the owner by ear from lettered sets
([Docs/decisions.md](Docs/decisions.md)); none is a measurement.

- **The two trumpets, the two gambas and the two mixtures sound less alike.**
  The Récit's Trompette has a resonator 4 halftones narrower, its tongue set
  closer by the wind, and more wind in its treble; the Hauptwerk's Viola da
  Gamba is 6 halftones wider (Silbermann's broad gamba) and the Récit's Gambe
  and Voix céleste 4 narrower (Cavaillé-Coll's keen one); the Plein jeu is
  progressive (its 1' rank from c0, its 2/3' from c1) and 2 halftones wider,
  the Mixtur 2 narrower. The Clairon shares the Trompette's tongue.
- **The trumpets are half as bright as 0.5.0 made them.** The flow under the
  open tongue is now half way between 0.4.0's and 0.5.0's (Trompete 3.5 to
  1.4, Posaune 3.3 to 1.5, Trompette 3.8 in the bass). The Trompete 8's
  eighth harmonic at C is 9 dB lower than in 0.7.0 (`ProbePipe`).
- Every pipe was regulated again, and the presets' Volume trims taken again
  from `RenderOregana --levels` (eight moved by 1 dB).
- The other open sets are recorded as they stand: the mechanism's noises,
  pipes that hear each other, each division's place in the church and the
  0.2.0 church were all kept as shipped.

### 0.7.0 — 2026-11-14 — a level for each division, and its meter

- **Four new controls, a level for each division** (`level_hauptwerk`,
  `level_positiv`, `level_recit`, `level_pedal`, −24 to +6 dB, 0 dB in
  every preset; owner, 2026-11-14): a mixer's fader on what that division's
  pipes give, before the church, the noises and the output stage, and on the
  division's own output. It is not part of the organ: it never reaches a pipe
  or the wind. At 0 dB the sound is the same, sample for sample.
- **`Engine::TakePeaks`**: each division's loudest sample on its own output
  since it was last asked, for a front end's level meters, whether or not
  those outputs are taken.

### 0.6.2 — 2026-11-13 — forty registrations by country and region

- **Forty more presets** after the organs (owner, 2026-11-13), five for each
  of eight schools: Germany (the north's choruses, reed consort and gapped
  flutes), Saxony (the builders' Pleno, Cornet and Tertian combinations and
  the 8' stops together), Bohemia (the Copulas, the Salicional, no reeds),
  the Netherlands (the Prestant alone, the psalms' full organ), France (the
  Plein jeu, Fonds and Cromorne at 415 Hz, the symphonic organ and its Grand
  choeur), Italy (the Principale and the ripieno's ranks on bellows), Spain
  (the Flautado, lleno, Corneta, nasardos and the battle trumpets) and
  England (the Diapasons and the voluntaries without pedals, the Victorian
  organ with its Swell). Each draws this organ's stops under their names
  here, with a temperament, pitch and wind typical of the school: none
  copies one organ, and none is a measurement. Their Volume trims are
  `RenderOregana --levels`'s. No earlier preset changed.

### 0.6.1 — 2026-11-13 — the mod wheel is the crescendo roller

- **The mod wheel turns the crescendo roller** for a front end with one
  keyboard (owner, 2026-11-13), in place of the swell pedal it was in 0.5.0:
  at rest only what is drawn sounds, and all the way up is the full organ.
  The wheel and the Crescendo control are one roller, as far over as the
  further of the two. The swell pedal is expression and the Swell control.

### 0.6.0 — 2026-11-13 — each division's own output

- **Each division can be taken on its own pair** (owner's request):
  `Engine::Render` and `Instrument::Render` fill eight more buffers when
  given them, the Hauptwerk's, the Positiv's, the Récit's and the Pedal's
  left and right. A pair carries what that division's pipes give before the
  church, the seat, the noises and the output stage, the Récit's through its
  swell box, at the Volume and as late as the main output. A pipe is on its
  own division's pair whichever keyboard a coupler plays it from. The main
  output is the same samples with or without them, so a mix of the pairs
  with the church alone needs Room turned full
  (`DivisionsHaveTheirOwnOutputs`, `DivisionOutputsAtEveryRate`).

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
Rossing, *The Physics of Musical Instruments*, section 16.3). Its flow pushes
the air in the mouth, so what drives the pipe is the rate at which that flow
changes, the jet-drive source (Verge et al., 1997; Fabre, Hirschberg and
Wijnands, 2000; Auvray, Fabre and Lagrée, 2012): it weights each harmonic of
the flow by its number, and a jet standing off the lip gives the strong
octave a recorded principal has. Nothing else makes the tone: the pipe speaks when the wind lets the loop's gain pass one,
settles where the jet saturates and the lip sheds vortices, goes flat as the
wind sags, and rings down on its own resonances when the pallet closes. The
jet's turbulence is the wind noise, strongest about the first harmonics and
falling 6 dB an octave above them (calibrated against two recorded organs),
and the puff of the wind's arrival is the chiff. Principals, open, tapered, stopped and chimney flutes, Quintadena,
strings and harmonic flutes differ only in the dimensions and voicing a pipe
maker would choose (`Stops.h`).

**Reed pipes** (`ReedPipe.h`). The tongue is a sprung mass that the boot's
pressure pushes shut against the shallot, whose curved face it rolls onto:
the face takes it up by a spring that grows from nothing, and the tongue's
curve closes the opening gradually. Wind flows through the opening by
Bernoulli's law into the resonator, a travelling-wave loop closed at the
shallot, a cone or a cylinder; flow, tongue and resonator are solved
together every sample. The tongue leads: it swings just above the note, the
resonator is cut short of it, and the tuner's correction is the tongue's
stiffness alone, as a tuning wire's is. The pitch falls up to 1.5 cents for
each 1 % the wind falls. How far the face reaches, how stiff the rolled
tongue is and how hard the wind closes it are estimates, the last set so
that the reeds speak in the time recorded ones take. Trumpets, the French Trompette, Basson
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
slow unrest of 0.3 %, and each flue's own wind wavers by 1.25 % under 8 Hz,
its own and no other pipe's: that figure is set by a recorded organ's held
notes (`Tools/benchmark_steadiness.py`). A reed follows its division's wind, which
walks its pitch as a recorded trumpet's walks. The Positiv and the Recit have a
tremulant each: a motor on the division's trunk, driven by its wind, whose
pallet lets a pulse of wind escape once a beat (`Tremulant` in `Wind.h`).
What reaches the pipes is the winding's answer to those pulses, so a beat
goes deepest where its rate meets the reservoir's bounce, and the motor
slows as its wind falls, so a chord's attack pulls the beat about (after
C. Pykett's account of such tremulants). These constants are voiced to
written accounts of such winding, not measured on an organ.

**Keys and stops** (`OreganaEngine.cpp`). MIDI channels 1 to 4 play the
Hauptwerk, Positiv, Recit and Pedal. Velocity is how fast the key opens the
pallet, which a tracker action passes on to the pipes' speech: it changes
the attack, never the level, and there is no sustaining pedal. A flue's
foot fills through the opening pallet and its toe: the pallet's share is
20 ms at an ordinary stroke and half that at a hard one, as a measured
chest's was at a short and an impulsive attack (Castellengo, 2004), so the
pipe breathes before its tone. A filled foot follows its wind through the
toe alone. CC11 is the
swell pedal. The Recit's box answers it as C. Pykett measured wooden boxes
to (`SwellBox.h`): shut, it takes hardly anything of the lowest notes, 3 dB
at 160 Hz and 6 dB an octave more above that, some 35 dB at 8 kHz; its
shutters are louvres, so the first tenth of the pedal's travel lets out
most of the treble and each tenth after it less. Each pipe is heard from where it stands: the keys
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
  rises by 19.5 dB with no step larger than 2.2 dB in 25 ms.
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

**Churech** (`Churech.h`). The same church as an effect for any sound,
which the Churech Rack Extension plays. Whatever changes in `Nave.h` changes
in both. The sound stands on the floor, 1.6 m up, at one of the four places
the church hears apart, 4, 9, 14 and 19.5 m from the listener at Size 1;
Distance shares it between the two places either side of the one asked
for, and the sound's own level falls as one over that distance, so the far
end of the nave is 13.8 dB quieter than the near one while the church keeps
its level.

| Control | What it does | As loaded |
| --- | --- | --- |
| Type | Chapel (Size 0.5, 1.6 s), Church (0.8, 2.8 s), Basilica (1.25, 4.5 s) or Cathedral (2, 7.5 s): voiced buildings, not measured ones | Church |
| Decay | the building's decay, from half to twice | x1 |
| Decay Bass, Decay Treble, Width, Seat | the church's own, as above | 1.3, 0.2, 0.8, on the axis |
| Distance | the sound from the nearest place to the furthest | nearest |
| Pre-delay | before the church, 0 to 200 ms, a new time faded in over 50 ms | 0 |
| Low Cut, High Cut | 12 dB per octave after the church, 20 Hz to 1 kHz and 1 to 20 kHz | open |
| Duck | the church drawn down by up to 24 dB as the sound rises from -40 to -16 dBFS, back in 0.4 s: voiced | off |
| Mix | the sound against the church, at equal power | 0.3 |
| Output | -24 to 12 dB | 0 dB |

Once the church has rung out under -140 dB with no sound in, it is emptied
and stops costing anything.

**One keyboard.** The engine has a keyboard for each division, on MIDI
channels 1 to 4. A front end with one stream of notes plays through
`OreganaInstrument.h`: every key plays the manual Keyboard names, and Pedal
Keys gives the Pedal nothing, the lowest key held up to g1 (an automatic
pedal, as loaded), or every key below c, which then plays no manual. A key
moves with either control while it is held. The mod wheel is the crescendo roller,
at rest only what is drawn and all the way up the full organ, as far over as
the further of it and the Crescendo control. The swell pedal is expression,
on top of the Swell control. The pitch wheel (MIDI pitch bend, `SetBend`) is the air's warmth:
each flue's loop is read between its samples, a cubic through the four
nearest, and made up to 1.7 % longer or shorter a sample at a time. Warmer
air stretches every time in a pipe alike, so its jet's transit, its mouth's
correction and the corners of its losses move with the loop, and the pipe
follows the bend within 0.7 cents. Fourteen of the 4,177 flues, all above
3.9 kHz, have loops too short for that and do not bend. At 48 kHz the engine's
samples go out untouched; at any other rate from 44.1 to 192 kHz they pass a
64-tap windowed sinc, flat to 18 kHz at 44.1 kHz and to 20 kHz above 48 kHz
with its images 80 dB down, by design and not yet measured.
A sound played in (the input, `Render`'s last buffers) is added to the
organ's own where the Hauptwerk stands, ahead of the church, so the Room,
the seat, the Volume and the output stage take it as they take the pipes;
off 48 kHz it reaches the engine through the same kernel turned round,
64 engine samples late.

**Imperfections.** An organ is thousands of hand-made pipes in an unheated
building. Each of these controls turns one such thing from none to plenty,
and each pipe keeps its own share of it whatever is played:

| Control | What it does | As loaded |
| --- | --- | --- |
| Tuning | every pipe off its pitch, flues by up to 10 cents and reeds by up to 20 | a quarter: a season after the tuner's visit |
| Temperature | flue pipes follow the air, 2.95 cents a degree, and reeds do not, so the reeds go out against the flues | 0: the 20 C the organ was tuned at |
| Voicing | every pipe off its rank's level (up to 3 dB), speech, chiff and wind noise | 0.6 |
| Wind | how far the reservoirs' sag, dip and bounce reach the pipes, with a slow unrest of the pressure, and how far each flue's own wind wavers | 0.5: as modelled |
| Wind Noise | the blower's hum (lower while it starts or runs down), the chests' hiss and the bellows' thud | 0.2 |
| Action | how worn the action is: the knocks of keys, coupled pallets, pedals, stop knobs, the swell's shutters and the tremulant's valve, a slider's rub, and the couplers' lost motion | 0.25 |
| Dead Pipes | pipes that do not speak, up to one in sixteen | none |
| Ciphers | pallets that hang part open when their key is let go, up to one key in 25: the note whines on for a few seconds while other keys are held and settles shut within 0.75 s once no key is played (so no note stays hanging indefinitely), or immediately if its stops go in or CC120 cuts all sound | none |
| Runnings | wind leaking into the channel a whole tone away, whose pipes whisper along, up to one channel in twelve | none |

Pitch (a1 from 415 to 466 Hz) and Temperament (equal, Werckmeister III,
Kirnberger III, Vallotti, quarter-comma meantone) are the tuner's. The
blower's and action's noises are estimates, added at the output; they never
drive a pipe.

No two ranks of one key speak alike, even on a well-kept organ: on c1 as
loaded, eight ranks of three divisions are heard 6 to 19 ms after the key,
each at its own moment, between 1.5 cents flat and 1.1 sharp, and take 10
to 151 ms to speak (`RegistersSpeakApart` in the engine suite prints them).

**Output.** The engine runs at 48 kHz, the rate its pipes are tuned at, and
ends in the portfolio's shared peak limiter at -1.5 dBFS, which full organ
does not reach on the demos. The factory registrations carry a Volume trim so
that a single stop is not 15 dB below full organ when it is loaded. The organ
presets are trimmed the same way, and they do not limit the crescendo roller:
it still brings on the whole stop list, a chapel organ's too.

## Known gaps

- **No plug-in and no release here.** This is the engine, its tools and
  tests, which the Rack Extension in the Rack repository plays; the README's
  screenshot and downloads wait for a plug-in and its main build.
- **CPU is above the portfolio's 10 % target for large registrations.** One
  256-frame block takes at most 4 % of a core on a single stop, 13 to 14 %
  on the Plenum demo (198 pipes) and 30 to 33 % on the Tutti demo (468
  pipes); sixteen keys of full organ take 62 to 64 %. Run in turn with it on
  the same machine, 0.10.0 took 13, 30 to 31 and 66 to 77 %: 0.11.0's feet,
  tremulant and swell box cost nothing measurable. Development container,
  GCC 13, Release, one thread. The plan is in
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
- **Parameter-matrix coverage** (U4). The default patch and the all-minimum,
  all-midpoint and all-maximum configurations run at 44.1, 48 and 96 kHz
  with block sizes of 1, 64 and 512. In the largest church the all-maximum
  case checks the level left by the treading bellows after 32 seconds;
  turning Wind Noise down must also leave the room below −60 dBFS.
  The full pairwise and per-preset
  rotation remains outside the registered release tests. The engine suite checks the extremes of
  every control together, that each control at its other end changes the
  sound and that none clicks when thrown under held notes, the fuzz target
  writes any value to any control, and `Oregana.ParameterReach` (U11) holds
  that after long play every control reaches the notes struck after it and,
  unless its design says otherwise, the notes held through it. Pitch,
  Temperament, Tuning, Temperature and Voicing are the tuner's and voicer's
  work by design: a speaking pipe, or one still ringing when its key is
  struck again, keeps what it was made with, and takes a change the next
  time it speaks.
- **The reeds' unrest is not set against a recording.** Since 0.12.0 a reed
  walks with its division's wind alone: by the suite's measure the Trompete
  8' on c1 moves 0.24 cents between half seconds, where 0.11.0's eddies
  moved it 0.15 and were set a little under a recorded Trumpet 8'.
  `Tools/benchmark_steadiness.py` has not been run on 0.12.0's reeds.
- **A reed's resonator does not follow the temperature.** In a real reed the
  warming resonator pulls the pitch up a little; here a reed does not move
  with Temperature at all. Letting the resonator's air warm was tried
  (2026-11-13) and taken out, because the resonator then set a reed's
  pitch. Since 0.12.0 the tongue does, and the warming has not been tried
  again.
- **Reed measurements cover only some families.** The owner chose the gentler
  cone reeds in 0.12.1 ([listening record](Docs/decisions.md)). The shallot
  has no resonance of its own in this model. Recorded comparisons of the
  Dulcian, Vox humana and Hautbois remain open.
- **The Krummhorn is dull above its fourth harmonic.** Its octave is as
  recorded since 0.12.0; its fifth to eighth harmonics stand 13 to 33 dB
  under the first, where a recorded one's stand 4 to 11 dB over it in the
  bass octave. A shorter resonator and more wind under the tongue
  brighten it and hand the pitch to the resonator.
- **The reeds' estimates.** How far the shallot's face reaches (0.22 of the
  tongue's rest opening), the rolled tongue's stiffness and the wind's push
  on each family's tongue are estimates no source gives.
- **Stopped pipes are weak in their octave above c1.** A recorded Gedackt
  has its second harmonic 59 dB down on C and 30 at c3, a Quintadena 33 to
  36; here they stand 44 to 52. Letting the jet's own flow radiate at the
  mouth brings them to the recorded ones' with a share of about a quarter,
  a number only the recordings give, so it was not taken (2026-11-19).
- **The Posaune 16' has too much fundamental**: 10 to 20 dB against its
  harmonics in the bass octave, by a recorded Basun 16'. A resonator cut to
  two thirds or half of its length takes 8 to 14 dB off the fundamental on
  the bench; whether that organ's Basun is so built is not known, and a
  full-length Posaune is the German kind, so it was not changed.
- **The octave does not speak first.** In a recorded principal the second
  harmonic reaches −6 dB of its steady level 199 to 225 ms before the
  fundamental on C, 81 to 86 ms on c0 and 38 ms on c1, and the fundamental
  takes 246 to 277 ms from −20 to −6 dB on C (Heers). Here the second
  comes after the fundamental, 8 ms on c1, 17 on c0 and longer on C, where
  the fundamental takes 108 ms
  ([report](Docs/research/physical-gaps-2026-11-18.md),
  `Tools/harmonic_onsets.py`). A foot that fills 25 times slower does not
  change it, nor does a jet whose gain at small swings is independent of
  the wind, as the jet-drive theory has it: the octave is in phase on its
  own only between 0.3 and 0.4 of the wind, and there its loop gain reaches
  1.14, where the recorded octave's growth (14 dB in 36 ms on C) asks about
  2. The jet's response across frequency and speed would have to be derived
  anew, and every flue family voiced again with it.
- **The stopped pipes match recorded ones only in part.** The Gedackt's and
  the Quintadena's twelfths stand within 4 and 5 dB of recorded ones, and
  the Quintadena's seventeenth; but the Gedackt's octave is 10 dB weak and
  its fifteenth 5 to 12 dB strong, the Quintadena's octave 13 to 16 dB weak
  and its seventh up to 24 dB strong in the bass, and the Rohrflöte's
  seventeenth is as recorded only on c0: 20 dB strong an octave below and 9
  to 21 dB weak above.
- **The flutes and strings are voiced to estimates.** Neither recorded organ
  has an open flute or a string measured beside these; their harmonics meet
  typical values within 6 and 5 dB.
- **The breath leads the tone by tens of milliseconds, not a hundred.** A
  recorded pipe hisses 13 to 23 dB under its steady level some 100 ms
  before its tone reaches a tenth of its own (Ilmenau). Here the foot fills
  as Castellengo measured a pallet's "short" attack (2004), and the breath
  stands 7 to 12 dB under its steady level in the 40 ms before the tone;
  100 ms ahead only the bass breathes (at 62 and 123 Hz on a Principal 16',
  measured at Voicing 0.3), not 247 or 494 Hz. Castellengo's "slow"
  attack would give the recorded lead and make every note speak some 100 ms
  late; how fast the recorded organ's pallets open is not known, and the owner chose the short attack for ordinary strokes. A reed's boot
  fills through the opening pallet at its key's speed. And no short note has been compared with a
  recording: the one set with known key times holds only notes of 4 s
  ([benchmark](Docs/research/benchmark-ilmenau-2026-11-13.md)).
- **No mouth tones.** Below the pressure at which it speaks, a flue's jet
  flaps across the lip at its mouth's own edge tone, and where that meets
  one of the pipe's resonances it sounds, kilohertz above the note, before
  the tone (Castellengo, 1999). Here the jet answers only near the note: its
  deflection passes a low-pass designed there. A jet driven by the mouth's
  whole flow, tried on the bench, made the pipe speak twice as fast and
  made no mouth tone. It needs the same new
  derivation of the jet's response across frequency and speed as the
  octave's lead ([report](Docs/research/physical-gaps-2026-11-18.md)).
- **The swell box does not ring.** A box's air is a resonator that its
  shutters' opening tunes, and its panels resonate and leak bass of their
  own (Pykett); here the box is only what it lets through, averaged over
  several boxes, and how much it absorbs within is an estimate.
- **Nothing is fitted to an organ.** Scales, voicing, winding, the church,
  the divisions' places, the pipes' hearing and the noises are typical
  values and estimates, stated as such beside the code; the swell box
  follows measurements averaged over several boxes, not one organ's. Two
  organs have been compared (the benchmarks above); the flue pipes' noise
  spectrum was set by them, and the trumpets' flow bounded by them.
- **The seat moves across the nave only.** Along it, Room mixes the organ
  and the church and the reflections stay those of a seat 18 m away. From a
  side seat the far ear has the organ later and weaker at every frequency
  alike: there is no head's shadow that grows with the pitch.
- **The church's first echoes are too sparse.** Two measured churches
  (Ilmenau, Heers) are as dense as noise 23 to 73 ms after the direct sound,
  this one only after 112 to 116 ms: each early reflection here is one image
  source, one click, where a church scatters each off its pews, columns and
  mouldings ([report](Docs/research/physical-gaps-2026-11-18.md),
  `NaveResponse`, `Tools/room_measures.py`). Two smaller differences: the
  air takes the treble at a rate, but Decay Treble takes it as a share of
  Decay, so a church set shorter keeps a long one's treble until Decay
  Treble is raised; and Room loads as a seat near the organ, as clear as
  Ilmenau's 8.8 m seat, where the measured seats 15 to 17 m away hear the
  organ 11 to 12 dB weaker against the church.
- **Four listening sets remain open.** The jet-drive flues, default voicing,
  tremulant and swell box use the documented candidate settings. The owner
  has selected the breath lead, tongue-led reeds and gentler cone-reed voicing.
  See the [listening decisions](Docs/decisions.md).

## Build

CMake 3.22 or newer and a C++20 compiler; the engine itself is C++17.

```sh
cmake -S . -B build-dsp -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-dsp --parallel 2
ctest --test-dir build-dsp --output-on-failure
```

`build-dsp/RenderOregana <registration> <phrase.mid> <out.wav> [id=value ...]`
plays a MIDI file through one registration with any control changed,
`RenderOregana --levels` reads each registration's level,
`build-dsp/ProbePipe` sounds one pipe alone, and `build-dsp/NaveResponse`
renders the church's answer to a click, which `Tools/room_measures.py`
measures beside a measured room's.

## Licensing

Oregana is source-available under the PolyForm Noncommercial License 1.0.0
with an additional permission for official builds; see [LICENSE](LICENSE) and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
