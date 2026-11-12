#pragma once

// The church the organ stands in, heard from the nave.
//
// First the reflections a listener gets before the church as a whole
// answers, by image sources: the organ on a west gallery 8 m up and 2 m from
// the wall behind it, a nave 14 m wide and 18 m to the vault, the listener on
// its axis 18 m from the organ. Then the reverberant field, a sixteen-line
// feedback delay network (Jot and Chaigne, AES 90, 1991) with a Householder
// matrix, as in Respira's Room.h: each line loses what its length costs of
// the decay asked for, more towards the treble, as stone, people and above
// all the air of a large space take it; four Schroeder allpasses make the
// field dense from its start, and each line's length drifts a tenth of a
// millisecond at its own slow rate, so no mode of the network stands still
// under a held chord. The two ears' fields come from orthogonal readouts of
// the lines.
//
// It is a voiced church of the size a three-manual organ stands in, not a
// model of any one building: the reflections follow from the dimensions
// above, the decay is the player's (Decay), and the surfaces' reflection
// factors are estimates (0.9 stone, 0.7 a ribbed vault, 0.6 a floor of pews).
// It runs at the pipes' rate.

#include "DspMath.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace oregana {

class Nave {
public:
    // Line lengths spread geometrically over 47 to 199 ms, each a prime
    // number of samples: 1.76 s of delay in all, a mode every 0.57 Hz.
    static constexpr int kLines = 16;
    static constexpr std::array<int, kLines> kLineSamples { {
        2251, 2477, 2731, 3011, 3313, 3643, 4019, 4423, 4871, 5351, 5903, 6491, 7159, 7879, 8677, 9551 } };
    static constexpr int kLineSize = 16384;
    // Each line's sign of the input, and of its share of the side readout:
    // rows of a Hadamard matrix, orthogonal to each other and to the sum.
    static constexpr std::array<float, kLines> kInputSign { { 1, 1, -1, -1, 1, 1, -1, -1, 1, 1, -1, -1, 1, 1, -1, -1 } };
    static constexpr std::array<float, kLines> kSideSign { { 1, -1, -1, 1, -1, 1, 1, -1, 1, -1, -1, 1, -1, 1, 1, -1 } };
    // The drift: 0.1 ms each way, at 0.07 to 0.5 Hz spread over the lines.
    // A tail's pitch moves by under half a cent.
    static constexpr double kDriftSamples = 4.8, kSlowestDrift = 0.07, kFastestDrift = 0.5;
    static constexpr int kTurn = 32;   // samples between the drift's steps
    static constexpr std::array<int, 4> kDiffuserSamples { { 101, 179, 283, 397 } };
    static constexpr int kDiffuserSize = 512;
    static constexpr float kDiffuserGain = 0.6f;

    // Each reflection: samples after the direct sound (the path's excess over
    // 19.24 m at 343 m/s), level against it (1/r, times the surface's factor)
    // and how it falls on the two ears.
    struct Reflection { int samples; float left, right; };
    static constexpr std::array<Reflection, 7> kReflections { {
        { 136, 0.571f, 0.571f },    // the floor: 20.21 m
        { 530, 0.752f, 0.752f },    // the wall behind the organ: 23.03 m
        { 637, 0.728f, 0.30f },     // the left wall: 23.80 m
        { 641, 0.30f, 0.728f },     // the right wall
        { 1825, 0.417f, 0.417f },   // the vault: 32.28 m
        { 2062, 0.459f, 0.19f },    // both walls, the left one last: 33.97 m
        { 2069, 0.19f, 0.459f },    // and the right one last
    } };
    static constexpr int kEarlySize = 4096;
    // The air and the people take the treble off a reflection and off what
    // enters the reverberant field: one-poles at 5 and 6 kHz, voiced.
    static constexpr double kEarlyCorner = 5000.0, kLateCorner = 6000.0;
    // At 1 the reflections above come to 1.7 times a plenum chord's own
    // level and the field, at a decay of 4.5 s, to 5.3 times: what 19 m from
    // the organ gives in a church where the two are equal at 4 m. Both are
    // scaled alike so that together they are as strong as the chord itself,
    // and Room then mixes the organ and the church at one loudness
    // (Oregana.Engine's NaveLevel).
    static constexpr float kEarlyGain = 0.18f, kLateGain = 0.18f;

