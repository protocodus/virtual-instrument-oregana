#!/usr/bin/env python3
"""Render the listening set: longer pieces than a README demo may be.

    python3 Tools/render_showcase.py <build-dir> <out-dir>

Writes each piece's MIDI into <out-dir>, plays it through <build-dir>'s
RenderOregana and draws its spectrogram (Tools/spectrogram.py: NumPy and
Matplotlib). Nothing here is committed: give an <out-dir> outside the
checkout.

    toccata                 the opening of J. S. Bach's Toccata in D minor,
                            BWV 565 (public domain), to the first chord's
                            resolution, then an original close, on the Tutti
    crescendo               an original hymn under the crescendo roller, from
                            the Gedackt alone to full organ
    imperfections-none      one phrase on the Trompete registration with every
    imperfections-default   imperfection control at 0, as the organ loads, and
    imperfections-plenty    with all of them high: a scale, a held chord of
                            fifths and octaves, three short chords and silence
"""
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from write_preset_midi import PEDAL, Phrase, n  # noqa: E402

TAIL = 7.0  # seconds rendered after the last key comes up: the church's answer


def toccata():
    p = Phrase(60)  # the beat is the second

    def flourish(at, register, run, hold, pace):
        """A mordent on A held `hold`, then `run` down to a long D, both hands
        an octave apart. Returns when the D comes up."""
        down = at + hold + 0.4
        for octave in (0, -12):
            p.mordent(at, hold, n(f"A{register}") + octave, n(f"G{register}") + octave)
            p.line(down, [(n(f"{name}{register}") + octave, pace) for name in run] + [(n(f"D{register}") + octave, 2.1)])
        return down + pace * len(run) + 2.1

    end = flourish(0.0, 5, ("G", "F", "E", "D", "C#"), 1.9, 0.12)
    end = flourish(end + 0.9, 4, ("E", "F", "C#"), 1.5, 0.16)
    end = flourish(end + 0.9, 3, ("G", "F", "E", "D", "C#"), 1.7, 0.14)

    # The Pedal's D, the diminished seventh built a note at a time over it,
    # and its resolution to D major.
    pedal = end + 0.8
    p.note(pedal, 10.5, "D2", PEDAL)
    resolve = pedal + 6.2
    for i, name in enumerate(("C#3", "E3", "G3", "Bb3", "C#4", "E4")):
        p.note(pedal + 1.0 + 0.45 * i, resolve - (pedal + 1.0 + 0.45 * i), name)
    p.chords(resolve, [(pedal + 10.5 - resolve, ["D3", "F#3", "A3", "D4"])])

    # Not Bach's: a run down the harmonic minor scale and a cadence, to hear
    # the full organ move and then close.
    at = pedal + 10.5 + 0.8
    run = ("D6", "C#6", "Bb5", "A5", "G5", "F5", "E5", "D5", "C#5", "Bb4", "A4", "G4", "F4", "E4", "D4", "C#4")
    for octave in (0, -12):
        p.line(at, [(n(name) + octave, 0.11) for name in run])
    at += 0.11 * len(run)
    p.chords(at, [(1.6, ["G3", "Bb3", "D4", "G4", "Bb4", "D5", "G5"]), (1.2, ["A3", "D4", "E4", "A4", "D5", "E5", "A5"]),
                  (1.4, ["A3", "C#4", "E4", "A4", "C#5", "E5", "A5"]),
                  (4.5, ["D3", "A3", "D4", "F#4", "A4", "D5", "F#5", "A5", "D6"])])
    p.line(at, [("Bb2", 1.6), ("A2", 2.6), ("D2", 4.5)], PEDAL)
    return p


