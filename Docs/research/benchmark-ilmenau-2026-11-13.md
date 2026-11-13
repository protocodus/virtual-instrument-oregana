# Oregana against a recorded organ: St. Jakobus, Ilmenau

The second comparison with recorded pipes (2026-11-13, engine at
`claude/project-thread-utg2o6` after 0.4.0), and the first whose key times
are known. It led to one change: the spectrum of the jet's turbulence.

## The reference

MusicSphere: Room and Organ Acoustic Measurements of St. Jakobus Church
Ilmenau (ROAM-JIL), Zenodo, 2026, Creative Commons Attribution 4.0:

- Part 3, recordings of individual pipe tones, <https://doi.org/10.5281/zenodo.19497225>.
  An Orgamat on the keyboard presses the keys from MIDI. Every C, B and F
  sharp of the manual is held 4 s with 4 s of silence after it, alone and
  together, on three stops and their combinations, through nine microphones:
  1 to 6 inside the organ, 7 at the key desk, 8 hung in the nave, 9 between
  the pews. The MIDI file is in each archive.
- Part 1, omnidirectional room impulse responses, <https://doi.org/10.5281/zenodo.19496982>.
- Part 4, full organ pieces on the same stops, is announced and was not yet
  published on this date, so there is no recorded short note to compare: the
  shortest is 4 s.

Used here: `Tone B.zip`, microphone 3, Register 1 (Principal 16') and
Register 4 (Doppelflöte 8') on the keys B2, B3, B4 and B5 alone (notes 0, 1,
3 and 7 of the file, 8 s apart from 0 s). They are compared with Oregana's
Hauptwerk Principal 16' and Rohrflöte 8'. Nothing of the set is in this
repository; it is fetched for the measurement. The organ stands near
a1 = 430 Hz (its B4 sounds 483 Hz).

## The protocol

`Tools/benchmark_speech.py` has it at its head.

```sh
build-dsp/RenderOregana "Gedackt 8" probe.mid principal16.wav --tail 3 pos_gedackt_8=0 hw_principal_16=1 room=0 action=0 blower=0
python3 Tools/benchmark_speech.py speech   "Mic 3 Register 1 Tone B.wav" -12 0 8 4 0:83 1:71 3:59 7:47
python3 Tools/benchmark_speech.py spectrum "Mic 3 Register 1 Tone B.wav" -12 0 8 4 0:83 1:71 3:59 7:47
python3 Tools/benchmark_speech.py speech   principal16.wav -12 1 26 4 0:83 1:71 2:59 3:47
```

The recording's first key-down is not at the MIDI file's time (its first
note is at 52 s, the sound at 0.14 s), so its times carry one unknown offset
and only differences between them are compared.

## Speaking and stopping

Milliseconds, the tone in its own band. Oregana's figures are with the change below; it moved none of them by more than 3 ms.

| Stop, sounding pitch | | -20 to -1 dB while speaking | Overshoot dB | -3 to -10 dB after key-up |
| --- | --- | --- | --- | --- |
| Principal 16', 494 Hz | recorded | 24 | 1.5 | 34 |
| | Oregana | 27 | 0 | 9 |
| 247 Hz | recorded | 20 | 3.1 | 21 |
| | Oregana | 50 | 1 | 16 |
| 123 Hz | recorded | 140 | 1.5 | 35 |
| | Oregana | 108 | 2 | 40 |
| 62 Hz | recorded | 151 | 3.0 | 168 |
| | Oregana | 187 | 2 | 82 |
| Doppelflöte 8' against Rohrflöte 8', 988 Hz | recorded | 40 | 2.6 | 39 |
| | Oregana | 31 | 0 | 5 |
| 494 Hz | recorded | 84 | 0.3 | 62 |
| | Oregana | 47 | 0 | 10 |
| 247 Hz | recorded | 44 | 0.9 | 16 |
| | Oregana | 74 | 1 | 22 |
| 123 Hz | recorded | 134 | 2.0 | 34 |
| | Oregana | 126 | 1 | 48 |

Oregana's pipes speak in the recorded pipes' time, within a factor of two
either way and with no lean. The recorded pipes overshoot a little more. What
follows key-up in the recording is the church as well as the pipe, even at a
microphone inside the organ, so the recorded figures there are upper bounds.

A note shorter than its pipe's speech stays under its level in Oregana (a
62 Hz pipe held 100 ms reaches -11 dB, a 494 Hz pipe held 30 ms -4 dB) and
rings on after the key; by the table a recorded pipe would do the same.

## The spectrum

