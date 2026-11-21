# Decisions by ear

What was put to the ear, what each letter was, which the owner chose and what
that licensed. A measurement that settles a question is quoted where the
question is, not here.

## Low-wind speech, harmonic flutes, reeds and wind-off tremulant (2026-11-22, 0.12.4)

Objective measurements settled six physical-model defects on `0.12.3` (`3e0a4ed`);
a 7-pair A/B listening set (`A` = shipping `0.12.3`, `B` = candidate `0.12.4`)
was rendered to verify each fix by ear before accepting `0.12.4`:

| Set | Material & What Was Heard | Matching & Trims | Outcome (`A` -> `B`) |
| --- | --- | --- | --- |
| `01-harmonic-flute-low-wind` | `Flute harmonique` B3 -> C4..C6 at `pressure=0.6`, `blower=0` | Unmatched (`A` `-56.7`, `B` `-48.1` dBFS) | C4–C6 RMS `-65.5 -> -46.8` dBFS; **B** chosen |
| `02-harmonic-flute-timbre` | `Flute harmonique` B3–C4–C#4–D4 attacks + G#6..D#7 chromatic run | Whole-file RMS `-40.24` dBFS (`A` `+0.00`, `B` `-0.33` dB) | C4 onset `-45.1 -> -41.9` dBFS, smooth upper harmonics; **B** chosen |
| `03-treble-and-bass-low-wind` | `Principal 16'` (C1, E1, C2) + `Siffloete 1'` (6.6–7.0 kHz) at `pressure=0.6`, `blower=0` | Unmatched (`A` `-47.3`, `B` `-43.2` dBFS) | 16' bass `-45.1 -> -39.7` dBFS, 1' treble `-61.4 -> -54.4` dBFS; **B** chosen |
| `04-stopped-treble-pitch-bend` | `Rohrfloete 4'` + `Principal 4'` D7/F#7/G#7 under ±10 °C pitch bend | Whole-file RMS `-46.52` dBFS (`A` `-0.48`, `B` `+0.00` dB) | Unbent `Rohrfloete` beating removed; ±29.5c unison lock; **B** chosen |
| `05-vox-humana-treble` | `Vox humana 8'` treble G5..C7 without tremulant | Whole-file RMS `-43.45` dBFS (`A` `+0.00`, `B` `-0.08` dB) | C7 formant mistuning `+73.8c -> 0.0c`, Q `6.2 -> 15`; **B** chosen |
| `06-half-drawn-harmonic-flutes` | Half-drawn (`0.5`) `Flute harmonique 8'` + `Flute octaviante 4'` + `Octavin 2'` C3..C6 | Unmatched (`A` `-49.8`, `B` `-43.0` dBFS) | C4–C6 RMS `-56.7 -> -41.8` dBFS; **B** chosen |
| `07-tremulant-wind-off` | `Krummhorn` + Positiv tremulant, `wind_supply=0` at `1.0 s` | Opening 0–1 s RMS `-28.51` dBFS (`A` `+0.00`, `B` `-0.01` dB) | Dead-organ tail `-63.6` (pk `-44.9`) -> `-134.9` dBFS (pk `-121.6`); **B** chosen |

The owner confirmed **B** by ear (`2026-11-22`). All six fixes are licensed by the
underlying physical measurements and confirmed audibly by the A/B set.

## Reed brightness (2026-11-19, 0.12.1)

In the promo's full choruses 0.12.0's reeds stood 1.5 to 4 dB brighter above
2 kHz than 0.11.0's, with the flues unchanged, and the owner heard the tutti
as harsh. Whether to voice the cone reeds gentler, a set in
`oregana/listening-2026-11-19-reed-brightness/` in the project's files: the
promo's France Grand choeur finale and Spain Batalla, the same MIDI and
changes through each letter, matched on whole-file RMS with the louder
trimmed down (at most 0.42 dB), each letter's pipe regulation regenerated.

| Letter | Every cone reed's flow (`Stops.h`) | Chosen |
| --- | --- | --- |
| A (0.12.0) | as voiced: the trumpet's 8' C 3.5 | |
| B | 0.8 of it: 2.8 | |
| C | 0.6 of it: 2.1, near 0.4.0's 2, the bottom of the comment's range | **C**, the owner's choice (2026-11-19) |

Licensed by that choice: the flows of the Trumpet, Posaune, Trompette and
Hautbois families at 0.6 of 0.12.0's. Against 0.11.0, 0.12.1 stands
within 0.5 dB at 2 to 4 kHz and 1 to 2 dB brighter above 4 kHz; the bass of
the chorus stays about 2 dB under 0.11.0's, which came with the tongue-led
reeds, not with the flow. A trial that curved the tongues 3 and 8 times as
much moved those bands by under 0.5 dB and was not put to the ear.

C put a fault in 0.12.0's reeds beyond `Oregana.Engine`'s 1.5 cents: a reed
whose stop opened under a held key sounded 2 cents flat of one whose key
went down after, because its boot filled at the opening pallet's rate for as
long as it spoke. The owner chose to fix the boot with C (2026-11-19): it
fills at the open pallet's rate once the pallet stands open, and both now
sound within 0.05 cents of the tuner's.

