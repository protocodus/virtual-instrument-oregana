#!/usr/bin/env python3
"""The room acoustic measures of an impulse response: Oregana's church
(Tools/NaveResponse) or a measured room's.

The protocol (Docs/research/physical-gaps-2026-11-18.md has the first results):

- A measured response is one source and one omnidirectional receiver of a
  SOFA file (AES69, SingleRoomSRIR), fetched for the measurement and never
  committed. Oregana's is NaveResponse's left ear: a microphone is one point,
  and the two ears' fields have little in common.
- Time 0 is the direct sound: the first sample within 20 dB of the
  response's peak (ISO 3382-1).
- Octave bands from 63 Hz to 8 kHz: third-order Butterworth band-passes
  between f / sqrt 2 and f sqrt 2, run forward.
- A measured response ends in noise. Its band's energy, in 10 ms blocks, is
  cut where its decay meets the noise, found as Lundeby et al. (Acustica 81,
  1995) do: the noise is the mean of the last tenth, the decay the line
  fitted from 0 dB to 10 dB over the noise, and the two re-found three times
  from where they cross. The energy after the crossing is the line's,
  carried on to the end. Oregana's response has no noise (`--clean`): its
  energy is integrated to the file's end, which must be long enough for the
  decay to have fallen well past 35 dB.
- The decay curve is the backward integral (Schroeder). EDT, T20 and T30 are
  the times a line fitted from 0 to -10, -5 to -25 and -5 to -35 dB takes to
  fall 60 dB.
- C80 and D50 are the energy before 80 ms against the energy after it, and
  the energy before 50 ms against the whole, in each band.
- The echo density is Abel and Huang's (AES 121, 2006): in a 20 ms Hann
  window, the share of the samples further from 0 than the window's own RMS,
  over erfc(1 / sqrt 2), the share a Gaussian noise has. 1 is as dense as
  noise; a lone reflection is near 0. It is taken on the whole response and
  printed 5 to 200 ms after time 0, with the mixing time: the first moment
  after which it stays at 0.9 or more for 20 ms.

usage: room_measures.py <response.wav> [--clean]
       room_measures.py <measured.sofa> <measurement> <receiver>
Needs NumPy, SciPy and, for SOFA, h5py.
"""
import sys

import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, sosfilt
from scipy.special import erfc

BANDS = (63, 125, 250, 500, 1000, 2000, 4000, 8000)
DENSITY_AT = (5, 10, 20, 50, 100, 200)   # ms after the direct sound


def load(argv):
    if argv[1].endswith(".sofa"):
        import h5py
        with h5py.File(argv[1], "r") as f:
            rate = float(np.array(f["Data.SamplingRate"])[0])
            return np.array(f["Data.IR"][int(argv[2]), int(argv[3]), :], dtype="f8"), rate, False
    rate, x = wavfile.read(argv[1])
    x = x.astype("f8")
    if x.ndim > 1:
        x = x[:, 0]
    return x, float(rate), "--clean" in argv


def blocks(energy, rate, ms=10.0):
    n = max(1, int(rate * ms / 1000.0))
    count = len(energy) // n
    return energy[:count * n].reshape(count, n).mean(axis=1), n


