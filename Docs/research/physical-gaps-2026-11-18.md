# Oregana: physical-model gaps, ranked

2026-11-18, against 0.9.0. The gaps it ranks are worked on from 0.10.0
(README, Version history).

## Summary

The biggest gap is in the flue pipes' speech and tone, not the wind or the
room: Oregana's principals are too "odd" in steady tone and miss the real
pipe's octave-first attack. Three recorded organs now agree on both, so these
two should come first.

1. **Principal tone is too odd.** Every recorded principal has its 2nd
   harmonic louder than its 3rd, by 4 to 18 dB. Oregana's has the 3rd louder,
   by 1 to 8 dB. This colours every chorus, mixture and plenum. An off-tree
   prototype of the *jet-drive* source (the standard modern flue model)
   brings the 2nd harmonic to −1 to −4 dB, where the recordings have it.
2. **The octave does not speak first.** On the new Heers recordings, a
   Montre 8' pipe's 2nd harmonic reaches −6 dB 18 to 225 ms before the
   fundamental, the longest in the bass. Oregana's leads by only 2 to 14 ms.
   A slower foot alone does not produce it.
3. **There are no mouth-tone forerunners.** Oregana's chiff can only ring the
   pipe's own resonances. Real pipes start with short inharmonic edge tones
   that rise in pitch with the wind.
4. **The tremulant is a fixed sine on the pressure.** A real tremulant's rate
   and depth come from the wind flowing through its valve, so they change
   with the registration.
5. Smaller gaps: ranks too even, a fixed two-band swell box, no audio-rate
   wind coupling on the chest, and a church never checked against a measured
   impulse response.

Nothing in the engine was changed. The prototypes ran on a scratch copy
outside the repository. Each fix that changes the sound is a candidate for an
A–Z listening test, with A as the shipping engine.

## What was compared

The engine is canonical Oregana at `main` (`2f149ec`), the 0.9.0 sound that
the Rack device pins at `3df6b7f`. The audit read `FluePipe.h`, `ReedPipe.h`,
`Wind.h`, `Nave.h`, `Stops.h` and `OreganaEngine.cpp`, along with the repo's
own research notes, so that already-known gaps are not counted twice.

Three recorded organs:

