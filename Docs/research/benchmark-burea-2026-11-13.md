# Oregana against a recorded organ: Bureå church

The first comparison of Oregana's pipes with recordings of real ones
(2026-11-13, engine at `claude/project-thread-utg2o6` after 0.4.0, default
controls). It measures; it changes nothing in the engine.

## The reference

The Bureå church sample set by Lars Palo, one recording for each pipe, made in
the church (44.1 kHz stereo): <https://familjenpalo.se/vpo/download/>, licensed
Creative Commons Attribution-ShareAlike 2.5 Sweden. It is a three-manual
neo-baroque organ (Huvudverk, Svällverk, Bröstverk and Pedal), near Oregana's
German core. The set is fetched for the measurement and is not in this
repository; nothing of it is redistributed here, only the figures below.

Three of its Huvudverk stops are compared with their nearest in Oregana:
Principal 8' with the Hauptwerk's Octave 8', Gedakt 8' with the Positiv's
Gedackt 8', Trumpet 8' with the Hauptwerk's Trompete 8'.

## The protocol

`Tools/benchmark_pipes.py` has it at its head. In short: notes 36 to 96 one at
a time, Oregana with Room 0, Action 0 and Wind Noise 0; 1.6 s of steady speech
of each; medians over each octave. The recordings hold the church, so the
attack includes the room's build-up, and each note's harmonics carry the
room's response: an octave's median evens that out, a single note's figure
does not. One organ, one microphone position: what holds across the octaves
is a finding, a single cell is not.

```sh
python3 Tools/benchmark_pipes.py midi hw.mid 0
build-dsp/RenderOregana "Gedackt 8" hw.mid principal.wav --tail 3 pos_gedackt_8=0 hw_octave_8=1 room=0 action=0 blower=0
python3 Tools/benchmark_pipes.py measure <decoded>/HVPrincipal8 recorded.json
python3 Tools/benchmark_pipes.py measure principal.wav oregana.json
python3 Tools/benchmark_pipes.py table recorded.json oregana.json
```

