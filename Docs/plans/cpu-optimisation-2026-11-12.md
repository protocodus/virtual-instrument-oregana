# CPU optimisation plan

Oregana 0.1.0 is above the portfolio's CPU target (U12: no 256-frame block
above 10 % of one core at the default settings) on its large registrations.
This is the measurement and the order in which to bring it down.

## Where it stands

`build-dsp/RenderOreganaPresetDemos --cpu --repeats 3 Docs/audio/midi`,
2026-11-12, development container, GCC 13, Release, one thread, 48 kHz,
256-frame blocks:

| Render | Pipes at most | Realtime factor | Worst block |
| --- | --- | --- | --- |
| Principal 8 demo | 12 | 0.010 | 1.7 % |
| Trompete demo | 40 | 0.028 | 4.5 % |
| Plenum demo | 198 | 0.067 | 14.6 % |
| Tutti demo | 468 | 0.160 | 38.6 % |
| Stress phrase, Plenum | | 0.098 | 20.7 % |
| Stress phrase, Tutti | | 0.343 | 65.9 % |

The cost is the pipes: a block's share follows the number speaking, about
0.08 % of a core for each. The church, the wind and the output stage are a
fixed 1 % or so (a single stop's whole demo). The 10 % target is therefore
about 120 pipes: a principal chorus with one mixture on a full chord.

These are development-machine figures, not Reason or DAW CPU.

## The order of work

1. **Profile first.** Split a pipe's cost between the air column's loop, the
   jet's delay and interpolation, the noise generator and the radiation
   filter, for flues and reeds apart, before changing anything.
2. **Several pipes to an instruction.** Pipes are independent of each other
   within a 32-sample tick: they share only the wind, which is constant across
   it. Laying the active pipes' states out side by side and stepping four or
   eight at once is the largest step available, and it leaves every sample as
   it is if each pipe's arithmetic keeps its order. The sum into the buses
   must keep its order too (`Oregana.Engine`, BlockPartitionInvariance).
3. **Bass pipes at a lower rate.** A 16' or 32' pipe has nothing above a few
   kilohertz. Running the lowest octaves' loops at half or a quarter of the
   rate would halve or quarter their cost, but it changes their sound a
   little (the loop filters and the jet's interpolation are designed for the
   rate), so it needs the regulation regenerated and an A-Z listening test.
4. **The small things**: the chimney branch tested per sample, the per-pipe
   noise generator, and pipes that ring far below audibility for seconds
   before they are let go (a 32' pipe's loop takes long to fall 120 dB).

## Where it stands after 0.5.0 (2026-11-13)

The same command, container and compiler:

| Render | Worst block at 0.4.0 | At 0.5.0 |
| --- | --- | --- |
| Principal 8 demo | 1.7 % | 1.9 % |
| Trompete demo | 4.5 % | 3.0 % |
| Plenum demo | 14.6 % | 8.4 % |
| Tutti demo | 38.6 % | 20.4 % |
| Stress phrase, Plenum | 20.7 % | 12.8 % |
| Stress phrase, Tutti | 65.9 % | 35.5 % |

Step 2 is done: flues and reeds run four to an instruction (`RunFour`), with
the same bits as one at a time. The church worked its 33 reflections out
sample by sample; it now does sixteen samples ahead at once, and it reflects
four divisions instead of one. A single stop costs about what it did, with
the pipes' hearing and the four places in the church added; a large
registration costs about half.

Later the same night the church's 32 lines went four to an instruction (the
church alone takes about half the time it did), and two or three pipes left
over of a kind are made up to a four with silent pipes instead of running
one at a time. Both leave every sample as it was; together they take 8 % off
the Tutti demo's instructions. The table's 0.5.0 column is from after them.

A profile of the Tutti demo after all of it (callgrind, instructions): the
flues' kernels 54 %, the reeds' 15 %, the tick's summing and hearing 12 %,
the church 10 %, the tick's bookkeeping 5 %. A third of a flue kernel is
reading and writing four pipes' delay lines one lane at a time, which only
a different layout of the pipes' memory would mend.

What is left that keeps the samples is small. The 10 % target for full
organ needs steps 3 and 4 above, which change the sound and need a
listening set.

## Not on the table

Replacing a settled pipe by a stored period of its own sound, or sharing one
pipe between the ranks of a mixture, would cut the cost several times over,
and neither is the instrument any more: a pipe that no longer takes wind
cannot sag, flutter under the tremulant, or speak again differently.