| Organ | Material | Used before |
| --- | --- | --- |
| Bureå (Sweden), neo-baroque | one sample per pipe, in the church | yes (`benchmark-burea`) |
| St. Jakobus, Ilmenau | Orgamat-played notes, mic inside the case, plus RIRs | yes (`benchmark-ilmenau`) |
| [Sint-Martinus, Heers](https://doi.org/10.5281/zenodo.22707773) (Belgium) | Orgamat-played C notes held 4 s, 9 mics, CC BY | **new here** |

From Heers, the Montre 8' and the Trompette 8' were measured at the
*UpperCenter* mic in front of the case. Each was compared with the Hauptwerk
Octave 8' and the Récit Trompette 8', rendered by `RenderOregana` with the
same notes (C2 to C6, 4 s each), Room 0, Action 0 and Wind Noise 0. Each
harmonic was tracked on its own (heterodyned, then low-passed;
`Tools/harmonic_onsets.py`). That shows when each harmonic speaks, not only
when the whole note does. Two takes of each recorded note agree within a few
ms. The Heers organ stands near a1 = 450 Hz.

The Heers recordings are fetched for measurement only, and nothing from them
is committed or redistributed.

## New measurements

**The octave speaks first in a real principal, and much earlier in the
bass.** Each figure below is how long before the fundamental the 2nd
harmonic reaches −6 dB of its steady level. Heers shows two takes of each
note.

| Note | Heers Montre 8' (ms) | Oregana Octave 8' (ms) |
| --- | --- | --- |
| C2 | 199 / 225 | 14 |
| C3 | 81 / 86 | 13 |
| C4 | 38 / 38 | 8 |
| C5 | 17 / 18 | 3 |
| C6 | +14 / −24 (too noisy) | 2 |

The fundamental itself also rises more slowly in the real bass. From −20 to
−6 dB it takes 246 to 277 ms at C2, against Oregana's 131 ms. From C4 up the
two agree within a few ms. Castellengo describes the same thing: the
transient "burst" of an open pipe is its 2nd harmonic.

**The 2nd harmonic is louder than the 3rd in every recorded principal and
quieter in Oregana's.** Each figure is H2 minus H3 in dB, steady speech.

| Organ and stop | Bass | Tenor | Middle | Treble |
| --- | --- | --- | --- | --- |
| Heers Montre 8' (C2, C3, C4, C5) | +14 | −6 | +7 | +18 |
| Ilmenau Principal 16' (62, 123, 247, 494 Hz) | +5 | +4 | +13 | +9 |
| Bureå Principal 8' (octaves from C2) | +8 | +14 | +10 | +16 |
| Oregana Octave 8' (C2 to C5) | −8 | −7 | −4 | +1 |

The known gap was "H2 is 10 dB weak". It is better stated as an odd/even
balance about 12 dB off, because Oregana's H3 is also 3 to 10 dB strong on
all three organs. A near-symmetric jet makes odd harmonics, and that is the
model's signature.

**A third organ confirms that the trumpets are dull, and that their speech is
right.** At C3 the Heers Trompette's H2 to H8 lie between +3 and −14 dB of H1
(most within ±4). At C4 they lie between 0 and −10 dB. Oregana's Récit
Trompette has −2 to −11 dB at C3 and −2 to −13 dB at C4, so it is 5 to 8 dB
duller from H3 up. In both, every harmonic reaches −6 dB within about 20 ms
of the others, so the reed's attack has no gap. The brightness is the owner's
choice by ear (0.8.0, `Docs/decisions.md`), and this is evidence for that
choice, not a reason to reverse it.

## Ranked gaps and proposed fixes

The ranking is by audible impact on ordinary registrations, then by how well
the evidence holds. *New* means the gap is not in the README's Known gaps.

| # | Gap | Evidence | Proposed fix | Effort |
| --- | --- | --- | --- | --- |
| 1 | Principal tone too odd: H2 under H3 (known, now sharper) | 3 organs: H2−H3 is +4 to +18 dB recorded, −8 to +1 dB here | Replace the flow-injection jet source with the jet-drive source, Δp ∝ dQ_jet/dt (Verge 1997, Fabre 2000, Auvray 2012), with its phase taken into the jet transit. Then revoice principals (lower swing, larger lip offset), regenerate `PipeRegulation.h`, and A/B it | Large: every flue family is revoiced |
| 2 | Octave does not speak first (**new**) | Heers: H2 leads the fundamental by 199 to 225 ms at C2 and 38 ms at C4; Oregana leads by 14 and 8 ms. A foot that fills 4× or 10× slower still leads by only 15 to 34 ms | Research first: the 2nd mode needs its own transient gain. In the model the jet's one-pole low-pass is designed at the note and starves 2f. Test regime competition under the jet-drive source while the foot rises, with Castellengo's 2004 rise times. Measure with this report's per-harmonic tracker | Medium: after #1, since the two share the jet |
| 3 | No mouth-tone (edge-tone) forerunners (**new**, beside the known breath-lead gap) | Castellengo 1999: inharmonic mouth tones, a few kHz and rising with pressure, precede the pipe tone. Oregana's chiff is a puff into the loop, so it can only ring the pipe's own harmonics | An edge-tone path: the free jet's own feedback from lip to flue, with frequency ∝ jet speed / cut-up, driven by the foot pressure and coupled into the loop. It is a jet oscillation, so "every sound is a pipe taking wind" still holds | Medium |
| 4 | Ranks too even (known) | Recorded octave spread 1.4 to 5 dB; Oregana under 1 dB at Voicing 0.3 | A listening test of Voicing 0.3 against 0.6 (two defensible pinnings, since part of the recorded spread is the room) | Small |
| 5 | Tremulant is a fixed raised-cosine dip (**new**) | Pykett and Dom Bédos: a retained-wind tremulant's rate and depth depend on the wind flowing through it, fade under a heavy registration, and vary from beat to beat | Model the valve as a sprung flap in the trunk inside the `Reservoir` equations, so the beat and its depth come from the flow. Keep Rate and Depth as the spring and the weight | Medium |
| 6 | Wind reaches the pipes at 1.5 kHz, as DC demand (**new**) | `Wind.h` steps every 32 samples, and demand is `air·√foot`. A sounding bass pipe's pulsation on the chest cannot modulate its neighbours (the "shake" of a 16' or 32' under full organ) | An audio-rate channel pressure per key, fed by each pipe's AC flow. Do this only if a listening test of full organ asks for it, since it costs CPU the engine does not have | Large, CPU |
| 7 | Swell box is a fixed two-band shelf at 500 Hz (**new**) | Pykett: a box is a Helmholtz resonator tuned by the shutters' opening and lets little low bass out when shut | A box resonance whose frequency moves with the shutter opening, plus a short reverberation inside the box | Small to medium |
| 8 | Church never checked against a measured impulse response (**new**) | Only T20 per octave has been compared (Ilmenau). Echo density, early reflections and C80 have not | A tool that renders the `Nave` impulse response and compares it with the Ilmenau RIR and the Heers SRIRs (both CC BY). Report first, change later | Small |
| 9 | Stopped pipes too perfect, Posaune fundamental, Krummhorn (known) | As in the README | No new evidence: Bourdon 8' from Heers failed to download (HTTP 504) and can be retried | n/a |

Two things were checked and need nothing: the reed's attack (every harmonic
speaks within about 20 ms of the others, as recorded), and tuning and
temperament (mutations and mixtures are pure to their key, as a tuner sets
them).

