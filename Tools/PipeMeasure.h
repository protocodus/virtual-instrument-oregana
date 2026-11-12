// Measurements of one pipe's sound, shared by the probe and the regulation
// generator: sounding frequency, level, harmonics and time to speak.
#pragma once

#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>

#include "DSP/DspMath.h"
#include "DSP/Stops.h"

namespace oregana::measure {

constexpr double kTwoPi = 6.283185307179586;

// The component of x[from, from + count) at hz, Hann-weighted.
inline std::complex<double> Component(const std::vector<float>& x, std::size_t from, std::size_t count, double hz,
                                      double rate) {
    std::complex<double> sum = 0.0;
    double weight = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
        const double hann = 0.5 - 0.5 * std::cos(kTwoPi * (static_cast<double>(i) + 0.5) / static_cast<double>(count));
        const double phase = -kTwoPi * hz * static_cast<double>(from + i) / rate;
        sum += static_cast<double>(x[from + i]) * hann * std::complex<double>(std::cos(phase), std::sin(phase));
        weight += hann;
    }
    return 2.0 * sum / weight;
}

// The sounding frequency of x[from, end) near `guess`: the phase a component
// at the guess gains between two halves of ever longer stretches.
inline double Hz(const std::vector<float>& x, std::size_t from, std::size_t end, double guess, double rate) {
    double hz = guess;
    for (double periods : { 4.0, 16.0, 64.0, 256.0, 1024.0 }) {
        const auto span = static_cast<std::size_t>(periods * rate / hz);
        if (from + 2 * span > end) break;
        const auto a = Component(x, end - 2 * span, span, hz, rate), b = Component(x, end - span, span, hz, rate);
        hz += std::arg(b * std::conj(a)) * rate / (kTwoPi * static_cast<double>(span));
    }
    return hz;
}

// The period of x[from, end) by autocorrelation, as a frequency within an
// octave and a half either side of `guess`: where a pipe has jumped to
// another note, the phase measurement above would follow a harmonic.
inline double CoarseHz(const std::vector<float>& x, std::size_t from, std::size_t end, double guess, double rate) {
    const auto shortest = static_cast<std::size_t>(rate / (guess * 2.8)), longest = static_cast<std::size_t>(rate / guess * 2.8);
    if (from + 3 * longest > end) return guess;
    // A treble pipe's period is a few samples and seldom a whole number of
    // them, and a reed's match peaks sharply: whole lags alone favour
    // whichever multiple of the period lands nearest a sample. Smoothed to
    // little more than the fundamental, each peak is broad enough to read
    // off the parabola through it.
    std::vector<double> y(end - from);
    const double smooth = 1.0 - std::exp(-kTwoPi * Min(1.5 * guess, 0.4 * rate) / rate);
    double one = 0.0, two = 0.0;
    for (std::size_t i = 0; i < y.size(); ++i) {
        one += smooth * (static_cast<double>(x[from + i]) - one);
        two += smooth * (one - two);
        y[i] = two;
    }
    const std::size_t span = y.size() - longest - 1;
    double energy = 0.0;
    for (std::size_t i = 0; i < span; ++i) energy += y[i] * y[i];
    std::vector<double> match(longest + 2, 0.0);
    for (std::size_t lag = shortest > 1 ? shortest - 1 : 1; lag <= longest + 1; ++lag) {
        double sum = 0.0, other = 0.0;
        for (std::size_t i = 0; i < span; ++i) {
            sum += y[i] * y[i + lag];
            other += y[i + lag] * y[i + lag];
        }
        match[lag] = sum / std::sqrt(energy * other + 1e-30);
    }
    // The first peak that matches nearly as well as any: a period, not its multiple.
    double best = -1.0, bestLag = rate / guess;
    for (std::size_t lag = Max<std::size_t>(shortest, 2); lag <= longest; ++lag) {
        const double before = match[lag - 1], here = match[lag], after = match[lag + 1];
        if (here < before || here <= after) continue;
        const double shift = 0.5 * (before - after) / (before - 2.0 * here + after);
        const double peak = here - 0.25 * (before - after) * shift;
        if (peak > best + 0.02) {
            best = peak;
            bestLag = static_cast<double>(lag) + shift;
        }
    }
    return rate / bestLag;
}

inline double Rms(const std::vector<float>& x, std::size_t from, std::size_t end) {
    double sum = 0.0;
    for (std::size_t i = from; i < end; ++i) sum += static_cast<double>(x[i]) * x[i];
    return std::sqrt(sum / static_cast<double>(end - from));
}

// Seconds until the sound's level over one period first reaches `share` of
// its steady level.
inline double SpeechSeconds(const std::vector<float>& x, double hz, double steadyRms, double share, double rate) {
    const auto period = static_cast<std::size_t>(rate / hz) + 1;
    double sum = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        sum += static_cast<double>(x[i]) * x[i];
        if (i >= period) sum -= static_cast<double>(x[i - period]) * x[i - period];
        if (i >= period && std::sqrt(sum / static_cast<double>(period)) >= share * steadyRms)
            return static_cast<double>(i) / rate;
    }
    return static_cast<double>(x.size()) / rate;
}

// A planned pipe alone at a steady wind, from silence, at unity gain.
inline std::vector<float> Sound(const PipePlan& plan, double tuning, std::size_t frames, double wind = 1.0) {
    std::vector<float> left(frames, 0.0f), right(frames, 0.0f), loop, jet, upper;
    FluePipe flue;
    ReedPipe reed;
    if (plan.reed) {
        reed.Design(plan.tongue, plan.hz, tuning);
        loop.assign(static_cast<std::size_t>(reed.loopLength), 0.0f);
        reed.loop = loop.data();
        reed.Clear();
        reed.gainLeft = 1.0f;
        reed.wind = static_cast<float>(wind);
    } else {
        flue.Design(plan.flue, plan.hz, tuning);
        loop.assign(static_cast<std::size_t>(flue.loopLength), 0.0f);
        jet.assign(static_cast<std::size_t>(flue.jetLength), 0.0f);
        upper.assign(static_cast<std::size_t>(flue.upperLength), 0.0f);
        flue.loop = loop.data();
        flue.jet = jet.data();
        flue.upper = upper.data();
        flue.Clear();
        flue.gainLeft = 1.0f;
        flue.wind = static_cast<float>(wind);
    }
    for (std::size_t done = 0; done < frames; done += FluePipe::kTick) {
        const int n = static_cast<int>(Min<std::size_t>(FluePipe::kTick, frames - done));
        if (plan.reed) {
            reed.Tick();
            reed.Process(&left[done], &right[done], n);
        } else {
            flue.Tick();
            flue.Process(&left[done], &right[done], n);
        }
    }
    return left;
}

} // namespace oregana::measure
