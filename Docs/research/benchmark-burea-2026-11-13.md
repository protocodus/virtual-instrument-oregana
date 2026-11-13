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

## More stops

Seven more ranks of the same organ against the nearest of Oregana's stops,
by the same protocol (the 16' stops measured with a4 as 220, the 4' with
880, so each key is searched at the pitch it sounds). The tables are from
after the trumpets' change. Nothing was changed to match them.

### Octave 4' (Hauptwerk) against the Octave 4'

| Notes | | Attack ms | Outside the harmonics dB | Level spread dB | H2 | H3 | H4 | H5 | H6 | H7 | H8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 36-47 | recorded | 52 | -34 | 3.4 | -1 | -9 | -13 | -23 | -24 | -30 | -27 |
| 36-47 | Oregana | 81 | -49 | 0.3 | -11 | -4 | -12 | -17 | -21 | -33 | -33 |
| 48-59 | recorded | 36 | -36 | 2.4 | +2 | -13 | -17 | -24 | -29 | -27 | -30 |
| 48-59 | Oregana | 43 | -47 | 0.3 | -10 | -7 | -16 | -25 | -29 | -45 | -43 |
| 60-71 | recorded | 43 | -38 | 3.2 | -4 | -21 | -21 | -31 | -36 | -37 | -41 |
| 60-71 | Oregana | 24 | -47 | 0.6 | -11 | -12 | -20 | -34 | -38 | -51 | -38 |
| 72-83 | recorded | 62 | -39 | 2.0 | -9 | -20 | -30 | -42 | -40 | -47 | -51 |
| 72-83 | Oregana | 14 | -47 | 0.6 | -12 | -16 | -25 | -46 | -39 | -42 | -32 |
| 84-96 | recorded | 148 | -37 | 2.7 | -16 | -25 | -42 | -41 | -53 | -56 | -62 |
| 84-96 | Oregana | 10 | -48 | 0.7 | -16 | -23 | -37 | -49 | -30 | -37 | -41 |

As the Principal 8': the recorded second harmonic is the strongest in the bass and the model's is 10 dB under it; the rest lies within a few dB up to c2.

### Rohrflöte 8' against the Rohrflöjt 8'

| Notes | | Attack ms | Outside the harmonics dB | Level spread dB | H2 | H3 | H4 | H5 | H6 | H7 | H8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 36-47 | recorded | 202 | -19 | 1.3 | -44 | -16 | -49 | -40 | -48 | -60 | -54 |
| 36-47 | Oregana | 191 | -31 | 0.5 | -50 | -19 | -46 | -37 | -59 | -36 | -61 |
| 48-59 | recorded | 137 | -23 | 5.1 | -42 | -22 | -40 | -23 | -40 | -36 | -36 |
| 48-59 | Oregana | 108 | -47 | 0.4 | -52 | -16 | -52 | -41 | -51 | -47 | -53 |
| 60-71 | recorded | 104 | -28 | 2.4 | -41 | -17 | -48 | -26 | -36 | -32 | -49 |
| 60-71 | Oregana | 66 | -46 | 0.3 | -54 | -12 | -57 | -45 | -45 | -56 | -47 |
| 72-83 | recorded | 62 | -29 | 6.4 | -38 | -22 | -36 | -22 | -37 | -38 | -51 |
| 72-83 | Oregana | 38 | -46 | 0.5 | -59 | -15 | -49 | -49 | -41 | -68 | -57 |
| 84-96 | recorded | 182 | -29 | 6.1 | -43 | -22 | -30 | -39 | -41 | -49 | -48 |
| 84-96 | Oregana | 20 | -45 | 0.4 | -58 | -24 | -44 | -45 | -47 | -60 | -72 |

Close in the third harmonic (within 6 dB). The recorded fifth is 15 to 25 dB stronger from c to c3: the chimney's own resonance, which the model's chimney gives too little of.

### Quintadena 8' against the Quintadena 8'

