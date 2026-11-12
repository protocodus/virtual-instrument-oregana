#pragma once

// The organ's own noises: the action's knocks and the wind's.
//
// A tracker action knocks: the key meets its bed and the pallet is pulled
// open, the pallet slaps back on its leather when the key rises, and a stop
// knob drags its slider across the chest. Each is a struck piece of wood,
// two decaying modes. Under them the blower hums in its room while it runs
// and the wind hisses from every joint of the chests while they hold any;
// on hand-pumped wind there is no hum, and a bellows lands with a thud of
// boards and leather each time the calcant treads the next one.
//
// None of it is measured. The modes are those of small wooden parts (a few
// hundred hertz, over in a few milliseconds), the levels are set by the
// Action and Blower controls against an estimate, and every knock differs a
// little from the last.

#include "DspMath.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace oregana {

struct KnockKind {
    double hz[2], seconds[2];   // the two modes and their decay times (to 1/e)
    double level;
};

inline constexpr KnockKind kKeyDown { { 380.0, 1150.0 }, { 0.012, 0.005 }, 1.0 };
inline constexpr KnockKind kKeyUp { { 250.0, 700.0 }, { 0.015, 0.006 }, 0.6 };
inline constexpr KnockKind kStopKnob { { 120.0, 420.0 }, { 0.04, 0.015 }, 2.0 };
inline constexpr KnockKind kBellowsTurn { { 62.0, 175.0 }, { 0.09, 0.035 }, 3.0 };

class Knocks {
public:
    static constexpr int kVoices = 12;

    void Clear() {
        for (Voice& v : voices) v.remaining = 0;
        random = 0x9E3779B9u;
    }

    // `strength` is the stroke against an ordinary one; `pan` -1 left .. 1 right.
    void Strike(const KnockKind& kind, double strength, double pan) {
        Voice* voice = &voices[0];
        for (Voice& v : voices)
            if (v.remaining < voice->remaining) voice = &v;
        // No two knocks alike: a tenth either way in pitch, a third in level.
        const double tune = 1.0 + 0.1 * NextNoise(random), gain = kind.level * strength * (1.0 + 0.3 * NextNoise(random));
        double longest = 0.0;
        for (int m = 0; m < 2; ++m) {
            const double w = kTwoPi * kind.hz[m] * tune / kPipeRate, r = std::exp(-1.0 / (kind.seconds[m] * kPipeRate));
            voice->turn[m] = static_cast<float>(2.0 * r * std::cos(w));
            voice->loss[m] = static_cast<float>(r * r);
            voice->now[m] = static_cast<float>(gain * std::sin(w));
            voice->before[m] = 0.0f;
            longest = Max(longest, kind.seconds[m]);
        }
        voice->left = static_cast<float>(0.5 * (1.0 - 0.6 * pan));
        voice->right = static_cast<float>(0.5 * (1.0 + 0.6 * pan));
        voice->remaining = static_cast<int>(10.0 * longest * kPipeRate);
    }

    void Process(float& left, float& right) {
        for (Voice& v : voices) {
            if (v.remaining == 0) continue;
            --v.remaining;
            float sum = 0.0f;
            for (int m = 0; m < 2; ++m) {
                sum += v.now[m];
                const float next = v.turn[m] * v.now[m] - v.loss[m] * v.before[m];
                v.before[m] = v.now[m];
                v.now[m] = next;
            }
            left += v.left * sum;
            right += v.right * sum;
        }
    }

private:
    struct Voice {
        float turn[2] {}, loss[2] {}, now[2] {}, before[2] {};
        float left = 0.0f, right = 0.0f;
        int remaining = 0;
    };
    std::array<Voice, kVoices> voices {};
    std::uint32_t random = 0x9E3779B9u;
};

// The blower's hum (noise under 150 Hz) and the chests' hiss (noise around
// 2 kHz), a field with nothing in common between the ears.
class WindNoise {
public:
    void Clear() {
        for (Side& s : sides) s = Side {};
        sides[0].random = 0x1234567u;
        sides[1].random = 0x89ABCDEu;
    }
    void Process(float hum, float hiss, float& left, float& right) {
        float* const out[2] = { &left, &right };
        for (int c = 0; c < 2; ++c) {
            Side& s = sides[c];
            const float white = NextNoise(s.random);
            s.hum += kHumPole * (white - s.hum);
            s.hum2 += kHumPole * (s.hum - s.hum2);
            s.low += kHissLow * (white - s.low);
            s.high += kHissHigh * (white - s.high);
            *out[c] += hum * kHum * s.hum2 + hiss * kHiss * (s.high - s.low);
        }
    }

private:
    // One-pole steps for 150 Hz, 1.2 kHz and 3.5 kHz at the pipes' rate.
    static constexpr float kHumPole = 0.01944f, kHissLow = 0.1454f, kHissHigh = 0.3675f;
    static constexpr float kHum = 0.35f, kHiss = 0.012f;
    struct Side {
        float hum = 0.0f, hum2 = 0.0f, low = 0.0f, high = 0.0f;
        std::uint32_t random = 1u;
    };
    std::array<Side, 2> sides {};
};

} // namespace oregana