## Off-tree experiments

The jet-drive source is the most promising lead, but it is not ready to ship.
All runs used `ProbePipe` on a scratch copy of `FluePipe.h` with the
principal family's voicing, at steady wind. The repository's research notes
said that no second mechanism for the principal's H2 had been found, so three
were tried.

| Experiment | Result |
| --- | --- |
| Jet-drive source, shipping voicing (swing 5, offset 1.15) | H2 rises only 1 to 7 dB. H3 and everything above H8 come up 10 to 30 dB, which is far too bright. Unusable as it stands |
| Jet-drive source, revoiced (swing 2, offset 1.1) | H2−H3 is +3, 0 and 0 dB at C2, C3 and C4 (shipping: −8, −7, −4), with H2 at −1 to −4 dB as recorded. The fundamental reaches −6 dB 424, 219 and 140 ms after the tone starts: about right at C2 (Heers about 270), too slow from C3 up (Heers about 110 and 55). The octave still does not lead |
| Jet-drive, offsets from 1.7 up | Unstable: the pitch jumps 30 to 80 cents or the pipe overblows. The usable range is narrow, so it needs a proper derivation and not a sweep |
| A jet whose gain grows with frequency (Rayleigh growth, 2f/f = 1.5 to 2) | H2 *falls* 4 to 11 dB. This is ruled out as the H2 fix |
| A slower foot (fill time ×4 and ×10) | The octave's lead grows only to 15 to 34 ms, against 200 ms recorded at C2. A slower foot alone does not explain it |

Read together: the steady odd/even balance comes from the source's form, and
jet-drive fixes about half of it with gentler voicing. The octave-first
attack needs a second-mode transient that neither source produces yet. Port
jet-drive properly first, with the vortex-shedding loss re-derived for it as
Fabre and Auvray do. Then put A (shipping) against B (jet-drive) in front of
the ear on a principal chorus and a bass line.

## The octave's lead under 0.10.0's jet

Gap 2 was taken up again once the jet-drive source shipped (0.10.0), on
`ProbePipe` with the principal's bass voicing (q 35, swing 2.3, offset 1.2,
shedding 4) at C, c0, c1 and c2, measured by `Tools/harmonic_onsets.py`.
The octave's lead is H1's −6 dB time less H2's; negative means it follows.

| Jet's strength against the foot's pressure p | Foot fills | Octave's lead, ms (C, c0, c1, c2) |
| --- | --- | --- |
| As shipped: the source ∝ p | ×1, ×4 | −31, −16, −8, −4 and −30, −15, −7, −4 |
| ∝ √p | ×1, ×4 | −31, −16, −8, −4 and −30, −15, −7, −4 |
| Independent of p | ×1, ×4 | −34, −20, −10, −5 and −31, −18, −7, −7 |
| Jet-drive's own law: swing ∝ 1/√p at the lip, strength ∝ √p (unchanged at nominal wind) | ×1, ×4, ×10, ×25 | −32 to −43 on C, −15 to −18 on c0, −7 to −14 on c1 |

