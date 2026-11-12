#!/usr/bin/env python3
"""Write the committed demo phrases, Docs/audio/midi/<NN>-<preset-slug>.mid.

Eight of the bank's seventeen registrations have a demo (README G3), chosen to
walk from one stop to full organ and past each family of pipes on the way:

    Principal 8   the organ's own voice, one open flue rank, in a hymn phrase
    Gedackt 8     the softest stop, a stopped wooden flute, in a pastorale
    Voix celeste  two string ranks tuned apart, under the swell pedal
    Cornet        the five-rank solo stop from middle C over a flute bass
    Krummhorn     a short-resonator reed in the tenor, with the tremulant
    Trompete      a full-length chorus reed in a trumpet tune
    Plenum        the principal chorus to its mixtures, a praeludium's opening
    Tutti         everything, in the opening of Bach's Toccata in D minor

The Tutti phrase is the first flourish and the first chord of BWV 565, which
is in the public domain; the others are original. The .mid files are the
committed sources the demo renderer reads; this script only records how they
were written. Re-run it only to change a phrase on purpose, and commit the
changed .mid files on their own:

    python3 Tools/write_preset_midi.py           # write Docs/audio/midi
    python3 Tools/write_preset_midi.py --check   # the committed files match

Every file is format 0 at 480 PPQ with a tempo and a track name. Oregana's
MIDI map (OreganaEngine.h): channel 1 plays the Hauptwerk, 2 the Positiv,
3 the Recit and 4 the Pedal; the manuals' keys are notes 36 to 96 and the
Pedal's 36 to 67. Velocity is how fast the key goes down, never how loud.
CC1 is the crescendo roller and CC11 the swell pedal. Every registration
couples its manual stops to the Hauptwerk, so the hands play on channel 1.

An organ's key is a switch for wind: a line here holds each key until the
next goes down, and lets a repeated key up REPEAT seconds before it goes down
again. Each phrase's last key comes up 9.5 to 10.5 s in, so every demo is 12 s
long and the renderer fades the church's answer at its end; the 2.5 s tail a
shorter phrase gets would cut the reverberation off while it is still loud.
"""
import argparse
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
MIDI_DIR = ROOT / "Docs/audio/midi"
BANK = ROOT / "Source/DSP/OreganaPresets.h"

PPQ = 480
HAUPTWERK, POSITIV, RECIT, PEDAL = range(4)
REPEAT = 0.06     # seconds a repeated key is up between its notes
ORNAMENT = 0.085  # seconds for each note of a trill
NAMES = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}


def n(name):
    """'C4' is MIDI 60; sharps as 'F#3', flats as 'Bb2'. A number is itself."""
    if isinstance(name, int):
        return name
    pitch = NAMES[name[0]]
    rest = name[1:]
    if rest[0] == "#":
        pitch, rest = pitch + 1, rest[1:]
    elif rest[0] == "b":
        pitch, rest = pitch - 1, rest[1:]
    return 12 * (int(rest) + 1) + pitch


