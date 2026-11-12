// Measurements of one pipe's sound, shared by the probe and the regulation
// generator: sounding frequency, level, harmonics and time to speak.
#pragma once

#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>

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
    const std::size_t span = end - from - longest;
    double best = -1.0, energy = 0.0;
    std::size_t bestLag = static_cast<std::size_t>(rate / guess);
    for (std::size_t i = 0; i < span; ++i) energy += static_cast<double>(x[from + i]) * x[from + i];
    for (std::size_t lag = shortest; lag <= longest; ++lag) {
        double sum = 0.0, other = 0.0;
        for (std::size_t i = 0; i < span; ++i) {
            sum += static_cast<double>(x[from + i]) * x[from + i + lag];
            other += static_cast<double>(x[from + i + lag]) * x[from + i + lag];
        }
        const double match = sum / std::sqrt(energy * other + 1e-30);
        // The first lag that matches nearly as well as any: a period, not its multiple.
        if (match > best + 0.02) {
            best = match;
            bestLag = lag;
        }
    }
    return rate / static_cast<double>(bestLag);
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

} // namespace oregana::measure