The octave follows by about two of the fundamental's periods at every
pitch, in every case: it is the jet's nonlinearity working on a growing
fundamental, never an oscillation of its own. The open loop at C shows why
(small signal, jet-drive's own law):

| Wind against nominal | 0.25 | 0.3 | 0.36 | 0.42 | 0.5 | 0.6 | 1.0 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| \|G\| at the fundamental | 0.77 | 0.91 | 1.03 | 1.10 | 1.17 | 1.22 | 1.26 |
| \|G\| at the octave, where in phase | (none) | 1.10 | 1.14 | 1.11 | 1.03 | (none) | (none) |

The octave is in phase only in the jet's second hydrodynamic mode, between
about 0.3 and 0.4 of the wind, and there it gains at most 14 % a round trip.
The recorded octave on C grows 14 dB in 36 ms, about 2.4 round trips, which
asks a loop gain near 2, while the recorded fundamental's 277 ms rise asks
only about 1.1. Under the shipping law (strength ∝ p) the octave's best is
0.99. No foot is slow enough to grow it from 1.14 in a window that narrow.

So the octave-first attack needs a jet whose amplification rises with
frequency at low speed (its growth along the cut-up goes as ωτ, so the
second hydrodynamic mode is amplified far more than the first) and is held
back at full wind by the jet's thickness. That is a new derivation of the
jet's response across frequency and speed, and every flue family would be
voiced again with it. The Known gap stays.

## The groove's pressure, measured (gap 6)

Gap 6 was built on a scratch copy of the engine at the tick rate: each
key's channel (groove) took its pressure every tick from the flow its
speaking pipes drew, by the engine's own pallet law (3 % down with every
stop drawn, going with the square of the flow), instead of from the stops
drawn. The flues drew their jets' flow, and the reeds the
flow through their toe holes, which pulses with the tongue (the boot's
mass balance in `ReedPipe`). On a held chord (c0, c1, e1, g1 and c2, with
the Pedal's C), the Hauptwerk's c1 channel:

| Registration | Steady pressure: dynamic (static) | Its swing, RMS | Strongest |
| --- | --- | --- | --- |
| Tutti | 0.9731 (0.9731) | 0.053 % | 131 Hz, the 16' reed on that key |
| Grand jeu | 0.9953 (0.9953) | 0.023 % | 131 Hz |
| Trompete | 0.9994 (0.9994) | 0.002 % | 262 Hz |

The swing is 66 dB under the wind or more, before a pipe's foot filters
it, so the reeds' pulsing cannot be heard in the flues' wind through the
pallet, and the steady pressure is the static law's to four places. It
cost no time that could be measured (the Tutti demo rendered in 1.5 to
1.6 s with and without it). Nothing changed that the ear could take, so it
was not shipped. Two things could make a groove heard and neither has a
source here: the groove's own resonance as a duct (its dimensions) and the
feet filling at once at a chord's attack (their volumes).

## Mouth tones and the breath's lead (gap 3)

**Mouth tones.** Castellengo traces a flue pipe's inharmonic forerunners to
the jet's own edge tone. Below the pressure at which the pipe speaks, the
jet flaps across the lip at the frequency its mouth alone would give, and
where that meets one of the pipe's resonances it is heard as a mouth tone:
for a pipe of average scale the strongest stood at 2300, 3000 and 3800 Hz on
255, 441 and 657 Pa, each the mouth's own edge tone at that pressure (1999).
Here the jet's deflection passes a one-pole low-pass designed at the note,
its corner some 360 Hz at c1, so nothing near an edge tone reaches the lip.
On `ProbePipe` with the principal's voicing, the jet was driven by the
mouth's whole flow, the wave going out as well as the one coming back. The
pipe spoke about twice as fast (90 % of its level in 161 against 324 ms at
C, 40 against 90 ms at c1, 12 against 25 ms at c3), 0.7 to 11 cents flat and
within 0.4 dB, and made no mouth tone. A mouth tone needs the jet to answer
far above the note, with its own feedback from lip to flue at the jet's
speed: the same new derivation of the jet's response across frequency and
speed that gap 2 asks for. Nothing was changed, and the two gaps go
together.

**The breath's lead.** Here a foot fills in `0.0012 + 0.35 / Hz` seconds,
the pallet's share and the toe's. Castellengo (2004) opened one tin pipe's
pallet (475 Hz) at chosen rates and recorded its foot: it crossed the
pressure at which the pipe speaks, 125 of 360 Pa, 4, 9 and 40 ms after the
pallet began to open ("impulsive", "short" and "slow"), and reached its
pressure in 26, 45 and 160 ms. The model's foot crosses that threshold at
0.43 of its time, so at 475 Hz the pallet's share would be 8.6, 20.4 and
92 ms where it is 1.2 ms: the shipping foot is five times faster than the
fastest measured. A slower pallet gives the pipe a breath before its tone,
as recorded ones have:

| Pallet's share | Principal 8' at velocity 80, fundamental at -6 dB (C, c0, c1, b1, c3) | Breath 10 to 50 ms before the tone |
| --- | --- | --- |
| 1.2 ms (shipping) | 130, 74, 44, 30, 22 ms | none |
| 8.6 ms (impulsive) | 155, 116, 70, 49, 31 ms | -10 to -13 dB |
| 20.4 ms (short) | 209, 122, 90, 63, 42 ms | -6 to -11 dB |

At Ilmenau the breath stands 13 to 23 dB under its steady level 100 ms
before the tone reaches a tenth of its own. On Ilmenau's keys of a
Principal 16', only the short pallet has any breath that early, at 123 and
62 Hz (-16 and -10 dB); none has it at 247 or 494 Hz. Castellengo's slow
pallet would give it everywhere, with every note some 100 ms later, and how
fast the Orgamat opens a pallet is not known. Which pallet is an ordinary
stroke is a choice for the ear: `oregana/listening-2026-11-18-gaps/3-breath-lead/`
has A (shipping), B (impulsive) and C (short), flues only (a reed's boot
takes no account of the pallet's speed, here or before).

The pallet's share is the pallet's opening, and counts only while it opens:
once a foot is within 2 % of its channel's pressure it follows the channel
through its toe alone, and a pipe that a stop's slider brings on under a
held key fills through its toe from the start. Given to every rise of the
wind, the 20.4 ms share bunched the crescendo roller's stops into a step of
3.1 dB in 25 ms (`CrescendoIsSmooth` holds it under 3; with the share as
it now counts, 2.2 as 0.11.0 ships, against 1.9 with 0.10.0's foot) and
held the Plenum half a decibel low, its feet slow to follow their wind's
bounce back. B and C were rendered again with it; the table above is theirs.

## The church against measured rooms (gap 8)

`Tools/NaveResponse` renders the church's answer to a click from the
Hauptwerk's place as the seat hears it: the organ's own sound and the
church's, mixed by Room as the engine mixes them. `Tools/room_measures.py`
measures it, or a measured response, by one protocol (at its head): the
decay by octave, C80, D50 and the echo density (Abel and Huang). The
measured responses are Ilmenau's (Part 1: the loudspeaker in front of the
organ, microphone 8 hung in the nave 8.8 m from it and microphone 9 between
the pews 14.5 m from it) and Heers' (Part 1's fine grid: the loudspeaker
under the façade's mouths, the array's omnidirectional microphone 16.8 m
from it on the axis and 18.5 m to one side). Oregana's seat is 18 m from
the organ. Both sets are fetched for the measurement and never committed.

**The decay, T20 by octave (s):**

| | 63 | 125 | 250 | 500 | 1k | 2k | 4k | 8k |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Ilmenau, 14.5 m | 1.73 | 1.81 | 2.04 | 2.23 | 2.26 | 2.10 | 1.70 | 1.04 |
| Heers, 16.8 m | 3.12 | 3.18 | 2.59 | 2.39 | 2.04 | 1.95 | 1.63 | 1.18 |
| Oregana as loaded (Decay 4.5, Bass 1.3, Treble 0.2) | 5.48 | 5.31 | 5.11 | 4.75 | 4.26 | 3.69 | 2.71 | 1.62 |
| Oregana, Decay 2.2, Bass 0.75, Treble 0.35 | 1.68 | 1.71 | 1.84 | 2.13 | 2.08 | 1.99 | 1.67 | 1.19 |

Set to Ilmenau's decay, the church follows it within 0.2 s in every octave.
Heers rings 1.4 times its middle in the bass, a stone church, as Oregana
loads (1.2 to 1.3); Ilmenau, galleried and wooden, 0.8. The treble is
where the two kinds of decay part. The air takes it at a rate (ISO 9613-1,
20 °C and 50 to 70 % humidity), so a church of 4.5 s keeps 0.57 to 0.63 of
its decay at 4 kHz and 0.27 to 0.33 at 8 kHz, and one of 2.2 s 0.73 to 0.77
and 0.43 to 0.51. As loaded, Oregana's keeps 0.60 and 0.36, and both
measured churches 0.73 to 0.76 and 0.46 to 0.53: each is the air's. But
Oregana takes the treble as a share of Decay, so a shorter church keeps the
long one's 0.60 and 0.36 until Decay Treble is raised (0.35 gives 0.79 and
0.56). Taking the air's loss as a rate would leave Decay Treble for the
surfaces alone.