class Phrase:
    def __init__(self, bpm):
        self.bpm = bpm
        self.events = []  # (tick, order, bytes); key-ups sort before key-downs

    def tick(self, beat):
        return round(beat * PPQ)

    def beats(self, seconds):
        return seconds * self.bpm / 60.0

    def note(self, beat, length, pitch, division=HAUPTWERK, velocity=80):
        pitch = n(pitch)
        if not 36 <= pitch <= (67 if division == PEDAL else 96):
            raise ValueError(f"note {pitch} is not a key of division {division}")
        self.events.append((self.tick(beat), 2, bytes([0x90 | division, pitch, velocity])))
        self.events.append((self.tick(beat + length), 1, bytes([0x80 | division, pitch, 64])))

    def line(self, beat, notes, division=HAUPTWERK, velocity=80):
        """(pitch, beats) pairs from `beat`, legato; a None pitch rests."""
        for i, (pitch, length) in enumerate(notes):
            if pitch is not None:
                again = i + 1 < len(notes) and notes[i + 1][0] is not None and n(notes[i + 1][0]) == n(pitch)
                self.note(beat, length - (self.beats(REPEAT) if again else 0.0), pitch, division, velocity)
            beat += length
        return beat

    def chords(self, beat, chords, division=HAUPTWERK, velocity=80):
        """(beats, pitches) pairs from `beat`. A key two chords in a row share
        stays down, as a hymn player ties it."""
        down = {}  # pitch -> the beat it went down
        for length, pitches in chords:
            pitches = [n(pitch) for pitch in pitches]
            for pitch in [pitch for pitch in down if pitch not in pitches]:
                self.note(down[pitch], beat - down.pop(pitch), pitch, division, velocity)
            for pitch in pitches:
                down.setdefault(pitch, beat)
            beat += length
        for pitch, since in down.items():
            self.note(since, beat - since, pitch, division, velocity)
        return beat

    def trill(self, beat, length, main, upper, division=HAUPTWERK, velocity=80):
        """From the upper note, as a baroque trill starts, for the first two
        thirds of `length`, then the main note held."""
        each = self.beats(ORNAMENT)
        pairs = max(2, int(length * 2 / 3 / each / 2))
        notes = [(upper, each), (main, each)] * pairs
        notes[-1] = (main, length - each * (2 * pairs - 1))
        return self.line(beat, notes, division, velocity)

    def mordent(self, beat, length, main, lower, division=HAUPTWERK, velocity=80, each=0.12):
        """Main, lower, main: the first two `each` seconds long."""
        each = self.beats(each)
        return self.line(beat, [(main, each), (lower, each), (main, length - 2 * each)], division, velocity)

    def ramp(self, start, end, controller, first, last, division=HAUPTWERK, step=0.1):
        """A controller moved evenly from `first` to `last`, a value every `step` beats."""
        count = max(1, round((end - start) / step))
        sent = None
        for i in range(count + 1):
            value = round(first + (last - first) * i / count)
            if value != sent:
                self.events.append((self.tick(start + (end - start) * i / count), 0, bytes([0xB0 | division, controller, value])))
            sent = value

    def seconds(self):
        return max(tick for tick, _, _ in self.events) / PPQ * 60 / self.bpm

    def smf(self, name):
        def vlq(value):
            out = [value & 0x7F]
            value >>= 7
            while value:
                out.append(0x80 | (value & 0x7F))
                value >>= 7
            return bytes(reversed(out))

        events = sorted(self.events, key=lambda e: (e[0], e[1]))
        # Two voices on one key would let it up under the one still holding it.
        down = set()
        for tick, _, message in events:
            key = (message[0] & 0x0F, message[1])
            if message[0] & 0xF0 == 0x90:
                if key in down:
                    raise ValueError(f"{name}: key {key} goes down twice at tick {tick}")
                down.add(key)
            elif message[0] & 0xF0 == 0x80:
                down.discard(key)

        tempo = round(60_000_000 / self.bpm)
        track = bytearray()
        track += b"\x00\xff\x03" + vlq(len(name)) + name.encode("ascii")
        track += b"\x00\xff\x51\x03" + tempo.to_bytes(3, "big")
        previous = 0
        for tick, _, message in events:
            track += vlq(tick - previous) + message
            previous = tick
        track += b"\x00\xff\x2f\x00"
        return (b"MThd" + struct.pack(">IHHH", 6, 0, 1, PPQ)
                + b"MTrk" + struct.pack(">I", len(track)) + track)


def principal_8():
    # A hymn phrase in four parts on the one open 8' rank, the bass in the
    # Pedal: a 4-3 suspension before the close, to hear two pipes a tone apart.
    p = Phrase(66)
    p.line(0, [("D5", 2), ("E5", 2), ("D5", 1), ("C5", 1), ("A4", 2), ("G4", 2.6)])
    p.line(0, [("B4", 1), ("A4", 1), ("B4", 1), ("C5", 1), ("B4", 1), ("A4", 1), ("G4", 1), ("F#4", 1), ("D4", 2.6)])
    p.line(0, [("G4", 1), ("F#4", 1), ("G4", 3), ("E4", 1), ("D4", 2), ("B3", 2.6)])
    p.line(0, [("G2", 1), ("F#2", 1), ("E2", 1), ("C3", 1), ("B2", 1), ("C3", 1), ("D3", 2), ("G2", 2.6)], PEDAL)
    return p


