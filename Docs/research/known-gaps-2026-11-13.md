# The known gaps, researched (2026-11-13)

What the literature and the bench say about each gap the README lists in the
sound, what was changed and what stays. One thing was changed: each flue's own
unrest. Typical values and estimates throughout; the two recorded organs are
the Ilmenau and Bureå benchmarks beside this document.

## A held pipe is not steady (changed)

Measured with `Tools/benchmark_steadiness.py` (its head has the protocol) on
the Ilmenau recording's held notes of 4 s, microphone 3, the fundamental
alone:

| | Pitch, cents RMS | Level, dB RMS |
| --- | --- | --- |
| Recorded, Principal 16' and Doppelflöte 8', eight notes | 0.45 to 1.2, mean 0.78 | 0.06 to 0.18, mean 0.14 |
| Oregana before, Principal 16' and Rohrflöte 8' | 0.2 to 0.3 (the flute to 0.69) | 0.01 to 0.05 |
| Oregana now | 0.42 to 0.93, mean 0.72 | 0.05 to 0.12, mean 0.09 |

- The recorded wander is even from 0.2 to 10 Hz and falls above.
- Cents stand to dB as about 7 to 1, as they do here for a change of the
  foot's pressure (0.55 cents and 0.08 dB for 1 %).
- In the take with two stops on one key (key 59), the flute's pitch track and
  the principal's are unrelated (correlation 0.04), where one pipe's own
  harmonics follow each other (0.83). So the wander is each pipe's own and
  not the division's wind. A jet's velocity is known to fluctuate by itself
  (Verge et al., JASA 95, 1119, 1994), and a laboratory organ's foot pressure
  stood 6 Pa in 160 about its mean over 10 s (Abel, Bergweiler and
  Gerhard-Multhaupt, arXiv physics/0506094).

The engine already had a slow unrest of the division's pressure (0.3 % RMS
near 1 Hz), which every pipe of the division shares. Now each flue's wind also
passes through its own noise under 8 Hz, 1.5 % RMS at Wind 0.5
(`kWaverDepth` in `OreganaEngine.cpp`), seeded by the pipe's place, so a
render repeats. The level's wander is still a third under the recorded one;
part of the recorded figure is the room's. `Oregana.Engine`'s `FluesWaver`
holds it.

### The reeds (added the same day, at the owner's word)

The Bureå set has one recording for each pipe, so its Trumpet 8' can be
measured the same way (the fundamental of each note from 36 to 59, medians by
octave; above that the trumpet's fundamental is too weak in the church for
the measurement):

| | Pitch, cents RMS | Level, dB RMS |
| --- | --- | --- |
| Recorded Principal 8', notes 36 to 71 | 0.38 to 0.69 | 0.08 to 0.11 |
| Recorded Gedakt 8', notes 36 to 71 | 0.97 to 1.22 | 0.21 to 0.33 |
| Recorded Trumpet 8', notes 36 to 59 | 0.77 to 0.88 | 0.10 to 0.18 |
| Oregana Trompete 8' before | 0.05 to 0.09 | 0.01 to 0.02 |
| Oregana Trompete 8' now | 0.41 to 0.78 | 0.02 to 0.04 |
| Oregana Vox humana now | 1.0 to 1.9 | 0.03 |
| Oregana Krummhorn now | 1.5 to 4.8 | 0.03 to 0.05 |

A second organ's flues agree with the first, and its reed walks as much as
its flues, mostly between 2 and 12 Hz. Respira's reeds gave the mechanism
(its `ReedModel.h` and `kTurbulence`): the jet through the gap is turbulent,
and its eddies push the tongue as they change the flow, so the tongue's phase
walks. Here the noise that already moved a reed's flow now also moves the
boot's push on its tongue, by a quarter of itself at Wind 0.5 (`kReedEddy`,
14 % RMS, where a jet's turbulence is 10 to 20 % of its speed). A push taken
as a share of the drop across the gap, which is nearer the physics, made the
tongue's own feedback random and threw the Krummhorn with every control at
its most, so it is the boot's pressure alone. The Krummhorn and the Vox
humana walk further on the same push because their resonators hold their
tongues less; no recording has checked them. The level's wander stays under
the recorded one's. `ReedsWaver` holds it, and on the bench (`ProbePipe`,
`PipeRegulation.h`) the push is 0 and a reed sounds sample for sample as
before.

Listening sets (A without, B with) are in the project files,
`oregana/listening-0.5.0/unrest/` for the flues and `unrest-reeds/` for the
reeds, not committed.

## The breath does not lead the tone (cause found, not changed)