| Stop, pitch | | H2 | H3 | H4 | H5 | Outside the harmonics dB |
| --- | --- | --- | --- | --- | --- | --- |
| Principal 16', 494 Hz | recorded | -5 | -14 | -16 | -21 | -34 |
| | Oregana before | -10 | -10 | -19 | -31 | -45 |
| | Oregana now | -10 | -10 | -19 | -31 | -36 |
| 247 Hz | recorded | +5 | -8 | -16 | -17 | -31 |
| | Oregana before | -10 | -6 | -14 | -22 | -39 |
| | Oregana now | -10 | -6 | -14 | -22 | -37 |
| 123 Hz | recorded | -2 | -6 | -11 | -12 | -32 |
| | Oregana now | -11 | -4 | -11 | -15 | -36 |
| 62 Hz | recorded | -2 | -7 | -17 | -14 | -31 |
| | Oregana now | -11 | -3 | -8 | -9 | -35 |
| Flute, 988 Hz | recorded | -2 | -30 | -26 | -27 | -27 |
| | Oregana before | -61 | -20 | -45 | -47 | -44 |
| | Oregana now | -61 | -21 | -46 | -47 | -31 |
| 494 Hz | recorded | -17 | -19 | -19 | -32 | -29 |
| | Oregana before | -57 | -12 | -52 | -48 | -45 |
| | Oregana now | -57 | -12 | -52 | -49 | -35 |

The recorded keys B2 and B3 stand only 38 to 45 dB over the microphone's
floor in their band, so their noise figures hold some of the room; B4 and B5
stand 47 to 65 dB over it. The Doppelflöte is an open flute with two mouths
and the Rohrflöte a chimney flute: their harmonics are not to be compared,
their noise is.

Where the noise lies, between the harmonics, in dB against the tone
(bands by harmonic number):

| | 0.5-1.5 | 1.5-2.5 | 2.5-3.5 | 3.5-4.5 | 4.5-6.5 | 6.5-8.5 | 8.5-12.5 | 12.5-16.5 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Principal 494 Hz, recorded | -40 | -39 | -44 | -44 | -46 | -47 | -52 | -55 |
| Oregana before | -66 | -64 | -60 | -58 | -55 | -56 | -54 | -54 |
| Oregana now | -49 | -49 | -44 | -44 | -42 | -45 | -46 | -49 |
| Flute 988 Hz, recorded | -32 | -32 | -40 | -42 | -39 | -45 | -44 | -53 |
| Oregana before | -53 | -71 | -49 | -60 | -50 | -56 | -49 | -51 |
| Oregana now | -36 | -53 | -34 | -47 | -41 | -48 | -43 | -46 |

## What it says, and what was done

1. **A real pipe's noise is strongest about its first harmonics** and falls
   15 to 20 dB by the twelfth. Oregana's turbulence was white, which left it
   25 dB short at the bottom and right at the top, and 9 to 17 dB short in
   all, as the Bureå comparison had found. The turbulence is now strongest
   between 0.15 and 0.7 times the note and falls 6 dB an octave above that
   (`kBreathLow`, `kBreathCorner` and `kBreathFloor` in `FluePipe.h`): the
   principal's noise in all is 2 to 6 dB under the recorded one's and the
   flute's 4 to 6 dB under it, where they were 8 to 17 dB under. Slower turbulence than 0.15 times the note made
   a small pipe's level wander, and in the top octave any of it threw a 7.4
   kHz pipe to the octave below, so it fades out from 4.2 to 5.9 kHz with
   that octave's gentler voicing. `PipeRegulation.h` was written again. A
   stopped pipe's noise still has a hole about its absent second harmonic,
   where the recorded open flute has none.
2. **The principal's second harmonic is 5 to 15 dB too weak here too**: two
   organs now. No voicing reaches it (lip offsets from 1.2 to 4, swings from
   3 to 5.3, Q, shedding, the jet's transit and the radiating corner all
   leave it between -5 and -13 dB, or stop the pipe). It stays a known gap:
   it needs a jet that can stand well off the lip and still start, which this
   one cannot.
3. **The breath leads the tone in the recording**: above the eighth harmonic
   it is 13 to 23 dB under its steady level 100 ms before the tone reaches a
   tenth of its own, where Oregana's pipes are silent until the tone begins.
   How fast the Orgamat presses a key is not known, so how much of the lead
   is the slow pallet and how much the pipe is not either. Nothing was
   changed.
4. **Short notes cannot be compared** until Part 4 is published.

## The church

Its reverberation time from Part 1 (the loudspeaker at the pipes, the
microphones in the nave and between the pews, T20 by octave):

| Hz | 63 | 125 | 250 | 500 | 1k | 2k | 4k | 8k |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| s | 1.4-1.9 | 1.7-1.9 | 1.9-2.0 | 2.1-2.2 | 2.2-2.3 | 2.1 | 1.7 | 1.0 |

A galleried town church: 2.2 s in the middle, shorter in the bass (0.65 to
0.85 of that under 125 Hz) and 0.74 of it at 4 kHz. Oregana reaches it with
Decay 2.2, Decay Bass 0.75 and Decay Treble near 0.35, inside the controls'
ranges. As loaded Oregana is a stone church (Decay Bass 1.3, 0.6 of the
middle at 4 kHz); one wooden-galleried church is no reason to move that.