def gedackt_8():
    # A pastorale in 6/8 on the stopped flute: every note speaks with its
    # chiff, over held notes in the left hand and the Subbass.
    p = Phrase(132)  # the beat is the quaver
    p.line(0, [("F4", 1), ("A4", 1), ("C5", 1), ("A4", 1), ("C5", 1), ("F5", 1),
               ("E5", 1), ("D5", 1), ("C5", 1), ("D5", 1), ("Bb4", 1), ("G4", 1),
               ("A4", 1), ("C5", 1), ("F5", 1), ("E5", 1), ("G5", 1), ("E5", 1), ("F5", 3.6)])
    p.line(0, [("A3", 6), ("G3", 6), ("A3", 3), ("Bb3", 3), ("A3", 3.6)])
    p.note(18, 3.6, "C4")
    p.line(0, [("F2", 6), ("C3", 3), ("Bb2", 3), ("F2", 3), ("C3", 3), ("F2", 3.6)], PEDAL)
    return p


def voix_celeste():
    # Slow chords for the two string ranks' beating, the swell box opening
    # through the first half and closing to the end.
    p = Phrase(50)
    p.chords(0, [(2, ["B3", "E4", "G#4", "B4"]), (2, ["B3", "E4", "G#4", "C#5"]), (2, ["A3", "E4", "G#4", "C#5"]),
                 (1, ["A3", "E4", "F#4", "B4"]), (1.3, ["G#3", "E4", "G#4", "B4"])])
    p.line(0, [("E2", 2), ("C#3", 2), ("A2", 2), ("B2", 1), ("E2", 1.3)], PEDAL)
    p.ramp(0, 4, 11, 70, 127)
    p.ramp(4, 8.3, 11, 127, 45)
    return p


def cornet():
    # A French recit de cornet: the solo above middle C, where the Cornet's
    # five ranks stand, the left hand below it on the Rohrflote alone.
    p = Phrase(76)
    p.line(0, [("D5", 1.5), ("E5", 0.5), ("F5", 1)])
    p.trill(3, 1, "E5", "F5")
    p.line(4, [("D5", 0.5), ("C5", 0.5), ("Bb4", 1), ("A4", 0.5), ("B4", 0.5), ("C#5", 0.5), ("D5", 0.5)])
    p.trill(8, 1.5, "E5", "F5")
    p.line(9.5, [("D5", 0.5), ("D5", 2.4)])
    p.chords(0, [(3, ["F3", "A3"]), (1, ["E3", "G3"]), (1, ["F3", "A3"]), (1, ["G3", "Bb3"]),
                 (2, ["E3", "A3"]), (2, ["E3", "G3"]), (2.4, ["F3", "A3"])])
    p.line(0, [("D3", 3), ("C#3", 1), ("D3", 1), ("G2", 1), ("A2", 4), ("D3", 2.4)], PEDAL)
    return p


def krummhorn():
    # A cromorne en taille: the reed sings in the tenor under the tremulant,
    # over a slow bass.
    p = Phrase(76)
    p.line(0, [("G3", 1), ("Bb3", 0.5), ("A3", 0.5), ("D4", 1.5), ("C4", 0.25), ("Bb3", 0.25),
               ("A3", 0.5), ("Bb3", 0.5), ("C4", 0.5), ("D4", 0.5), ("Eb4", 1), ("D4", 0.5), ("C4", 0.5), ("Bb3", 0.5)])
    p.trill(8.5, 1.2, "A3", "Bb3")
    p.line(9.7, [("G3", 2.6)])
    p.line(0, [("G2", 2), ("Bb2", 2), ("F2", 2), ("C3", 2), ("D3", 1.7), ("G2", 2.6)], PEDAL)
    return p


def trompete():
    # A trumpet tune in D over the principals, the Pedal's own trumpet under it.
    p = Phrase(100)
    p.line(0, [("A4", 1), ("D5", 1), ("D5", 0.5), ("E5", 0.5), ("F#5", 1),
               ("A5", 1.5), ("G5", 0.5), ("G5", 0.5), ("F#5", 0.5), ("E5", 0.5), ("C#5", 0.5),
               ("F#5", 1), ("G5", 0.5), ("F#5", 0.5)])
    p.trill(10, 1.5, "E5", "F#5")
    p.line(11.5, [("D5", 0.5), ("D5", 4.4)])
    p.chords(0, [(4, ["D3", "A3", "D4"]), (2, ["F#3", "A3", "D4"]), (1, ["G3", "B3", "D4"]), (1, ["A3", "C#4", "E4"]),
                 (1, ["A3", "D4", "F#4"]), (1, ["B3", "D4", "G4"]), (2, ["A3", "C#4", "E4"]),
                 (4.4, ["D3", "A3", "D4", "F#4", "A4"])])
    p.line(0, [("D3", 4), ("F#3", 2), ("G3", 1), ("A3", 1), ("D3", 1), ("G2", 1), ("A2", 2), ("D2", 4.4)], PEDAL)
    return p


