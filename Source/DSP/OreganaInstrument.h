#pragma once

// Oregana as a front end with one keyboard and its own sample rate plays it.
//
// The engine has a keyboard for each division and runs at 48 kHz only
// (OreganaEngine.h). This is what stands between it and a host that has one
// stream of notes and any rate: both front ends play through it, so they
// agree.
//
//   Keys        every key plays the manual the Keyboard control names, and
//               the Pedal as Pedal Keys says (OreganaControls.h). A key may
//               be held by more than one source at once (a keyboard and a
//               CV gate): it rises when the last one lets go.
//   Mod wheel   the swell pedal, heel down: at rest the box is open, and
//   Expression  turning it up closes it (owner, 2026-11-13: loud at 0).
//               Expression is the same pedal toe down. Both act on top of
//               the Swell control: any of the three can shut the box, and
//               all must be open for it to be open. The crescendo roller
//               is the Crescendo control alone; the engine's own MIDI map
//               (Engine::Midi, which the demo phrases play) keeps CC1 on
//               the roller.
//   Rate        at 48 kHz the engine's samples go out as they are. At any
//               other rate they are resampled through a windowed sinc of
//               kTaps taps, flat to 18 kHz at 44.1 kHz and to 20 kHz above
//               48 kHz, with its images 80 dB down; that costs kTaps / 2
//               engine samples of latency, which LatencySamples includes.
//
// JUCE-free, C++17, nothing allocated after construction. It holds the
// engine: keep it on the heap.

#include "OreganaEngine.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace oregana {

class Instrument {
public:
    static constexpr int kSources = 2;   // who can hold a key: 0 the keyboard, 1 a CV gate

    Instrument() { Prepare(kPipeRate); }

    // Not for the audio thread: designs the resampler for the host's rate
    // (44.1 to 192 kHz), then resets.
    void Prepare(double hostRate) {
        rate = static_cast<std::uint32_t>(Clamp(hostRate == hostRate ? hostRate : kPipeRate, 44100.0, 192000.0) + 0.5);
        direct = rate == kEngineRate;
        if (!direct) Design();
        Reset();
    }

    void Reset() {
        engine.Reset();
        for (auto& source : held) source.fill(0);
        for (auto& division : sent) division.fill(false);
        for (auto& channel : history) channel.fill(0.0f);
        phase = 0;
    }

    void SetControls(const Controls& controls, double wheel = 0.0, double expression = 1.0) {
        Controls played = controls;
        played[kSwell] *= Min(Max(1.0 - wheel, 0.0), 1.0) * Min(Max(expression, 0.0), 1.0);
        engine.SetControls(played);
        Sync();
    }

    // The pitch wheel, -1 .. 1: the air's warmth (Engine::SetBend).
    void SetBend(double bend) { engine.SetBend(bend); }

    void KeyDown(int note, int velocity, int source = 0) {
        if (note < 0 || note > 127 || source < 0 || source >= kSources) return;
        held[static_cast<std::size_t>(source)][static_cast<std::size_t>(note)] = static_cast<std::uint8_t>(Clamp(velocity, 1, 127));
        Sync();
    }
    void KeyUp(int note, int source = 0) {
        if (note < 0 || note > 127 || source < 0 || source >= kSources) return;
        held[static_cast<std::size_t>(source)][static_cast<std::size_t>(note)] = 0;
        Sync();
    }
    void AllKeysUp() {
        for (auto& source : held) source.fill(0);
        Sync();
    }
    void AllSoundOff() {
        AllKeysUp();
        engine.AllSoundOff();
    }

    void Render(float* left, float* right, int frames) {
        if (direct) {
            engine.Render(left, right, frames);
            return;
        }
        while (frames > 0) {
            const int count = Min(frames, kChunk);
            // The engine's samples these frames pass, after the kTaps the
            // last ones left.
            const int fresh = static_cast<int>((phase + static_cast<std::uint64_t>(count) * kEngineRate) / rate);
            engine.Render(&history[0][kTaps], &history[1][kTaps], fresh);
            int passed = 0;
            for (int i = 0; i < count; ++i) {
                phase += kEngineRate;
                while (phase >= rate) {
                    phase -= rate;
                    ++passed;
                }
                const float at = static_cast<float>(phase) * phaseScale;
                const int row = static_cast<int>(at);
                const float mix = at - static_cast<float>(row);
                const float* a = &kernel[static_cast<std::size_t>(row * kTaps)];
                const float* b = a + kTaps;
                const float* x = &history[0][static_cast<std::size_t>(passed)];
                const float* y = &history[1][static_cast<std::size_t>(passed)];
                float l = 0.0f, r = 0.0f;
                for (int k = 0; k < kTaps; ++k) {
                    const float c = a[k] + mix * (b[k] - a[k]);
                    l += c * x[k];
                    r += c * y[k];
                }
                left[i] = l;
                right[i] = r;
            }
            for (auto& channel : history) std::memmove(channel.data(), channel.data() + fresh, kTaps * sizeof(float));
            left += count;
            right += count;
            frames -= count;
        }
    }