Castellengo (ISMA 2004, "Flute-like instrument transients: an analytical
study of mouth-tone production versus pressure rise time") measured three
attacks of one organ pipe: the foot reached its pressure in 160, 45 and 26 ms,
and below a threshold pressure the pipe gives only mouth tones and noise,
which "precede the onset" of the pipe's tone. The lead is the time the foot
spends under that threshold.

Here a foot fills in 0.0012 + 0.35 / Hz seconds (2.6 ms at 247 Hz). On the
bench (`ProbePipe`, the principal's voicing), with that time multiplied:

| Foot fill at 247 Hz | Breath at -20 dB | Tone at -20 dB | Tone -20 to -1 dB |
| --- | --- | --- | --- |
| 2.6 ms (now) | 0 ms | 25 ms | 55 ms |
| 10 ms | 0 ms | 55 ms | 60 ms |
| 26 ms | 25 ms | 80 ms | 55 ms |
| 52 ms | 30 ms | 130 ms | 60 ms |

So the model has the mechanism: a slower foot makes the breath lead by 55 to
100 ms, and the tone's own growth, which matches the recording, does not
change. What changes is how late every flue speaks, 30 to 100 ms, and the
recording's key times carry an unknown offset, so it cannot say which is
right. That is a choice for the ear: `oregana/listening-0.5.0/pallet/` has A
(now), B and C. Nothing was changed.

## A principal's second harmonic is 10 dB weak (stays)

Fletcher and Douglas (JASA 68, 767, 1980) give the jet's flow into the pipe as
U = V0 {1 + tanh[(y0 + a sin wt) / b]}: with no offset y0 of the jet from the
lip the even harmonics vanish, and with a swing a/b near 2, as in normal
speech, the second harmonic of that flow is greatest about 10 dB under the
first, near y0/b of 1 to 1.5. Their own 330 Hz pipe radiated a second harmonic
7 dB under the first. Fletcher (1999) has the third harmonic gone at y0 =
1.1 b and the fourth at 1.7 b.

The principal here is voiced at offset 1.2 and gives -10 dB: the theory's
maximum for this mechanism. On the bench an offset of 2 or more does not
speak (the small-signal slope sech² y0 is too low to start), and drive 3 with
offset 1.5 gives H2 -9 dB, H3 -25 dB and takes 1 s to speak. The mouth's end
correction also leaves the second resonance 23 cents sharp at 247 Hz, some
4 dB off the harmonic. The recorded principals' -5 to +5 dB is therefore
beyond a jet of this kind: it needs a source this model does not have (Pykett
describes the principal's flow as short asymmetric pulses, and a jet thrown
outside the pipe at the onset that takes fifty cycles to settle). No
defensible second mechanism was found, so nothing was changed.

## The stopped pipes are too perfect (stays)

A stopped pipe's even harmonics fall on its antiresonances. A source at the
mouth that radiates directly (the jet's own flow, a monopole) would put them
some 50 to 60 dB under the fundamental, by a rough estimate of that path; the recorded Quintadena has them at -35 dB and the model at
-70 dB. Nothing in the literature read gives a figure for a real stopped
pipe's even harmonics that would ground a constant, so none was drawn.

## The reeds

- **Why a tongue-led reed follows the wind.** The tongue rests at
  y = 1 - closing × (P - p), near the shallot's face, and the time it spends
  shut each cycle moves with that rest: about 9 cents for 1 % of wind in the
  tongue-led voicings. Real reeds do rise with pressure, linearly, and blown
  pitch stands well above the plucked tongue's (Plitnik and Knox 1995;
  Miklós, Angster and others 2003), but no figure in cents per pascal was
  found to set the model by. Rucz, Angster and Miklós (DAGA 2022) treat
  free tongues that blow open, where a tongue tuned 1 to 2 semitones under
  the resonator gives a pitch that does not follow the pressure: not a
  beating reed. The Vox humana's 2.3 cents for 1 % stays.
- **Krummhorn.** Pykett's measured Krummhorn (C3) has its even harmonics
  suppressed "very nicely", as the model's; the Bureå one has every harmonic
  strong. The stop differs between builders, so the gap is one recorded
  organ's and is reworded so.
- **Posaune 16'.** Pykett's Trombone 16' (C2) has its fundamental some 20 dB
  under the second harmonic, and notes that a half-length cone does not
  strengthen the fundamental. On the bench at 41 Hz, with the resonator's
  first resonance at 1.5 and 2 times the note (two thirds and half length),
  the fundamental falls 8 and 14 dB against the rest; at full length, flow 8
  for 5 gives 2 to 5 dB more upper harmonics and takes 390 ms to speak for
  145. Whether the Bureå Basun's resonators are short is not known, and the
  full-length Posaune of the German organs this stop list follows has a
  strong fundamental. Not changed; a shorter resonator is a candidate for a
  listening test should the owner want that Posaune.
- **Trumpets' bass.** More flow closes the 8 dB and slows the speech (as
  above); no new evidence.

## Smaller gaps

- **Bass decay under Decay Bass.** The church's bass shelf is first order at
  250 Hz, so at 80 Hz it has not reached its full depth (1.9 for 2, 0.59 for
  0.5). A second-order shelf in each of the delay lines would close it for
  CPU the engine does not have to spare; left.
- **Ranks too even.** The recorded spread of 1.4 to 5 dB within an octave
  holds the room's; the Voicing control reaches 3 dB. No recording separates
  the two, so the default stays.
- **Coupler lag under a held key**, **the seat along the nave**: design work,
  not research; untouched.

## Sources

- N. H. Fletcher and L. M. Douglas, "Harmonic generation in organ pipes,
  recorders and flutes", JASA 68, 767 (1980).
- N. H. Fletcher, "The nonlinear physics of musical instruments", Rep. Prog.
  Phys. 62, 723 (1999).
- M. P. Verge and others, "Jet formation and jet velocity fluctuations in a
  flue organ pipe", JASA 95, 1119 (1994).
- M. Castellengo, "Flute-like instrument transients", ISMA 2004, Nara.
- M. Abel, S. Bergweiler and R. Gerhard-Multhaupt, "Synchronization of organ
  pipes", arXiv physics/0506094.
- P. Rucz, J. Angster and A. Miklós, "Physics-based model of the sound
  generation by dynamic lingual organ pipes", DAGA 2022.
- G. R. Plitnik and A. Knox (1995) and A. Miklós, J. Angster and others
  (2003) on lingual pipes, from their abstracts.
- C. Pykett's pages on how flue and reed pipes speak, pykett.org.uk.