## The reeds led by their tongues (2026-11-19, 0.12.0)

Whether the tongue-led reeds are an improvement, a set in
`oregana/listening-2026-11-19-reeds/` in the project's files. Each plays the
same MIDI through the same preset, its letters matched on whole-file RMS with
the louder trimmed down.

| Set | A (0.11.0) | B (0.12.0) | Trim | Chosen |
| --- | --- | --- | --- | --- |
| `1-trompete`: the Trompete's demo | the resonator leads the reed, which speaks in 7 to 71 ms | the tongue leads, rolls onto its shallot and speaks in 34 to 149 ms | B −0.89 dB | **B**, the owner's choice (2026-11-19) |
| `2-krummhorn`: the Krummhorn's demo | as above, its second harmonic 27 dB under the first | as above, the second harmonic level with the first | A −0.13 dB | **B**, the owner's choice (2026-11-19) |
| `3-tutti`: the Tutti's demo | every reed as in 0.11.0 | every reed as in 0.12.0 | A −0.19 dB | **B**, the owner's choice (2026-11-19) |

The owner said to go with the candidate (2026-11-19), for all three sets.
Licensed by that choice:

- **B, the reeds of 0.12.0.** The measurements chose the mechanism, not the
  ear: a recorded trumpet speaks in 90 to 195 ms and flattens as its wind
  rises, as B does and A does not. What is left to the ear is the voicing
  that came with it. The trumpets are brighter than the ones the owner chose
  by ear for 0.8.0 (the harmonics 2 to 8 of C stand 4 to 9 dB nearer the
  recorded ones), and the closings, tongue pitches and resonator lengths in
  `Stops.h` are voiced, not measured.

Not put to the ear, because a measurement or the limit settled them: the
branch's kick at a reed's onset (two constants no source gives), its second
harmonic for the flues (measured further from the recordings) and a stopped
pipe's sound straight from its mouth (it needs a coefficient fitted to the
recordings, and the bass Gedackt moves away from them).

## The breath's lead, the voicing, the tremulant and the swell box (2026-11-18, 0.11.0)

Four questions [the gaps report](research/physical-gaps-2026-11-18.md) left
to the ear, each a set in `oregana/listening-2026-11-18-gaps/` in the
project's files. Each plays the same MIDI through the same presets, its
letters matched on whole-file RMS with the louder trimmed down (at most
−3.17 dB, where a swell box resting part open made B louder).

| Set | A (0.10.0) | Candidates | Chosen |
| --- | --- | --- | --- |
| `3-breath-lead`: Principal 8 struck hard, ordinarily and softly, detached, in a run and a chord; a hymn; the Gedackt; the Plenum | a flue's foot fills through its pallet in 1.2 ms, five times faster than the fastest measured | B 8.6 ms and C 20.4 ms, Castellengo's measured impulsive and short attacks (2004) | **C**, by the owner's ear (2026-11-18) |
| `4-voicing`: Principal 8 in a scale and a hymn; the Plenum | Voicing loads at 0.3 | B: at 0.6 | not yet judged: **B** taken |
| `5-tremulant`: Krummhorn, Vox humana, Voix celeste, Full Swell | the tremulant a fixed dip on its division's pressure at exactly the Rate | B: a motor on the trunk that spills wind once a beat, the trunk ringing as the reservoir does | not yet judged: **B** taken |
| `7-swell-box`: Full Swell's pedal opened and shut; Voix celeste; Church Evensong; Hautbois shut | two bands faded evenly in decibels | B: C. Pykett's measured wooden boxes, louvres opening as the sine of their angle | not yet judged: **B** taken |

The owner chose the breath's lead by ear. The other three are taken by the
standing rule that the candidate wins unless the owner says otherwise.
Licensed, the breath's lead by that choice and the rest until the owner
judges them:

- **C, the breath's lead, chosen by ear.** The pallet's share of a flue's
  fill is Castellengo's short attack, so an ordinary stroke breathes before
  its tone and a hard one is the impulsive attack. The times are measured,
  on one tin pipe's chest; which of them an ordinary stroke is, the owner
  chose by ear. C was also the one recommended, its breath the nearer a
  recorded organ's (it alone breathes 100 ms before the tone, in the bass),
  but that comparison does not settle it: how fast the recorded organ's
  pallets open is not known. The pallet's share counts only while the
  pallet opens, which was found after the set's first rendering; B and C
  were rendered again with it.
- **B, Voicing 0.6.** Each pipe stands up to 1.8 dB off its rank's level as
  loaded, where recorded octaves differ by 1.4 to 5 dB, part of it the
  room's. Chapel Italian (0.4), Chapel Village (0.5) and Chapel Derelict
  (0.8) keep their own, so the first two now load more even than the
  default.
- **B, the spilled-wind tremulant.** Its motor, valve and trunk are voiced
  to written accounts of such winding, not measured.
- **B, Pykett's swell box.** Several boxes averaged, not one organ's; the
  air in the box and its panels do not resonate. Registrations resting the
  pedal part open are brighter and louder for it, and every preset's Volume
  trim was read again.