def plenum():
    # A north German praeludium's opening: runs over a pedal point, then the
    # full chords the plenum is drawn for, closing on the major.
    p = Phrase(84)
    p.line(0, [(name, 0.25) for name in ("G5", "F5", "Eb5", "D5", "C5", "Bb4", "A4", "G4")])
    p.chords(2, [(2, ["G3", "Bb3", "D4", "G4", "Bb4", "D5"])])
    p.line(4, [(name, 0.25) for name in ("A5", "G5", "F#5", "Eb5", "D5", "C5", "Bb4", "A4")])
    p.chords(6, [(2, ["A3", "C4", "D4", "F#4", "A4", "D5"])])
    p.chords(8, [(1, ["G3", "Bb3", "D4", "Bb4", "D5", "G5"]), (1, ["G3", "C4", "Eb4", "A4", "C5", "Eb5"]),
                 (0.75, ["A3", "D4", "G4", "A4", "D5"]), (0.75, ["A3", "D4", "F#4", "A4", "D5"]),
                 (2.3, ["G3", "B3", "D4", "G4", "B4", "D5", "G5"])])
    p.line(0, [("G2", 4), ("F#2", 4), ("G2", 1), ("Eb3", 1), ("D3", 1.5), ("G2", 2.3)], PEDAL)
    return p


def tutti():
    # J. S. Bach, Toccata in D minor, BWV 565: the first flourish in octaves,
    # then the diminished seventh built a note at a time over the Pedal's D
    # and its resolution to D major. The beat is the second.
    p = Phrase(60)
    for octave in (0, -12):
        p.mordent(0.0, 1.5, n("A5") + octave, n("G5") + octave)
        p.line(1.8, [(n(name) + octave, 0.13) for name in ("G5", "F5", "E5", "D5", "C#5")] + [(n("D5") + octave, 1.55)])
    p.note(4.3, 5.6, "D2", PEDAL)
    for i, name in enumerate(("C#3", "E3", "G3", "Bb3", "C#4", "E4")):
        p.note(4.9 + 0.3 * i, 3.0 - 0.3 * i, name)
    p.chords(7.9, [(2.0, ["D3", "F#3", "A3", "D4"])])
    return p


# (program number, slug, phrase): the slug is the preset's name as the renderer slugs it.
DEMOS = [
    (1, "principal-8", principal_8),
    (2, "gedackt-8", gedackt_8),
    (5, "voix-celeste", voix_celeste),
    (7, "cornet", cornet),
    (9, "krummhorn", krummhorn),
    (12, "trompete", trompete),
    (14, "plenum", plenum),
    (17, "tutti", tutti),
]


def files():
    bank = [re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")
            for name in re.findall(r'Registration\("([^"]+)"', BANK.read_text(encoding="utf-8"))]
    result = {}
    for number, slug, build in DEMOS:
        if number > len(bank) or bank[number - 1] != slug:
            sys.exit(f"demo {number:02d}-{slug} is not preset {number} of the bank {bank}")
        phrase = build()
        if not 9.5 < phrase.seconds() <= 10.5:
            sys.exit(f"{slug}: its last key comes up at {phrase.seconds():.2f} s, outside 9.5-10.5 s")
        result[f"{number:02d}-{slug}.mid"] = phrase.smf(slug)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="fail unless the committed files match")
    args = parser.parse_args()
    expected = files()
    found = {path.name: path.read_bytes() for path in MIDI_DIR.glob("*.mid")}
    if args.check:
        stale = sorted(set(found) ^ set(expected) | {name for name in expected if found.get(name) != expected[name]})
        if stale:
            sys.exit(f"Docs/audio/midi differs from {Path(__file__).name} in: {', '.join(stale)}")
        print(f"{len(expected)} demo phrases match {Path(__file__).name}")
        return
    MIDI_DIR.mkdir(parents=True, exist_ok=True)
    for name in set(found) - set(expected):
        (MIDI_DIR / name).unlink()
    for name, data in expected.items():
        (MIDI_DIR / name).write_bytes(data)
        print(name)


if __name__ == "__main__":
    main()
