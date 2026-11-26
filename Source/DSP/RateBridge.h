#pragma once

// A host at any rate from 44.1 to 192 kHz in front of a process that runs
// at 48 kHz only: the organ (OreganaInstrument.h) and its church played as
// an effect. One copy, so that both hear the same resampler.
//
// At 48 kHz the host's samples go to the process and back as they are. At
// any other rate they pass a windowed sinc of kTaps taps, flat to 18 kHz at
// 44.1 kHz and to 20 kHz above 48 kHz, with its images 80 dB down. The
// output is gathered through it, kTaps / 2 engine samples late; an input
// is spread through the same kernel turned round, kTaps / 2 more.
//
// JUCE-free, C++17, nothing allocated after construction.

#include "DspMath.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace oregana {

// `Outputs`: the most channels the process gives back; it takes two in.
template <int Outputs>
class RateBridge {
public:
    static constexpr std::uint32_t kEngineRate = 48000;
    static constexpr int kTaps = 64;

    // Not for the audio thread: designs the kernel for the host's rate
    // (44.1 to 192 kHz), then resets.
    void Prepare(double hostRate) {
        rate = static_cast<std::uint32_t>(Clamp(hostRate == hostRate ? hostRate : kPipeRate, 44100.0, 192000.0) + 0.5);
        direct = rate == kEngineRate;
        if (!direct) Design();
        Reset();
    }

    void Reset() {
        for (auto& channel : history) channel.fill(0.0f);
        for (auto& channel : inbox) channel.fill(0.0f);
        phase = 0;
    }

    bool Direct() const { return direct; }
    std::uint32_t Rate() const { return rate; }

    // Host samples from an input to the output it comes back on, besides
    // what the process adds itself.
    int LatencySamples() const { return direct ? 0 : static_cast<int>(kTaps * static_cast<double>(rate) / kEngineRate + 0.5); }

    // The last kTaps engine samples channel `n` gave, which the next output
    // is gathered from: a channel taken up again after a while is filled
    // with what it would have given.
    float* History(int n) { return history[static_cast<std::size_t>(n)].data(); }

    // Plays `frames` host frames: `in`, two buffers or none, and `outputs`
    // channels of `out`. `process(in, out, n)` makes n engine samples of the
    // first `outputs` channels from two of input. Off 48 kHz `in` is never
    // null for it: what an input left when it was unplugged plays out.
    template <class Process>
    void Render(const float* const* in, float* const* out, int outputs, int frames, Process&& process) {
        if (direct) {
            process(in, out, frames);
            return;
        }
        float* fresher[Outputs];
        for (int n = 0; n < outputs; ++n) fresher[n] = &history[static_cast<std::size_t>(n)][kTaps];
        const float* const taken[2] = { inbox[0].data(), inbox[1].data() };
        int done = 0;
        while (frames > 0) {
            const int count = Min(frames, kChunk);
            // The engine's samples these frames pass, after the kTaps the
            // last ones left.
            const int fresh = static_cast<int>((phase + static_cast<std::uint64_t>(count) * kEngineRate) / rate);
            // Each input frame is spread over the kTaps engine samples the
            // output frame at its moment is gathered from, kTaps / 2 + 1 on,
            // so that the samples made now have all they will get.
            if (in != nullptr)
                Walk(count, [&](int i, int passed, const float* a, const float* b, float mix) {
                    const float l = inputScale * in[0][done + i], r = inputScale * in[1][done + i];
                    float* x = &inbox[0][static_cast<std::size_t>(passed)];
                    float* y = &inbox[1][static_cast<std::size_t>(passed)];
                    for (int k = 0; k < kTaps; ++k) {
                        const float c = a[k] + mix * (b[k] - a[k]);
                        x[k] += c * l;
                        y[k] += c * r;
                    }
                });
            process(taken, fresher, fresh);
            for (auto& channel : inbox) {
                std::memmove(channel.data(), channel.data() + fresh, (channel.size() - static_cast<std::size_t>(fresh)) * sizeof(float));
                std::memset(channel.data() + channel.size() - fresh, 0, static_cast<std::size_t>(fresh) * sizeof(float));
            }
            Walk(count, [&](int i, int passed, const float* a, const float* b, float mix) {
                for (int n = 0; n < outputs; ++n) {
                    const float* z = &history[static_cast<std::size_t>(n)][static_cast<std::size_t>(passed)];
                    float sum = 0.0f;
                    for (int k = 0; k < kTaps; ++k) sum += (a[k] + mix * (b[k] - a[k])) * z[k];
                    out[n][done + i] = sum;
                }
            });
            for (int n = 0; n < outputs; ++n) {
                float* samples = history[static_cast<std::size_t>(n)].data();
                std::memmove(samples, samples + fresh, kTaps * sizeof(float));
            }
            for (int i = 0; i < count; ++i) {
                phase += kEngineRate;
                while (phase >= rate) phase -= rate;
            }
            done += count;
            frames -= count;
        }
    }

private:
    static constexpr int kPhases = 256, kChunk = 64;
    // The most engine samples kChunk frames pass, at 44.1 kHz.
    static constexpr int kFresh = kChunk * 48000 / 44100 + 2;

    // Each of the next `count` host frames' place: the engine samples passed
    // since the chunk's first, and the kernel's two rows and the mix
    // between them.
    template <class Frame>
    void Walk(int count, Frame&& frame) const {
        std::uint64_t at = phase;
        int passed = 0;
        for (int i = 0; i < count; ++i) {
            at += kEngineRate;
            while (at >= rate) {
                at -= rate;
                ++passed;
            }
            const float place = static_cast<float>(at) * phaseScale;
            const int row = static_cast<int>(place);
            const float* a = &kernel[static_cast<std::size_t>(row * kTaps)];
            frame(i, passed, a, a + kTaps, place - static_cast<float>(row));
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
        // The kernel sums to 1 over engine samples; an input frame stands
        // for kEngineRate / rate of them.
        inputScale = static_cast<float>(kEngineRate) / static_cast<float>(rate);
    }

    std::uint32_t rate = kEngineRate;
    bool direct = true;
    std::uint64_t phase = 0;                                        // the output's place between two engine samples, in 1 / rate
    float phaseScale = 0.0f, inputScale = 1.0f;
    std::array<std::array<float, kTaps + kFresh>, Outputs> history {};
    // The input as the process is to take it, from its next sample on.
    std::array<std::array<float, kTaps + kFresh>, 2> inbox {};
    std::array<float, (kPhases + 1) * kTaps> kernel {};
};

} // namespace oregana