**The organ against the church** (C80 and D50 the mean of 500 Hz and
1 kHz; the direct sound the energy within 2.5 ms of the peak):

| | Direct against the rest | C80 | D50 |
| --- | --- | --- | --- |
| Ilmenau, 8.8 m | -1.2 dB | +0.1 dB | 42 % |
| Ilmenau, 14.5 m | -5.9 dB | -0.8 dB | 35 % |
| Heers, 16.8 m | -7.6 dB | -2.3 dB | 27 % |
| Heers, 18.5 m to one side | -16.6 dB | -3.5 dB | 17 % |
| Oregana as loaded (Room 0.6, Decay 4.5) | +4.7 dB | +0.2 dB | 45 % |
| Oregana, Room 0.9, Decay 4.5 | -3.1 dB | -3.6 dB | 22 % |
| Oregana, Decay 2.2, Room 0.6 | +6.8 dB | +4.4 dB | 66 % |
| Oregana, Decay 2.2, Room 0.8 | +2.5 dB | +1.8 dB | 49 % |
| Oregana, Decay 2.2, Room 1.0 (the church alone) | none | -0.7 dB | 31 % |

As loaded, Oregana's seat is as clear as Ilmenau's at 8.8 m: Room loads as
a seat near the organ, and the measured seats 15 to 17 m away hear the
organ's own sound 11 to 12 dB weaker against the church. In the loaded
church, Room 0.8 to 0.9 reaches them. In a church set to the measured ones'
decay it cannot: there the church alone is as clear as Ilmenau's 14.5 m
seat, and nothing reaches Heers'. A shorter Decay weakens Oregana's field
but not its first reflections, so a short church is heard mostly in its
first 80 ms. By Barron's revised theory a church's own sound, without the
direct sound, has a C80 of -1.8 dB at 2.2 s, against Oregana's -0.7 dB: a
small part of the difference. The rest is where Room puts the seat, which
is the player's. The loudspeakers here are more directional than an
organ's case, which would only lower the measured figures further.

