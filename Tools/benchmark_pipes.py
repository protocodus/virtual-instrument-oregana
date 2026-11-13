#!/usr/bin/env python3
"""Measures single held pipes of a recorded organ and of Oregana, and sets them side by side.

The protocol (Docs/research/benchmark-burea-2026-11-13.md has the first results):

- The reference is a sample set with one file for each pipe, decoded to WAV in
  folders named by rank, each file named by its MIDI note (`060-C.wav`). It is
  fetched for the measurement and never committed.
- Oregana plays the same notes, 36 to 96, one at a time through RenderOregana:
  3 s held, 5 s apart, Room 0, Action 0 and Wind Noise 0, every other control
  as the organ loads. `midi` writes that phrase.
- Each note is measured over 1.6 s of steady speech, from 0.8 s after its
  onset: the pitch, each harmonic's level against the first, and the share of
  the power between 150 Hz and 10 kHz that lies outside the harmonics (within
  1.2 % or 8 Hz of each). The attack is the time its 10 ms level takes from a
  tenth to nine tenths of the steady one.
- A recording made in a church holds its room: its attack is the room's
  build-up as well as the pipe's, and each note's harmonics carry the room's
  response at their frequencies. Medians over an octave are compared, and
  the attack is reported, not judged.

usage: benchmark_pipes.py midi <out.mid> <channel>
       benchmark_pipes.py measure <folder | oregana.wav> <out.json> [a4]
       benchmark_pipes.py table <reference.json> <oregana.json>
"""
import glob
import json
import os
import struct
import sys

FIRST, LAST, HELD, APART, LEAD = 36, 96, 3.0, 5.0, 0.5


def midi(path, channel):
    def vlq(value):
        out = [value & 0x7F]
        while value > 0x7F:
            value >>= 7
            out.append(0x80 | (value & 0x7F))
        return bytes(reversed(out))

    ticks = 480   # a beat a second
    track = b"\x00\xff\x51\x03" + (1000000).to_bytes(3, "big")
    last = 0
    for k, note in enumerate(range(FIRST, LAST + 1)):
        on = round((LEAD + APART * k) * ticks)
        off = on + round(HELD * ticks)
        track += vlq(on - last) + bytes([0x90 | channel, note, 80]) + vlq(off - on) + bytes([0x80 | channel, note, 0])
        last = off
    track += b"\x00\xff\x2f\x00"
    with open(path, "wb") as f:
        f.write(b"MThd" + struct.pack(">IHHH", 6, 0, 1, ticks) + b"MTrk" + struct.pack(">I", len(track)) + track)


def measure_note(x, rate, note, a4):
    import numpy as np

    x = x.mean(1) if x.ndim > 1 else x
    hop, window = int(0.002 * rate), int(0.010 * rate)
    level = np.sqrt(np.convolve(x * x, np.ones(window) / window, "same"))[::hop]
    steady = np.median(level[int(1.0 * rate / hop):int(2.4 * rate / hop)])
    if steady < 1e-5:
        return None
    on = int(np.argmax(level > 0.1 * steady))
    up = on + int(np.argmax(level[on:] > 0.9 * steady))
    start = on * hop + int(0.8 * rate)
    held = x[start:start + int(1.6 * rate)]
    size = 1 << 18
    spectrum = np.abs(np.fft.rfft(held * np.hanning(len(held)), size))
    hz = np.fft.rfftfreq(size, 1 / rate)

    def peak(at, width):
        low, high = np.searchsorted(hz, [at * (1 - width), at * (1 + width)])
        k = low + int(np.argmax(spectrum[low:high]))
        a, b, c = np.log(spectrum[k - 1:k + 2] + 1e-30)
        return hz[k] + 0.5 * (a - c) / (a - 2 * b + c) * (hz[1] - hz[0]), spectrum[k]

    guess = a4 * 2 ** ((note - 69) / 12)
    f0, _ = peak(guess, 0.03)
    harmonics = []
    for k in range(1, 13):
        if k * f0 > 0.45 * rate:
            break
        harmonics.append(peak(k * f0, 0.015)[1])
    # The pitch from the strongest of the first four harmonics.
    best = int(np.argmax(harmonics[:4])) + 1
    f0 = peak(best * f0, 0.01)[0] / best
    power = spectrum ** 2
    harmonic = np.zeros(len(hz), bool)
    for k in range(1, int(hz[-1] / f0) + 1):
        harmonic |= np.abs(hz - k * f0) < max(8.0, 0.012 * k * f0)
    band = (hz > 150) & (hz < 10000)
    return {
        "note": note,
        "hz": f0,
        "cents": 1200 * np.log2(f0 / guess),
        "attack_ms": 1000 * (up - on) * hop / rate,
        "level": 20 * np.log10(steady),
        "harmonics": [20 * np.log10(h / harmonics[0] + 1e-12) for h in harmonics],
        "noise_db": 10 * np.log10(power[band & ~harmonic].sum() / power[band].sum() + 1e-12),
    }


def measure(source, out, a4):
    import soundfile as sf

    notes = []
    if os.path.isdir(source):
        for path in sorted(glob.glob(os.path.join(source, "*.wav"))):
            note = int(os.path.basename(path)[:3])
            if FIRST <= note <= LAST:
                x, rate = sf.read(path)
                notes.append(measure_note(x, rate, note, a4))
    else:
        x, rate = sf.read(source)
        for k, note in enumerate(range(FIRST, LAST + 1)):
            begin = int((LEAD - 0.2 + APART * k) * rate)
            notes.append(measure_note(x[begin:begin + int(4.5 * rate)], rate, note, a4))
    notes = [m for m in notes if m]
    with open(out, "w") as f:
        json.dump(notes, f)
    print(f"{source}: {len(notes)} notes")


def table(reference, oregana):
    import numpy as np

    sets = [("recorded", json.load(open(reference))), ("Oregana", json.load(open(oregana)))]
    for name, notes in sets:
        cents = np.array([m["cents"] for m in notes])
        # Each note against the median of the five around it.
        local = np.array([cents[i] - np.median(cents[max(0, i - 2):i + 3]) for i in range(len(cents))])
        print(f"{name}: a4 {440 * 2 ** (np.median(cents) / 1200):.1f} Hz, tuning scatter {local.std():.1f} cents (largest {np.abs(local).max():.1f})")
    print("| Notes | | Attack ms | Outside the harmonics dB | Level spread dB | H2 | H3 | H4 | H5 | H6 | H7 | H8 |")
    print("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |")
    for low, high in [(36, 47), (48, 59), (60, 71), (72, 83), (84, 96)]:
        for name, notes in sets:
            some = [m for m in notes if low <= m["note"] <= high]
            harmonics = [np.median([m["harmonics"][k] for m in some if len(m["harmonics"]) > k] or [float("nan")]) for k in range(1, 8)]
            print(f"| {low}-{high} | {name} | {np.median([m['attack_ms'] for m in some]):.0f} | {np.median([m['noise_db'] for m in some]):.0f} | "
                  f"{np.std([m['level'] for m in some]):.1f} | " + " | ".join(f"{h:+.0f}" for h in harmonics) + " |")


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "midi":
        midi(sys.argv[2], int(sys.argv[3]))
    elif len(sys.argv) in (4, 5) and sys.argv[1] == "measure":
        measure(sys.argv[2], sys.argv[3], float(sys.argv[4]) if len(sys.argv) == 5 else 440.0)
    elif len(sys.argv) == 4 and sys.argv[1] == "table":
        table(sys.argv[2], sys.argv[3])
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