The set's files are WavPack under a `.wav` name; `ffmpeg -i in.wav -c:a
pcm_s16le out.wav` decodes one.

## Results

Harmonics are in dB against the first. "Outside the harmonics" is the share of
the power from 150 Hz to 10 kHz that is not within 1.2 % (or 8 Hz) of a
harmonic. "Level spread" is the standard deviation of the notes' levels in
the octave.

### Principal 8' against the Octave 8'

Recorded: a1 441.9 Hz, tuning scatter 0.7 cents (largest 3.2). Oregana: 440.0 Hz, 0.9 cents (largest 2.2).

| Notes | | Attack ms | Outside the harmonics dB | Level spread dB | H2 | H3 | H4 | H5 | H6 | H7 | H8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 36-47 | recorded | 276 | -34 | 3.2 | -2 | -10 | -14 | -25 | -29 | -31 | -33 |
| 36-47 | Oregana | 182 | -49 | 0.5 | -11 | -3 | -9 | -12 | -16 | -26 | -25 |
| 48-59 | recorded | 83 | -34 | 1.4 | +7 | -7 | -11 | -21 | -25 | -26 | -27 |
| 48-59 | Oregana | 81 | -49 | 0.4 | -11 | -5 | -13 | -18 | -22 | -35 | -35 |
| 60-71 | recorded | 127 | -38 | 3.7 | +4 | -6 | -12 | -22 | -22 | -24 | -26 |
| 60-71 | Oregana | 42 | -47 | 0.4 | -10 | -8 | -17 | -27 | -31 | -49 | -43 |
| 72-83 | recorded | 135 | -38 | 2.8 | -2 | -18 | -22 | -27 | -37 | -34 | -37 |
| 72-83 | Oregana | 24 | -47 | 0.6 | -11 | -13 | -22 | -38 | -40 | -46 | -34 |
| 84-96 | recorded | 182 | -43 | 4.1 | -12 | -28 | -36 | -35 | -40 | -43 | -45 |
| 84-96 | Oregana | 14 | -48 | 0.5 | -13 | -18 | -29 | -49 | -35 | -39 | -32 |

### Gedakt 8' against the Gedackt 8'

Recorded: a1 441.8 Hz, tuning scatter 0.5 cents (largest 1.5). Oregana: 440.0 Hz, 0.9 cents (largest 2.1).

| Notes | | Attack ms | Outside the harmonics dB | Level spread dB | H2 | H3 | H4 | H5 | H6 | H7 | H8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 36-47 | recorded | 183 | -26 | 5.3 | -59 | -10 | -40 | -21 | -45 | -62 | -57 |
| 36-47 | Oregana | 159 | -37 | 0.7 | -66 | -11 | -51 | -34 | -49 | -55 | -50 |
| 48-59 | recorded | 132 | -30 | 4.7 | -41 | -18 | -38 | -40 | -39 | -54 | -43 |
| 48-59 | Oregana | 89 | -46 | 0.4 | -64 | -16 | -47 | -40 | -44 | -65 | -46 |
| 60-71 | recorded | 128 | -34 | 3.6 | -36 | -15 | -35 | -36 | -39 | -46 | -46 |
| 60-71 | Oregana | 52 | -46 | 0.3 | -59 | -21 | -43 | -46 | -37 | -64 | -57 |
| 72-83 | recorded | 112 | -30 | 3.2 | -34 | -20 | -36 | -45 | -38 | -42 | -40 |
| 72-83 | Oregana | 32 | -45 | 0.3 | -54 | -26 | -39 | -56 | -44 | -54 | -67 |
| 84-96 | recorded | 124 | -34 | 4.2 | -30 | -29 | -39 | -45 | -44 | -56 | -66 |
| 84-96 | Oregana | 18 | -45 | 0.6 | -50 | -31 | -33 | -52 | -57 | -51 | -64 |

### Trumpet 8' against the Trompete 8'

Recorded: a1 441.9 Hz, tuning scatter 0.4 cents (largest 1.8). Oregana: 440.1 Hz, 2.1 cents (largest 6.0).

| Notes | | Attack ms | Outside the harmonics dB | Level spread dB | H2 | H3 | H4 | H5 | H6 | H7 | H8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 36-47 | recorded | 90 | -29 | 1.7 | +4 | +1 | +2 | +0 | +1 | +4 | +3 |
| 36-47 | Oregana | 84 | -45 | 0.9 | -3 | -7 | -12 | -16 | -20 | -22 | -24 |
| 48-59 | recorded | 124 | -29 | 2.0 | -1 | -1 | -4 | -3 | +1 | +2 | -1 |
| 48-59 | Oregana | 40 | -46 | 0.5 | -4 | -8 | -12 | -17 | -21 | -24 | -25 |
| 60-71 | recorded | 195 | -36 | 1.9 | -6 | -1 | -1 | -6 | -4 | -9 | -15 |
| 60-71 | Oregana | 24 | -45 | 0.4 | -4 | -8 | -13 | -19 | -23 | -27 | -27 |
| 72-83 | recorded | 101 | -38 | 2.4 | -2 | -4 | -8 | -10 | -15 | -20 | -25 |
| 72-83 | Oregana | 15 | -41 | 0.6 | -4 | -9 | -14 | -20 | -26 | -30 | -28 |
| 84-96 | recorded | 70 | -31 | 6.0 | +5 | -2 | -6 | -10 | -20 | -30 | -36 |
| 84-96 | Oregana | 12 | -39 | 0.5 | -5 | -12 | -18 | -21 | -23 | -27 | -30 |

### The action and the wind

Measured on the raw files, so only as good as the set's own levelling:

| | Recorded | Oregana at its defaults |
| --- | --- | --- |
| A manual key's knock, loudest 10 ms, against one Principal 8' pipe (keys 48 to 72, median) | -18 dB (-27 to -3) | -17 dB (-23 to -16), Action 0.25 |
| The blower, against one Principal 8' pipe | -36 dB (the set's motor recording) | -25 dB, Wind Noise 0.2 (hum and the chests' hiss together) |

## What it says

1. **The trumpet was far too dull.** The recorded Trumpet's first eight
   harmonics are level within a few dB up to c1 and fall only above;
   Oregana's fell 4 dB a harmonic everywhere, so its eighth was 20 to 27 dB
   weaker than the recorded one's in the bass and tenor. This was the largest
   difference found, and it was the voicing after all: see The trumpet again,
   below.
2. **The principal's second harmonic is about 10 dB too weak.** Recorded, the
   second harmonic stands 8 to 16 dB above the third in every octave; in
   Oregana it is below the third in the bass and level with it in the
   middle. A larger lip offset raises it only 3 dB before the pipe stops
   speaking (at 1.6 times the present offset no pipe from C to c4 speaks;
   at 1.4 times a pipe takes half as long again to speak). So this too is the
   model's (a jet that swings symmetrically about a small offset), not a
   voicing choice, and nothing was changed. A gentler jet does the same as a
   larger offset: at c0, a swing of 4 jet half-widths for 5.5 gives 3 dB more
   of the second harmonic and takes a third longer to speak (127 ms to half
   for 96), and a swing of 2.5 gives 5 dB and takes three times as long.
3. **The Gedackt is close.** Odd harmonics lead in both, the third within
   6 dB in every octave. The recorded pipes have more of the even harmonics
   (the second at -30 to -40 dB against -50 to -65 dB), as real stopped
   pipes do.
4. **Real pipes are noisier.** 9 to 16 dB more of the recorded power lies
   outside the harmonics for the flues, and as much for the trumpet below
   c2. How the set's recordings were cleaned is not known, so this is a lower
   bound on the difference if anything.
5. **Real ranks are less even.** The recorded notes of an octave differ by 1.4
   to 5 dB (standard deviation), Oregana's by 0.3 to 0.9 dB at its default
   Voicing of 0.3. Part of the recorded spread is the room.
6. **The key knock is where the recording has it** at the default Action:
   17 against 18 dB under one Principal pipe. The recorded knocks vary far
   more from key to key.
7. **The tuning is as tight.** A freshly tuned organ's notes scatter 0.4 to
   0.7 cents about their neighbours; Oregana's default Tuning of 0.25 gives
   0.9 cents for flues and 2.1 for reeds, a season after tuning as its
   comment says.
8. **Attacks cannot be compared from this set**: the recorded ones are 70 to
   280 ms with the church's build-up in them, Oregana's 12 to 180 ms without.
   Rendering Oregana through its church at the microphone's distance would
   be the comparison, and the microphone's distance is not known.

The principal's harmonic, the noise and the evenness are Known gaps in the
README.

## The trumpet again

The first reading of finding 1 was that the tongue's motion was wrong, since
more `flow` brightened the pipe only with the shallot's pressure swinging to
four times the boot's. A published simulation of a reed pipe that agrees with
its authors' measurements (Rucz, Preukschat, Angster, Augusztinovicz and
Miklós, Simulation of the Sound Generation of Lingual Organ Pipes, DAGA 2014,
figure 3) shows both things the model already did: a tongue that swings
almost sinusoidally and touches the shallot briefly, and a shallot pressure
that sits near the boot's while the tongue is open and falls to about three
times that below zero each time it shuts. So the motion was right and the
opening was too small. An estimate from typical sizes (`Stops.h`, beside the
trumpets) gives a flow of about 8 in the bass where the voicing had 2.

`ProbePipe reed 65.4`, the trumpet's voicing, flow alone changed:

| Flow | H2 to H8 against the first, dB | Shallot's lowest pressure, boots | Speaks (half) |
| --- | --- | --- | --- |
| 2 | -3 -7 -11 -16 -19 -21 -24 | -2.6 | 72 ms |
| 4 | -2 -4 -7 -10 -11 -9 -10 | -3.8 | 72 ms |
| 6 | -1 -3 -5 -7 -8 -4 -4 | -4.4 | 80 ms |
| 8 | -1 -2 -4 -6 -6 -1 -1 | -4.8 | 132 ms |

The pitch moves by under 1 cent across the table. The trumpets now have 6 in
the bass falling to 1.6 at the top (it was 2 to 1.2), the Posaunes 5 and the
Trompette 6.5, and every pipe was regulated again. The same measurement
after the change:

| Notes | | H2 | H3 | H4 | H5 | H6 | H7 | H8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 36-47 | recorded | +4 | +1 | +2 | +0 | +1 | +4 | +3 |
| 36-47 | Oregana | -1 | -3 | -5 | -8 | -8 | -4 | -5 |
| 48-59 | recorded | -1 | -1 | -4 | -3 | +1 | +2 | -1 |
| 48-59 | Oregana | -2 | -4 | -6 | -9 | -10 | -7 | -9 |
| 60-71 | recorded | -6 | -1 | -1 | -6 | -4 | -9 | -15 |
| 60-71 | Oregana | -2 | -4 | -7 | -10 | -11 | -10 | -13 |
| 72-83 | recorded | -2 | -4 | -8 | -10 | -15 | -20 | -25 |
| 72-83 | Oregana | -2 | -5 | -8 | -11 | -14 | -14 | -19 |
| 84-96 | recorded | +5 | -2 | -6 | -10 | -20 | -30 | -36 |
| 84-96 | Oregana | -4 | -8 | -14 | -15 | -14 | -16 | -26 |

The flow is an estimate chosen on the model's own grounds and checked against
one recorded stop, not fitted to it; whether it is the better trumpet is for
the ear (listening set 4, 2026-11-13, not yet judged). The tables above this
section are from before the change.