def crescendo():
    p = Phrase(72)
    p.chords(0, [
        (1, ["E4", "G4", "C5"]), (1, ["D4", "G4", "D5"]), (1, ["E4", "A4", "C5"]), (1, ["E4", "G4", "B4"]),
        (1, ["F4", "A4", "C5"]), (1, ["E4", "G4", "C5"]), (1, ["F4", "A4", "C5"]), (1, ["D4", "G4", "B4"]),
        (1, ["G4", "C5", "E5"]), (1, ["F4", "C5", "F5"]), (1, ["G4", "D5", "G5"]), (1, ["G4", "C5", "E5"]),
        (1, ["E4", "C5", "E5"]), (1, ["F4", "A4", "D5"]), (1, ["F4", "B4", "D5"]), (1, ["E4", "C5", "E5"]),
        (1, ["F3", "C4", "F4", "A4", "C5", "F5"]), (1, ["G3", "C4", "E4", "G4", "C5", "G5"]),
        (1, ["F3", "A3", "D4", "F4", "A4", "D5", "A5"]), (1, ["A3", "E4", "A4", "C#5", "E5", "A5"]),
        (1, ["A3", "D4", "F4", "A4", "D5", "F5", "A5"]), (1, ["G3", "D4", "F4", "G4", "B4", "D5", "G5"]),
        (1, ["G3", "C4", "E4", "G4", "C5", "E5", "G5"]), (1, ["G3", "C4", "D4", "G4", "C5", "D5", "G5"]),
        (1, ["G3", "B3", "D4", "G4", "B4", "D5", "G5"]), (1, ["A3", "C4", "E4", "A4", "C5", "E5", "A5"]),
        (1, ["A3", "C4", "F4", "A4", "C5", "F5", "A5"]), (1, ["G3", "C4", "E4", "G4", "C5", "E5", "G5"]),
        (2, ["G3", "B3", "F4", "G4", "B4", "D5", "G5"]), (6, ["C3", "G3", "C4", "E4", "G4", "C5", "E5", "C6"]),
    ])
    p.line(0, [(name, 1) for name in ("C3", "B2", "A2", "G2", "F2", "E2", "D3", "G2",
                                      "C3", "A2", "B2", "C3", "A2", "F2", "G2", "C3",
                                      "F2", "E2", "D3", "C#3", "D3", "B2", "C3", "G2",
                                      "G2", "A2", "F2")] + [("G2", 3), ("C2", 6)], PEDAL)
    p.ramp(1, 29, 1, 0, 127, step=0.25)
    return p


def imperfections():
    p = Phrase(60)
    for i, name in enumerate(("D4", "E4", "F#4", "G4", "A4", "B4", "C#5", "D5")):
        p.note(0.6 * i, 0.5, name)
    p.note(5.2, 5.0, "D2", PEDAL)
    p.chords(5.2, [(5.0, ["D3", "A3", "D4", "A4", "D5"])])
    for i in range(3):
        p.chords(10.8 + 0.5 * i, [(0.25, ["D3", "F#3", "A3", "D4", "F#4", "A4", "D5"])])
        p.note(10.8 + 0.5 * i, 0.25, "D3", PEDAL)
    return p


NONE = ["tuning=0", "temperature=0", "voicing=0", "wind=0", "blower=0", "action=0"]
PLENTY = ["tuning=1", "temperature=8", "voicing=1", "wind=1", "blower=1", "action=1",
          "dead_pipes=0.6", "ciphers=1", "runnings=1"]

# (file, phrase, registration, controls set over the registration's)
PIECES = [
    ("toccata", toccata, "Tutti", []),
    # Volume 3 dB: under the Gedackt registration's own trim, which would
    # carry full organ into the limiter.
    ("crescendo", crescendo, "Gedackt 8", ["volume=3"]),
    ("imperfections-none", imperfections, "Trompete", NONE),
    ("imperfections-default", imperfections, "Trompete", []),
    ("imperfections-plenty", imperfections, "Trompete", PLENTY),
]


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    build, out = Path(sys.argv[1]), Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    for name, phrase, registration, controls in PIECES:
        midi, wav = out / f"{name}.mid", out / f"{name}.wav"
        midi.write_bytes(phrase().smf(name))
        subprocess.run([str(build / "RenderOregana"), registration, str(midi), str(wav), "--tail", str(TAIL)] + controls,
                       check=True)
        subprocess.run([sys.executable, str(Path(__file__).resolve().parent / "spectrogram.py"), str(wav),
                        str(out / f"{name}.png"), "--title", f"{name} ({registration})"], check=True)


if __name__ == "__main__":
    main()