**The echo density** (1 is as dense as noise; the mixing time is when it
stays at 0.9 or more):

| | 5 ms | 10 ms | 20 ms | 50 ms | 100 ms | Mixing time |
| --- | --- | --- | --- | --- | --- | --- |
| Ilmenau, 8.8 m | 0.21 | 0.42 | 0.64 | 0.93 | 1.01 | 73 ms |
| Ilmenau, 14.5 m | 0.27 | 0.40 | 0.54 | 0.98 | 1.09 | 38 ms |
| Heers, 16.8 m | 0.25 | 0.38 | 0.84 | 1.01 | 0.95 | 23 ms |
| Heers, 18.5 m to one side | 0.64 | 0.79 | 0.73 | 0.96 | 1.00 | 39 ms |
| Oregana as loaded | 0.02 | 0.09 | 0.11 | 0.46 | 0.84 | 116 ms |
| Oregana, Decay 2.2, Room 1.0 | 0.07 | 0.12 | 0.10 | 0.53 | 0.84 | 112 ms |

This is the largest difference. Both churches are as dense as noise 23 to
73 ms after the direct sound; Oregana's only after 112 to 116 ms, and at
20 ms it stands at 0.1 against 0.5 to 0.8. Its first reflections are single
image sources, each one click, where a church scatters each off its pews,
columns and mouldings into a cluster, and its reverberant field needs
100 ms to fill. The fix to try first is an early field that scatters, each
image source spread into a short dense cluster, measured with this tool
until it fills as these churches do, and then put to the ear on staccato
chords. Nothing in the engine was changed.

## Housekeeping and open decisions

- **The breath-lead set was never judged.** `oregana/listening-0.5.0/pallet/`
  had A (the foot fills in 2.6 ms) against slower feet. It is no longer in
  the project files. The project rule says the candidate wins, but there were
  two candidates, so it needs a re-render and a choice. Done:
  `3-breath-lead`, above.
