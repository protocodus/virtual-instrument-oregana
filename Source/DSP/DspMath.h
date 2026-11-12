#pragma once

// The Rack Extension toolchain compiles with -fno-builtin, which turns the
// libc++ spellings of these into out-of-line calls (its sqrt is a bit-serial
// software routine there). The builtins are the same IEEE operations in line.
// Only these run per sample; the pipes' designs use <cmath> freely.

#include <cmath>
#include <cstdint>

namespace oregana {

#if defined(__clang__) && defined(__has_builtin)
#if __has_builtin(__builtin_elementwise_sqrt)
#define OREGANA_ELEMENTWISE_SQRT 1
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#if defined(OREGANA_ELEMENTWISE_SQRT)
inline float Sqrt(float x) { return __builtin_elementwise_sqrt(x); }
inline double Sqrt(double x) { return __builtin_elementwise_sqrt(x); }
#else
inline float Sqrt(float x) { return __builtin_sqrtf(x); }
inline double Sqrt(double x) { return __builtin_sqrt(x); }
#endif
inline float Abs(float x) { return __builtin_fabsf(x); }
inline double Abs(double x) { return __builtin_fabs(x); }
#else
inline float Sqrt(float x) { return std::sqrt(x); }
inline double Sqrt(double x) { return std::sqrt(x); }
inline float Abs(float x) { return std::fabs(x); }
inline double Abs(double x) { return std::fabs(x); }
#endif

template <typename T> inline T Min(T a, T b) { return a < b ? a : b; }
template <typename T> inline T Max(T a, T b) { return a > b ? a : b; }
template <typename T> inline T Clamp(T x, T low, T high) { return x < low ? low : (x > high ? high : x); }

constexpr double kPi = 3.141592653589793;
constexpr double kTwoPi = 6.283185307179586;

// Every pipe runs at this rate whatever the host's: a pipe's tuning and
// regulation (PipeRegulation.h) are measured from the model at this rate, as a
// tuner measures a pipe in its own air, and would not hold at another.
constexpr double kPipeRate = 48000.0;
constexpr double kSoundSpeed = 343.0;   // m/s at 20 C

// White noise in -1..1 from a 32-bit linear congruential generator (Numerical
// Recipes' constants): each pipe owns one, so its turbulence is its own and a
// render is the same every time.
inline float NextNoise(std::uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    return static_cast<float>(static_cast<std::int32_t>(state)) * 4.6566129e-10f;
}

} // namespace oregana