    // `rt60` is the time the field takes to fall 60 dB at low frequencies;
    // it falls steadily faster towards the top, `trebleShare` of it at half
    // the rate (each line's one-pole: its gain is the line's decay at 0 Hz
    // and there).
    void Set(double rt60, double trebleShare) {
        for (std::size_t i = 0; i < kLines; ++i) {
            const double seconds = kLineSamples[i] / kPipeRate;
            const double low = std::pow(10.0, -3.0 * seconds / rt60), high = std::pow(10.0, -3.0 * seconds / (rt60 * trebleShare));
            linePole[i] = static_cast<float>((low - high) / (low + high));
            lineGain[i] = static_cast<float>(low) * (1.0f - linePole[i]);
            // The lines' drift rates interleave across their lengths, so
            // neighbours do not move together.
            const double share = static_cast<double>((i * 7u) % kLines) / (kLines - 1);
            const double step = kTwoPi * (kSlowestDrift + share * (kFastestDrift - kSlowestDrift)) * kTurn / kPipeRate;
            turnCos[i] = std::cos(step);
            turnSin[i] = std::sin(step);
        }
        earlyPole = static_cast<float>(std::exp(-kTwoPi * kEarlyCorner / kPipeRate));
        latePole = static_cast<float>(std::exp(-kTwoPi * kLateCorner / kPipeRate));
    }

    // 0: both ears hear one field. 1: two fields with nothing in common, as
    // ears (or microphones) a head apart do above a few hundred hertz.
    void SetWidth(double width) {
        const double rho = 1.0 - Clamp(width, 0.0, 1.0);
        lateMid = static_cast<float>(std::sqrt(0.5 * (1.0 + rho)));
        lateSide = static_cast<float>(std::sqrt(0.5 * (1.0 - rho)));
    }

    void Clear() {
        for (std::size_t i = 0; i < kLines; ++i) {
            lineBuffer[i].fill(0.0f);
            lineLow[i] = 0.0f;
            // Each line starts its drift at its own phase, the golden angle apart.
            driftCos[i] = std::cos(2.399963229728653 * static_cast<double>(i));
            driftSin[i] = std::sin(2.399963229728653 * static_cast<double>(i));
            offset[i] = static_cast<float>(kDriftSamples * driftSin[i]);
            offsetStep[i] = 0.0f;
        }
        for (auto& buffer : diffuserBuffer) buffer.fill(0.0f);
        earlyBuffer.fill(0.0f);
        write = 0;
        untilTurn = 0;
        earlyLeft = earlyRight = late = 0.0f;
        resting = true;
        quietFor = 0;
    }

