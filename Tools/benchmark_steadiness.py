#!/usr/bin/env python3
"""Measures how steady a held note is, in a recording and in Oregana.

The protocol (Docs/research/known-gaps-2026-11-13.md has the first results):

- The reference is a recording of single held notes whose key times are
  known: a first key-down, a period between notes and the time each is held.
  It is fetched for the measurement and never committed.
- Oregana plays the same keys through RenderOregana with one stop drawn,
  Room 0, Action 0 and Wind Noise 0, every other control as the organ loads.
- A note's fundamental is found in its steady part (from 1 s after key-down
  to 0.4 s before key-up) and taken alone: a fourth order Butterworth
  band-pass 6 % either side of it, run forward and backward. Its pitch and
  level are those of its analytic signal, averaged over 10 ms.
- Printed for each note: the standard deviation of the pitch in cents and of
  the level in dB, each as a whole and in three bands of the fluctuation's
  own rate: drift (under 2 Hz), flutter (2 to 12 Hz, where a reservoir
  bounces) and roughness (12 to 40 Hz).
- A steady tone stays steady in a room, so a recording made in a church
  shows the pipe's and the wind's own movement; noise in the band adds a
  floor, which the same measurement of Oregana with its turbulence shows.

usage: benchmark_steadiness.py <wav> <semitones> <first s> <period s> <hold s> <index:key>...
       <semitones> moves a key to the pitch its pipe sounds (-12 for a 16' stop).
Needs NumPy, SciPy and ffmpeg.
"""
import subprocess
import sys

import numpy as np
from scipy.signal import butter, hilbert, sosfiltfilt

RATE = 48000
BANDS = (("drift", 0.0, 2.0), ("flutter", 2.0, 12.0), ("rough", 12.0, 40.0))


def load(path):
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-f", "f32le", "-ac", "1", "-ar", str(RATE), "-"],
                         capture_output=True, check=True).stdout
    return np.frombuffer(raw, "<f4").astype("f8")


def parts(track, step):
    """The standard deviation of a track sampled every `step` seconds, whole and in BANDS."""
    track = track - track.mean()
    spectrum = np.fft.rfft(track)
    rates = np.fft.rfftfreq(len(track), step)
    out = [track.std()]
    for _, low, high in BANDS:
        kept = np.where((rates > low) & (rates <= high), spectrum, 0.0)
        out.append(np.fft.irfft(kept, len(track)).std())
    return out


def steadiness(sound, on, hold, guess):
    held = sound[int((on + 1.0) * RATE):int((on + hold - 0.4) * RATE)]
    spectrum = np.abs(np.fft.rfft(held * np.hanning(len(held))))
    hz = np.fft.rfftfreq(len(held), 1.0 / RATE)
    near = (hz > guess * 0.94) & (hz < guess * 1.06)
    f0 = hz[near][np.argmax(spectrum[near])]
    # The whole note with a margin, so the filter's ends fall outside the steady part.
    whole = sound[int((on + 0.5) * RATE):int((on + hold - 0.1) * RATE)]
    band = sosfiltfilt(butter(4, [f0 * 0.94, f0 * 1.06], "bandpass", fs=RATE, output="sos"), whole)
    analytic = hilbert(band)[int(0.5 * RATE):-int(0.3 * RATE)]
    block = RATE // 100
    count = len(analytic) // block
    turn = np.angle(analytic[1:] * np.conj(analytic[:-1]))
    pitch = turn[:count * block - 1]
    pitch = np.append(pitch, pitch[-1]).reshape(count, block).mean(axis=1) * RATE / (2.0 * np.pi)
    level = np.abs(analytic[:count * block]).reshape(count, block).mean(axis=1)
    cents = 1200.0 * np.log2(pitch / pitch.mean())
    decibels = 20.0 * np.log10(level / level.mean())
    return f0, parts(cents, 0.01), parts(decibels, 0.01)


def main():
    if len(sys.argv) < 7:
        sys.exit(__doc__)
    sound = load(sys.argv[1])
    semitones, first, period, hold = (float(a) for a in sys.argv[2:6])
    print("note     Hz   cents: all drift flutter rough    dB: all drift flutter rough")
    for arg in sys.argv[6:]:
        index, key = (int(a) for a in arg.split(":"))
        guess = 440.0 * 2.0 ** ((key + semitones - 69) / 12.0)
        # 6 % either side finds an organ standing as far off as a1 = 415 or 466 Hz.
        f0, cents, decibels = steadiness(sound, first + index * period, hold, guess)
        print("%4d %7.1f   %9.2f %5.2f %7.2f %5.2f   %7.2f %5.2f %7.2f %5.2f" % (key, f0, *cents, *decibels))


if __name__ == "__main__":
    main()
