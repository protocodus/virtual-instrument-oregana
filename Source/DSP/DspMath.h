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

// Four pipes to an instruction (FluePipe.h, ReedPipe.h). The GNU vector types
// survive the Rack toolchain, which vectorizes nothing by itself, as NEON and
// SSE2. They are used for locals only: a member of this type would be read
// with aligned loads the heap does not promise. Each lane does a pipe's own
// arithmetic in its own order, so four pipes together give the bits of four
// pipes apart.
#if defined(__GNUC__) || defined(__clang__)
#define OREGANA_FOUR 1
typedef float Float4 __attribute__((vector_size(16)));
typedef std::int32_t Int4 __attribute__((vector_size(16)));
typedef std::uint32_t UInt4 __attribute__((vector_size(16)));
inline Float4 Four(float x) { return Float4 { x, x, x, x }; }
inline Float4 ToFloat(Int4 x) { return __builtin_convertvector(x, Float4); }
inline Int4 Truncated(Float4 x) { return __builtin_convertvector(x, Int4); }
// a where the comparison held, else b.
inline Float4 Choose(Int4 held, Float4 a, Float4 b) { return (Float4)((held & (Int4)a) | (~held & (Int4)b)); }
// a where the comparison held, else 0; and a where it did not.
inline Float4 Where(Int4 held, Float4 a) { return (Float4)(held & (Int4)a); }
inline Float4 Unless(Int4 held, Float4 a) { return (Float4)(~held & (Int4)a); }
inline Float4 Abs(Float4 x) { return (Float4)((Int4)x & Int4 { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff }); }
inline Float4 Sqrt(Float4 x) {
#if defined(OREGANA_ELEMENTWISE_SQRT)
    return __builtin_elementwise_sqrt(x);
#else
    return Float4 { Sqrt(x[0]), Sqrt(x[1]), Sqrt(x[2]), Sqrt(x[3]) };
#endif
}
// Unaligned 16-byte storage for cached SIMD constant packs inside heap objects.
struct PackedFloat4 {
    float lane[4];
};
inline Float4 Load4(const float* p) {
    Float4 v;
    __builtin_memcpy(&v, p, sizeof v);
    return v;
}
inline Float4 Load4(const PackedFloat4& p) { return Load4(p.lane); }
inline void Store4(float* p, Float4 v) { __builtin_memcpy(p, &v, sizeof v); }
inline void Store4(PackedFloat4& p, Float4 v) { Store4(p.lane, v); }
inline bool AnyLane(Int4 mask) {
    std::uint64_t bits[2];
    __builtin_memcpy(bits, &mask, sizeof bits);
    return (bits[0] | bits[1]) != 0;
}
inline void Transpose4x4(Float4& a, Float4& b, Float4& c, Float4& d) {
#if defined(__has_builtin) && __has_builtin(__builtin_shufflevector)
    const Float4 t0 = __builtin_shufflevector(a, b, 0, 4, 1, 5);
    const Float4 t1 = __builtin_shufflevector(a, b, 2, 6, 3, 7);
    const Float4 t2 = __builtin_shufflevector(c, d, 0, 4, 1, 5);
    const Float4 t3 = __builtin_shufflevector(c, d, 2, 6, 3, 7);
    a = __builtin_shufflevector(t0, t2, 0, 1, 4, 5);
    b = __builtin_shufflevector(t0, t2, 2, 3, 6, 7);
    c = __builtin_shufflevector(t1, t3, 0, 1, 4, 5);
    d = __builtin_shufflevector(t1, t3, 2, 3, 6, 7);
#else
    const Float4 r0 { a[0], b[0], c[0], d[0] }, r1 { a[1], b[1], c[1], d[1] };
    const Float4 r2 { a[2], b[2], c[2], d[2] }, r3 { a[3], b[3], c[3], d[3] };
    a = r0;
    b = r1;
    c = r2;
    d = r3;
#endif
}
#endif

// White noise in -1..1 from a 32-bit linear congruential generator (Numerical
// Recipes' constants): each pipe owns one, so its turbulence is its own and a
// render is the same every time.
inline float NextNoise(std::uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    return static_cast<float>(static_cast<std::int32_t>(state)) * 4.6566129e-10f;
}
#if defined(OREGANA_FOUR)
inline Float4 NextNoise(UInt4& state) {
    state = state * 1664525u + 1013904223u;
    return ToFloat((Int4)state) * 4.6566129e-10f;
}
#endif

} // namespace oregana