def cut(energy, rate):
    """Where the decay meets the noise, in samples, and the energy the line carries on after it."""
    level, n = blocks(energy, rate)
    db = 10.0 * np.log10(level + 1e-300)
    t = np.arange(len(db))
    noise = np.mean(db[-max(1, len(db) // 10):])
    peak = int(np.argmax(db))
    crossing = len(db) - 1
    for _ in range(4):
        above = np.where(db[peak:] > noise + 10.0)[0]
        stop = peak + (above[-1] if len(above) else 1)
        if stop - peak < 3:
            break
        slope, start = np.polyfit(t[peak:stop], db[peak:stop], 1)
        if slope >= 0.0:
            break
        crossing = int(min(len(db) - 1, max(peak + 1, (noise - start) / slope)))
        tail = db[min(len(db) - 1, crossing + int(-5.0 / slope)):]
        if len(tail) >= max(1, len(db) // 10):
            noise = np.mean(tail)
    # The line's energy after the crossing, to the end of time.
    slope, start = np.polyfit(t[peak:crossing + 1], db[peak:crossing + 1], 1) if crossing - peak >= 3 else (-1.0, db[peak])
    decay = -slope * np.log(10.0) / 10.0   # per block
    after = 10.0 ** ((start + slope * crossing) / 10.0) * n / decay if decay > 0 else 0.0
    return crossing * n, after


def decay_curve(energy, rate, clean):
    if clean:
        return np.cumsum(energy[::-1])[::-1]
    end, after = cut(energy, rate)
    curve = np.zeros(len(energy))
    curve[:end] = np.cumsum(energy[:end][::-1])[::-1] + after
    return curve


def fit(curve, rate, top, bottom):
    db = 10.0 * np.log10(curve / curve[0] + 1e-300)
    inside = np.where((db <= top) & (db >= bottom))[0]
    if len(inside) < 10 or db.min() > bottom:
        return np.nan
    slope, _ = np.polyfit(inside / rate, db[inside], 1)
    return -60.0 / slope


def density(x, rate):
    n = int(0.02 * rate)
    w = np.hanning(n)
    w /= w.sum()
    rms = np.sqrt(np.convolve(x * x, w, "same"))
    outside = np.convolve((np.abs(x) > rms).astype("f8"), w, "same")
    return outside / erfc(1.0 / np.sqrt(2.0))


def main():
    if len(sys.argv) < 2 or (sys.argv[1].endswith(".sofa") and len(sys.argv) < 4):
        sys.exit(__doc__)
    x, rate, clean = load(sys.argv)
    onset = int(np.argmax(np.abs(x) >= np.max(np.abs(x)) * 0.1))
    x = x[onset:]
    print(f"direct sound at {1000.0 * onset / rate:.1f} ms")
    print("band Hz    EDT s  T20 s  T30 s  C80 dB  D50 %")
    early80, early50 = int(0.08 * rate), int(0.05 * rate)
    mids = {}
    for f in BANDS:
        sos = butter(3, [f / np.sqrt(2.0), min(f * np.sqrt(2.0), 0.45 * rate)], "bandpass", fs=rate, output="sos")
        energy = sosfilt(sos, x) ** 2
        curve = decay_curve(energy, rate, clean)
        edt, t20, t30 = fit(curve, rate, 0.0, -10.0), fit(curve, rate, -5.0, -25.0), fit(curve, rate, -5.0, -35.0)
        c80 = 10.0 * np.log10((curve[0] - curve[early80]) / curve[early80])
        d50 = 100.0 * (curve[0] - curve[early50]) / curve[0]
        mids[f] = (edt, t20, t30, c80, d50)
        print(f"{f:7d}  {edt:6.2f} {t20:6.2f} {t30:6.2f} {c80:7.1f} {d50:6.0f}")
    edt, t20, t30, c80, d50 = (np.mean([mids[500][i], mids[1000][i]]) for i in range(5))
    print(f"500-1k   {edt:6.2f} {t20:6.2f} {t30:6.2f} {c80:7.1f} {d50:6.0f}")
    ned = density(x, rate)
    at = " ".join(f"{ms} ms {ned[int(ms * rate / 1000)]:.2f}" for ms in DENSITY_AT)
    held = int(0.02 * rate)
    dense = (ned >= 0.9).astype(int)
    run = np.convolve(dense, np.ones(held, dtype=int), "valid")
    mixing = np.where(run == held)[0]
    print(f"echo density: {at}; mixing time {1000.0 * mixing[0] / rate:.0f} ms" if len(mixing) else
          f"echo density: {at}; never dense")


if __name__ == "__main__":
    main()
