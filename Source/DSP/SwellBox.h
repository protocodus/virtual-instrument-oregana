#pragma once

// The swell box the Recit stands in, after C. Pykett's measurements of
// wooden boxes ("Swell boxes and swell pedals, Part 1", 2023: a loud chord
// recorded through several boxes with their shutters at each tenth of their
// travel, averaged and smoothed, against the box open). Shut, a box takes
// hardly anything of the lowest notes, 3 dB at about 160 Hz and then more
// and more, some 35 dB at 8 kHz. The first tenth of the shutters' travel
// lets out most of the treble and each tenth after it less, and part open
// the highest frequencies level off instead of falling further.
//
// Read as the box's sound shared out (statistical room acoustics): what the
// pipes put into the box leaves through the shutters' opening, a of their
// whole; through the walls, which pass less as the mass law has it, 6 dB an
// octave; and into the box's own absorption, taken as much as its shutters
// open (an estimate, which shapes only the part-open box, within about
// 2 dB). So the box passes (a + x) / (1 + a + x) of the sound's power, with
// x = (kBoxHz / f)^2 the walls' share, and the Recit is heard against the
// box open, as its pipes are regulated. Louvres turned through k of their
// right angle open as sin(k pi / 2), and k is the pedal's place. kBoxHz puts
// the shut box's 3 dB at Pykett's 160 Hz; 35 dB at 8 kHz follows from it.
// Not modelled: the air in the box rings as a Helmholtz resonator that the
// shutters' opening tunes, and the panels resonate and leak bass of their
// own (README, Known gaps).
//
// As a filter, the box against open is
//   (1 + sqrt(a) s/w) / (1 + s/w) * (1 + sqrt(2) s/w) / (1 + sqrt(1 + a) s/w),
// w = 2 pi kBoxHz: two first-order sections, each 1 with the box open. Each
// is written as the sound plus a share of a high pass, so that open the box
// gives back the sound itself, sample for sample. Bilinear, warped to
// kBoxHz at 48 kHz.

#include "DspMath.h"

#include <cmath>

namespace oregana {

struct SwellBox {
    // tan(pi kBoxHz / 48 kHz), kBoxHz = 160 Hz sqrt(sqrt(2) - 1) = 102.98 Hz.
    static constexpr float kWarp = 0.0067398055f;
    static constexpr float kSqrt2 = 1.41421356f;   // the same float as Sqrt(2.0f)
    static constexpr float kFirstScale = 1.0f / (kWarp + 1.0f), kFirstPole = (1.0f - kWarp) / (kWarp + 1.0f);

    // One channel: the sound its high passes last took in and gave out.
    struct Channel {
        float in = 0.0f, first = 0.0f, between = 0.0f, second = 0.0f;
    };

    float open = 1.0f, target = 1.0f;   // the shutters' opening, of their whole
    float firstShare = 0.0f, secondShare = 0.0f;
    float secondScale = 1.0f / (kWarp + kSqrt2), secondPole = (kSqrt2 - kWarp) / (kWarp + kSqrt2);

    // The pedal's place, 0 shut to 1 open.
    void Set(double pedal) { target = static_cast<float>(std::sin(0.5 * kPi * pedal)); }
    void Reset() {
        open = target;
        Shape();
    }
    // Once a sample: the shutters move `glide` of the way to where the pedal
    // has put them.
    void Glide(float glide) {
        if (open == target) return;
        open += glide * (target - open);
        if (Abs(target - open) < 1.0e-6f) open = target;
        Shape();
    }
    float Process(Channel& s, float sound) const {
        s.first = kFirstScale * (sound - s.in) + kFirstPole * s.first;
        s.in = sound;
        const float between = sound + firstShare * s.first;
        s.second = secondScale * (between - s.between) + secondPole * s.second;
        s.between = between;
        return between + secondShare * s.second;
    }
    // Once in a while: a silent box's high passes die away to nothing
    // rather than into denormals.
    static void Settle(Channel& s) {
        if (Abs(s.first) < 1.0e-20f) s.first = 0.0f;
        if (Abs(s.second) < 1.0e-20f) s.second = 0.0f;
    }
    void Shape() {
        const float across = Sqrt(1.0f + open);
        firstShare = Sqrt(open) - 1.0f;
        secondShare = kSqrt2 - across;
        secondScale = 1.0f / (kWarp + across);
        secondPole = (across - kWarp) / (kWarp + across);
    }
};

} // namespace oregana