    // What a front end reports to its host, samples at the host's rate: the
    // output stage's look-ahead and, off 48 kHz, half the resampler.
    int LatencySamples() const {
        if (direct) return engine.LatencySamples();
        return static_cast<int>((engine.LatencySamples() + kTaps / 2) * static_cast<double>(rate) / kPipeRate + 0.5);
    }
    double SampleRate() const { return rate; }
    const Engine& Organ() const { return engine; }

private:
    static constexpr std::uint32_t kEngineRate = 48000;
    static constexpr int kTaps = 64, kPhases = 256, kChunk = 64;
    // The most engine samples kChunk frames pass, at 44.1 kHz.
    static constexpr int kFresh = kChunk * 48000 / 44100 + 2;

    // Each division's keys as the held ones and the two controls want them,
    // against what the engine was last told.
    void Sync() {
        const Controls& controls = engine.GetControls();
        const int manual = static_cast<int>(controls[kKeyboard]), pedal = static_cast<int>(controls[kPedalMode]);
        const auto velocity = [&](int note) {
            const auto n = static_cast<std::size_t>(note);
            return Max(held[0][n], held[1][n]);
        };
        int lowest = -1;
        for (int note = 0; note < 128 && lowest < 0; ++note)
            if (velocity(note)) lowest = note;
        for (int note = 0; note < 128; ++note) {
            const int down = velocity(note);
            const bool split = pedal == kPedalSplit && note < kSplitNote;
            for (int d = 0; d < kDivisions; ++d) {
                const bool want = down && (d == kPedal ? split || (pedal == kPedalBass && note == lowest) : d == manual && !split);
                bool& is = sent[static_cast<std::size_t>(d)][static_cast<std::size_t>(note)];
                if (want == is) continue;
                is = want;
                if (want) engine.KeyDown(d, note, down);
                else engine.KeyUp(d, note);
            }
        }
    }

    // A Kaiser-windowed sinc (beta 8: 80 dB) at each of kPhases + 1 places
    // between two engine samples. The band ends 1.9 kHz, half the window's
    // transition, under the lower of the two Nyquist rates, and at 22 kHz
    // when the host's is the higher: nothing an organ gives is lost above it.
    void Design() {
        const auto bessel = [](double x) {
            double sum = 1.0, term = 1.0;
            for (int k = 1; k < 40; ++k) {
                term *= (0.5 * x / k) * (0.5 * x / k);
                sum += term;
            }
            return sum;
        };
        const double beta = 8.0, half = 0.5 * kTaps;
        const double corner = Min(22000.0, 0.5 * rate - 1900.0) / kPipeRate;
        for (int row = 0; row <= kPhases; ++row) {
            double taps[kTaps], sum = 0.0;
            for (int k = 0; k < kTaps; ++k) {
                // Tap k's sample stands this far from the output's moment.
                const double d = k - half + 1.0 - static_cast<double>(row) / kPhases;
                const double edge = 1.0 - (d / half) * (d / half);
                const double window = edge > 0.0 ? bessel(beta * std::sqrt(edge)) / bessel(beta) : 0.0;
                const double turn = kTwoPi * corner * d;
                taps[k] = window * (Abs(turn) < 1.0e-9 ? 1.0 : std::sin(turn) / turn);
                sum += taps[k];
            }
            for (int k = 0; k < kTaps; ++k) kernel[static_cast<std::size_t>(row * kTaps + k)] = static_cast<float>(taps[k] / sum);
        }
        phaseScale = static_cast<float>(kPhases) / static_cast<float>(rate);
    }

    Engine engine;
    std::array<std::array<std::uint8_t, 128>, kSources> held {};    // each source's keys: the velocity it holds one with, or 0
    std::array<std::array<bool, 128>, kDivisions> sent {};
    std::uint32_t rate = kEngineRate;
    bool direct = true;
    std::uint64_t phase = 0;                                        // the output's place between two engine samples, in 1 / rate
    float phaseScale = 0.0f;
    std::array<std::array<float, kTaps + kFresh>, 2> history {};
    std::array<float, (kPhases + 1) * kTaps> kernel {};
};

} // namespace oregana