| Notes | | Attack ms | Outside the harmonics dB | Level spread dB | H2 | H3 | H4 | H5 | H6 | H7 | H8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 36-47 | recorded | 83 | -20 | 3.0 | -33 | -8 | -25 | -25 | -31 | -43 | -31 |
| 36-47 | Oregana | 96 | -52 | 0.5 | -73 | -5 | -70 | -8 | -59 | -20 | -50 |
| 48-59 | recorded | 47 | -28 | 3.6 | -34 | -8 | -30 | -27 | -34 | -39 | -36 |
| 48-59 | Oregana | 63 | -54 | 0.3 | -73 | -4 | -66 | -14 | -52 | -27 | -45 |
| 60-71 | recorded | 43 | -29 | 3.1 | -36 | -8 | -32 | -26 | -33 | -38 | -37 |
| 60-71 | Oregana | 39 | -52 | 0.4 | -74 | -5 | -57 | -21 | -47 | -35 | -41 |
| 72-83 | recorded | 37 | -25 | 11.7 | -35 | -12 | -35 | -28 | -34 | -40 | -42 |
| 72-83 | Oregana | 25 | -51 | 0.6 | -75 | -11 | -51 | -28 | -41 | -45 | -35 |
| 84-96 | Oregana | 14 | -52 | 0.6 | -67 | -18 | -46 | -37 | -33 | -49 | -46 |

The twelfth agrees. The model's fifth and seventh harmonics are 17 and 23 dB too strong in the bass octave, and its even harmonics are 35 to 40 dB too weak: a real stopped pipe is not a perfect one.

### Subbass 16' against the Subbas 16'

| Notes | | Attack ms | Outside the harmonics dB | Level spread dB | H2 | H3 | H4 | H5 | H6 | H7 | H8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 36-47 | recorded | 259 | -10 | 3.5 | -43 | -8 | -46 | -27 | -40 | -54 | -40 |
| 36-47 | Oregana | 324 | -26 | 0.9 | -65 | -9 | -53 | -30 | -50 | -50 | -52 |
| 48-59 | recorded | 135 | -21 | 5.4 | -51 | -13 | -40 | -40 | -42 | -61 | -60 |
| 48-59 | Oregana | 155 | -35 | 0.6 | -65 | -14 | -48 | -37 | -46 | -61 | -46 |
| 60-71 | recorded | 125 | -23 | 6.0 | -43 | -23 | -43 | -64 | -51 | -65 | -57 |
| 60-71 | Oregana | 95 | -47 | 0.3 | -61 | -18 | -45 | -42 | -41 | -68 | -51 |

Close: the third and fifth harmonics within 5 dB over the two lower octaves.

### Gamba 8' against the Salicional 8'

| Notes | | Attack ms | Outside the harmonics dB | Level spread dB | H2 | H3 | H4 | H5 | H6 | H7 | H8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 36-47 | recorded | 180 | -18 | 3.4 | -42 | -13 | -46 | -38 | -47 | -53 | -51 |
| 36-47 | Oregana | 152 | -52 | 0.7 | -13 | -3 | -8 | -6 | -8 | -12 | -12 |
| 48-59 | recorded | 162 | -29 | 3.6 | -9 | -15 | -18 | -27 | -36 | -34 | -39 |
| 48-59 | Oregana | 80 | -51 | 0.3 | -12 | -3 | -8 | -7 | -11 | -18 | -18 |
| 60-71 | recorded | 217 | -33 | 3.7 | -5 | -14 | -20 | -26 | -31 | -40 | -45 |
| 60-71 | Oregana | 46 | -50 | 0.4 | -12 | -3 | -9 | -11 | -15 | -25 | -23 |
| 72-83 | recorded | 195 | -36 | 4.1 | -8 | -26 | -28 | -41 | -40 | -42 | -50 |
| 72-83 | Oregana | 26 | -49 | 0.4 | -11 | -4 | -13 | -18 | -22 | -34 | -33 |
| 84-96 | recorded | 247 | -36 | 8.3 | -9 | -25 | -28 | -39 | -40 | -44 | -47 |
| 84-96 | Oregana | 14 | -50 | 0.6 | -10 | -9 | -17 | -28 | -31 | -50 | -47 |

