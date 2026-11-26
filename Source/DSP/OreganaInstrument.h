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
//   Mod wheel   the crescendo roller (owner, 2026-11-13): at rest only
//               what is drawn sounds, and all the way up is the full
//               organ. It and the Crescendo control are one roller, as far
//               over as the further of the two.
//   Expression  the swell pedal, on top of the Swell control: either can
//               shut the box, and both must be open for it to be open.
//   Rate        at 48 kHz the engine's samples go out as they are; at any
//               other rate through RateBridge.h, which costs kTaps / 2
//               engine samples of latency, which LatencySamples includes.
//   Input       another sound played in the church (Engine::Render). Off
//               48 kHz RateBridge spreads it into the engine kTaps / 2
//               engine samples more than LatencySamples says, which is
//               the organ's.
//
// JUCE-free, C++17, nothing allocated after construction. It holds the
// engine: keep it on the heap.

#include "OreganaEngine.h"
#include "RateBridge.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace oregana {

class Instrument {
public:
    static constexpr int kSources = 2;   // who can hold a key: 0 the keyboard, 1 a CV gate

    Instrument() { Prepare(kPipeRate); }

    // Not for the audio thread: designs the resampler for the host's rate
    // (44.1 to 192 kHz), then resets.
    void Prepare(double hostRate) {
        bridge.Prepare(hostRate);
        Reset();
    }

    void Reset() {
        engine.Reset();
        for (auto& source : held) source.fill(0);
        for (auto& division : sent) division.fill(false);
        lift.fill(64);
        bridge.Reset();
        divisionsConnected = false;
    }

    void SetControls(const Controls& controls, double wheel = 0.0, double expression = 1.0) {
        Controls played = controls;
        played[kCrescendo] = Max(played[kCrescendo], Min(Max(wheel, 0.0), 1.0));
        played[kSwell] *= Min(Max(expression, 0.0), 1.0);
        engine.SetControls(played);
        Sync();
    }

    void SetMasterTune(double cents) { engine.SetMasterTune(cents); }

    // The pitch wheel, -1 .. 1: the air's warmth (Engine::SetBend).
    void SetBend(double bend) { engine.SetBend(bend); }

    void KeyDown(int note, int velocity, int source = 0) {
        if (note < 0 || note > 127 || source < 0 || source >= kSources) return;
        held[static_cast<std::size_t>(source)][static_cast<std::size_t>(note)] = static_cast<std::uint8_t>(Clamp(velocity, 1, 127));
        lift[static_cast<std::size_t>(note)] = 64;   // until it is let go: a key the controls move off its manual lifts ordinarily
        Sync();
    }
    // `velocity`: how fast the key is let go (Engine::KeyUp).
    void KeyUp(int note, int source = 0, int velocity = 64) {
        if (note < 0 || note > 127 || source < 0 || source >= kSources) return;
        held[static_cast<std::size_t>(source)][static_cast<std::size_t>(note)] = 0;
        lift[static_cast<std::size_t>(note)] = static_cast<std::uint8_t>(Clamp(velocity, 1, 127));
        Sync();
    }
    void AllKeysUp() {
        for (auto& source : held) source.fill(0);
        lift.fill(64);
        Sync();
    }
    void AllSoundOff() {
        AllKeysUp();
        engine.AllSoundOff();
    }

    // `divisions`: Engine::kDivisionOutputs more buffers, or none; `input`:
    // a left and a right buffer, or none (Engine::Render).
    void Render(float* left, float* right, int frames, float* const* divisions = nullptr, const float* const* input = nullptr) {
        if (frames <= 0) return;
        float* out[2 + Engine::kDivisionOutputs] = { left, right };
        if (divisions != nullptr) {
            for (std::size_t n = 0; n < Engine::kDivisionOutputs; ++n) {
                out[2 + n] = divisions[n];
                if (!divisionsConnected && !bridge.Direct()) engine.CopyDivisionHistory<Bridge::kTaps>(n, bridge.History(static_cast<int>(2 + n)));
            }
        }
        divisionsConnected = divisions != nullptr;
        bridge.Render(input, out, divisionsConnected ? 2 + Engine::kDivisionOutputs : 2, frames,
                      [&](const float* const* in, float* const* to, int n) { engine.Render(to[0], to[1], n, divisionsConnected ? to + 2 : nullptr, in); });
    }

    // What a front end reports to its host, samples at the host's rate: the
    // output stage's look-ahead and, off 48 kHz, half the resampler.
    int LatencySamples() const {
        if (bridge.Direct()) return engine.LatencySamples();
        return static_cast<int>((engine.LatencySamples() + Bridge::kTaps / 2) * static_cast<double>(bridge.Rate()) / kPipeRate + 0.5);
    }
    double SampleRate() const { return bridge.Rate(); }
    const Engine& Organ() const { return engine; }
    // Each division's peak since the last call, for a front end's meters.
    std::array<float, kDivisions> TakePeaks() { return engine.TakePeaks(); }

private:
    using Bridge = RateBridge<2 + Engine::kDivisionOutputs>;

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
        for (int note = kFirstNote; note < kFirstNote + kPedalKeys && lowest < 0; ++note)
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
                else engine.KeyUp(d, note, lift[static_cast<std::size_t>(note)]);
            }
        }
    }

    Engine engine;
    std::array<std::array<std::uint8_t, 128>, kSources> held {};    // each source's keys: the velocity it holds one with, or 0
    std::array<std::array<bool, 128>, kDivisions> sent {};
    std::array<std::uint8_t, 128> lift {};   // the velocity each key was last let go with
    bool divisionsConnected = false;
    Bridge bridge;
};

} // namespace oregana