- **Two README lines are stale.** The Known gap "The new sound has not been
  listened to" predates the 2026-11-14 verdicts on the pipes' hearing, the
  places and the mechanism. `benchmark-burea-2026-11-13.md` still says that
  listening set 4 is not yet judged.
- **The Rack pin is current in sound.** Canonical `main` is 8 commits past
  the pin `3df6b7f`, but they are tests, CI and docs only.
- **The per-harmonic tracker** is `Tools/harmonic_onsets.py`, with its
  protocol at its head.

## Where each gap stands (0.11.0)

| # | Gap | Where it stands |
| --- | --- | --- |
| 1 | Principal tone too odd | 0.10.0: the jet-drive source, every flue family voiced again |
| 2 | The octave does not speak first | Open: the jet's response across frequency and speed must be derived anew (above) |
| 3 | No mouth tones; the breath's lead | Mouth tones open, with gap 2. 0.11.0: the pallet's share is Castellengo's short attack (`3-breath-lead`: C, the owner's choice by ear) |
| 4 | Ranks too even | 0.11.0: Voicing loads at 0.6 (`4-voicing`, not yet judged) |
| 5 | The tremulant a fixed dip | 0.11.0: a motor on the trunk that spills wind (`5-tremulant`, not yet judged) |
| 6 | Wind at the tick rate | Measured on a scratch copy, 66 dB under the wind: not shipped (above) |
| 7 | The swell box a two-band shelf | 0.11.0: Pykett's measured wooden boxes (`7-swell-box`, not yet judged); the box's own resonance is not modelled |
| 8 | The church never checked | 0.11.0's `NaveResponse` and `room_measures.py` (above): the decay follows a measured church's once set to it; the first echoes are too sparse, and nothing was changed |
| 9 | Stopped pipes, Posaune, Krummhorn | Nothing new |

Every set not yet judged ships its candidate, by the project's rule that the
candidate wins until the owner says otherwise ([decisions](../decisions.md)).

## Sources

- [MusicSphere: Sint-Martinus Church Heers, Part 2: recordings of individual
  pipe tones](https://doi.org/10.5281/zenodo.22707773) (Zenodo, 2026, CC BY),
  with its [overview](https://doi.org/10.5281/zenodo.22111389) and
  [Part 1 SRIRs](https://doi.org/10.5281/zenodo.22686848)
- [MusicSphere: St. Jakobus Ilmenau, Part 1
  RIRs](https://doi.org/10.5281/zenodo.19496982) and [Part 3 pipe
  tones](https://doi.org/10.5281/zenodo.19497225)
- M. Castellengo, [Acoustical analysis of initial transients in flute-like
  instruments](https://www.lam.jussieu.fr/Membres/Castellengo/publications/1999a1-Flute%20transients%20Analysis.pdf),
  Acustica 85 (1999), and [Transient and pressure rise
  time](https://www.lam.jussieu.fr/Membres/Castellengo/publications/2004c-Transient_Pressure%20rise%20time-ISMA.pdf),
  ISMA 2004
- C. Pykett, [Tremulant simulation in digital
  organs](https://colinpykett.org.uk/digitaltrems.htm) and [Swell boxes:
  their effect on pipe
  sounds](https://colinpykett.org.uk/swell-boxes-and-swell-pedals-part1.htm)
- Jet-drive flue source: M.-P. Verge et al., JASA 101 (1997); B. Fabre,
  A. Hirschberg and A. Wijnands, Acustica 86 (2000); R. Auvray, B. Fabre and
  P.-Y. Lagrée, JASA 131 (2012), summarised in [this
  overview](https://swang251.github.io/2020/09/30/The-Jet-Drive-Model).
  Cited from these summaries and not re-read in full here.
- Room measures: ISO 3382-1 (decay times, C80, D50); A. Lundeby et al.,
  Acustica 81 (1995), the noise's cut; J. S. Abel and P. Huang, AES 121
  (2006), the echo density; M. Barron and L.-J. Lee, JASA 84 (1988), the
  revised theory; ISO 9613-1, the air's absorption
- Oregana's own notes: `Docs/research/known-gaps-2026-11-13.md`,
  `benchmark-ilmenau-2026-11-13.md`, `benchmark-burea-2026-11-13.md`,
  `Docs/decisions.md`
