# Decisions by ear

What was put to the ear, what each letter was, which the owner chose and what
that licensed. A measurement that settles a question is quoted where the
question is, not here.

## The breath's lead, the voicing, the tremulant and the swell box (2026-11-18, 0.11.0)

Four questions [the gaps report](research/physical-gaps-2026-11-18.md) left
to the ear, each a set in `oregana/listening-2026-11-18-gaps/` in the
project's files. Each plays the same MIDI through the same presets, its
letters matched on whole-file RMS with the louder trimmed down (at most
−3.17 dB, where a swell box resting part open made B louder).

| Set | A (0.10.0) | Candidates | Chosen |
| --- | --- | --- | --- |
| `3-breath-lead`: Principal 8 struck hard, ordinarily and softly, detached, in a run and a chord; a hymn; the Gedackt; the Plenum | a flue's foot fills through its pallet in 1.2 ms, five times faster than the fastest measured | B 8.6 ms and C 20.4 ms, Castellengo's measured impulsive and short attacks (2004) | not yet judged: **C** taken |
| `4-voicing`: Principal 8 in a scale and a hymn; the Plenum | Voicing loads at 0.3 | B: at 0.6 | not yet judged: **B** taken |
| `5-tremulant`: Krummhorn, Vox humana, Voix celeste, Full Swell | the tremulant a fixed dip on its division's pressure at exactly the Rate | B: a motor on the trunk that spills wind once a beat, the trunk ringing as the reservoir does | not yet judged: **B** taken |
| `7-swell-box`: Full Swell's pedal opened and shut; Voix celeste; Church Evensong; Hautbois shut | two bands faded evenly in decibels | B: C. Pykett's measured wooden boxes, louvres opening as the sine of their angle | not yet judged: **B** taken |

Each is taken by the standing rule that the candidate wins unless the owner
says otherwise; of the breath's two candidates, C is the one recommended to
the owner. Licensed until then:

- **C, the breath's lead.** The pallet's share of a flue's fill is
  Castellengo's short attack, so an ordinary stroke breathes before its tone
  and a hard one is the impulsive attack. The times are measured, on one tin
  pipe's chest; which of them an ordinary stroke is was left to the ear. C
  is recommended because its breath is the nearer a recorded organ's: it
  alone breathes 100 ms before the tone, in the bass. The pallet's share
  counts only while the pallet opens, which was found after the set's first
  rendering; B and C were rendered again with it.
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
