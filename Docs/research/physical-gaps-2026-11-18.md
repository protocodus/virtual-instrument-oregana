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

## Housekeeping and open decisions

- **The breath-lead set was never judged.** `oregana/listening-0.5.0/pallet/`
  had A (the foot fills in 2.6 ms) against slower feet. It is no longer in
  the project files. The project rule says the candidate wins, but there were
  two candidates, so it needs a re-render and a choice. It would be best to
  render it beside the jet-drive A/B, since both change the attack.
- **Two README lines are stale.** The Known gap "The new sound has not been
  listened to" predates the 2026-11-14 verdicts on the pipes' hearing, the
  places and the mechanism. `benchmark-burea-2026-11-13.md` still says that
  listening set 4 is not yet judged.
- **The Rack pin is current in sound.** Canonical `main` is 8 commits past
  the pin `3df6b7f`, but they are tests, CI and docs only.
- **The per-harmonic tracker** is `Tools/harmonic_onsets.py`, with its
  protocol at its head.

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
- Oregana's own notes: `Docs/research/known-gaps-2026-11-13.md`,
  `benchmark-ilmenau-2026-11-13.md`, `benchmark-burea-2026-11-13.md`,
  `Docs/decisions.md`