    // Takes the organ's sound and returns what the church adds for each ear.
    void Process(float left, float right, float& roomLeft, float& roomRight) {
        // At rest every buffer and state is zero, and silence in gives
        // silence out and leaves them so.
        if (resting) {
            if (left == 0.0f && right == 0.0f) {
                roomLeft = roomRight = 0.0f;
                return;
            }
            resting = false;
        }
        if (untilTurn == 0) {
            TurnDrift();
            untilTurn = kTurn;
        }
        --untilTurn;

        std::array<float, kLines> out;
        float sum = 0.0f;
        for (std::size_t i = 0; i < kLines; ++i) {
            offset[i] += offsetStep[i];
            float read = static_cast<float>(write - kLineSamples[i]) - offset[i];
            if (read < 0.0f) read += static_cast<float>(kLineSize);
            const int at = static_cast<int>(read);
            const float between = read - static_cast<float>(at);
            const auto& buffer = lineBuffer[i];
            const float older = buffer[static_cast<std::size_t>(at & (kLineSize - 1))];
            const float newer = buffer[static_cast<std::size_t>((at + 1) & (kLineSize - 1))];
            lineLow[i] = lineGain[i] * (older + between * (newer - older)) + linePole[i] * lineLow[i];
            out[i] = lineLow[i];
            sum += out[i];
        }

        const float input = 0.5f * (left + right);
        earlyBuffer[static_cast<std::size_t>(write & (kEarlySize - 1))] = input;
        float reflectedLeft = 0.0f, reflectedRight = 0.0f;
        for (const Reflection& r : kReflections) {
            const float x = earlyBuffer[static_cast<std::size_t>((write - r.samples) & (kEarlySize - 1))];
            reflectedLeft += r.left * x;
            reflectedRight += r.right * x;
        }
        earlyLeft = reflectedLeft + earlyPole * (earlyLeft - reflectedLeft);
        earlyRight = reflectedRight + earlyPole * (earlyRight - reflectedRight);

        late = input + latePole * (late - input);
        float spread = late;
        for (std::size_t i = 0; i < kDiffuserSamples.size(); ++i) {
            auto& buffer = diffuserBuffer[i];
            const float delayed = buffer[static_cast<std::size_t>((write - kDiffuserSamples[i]) & (kDiffuserSize - 1))];
            const float v = spread + kDiffuserGain * delayed;
            buffer[static_cast<std::size_t>(write & (kDiffuserSize - 1))] = Settled(v);
            spread = delayed - kDiffuserGain * v;
        }

        const float feedback = 2.0f / kLines * sum;
        float side = 0.0f;
        bool quiet = left == 0.0f && right == 0.0f;
        for (std::size_t i = 0; i < kLines; ++i) {
            const float in = Settled(out[i] - feedback + kInputSign[i] * spread);
            lineBuffer[i][static_cast<std::size_t>(write & (kLineSize - 1))] = in;
            quiet = quiet && in == 0.0f;
            side += kSideSign[i] * out[i];
        }
        write = (write + 1) & (kLineSize - 1);

        const float mid = kLateGain * lateMid * sum, apart = kLateGain * lateSide * side;
        roomLeft = kEarlyGain * earlyLeft + mid + apart;
        roomRight = kEarlyGain * earlyRight + mid - apart;

        // Silence in and silence written to every line for as long as the
        // longest buffer takes to wrap: every buffer holds only zeros.
        quietFor = quiet ? quietFor + 1 : 0;
        if (quietFor >= kLineSize) {
            resting = true;
            quietFor = 0;
            lineLow.fill(0.0f);
            earlyLeft = earlyRight = late = 0.0f;
        }
    }

private:
    // What a buffer stores: silence below the smallest normal float.
    // Subnormals would otherwise circulate for ever, and the room never rest.
    static float Settled(float x) { return Abs(x) < 1.1754944e-38f ? 0.0f : x; }

    // The drifts' phasors turn by one step, kept at length 1 by a first-order
    // correction, and each line's offset ramps to its new place over the step.
    void TurnDrift() {
        for (std::size_t i = 0; i < kLines; ++i) {
            const double c = driftCos[i] * turnCos[i] - driftSin[i] * turnSin[i];
            const double s = driftSin[i] * turnCos[i] + driftCos[i] * turnSin[i];
            const double norm = 1.5 - 0.5 * (c * c + s * s);
            driftCos[i] = c * norm;
            driftSin[i] = s * norm;
            offsetStep[i] = (static_cast<float>(kDriftSamples * driftSin[i]) - offset[i]) * (1.0f / kTurn);
        }
    }

    std::array<std::array<float, kLineSize>, kLines> lineBuffer {};
    std::array<float, kLines> lineGain {}, linePole {}, lineLow {}, offset {}, offsetStep {};
    std::array<double, kLines> driftCos {}, driftSin {}, turnCos {}, turnSin {};
    std::array<std::array<float, kDiffuserSize>, 4> diffuserBuffer {};
    std::array<float, kEarlySize> earlyBuffer {};
    int write = 0, untilTurn = 0;
    float earlyPole = 0.0f, latePole = 0.0f, earlyLeft = 0.0f, earlyRight = 0.0f, late = 0.0f;
    float lateMid = 0.7071f, lateSide = 0.7071f;
    bool resting = true;
    int quietFor = 0;
};

} // namespace oregana
