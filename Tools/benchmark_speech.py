#!/usr/bin/env python3
"""Measures how single held notes speak and stop, in a recording and in Oregana.

The protocol (Docs/research/benchmark-ilmenau-2026-11-13.md has the first results):

- The reference is a recording of single notes whose key times are known: a
  first key-down, a period between notes and the time each is held. It is
  fetched for the measurement and never committed.
- Oregana plays the same keys through RenderOregana with one stop drawn,
  Room 0, Action 0 and Wind Noise 0, every other control as the organ loads.
- A note's tone is its band from 0.8 to 6.5 times its fundamental (a fourth
  order Butterworth band-pass), its breath the band above 8 times it. Levels
  are 5 ms RMS against the steady level, the mean over the second half of
  the hold.
- `speech` prints when after key-down the tone passes -20, -6 and -1 dB, its
  overshoot, and when after key-up it has fallen 3, 10, 20 and 30 dB. A
  recording's key times may carry one unknown offset: compare differences
  between these times, not the times.
- `spectrum` prints the steady harmonics against the first, the share of the
  power from 100 Hz to 10 kHz outside the harmonics (within 1.2 % or 8 Hz of
  each), and the breath's level against its own steady level from 150 ms
  before the tone reaches a tenth of its level to 200 ms after.
- A recording made in a church holds its room: what follows key-up is the
  room as well as the pipe.

usage: benchmark_speech.py speech|spectrum <wav> <semitones> <first s> <period s> <hold s> <index:key>...
       <semitones> moves a key to the pitch its pipe sounds (-12 for a 16' stop).
Needs NumPy, SciPy and ffmpeg.
"""
import subprocess
import sys

import numpy as np
from scipy.signal import butter, sosfilt

BREATH_AT = (-150, -100, -50, -20, 0, 20, 50, 100, 200)   # ms about the tone's start


def load(path, rate=48000):
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-f", "f32le", "-ac", "1", "-ar", str(rate), "-"],
                         capture_output=True, check=True).stdout
    return np.frombuffer(raw, "<f4").astype("f8"), rate


def level(x, rate, window=0.005):
    n = int(window * rate)
    c = np.cumsum(np.concatenate([[0.0], x * x]))
    return np.sqrt((c[n:] - c[:-n]) / n)


def band(x, rate, low, high):
    return sosfilt(butter(4, [low, min(high, 0.45 * rate)], "band", fs=rate, output="sos"), x)


def rms(x):
    return np.sqrt(np.mean(x * x))


def speech(x, rate, on, hold, f0, lead=0.1):
    start = int((on - lead) * rate)
    tone = band(x[start:start + int((lead + hold + 3.0) * rate)], rate, 0.8 * f0, 6.5 * f0)
    steady = rms(tone[int((lead + 0.5 * hold) * rate):int((lead + hold - 0.3) * rate)])
    e = level(tone, rate)
    t = np.arange(len(e)) / rate - lead + 0.0025
    db = 20 * np.log10(e / steady + 1e-12)

    def up(threshold):
        return 1000 * t[np.argmax(db >= threshold)]

    def down(threshold):
        above = np.where((t >= hold - 0.05) & (db >= threshold))[0]
        return 1000 * (t[above[-1]] - hold) if len(above) else float("nan")

    return {"-20": up(-20), "-6": up(-6), "-1": up(-1), "overshoot dB": db[(t > 0) & (t < 1)].max(),
            "fell 3": down(-3), "fell 10": down(-10), "fell 20": down(-20), "fell 30": down(-30)}


def spectrum(x, rate, on, f0, lead=0.25):
    held = x[int((on + 1.5) * rate):int((on + 3.5) * rate)]
    power = np.abs(np.fft.rfft(held * np.hanning(len(held)))) ** 2
    f = np.fft.rfftfreq(len(held), 1 / rate)
    near = (f > 0.97 * f0) & (f < 1.03 * f0)
    f0 = f[near][np.argmax(power[near])]
    harmonics, inside = [], np.zeros(len(f), bool)
    for h in range(1, 13):
        k = np.abs(f - h * f0) < max(0.012 * h * f0, 8)
        inside |= k
        harmonics.append(power[k].sum())
    span = (f > 100) & (f < 10000)
    outside = 10 * np.log10(power[span & ~inside].sum() / power[span].sum())

    onset = x[int((on - lead) * rate):int((on + 1.0) * rate)]
    tone = level(band(onset, rate, 0.8 * f0, 6.5 * f0), rate, 0.01)
    breath = level(band(onset, rate, 8 * f0, 16000), rate, 0.01)
    t = np.arange(len(tone)) / rate - lead
    tone_steady, breath_steady = rms(band(held, rate, 0.8 * f0, 6.5 * f0)), rms(band(held, rate, 8 * f0, 16000))
    start = t[np.argmax(tone >= 0.1 * tone_steady)]
    about = [20 * np.log10(breath[np.argmin(np.abs(t - (start + ms / 1000)))] / breath_steady + 1e-9) for ms in BREATH_AT]
    return f0, [10 * np.log10(h / harmonics[0] + 1e-12) for h in harmonics[1:8]], outside, about


def main():
    if len(sys.argv) < 8 or sys.argv[1] not in ("speech", "spectrum"):
        sys.exit(__doc__)
    x, rate = load(sys.argv[2])
    semitones = int(sys.argv[3])
    first, period, hold = map(float, sys.argv[4:7])
    x = np.concatenate([np.zeros(rate // 2), x])   # room for the lead before a note at the file's start
    first += 0.5
    for word in sys.argv[7:]:
        index, key = map(int, word.split(":"))
        f0 = 440.0 * 2 ** ((key - 69 + semitones) / 12)
        on = first + index * period
        if sys.argv[1] == "speech":
            print(f"key {key} ({f0:.0f} Hz): " + ", ".join(f"{name} {value:.0f}" for name, value in speech(x, rate, on, hold, f0).items()))
        else:
            f0, harmonics, outside, about = spectrum(x, rate, on, f0)
            print(f"key {key} ({f0:.1f} Hz): H2-H8 " + " ".join(f"{h:+.0f}" for h in harmonics) + f", outside the harmonics {outside:.0f} dB, breath "
                  + " ".join(f"{b:+.0f}" for b in about))


if __name__ == "__main__":
    main()