## Flue pipes driven by their jets' change (2026-11-18, 0.10.0)

The jet-drive source is the standard model of a flue pipe and was adopted on
its physics and on three recorded organs' harmonics
([the report](research/physical-gaps-2026-11-18.md)); the ear judges whether
the organ is the better for it. The set
(`oregana/listening-2026-11-18-gaps/1-flue-pipes/` in the project's files)
plays four README demo phrases, each through its preset as loaded: Principal
8, Gedackt 8, Voix celeste and Plenum. Matched on whole-file RMS, the louder
letter trimmed down (at most −0.43 dB).

| Set | A (0.9.0) | B (0.10.0) | Chosen |
| --- | --- | --- | --- |
| `1-principal`, `2-gedackt`, `3-strings`, `4-plenum` | the jet drives the pipe by its flow | by its flow's rate of change, every flue family voiced again, the lip shedding more | not yet judged: **B** taken, by the standing rule that the candidate wins unless the owner says otherwise |

Licensed, until the owner says otherwise: the jet-drive source and the
voicing 0.10.0 ships. The voicing is set by measurement where the recorded
organs have the stop (principals, Gedackt, Quintadena, Rohrflöte) and is an
estimate where they have not (flutes, strings).

## Similar stops set apart (2026-11-14, 0.8.0)

Three pairs of stops sounded too much alike to tell apart in a registration.
Each set (`oregana/listening-2026-11-14-similar-stops/` in the project's
files) played c1 to g1 and a C major chord on the Hauptwerk, then on the
Récit with the box open; every candidate's pipes were regulated again and the
files matched on whole-file RMS (trims at most +0.13 dB). The owner listened
on the published listening page.

| Set | A (0.7.0) | B | C | Chosen |
| --- | --- | --- | --- | --- |
| Trompete 8' / Trompette 8' | as shipped | Trompette: resonator 4 halftones narrower, tongue closer (closing 0.5 to 0.7) | B, and the Trompette's treble on more wind (top flow 2.0 to 3.0) | **C** |
| Viola da Gamba 8' / Gambe 8' | as shipped | Viola 4 halftones wider, Gambe and Voix céleste 4 narrower | Viola 6 wider, Gambe and Voix céleste 5 narrower | **C** |
| Mixtur V / Plein jeu IV | as shipped | Plein jeu progressive (1' from c0, 2/3' from c1) and 2 wider, Mixtur 2 narrower | Plein jeu progressive and 3 wider, Mixtur 3 narrower | **B** |

Licensed: each direction is a historical voicing (Silbermann's broad gamba
against Cavaillé-Coll's keen one, a progressive French Plein jeu against a
sharp German Mixtur, Cavaillé-Coll's higher treble pressure), and the ear
chose how far. The amounts are voiced, not measured. The Trompette's treble
was applied together with the trumpets' brightness below: its top flow is
half as much again as the trumpets' law then gives (1.75 to 2.6), the ratio
C applied (2.0 to 3.0). The Clairon shares the Trompette's family and takes
the same tongue.

## The open sets of 0.5.0 and 0.2.0 (verdicts 2026-11-14)

These had waited since 2026-11-13 (`oregana/listening-0.5.0/`) and
2026-11-12 (`oregana/listening-0.2.0/church-A-B/`). In each, the last letter
was what the engine already played. The owner listened on the published
listening page.

| Set | The question | Letters | Chosen | What it licensed |
| --- | --- | --- | --- | --- |
| `1-mechanism-film`, `1-mechanism-close` | The added noises of the mechanism (coupled pallets, pedals, shutters, the tremulant's valve, sliders, the blower's run-up) | A 0.4.0, B with them | **B**, both | keep them as shipped |
| `2-hearing-film`, `2-hearing-close` | Neighbouring flue pipes hear each other and lock when nearly in tune | A 0.4.0, B with it | **B**, both | keep it as shipped |
| `3-places-church` | Each division reflected from its own place in the church | A 0.4.0, B set 2's B, C B and the places | **C** | keep it as shipped |
| `4-trumpet-solo`, `4-trumpet-toccata` | How bright the trumpets are (the flow under the open tongue) | A 0.4.0's flow, B half way, C 6 in the bass from typical sizes | **B**, both | changed in 0.8.0 to B |
| `church-A-B` (0.2.0) | 0.1.0's church against 0.2.0's | A 0.1.0, B 0.2.0 | **B** | keep it as shipped |

The trumpets' B has no derivation of its own: the key said so, "a degree for
the ear". It lies between two values that each have one (0.4.0's, and the
typical sizes that give 6 to 8), so it is read as a voicer's choice of how
far the tongue is curved within that range, chosen by ear. In 0.8.0 the
Trompete is 3.5 to 1.4, the Posaune 3.3 to 1.5 and the Trompette 3.8 in the
bass. On the voicer's bench (`ProbePipe`) the Trompete 8's eighth harmonic at
C stands 13 dB under the first, against 4 dB at 6 and 24 dB at 0.4.0's 2.
