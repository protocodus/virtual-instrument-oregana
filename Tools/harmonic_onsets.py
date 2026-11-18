#!/usr/bin/env python3
"""Times each harmonic's attack and release in held single notes, in a
recording and in Oregana.

The protocol (Docs/research/physical-gaps-2026-11-18.md has the first results):

- The reference is a recording of single held notes whose key times are
  known (an Orgamat's, or a sample set's one note a file). It is fetched for
  the measurement and never committed. Oregana plays the same keys through
  RenderOregana with one stop drawn, Room 0, Action 0 and Wind Noise 0.
- A note's fundamental is found in its steady half (from half the hold to
  0.2 s before key-up): the peak of the Hann-windowed spectrum within 6 % of
  the key's pitch at the given a1, read off the parabola through its log.
- Each harmonic k is taken alone: the note is moved down by k times that
  fundamental and low-passed (fourth-order Butterworth at 0.35 of the
  fundamental, at most 60 Hz, run forward and backward); its level is twice
  the magnitude of what is left. Its steady level is the median over the
  steady half.
- Printed for each note: the fundamental; harmonics 2 to 8 in dB against the
  first; the time from key-down at which each of the first six first reaches
  -20 dB, and -6 dB, of its own steady level; the octave's lead, how much
  sooner the second than the first reaches -6 dB; the first's rise from -20
  to -6 dB; the time after key-up at which each of the first four last
  stands above -20 dB; and the breath's lead: the level above eight times the
  fundamental over the 40 ms that end 10 ms before the first reaches -20 dB,
  in dB against that band's steady level.

usage: harmonic_onsets.py <wav> <key-down s,...> <hold s> <key,...> [a1 Hz]
       a1 defaults to 440; the keys are MIDI note numbers at the pitch sounded.
Needs NumPy, SciPy and ffmpeg.
"""
import subprocess
import sys

import numpy as np
from scipy.signal import butter, sosfilt, sosfiltfilt

RATE = 48000
HARMONICS = 8


def load(path):
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-f", "f32le", "-ac", "1", "-ar", str(RATE), "-"],
                         capture_output=True, check=True).stdout
    return np.frombuffer(raw, "<f4").astype("f8")


def fundamental(steady, guess):
    spectrum = np.abs(np.fft.rfft(steady * np.hanning(len(steady))))
    hz = np.fft.rfftfreq(len(steady), 1.0 / RATE)
    i = int(np.argmax(np.where((hz > guess * 0.94) & (hz < guess * 1.06), spectrum, 0.0)))
    a, b, c = np.log(spectrum[i - 1:i + 2] + 1e-30)
    return (i + 0.5 * (a - c) / (a - 2.0 * b + c)) * RATE / len(steady)


def note(sound, down, hold, key, a1):
    lead_in = 0.3
    segment = sound[int((down - lead_in) * RATE):int((down + hold + 2.0) * RATE)]
    t = np.arange(len(segment)) / RATE - lead_in
    steady = (t > hold * 0.5) & (t < hold - 0.2)
    f0 = fundamental(segment[steady], a1 * 2.0 ** ((key - 69) / 12.0))
    sos = butter(4, min(0.35 * f0, 60.0), fs=RATE, output="sos")

    def first(db, threshold):
        at = np.where((t > -0.05) & (t < 1.5) & (db >= threshold))[0]
        return 1000.0 * t[at[0]] if len(at) else np.nan

    levels, up20, up6, falls = [], [], [], []
    for k in range(1, HARMONICS + 1):
        if k * f0 > 0.45 * RATE:
            break
        moved = segment * np.exp(-2j * np.pi * k * f0 * t)
        envelope = 2.0 * np.abs(sosfiltfilt(sos, moved.real) + 1j * sosfiltfilt(sos, moved.imag))
        level = np.median(envelope[steady])
        db = 20.0 * np.log10(envelope / level + 1e-12)
        levels.append(level)
        up20.append(first(db, -20.0))
        up6.append(first(db, -6.0))
        above = np.where((t > hold - 0.05) & (t < hold + 1.5) & (db >= -20.0))[0]
        falls.append(1000.0 * (t[above[-1]] - hold) if len(above) else np.nan)

    harmonics = 20.0 * np.log10(np.array(levels) / levels[0])
    high = sosfilt(butter(4, min(8.0 * f0, 0.4 * RATE), "high", fs=RATE, output="sos"), segment)

    def rms(start, end):
        part = high[int((start + lead_in) * RATE):int((end + lead_in) * RATE)]
        return np.sqrt(np.mean(part * part)) + 1e-15

    breath = np.nan if np.isnan(up20[0]) else \
        20.0 * np.log10(rms(up20[0] / 1000.0 - 0.05, up20[0] / 1000.0 - 0.01) / rms(hold * 0.5, hold - 0.2))
    octave = up6[0] - up6[1] if len(up6) > 1 else np.nan
    return f0, harmonics, up20, up6, octave, up6[0] - up20[0], falls, breath


def main():
    if len(sys.argv) < 5:
        sys.exit(__doc__)
    sound = load(sys.argv[1])
    downs = [float(v) for v in sys.argv[2].split(",")]
    hold = float(sys.argv[3])
    keys = [int(v) for v in sys.argv[4].split(",")]
    a1 = float(sys.argv[5]) if len(sys.argv) > 5 else 440.0
    row = lambda values: " ".join(f"{v:4.0f}" for v in values)
    print("key      f0 | H2..H8 dB against H1        | -20 dB ms, H1..H6           | -6 dB ms, H1..H6"
          "            | octave | rise | fall ms, H1..H4    | breath dB")
    for down, key in zip(downs, keys):
        f0, harmonics, up20, up6, octave, rise, falls, breath = note(sound, down, hold, key, a1)
        print(f"{key:3d} {f0:7.2f} | {row(harmonics[1:])} | {row(up20[:6])} | {row(up6[:6])} | {octave:6.0f} | {rise:4.0f} |"
              f" {row(falls[:4])} | {breath:5.1f}")


if __name__ == "__main__":
    main()