Only partly comparable: a Gamba is the keener string, and the recorded bass octave is stopped pipes. Above it the model's third and higher harmonics are 10 to 20 dB stronger and its second 3 to 7 dB weaker; the recorded string takes 160 to 250 ms to speak, the model's 14 to 80 ms.

### Posaune 16' against the Basun 16'

| Notes | | Attack ms | Outside the harmonics dB | Level spread dB | H2 | H3 | H4 | H5 | H6 | H7 | H8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 36-47 | recorded | 109 | -34 | 2.1 | +10 | +13 | +12 | +18 | +15 | +22 | +12 |
| 36-47 | Oregana | 56 | -50 | 1.3 | -1 | -4 | -6 | -9 | -9 | -6 | -6 |
| 48-59 | recorded | 150 | -30 | 1.8 | +2 | +2 | -0 | +2 | +8 | +7 | +5 |
| 48-59 | Oregana | 55 | -49 | 1.0 | -2 | -4 | -7 | -9 | -11 | -8 | -10 |
| 60-71 | recorded | 135 | -36 | 1.7 | +1 | +5 | +7 | +8 | +7 | +3 | -3 |
| 60-71 | Oregana | 29 | -48 | 0.7 | -2 | -5 | -8 | -10 | -12 | -10 | -13 |

The recorded reed's harmonics stand 10 to 22 dB above its fundamental in the bass octave and the model's 1 to 9 dB below: the model's fundamental is 10 to 20 dB too strong against the rest, after the flow's change.

### Krummhorn 8' against the Krummhorn 8'

| Notes | | Attack ms | Outside the harmonics dB | Level spread dB | H2 | H3 | H4 | H5 | H6 | H7 | H8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 36-47 | recorded | 47 | -32 | 1.7 | +10 | +14 | +8 | +5 | +11 | +10 | +4 |
| 36-47 | Oregana | 24 | -56 | 0.5 | -27 | -2 | -25 | -5 | -20 | -10 | -23 |
| 48-59 | recorded | 69 | -35 | 2.2 | +7 | +5 | +3 | +5 | +5 | +8 | +6 |
| 48-59 | Oregana | 16 | -54 | 0.4 | -27 | -2 | -25 | -5 | -21 | -11 | -23 |
| 60-71 | recorded | 64 | -35 | 3.1 | -2 | +1 | +1 | +4 | -12 | -8 | -16 |
| 60-71 | Oregana | 12 | -47 | 0.4 | -26 | -2 | -25 | -6 | -21 | -12 | -24 |
| 72-83 | recorded | 62 | -42 | 2.0 | +4 | -0 | -14 | -8 | -15 | -15 | -27 |
| 72-83 | Oregana | 10 | -51 | 0.6 | -26 | -2 | -24 | -7 | -25 | -15 | -32 |
| 84-96 | recorded | 16 | -39 | 4.1 | +4 | +6 | +4 | -8 | -14 | -18 | -20 |
| 84-96 | Oregana | 10 | -46 | 0.6 | -28 | -4 | -31 | -12 | -28 | -23 | -31 |

The largest difference found. The recorded pipe has every harmonic strong; the model's cylinder gives the odd ones and holds the even ones 25 dB down.

### The Krummhorn led by its tongue

The model's Krummhorn is led by its resonator (tongue 2.0 times the pipe's
pitch, Q 10), and a real one by its tongue. A voicing led by the tongue
(tongue 1.03, Q 40, closing 0.5, flow 2, the resonator 1.25 times as long)
gives the recorded spectrum, and its pitch then moves 3 to 5 cents for each
1 % of wind, where the present one moves 0.6 and the trumpet 0.3. A real
reed moves far less than that, so the candidate was not kept: the tongue's
pitch here leans on the wind more than a real tongue's does, and that has to
be put right before any reed can be led by its tongue. It is also why the
resonator's warming could not be modelled (README, Known gaps).

