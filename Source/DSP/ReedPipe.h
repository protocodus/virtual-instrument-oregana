#pragma once

// A reed pipe: a brass tongue beating against the opening of the shallot, the
// wind it chops, and the resonator above.
//
// The tongue is a sprung mass at rest a little open. The boot's pressure,
// less the shallot's, pushes it shut; where it meets the shallot it lands on
// a much stiffer spring. Its curve closes the opening gradually, which the
// opening law's soft start stands for (a flat tongue shuts at once and sounds
// brassy, a curved one sounds round). Wind flows through the opening by
// Bernoulli's law into the shallot, where the resonator answers with its own
// pressure, so the flow, the tongue and the resonator are solved together
// each sample. The tongue, not the resonator, sets the pitch, as in a real
// reed pipe, and the tuner's correction (PipeRegulation.h) is its stiffness,
// the tuning wire.
//
// The resonator is a travelling-wave loop closed at the shallot and open at
// its far end (one-pole radiation loss, as FluePipe.h). A cylinder resonates
// on odd harmonics a quarter-wave long. A cone adds the mass of air its
// missing tip would hold, in parallel at the shallot, which moves the
// resonances to every harmonic of a pipe open at both ends (Fletcher and
// Rossing, section 8.7). A reed speaks while that load is mass-like at the
// note, which is why organ builders cut resonators a little short.

#include "DspMath.h"
#include "FluePipe.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace oregana {

struct ReedVoicing {
    bool cone = true;        // conical resonator, else cylindrical
    double length = 1.06;    // the resonator's first resonance against the note
    double apex = 0.15;      // a cone's missing tip, in resonator lengths
    double scale = 0.0;      // the open end's diameter, halftones against Normalmensur
    double q = 25.0;         // Q of the resonator's first resonance
    double flow = 0.45;      // the open tongue's flow at nominal wind, against the resonator's wave impedance
    double closing = 0.9;    // the wind's static push on the tongue: 1 holds it just shut
    double curve = 0.2;      // the tongue's curvature: the opening over which it closes softly
    double tongue = 1.03;    // the tongue's own frequency against the note
    double tongueQ = 40.0;   // the tongue's own damping
    double boot = 0.2;       // the toe hole's resistance: how far the open tongue's flow pulls the boot's pressure down
    double noise = 0.01;     // turbulence in the opening
    double speech = 1.0;     // how slowly the boot fills
};

struct ReedPipe {
    int loopLength = 1;
    float allpass = 0.0f, lowpass = 1.0f, loopGain = 0.0f;
    float mass = 0.0f, impedance = 1.0f;   // the cone's parallel mass (T / 2M) and the shallot's impedance with it
    float stiffness = 0.0f, damping = 0.0f, freeNum = 1.0f, freeDen = 1.0f, closing = 0.0f, landing = 0.0f, landingLoss = 0.0f;
    float flow = 0.0f, curve = 0.1f, noise = 0.0f, drain = 0.0f, bootDrop = 0.2f;
    float attack = 1.0f, release = 1.0f;
    float gainLeft = 0.0f, gainRight = 0.0f;
    double air = 0.0;
    double attackSeconds = 0.008;
    double palletSeconds = 0.007, bootSeconds = 0.008, releaseSeconds = 0.008;
    float kickNominal = 0.0f, kick = 0.0f, noiseNominal = 0.0f;

    static constexpr float kRoll = 0.22f, kInvFourRoll = 1.0f / (4.0f * kRoll);

    float* loop = nullptr;
    int loopAt = 0;
    // Eight dynamic per-sample states kept contiguous for 4-lane state transpose.
    float allpassIn = 0.0f, allpassOut = 0.0f, lowpassOut = 0.0f, tongue = 1.0f;
    float speed = 0.0f, massFlow = 0.0f, shallot = 0.0f, foot = 0.0f;
    float wind = 0.0f;
    float fill = 0.0f;
    float radiates = 1.0f;
    std::uint32_t random = 1u;
    int quiet = 0;

    // `tuning` is the tuner's correction on the tongue wire against the note:
    // the resonator is cut by the builder for the note alone, and its open-end
    // lowpass and allpass lag are taken at the sounding frequency `hz`.
    // Buffer needed: loopLength floats.
    void Design(const ReedVoicing& v, double hz, double tuning) {
        const double wNote = kTwoPi * hz / kPipeRate, wRes = kTwoPi * hz * v.length / kPipeRate;
        const double radius = 0.5 * PipeDiameter(v.scale, hz);
        const double corner = Min(kSoundSpeed / (kTwoPi * radius), 0.45 * kPipeRate);
        const double lp = 1.0 - std::exp(-kTwoPi * corner / kPipeRate);
        double lpGain, lpLag;
        detail::OnePole(lp, wNote, lpGain, lpLag);
        // There and back takes half a period of a cylinder's first resonance.
        // A cone's lies at tan(kL) = -k x0 for a tip x0 beyond the shallot,
        // near a whole period.
        double pass = kPi / wRes;
        if (v.cone) {
            double u = kPi;
            for (int i = 0; i < 20; ++i) u = kPi - std::atan(u * v.apex);
            pass = 2.0 * u / wRes;
            mass = static_cast<float>(1.0 / (v.apex * pass));
        } else {
            mass = 0.0f;
        }
        impedance = 1.0f / (1.0f + mass);
        const double rest = pass - lpLag / wNote;
        loopLength = Max(1, static_cast<int>(std::lround(rest - 1.1)));
        allpass = static_cast<float>(detail::AllpassFor(rest - loopLength, wNote));
        lowpass = static_cast<float>(lp);
        const double perPeriod = 1.0 - kPi / v.q;
        const double walls = Min((v.cone ? perPeriod : std::sqrt(perPeriod)) / lpGain, 0.9999);
        loopGain = static_cast<float>(-walls);
        // What the open end radiates of the wave inside, at the note: all
        // that its low-pass does not turn back (Run, `out`).
        radiates = static_cast<float>(std::sqrt(1.0 + lpGain * lpGain - 2.0 * lpGain * std::cos(lpLag)));

        // Frequency-warped symplectic stiffness and contact stiffness for the
        // tuning wire and the curved shallot face.
        const double wt = kTwoPi * hz * tuning * v.tongue / kPipeRate;
        const double sHalf = std::sin(0.5 * wt);
        const double stiff = 4.0 * sHalf * sHalf;
        const double sHard = std::sin(0.5 * Min(2.5 * wt, 1.4));
        const double hard = Max(0.0, 4.0 * sHard * sHard - stiff);
        stiffness = static_cast<float>(stiff);
        damping = static_cast<float>(2.0 * sHalf / v.tongueQ);
        freeNum = 1.0f - 0.5f * damping;
        freeDen = 1.0f + 0.5f * damping;
        closing = static_cast<float>(v.closing);
        landing = static_cast<float>(hard);
        landingLoss = static_cast<float>(hard / (2.0 * sHalf));
        flow = static_cast<float>(v.flow);
        curve = static_cast<float>(v.curve);
        noiseNominal = static_cast<float>(v.noise);
        noise = noiseNominal;
        bootDrop = static_cast<float>(v.boot);
        palletSeconds = 0.007;
        bootSeconds = v.speech * (0.004 + 0.25 / hz);
        attackSeconds = palletSeconds + bootSeconds;
        releaseSeconds = 0.004 + 0.5 / hz;
        kickNominal = v.cone ? 0.72f : 0.56f;
        SetTouch(1.0);
        SetRelease(1.0);
        air = 2.0 * radius * radius / (0.1555 * 0.1555);
    }

    // How fast the key opens the pallet: keep steady-state boot pressure drop
    // (drain / attack == bootDrop) independent of key velocity.
    void SetTouch(double slowness) {
        const double s = Clamp(slowness, 0.2, 8.0);
        const double riseSeconds = palletSeconds * s + bootSeconds * (0.35 + 0.65 * s);
        attack = static_cast<float>(1.0 - std::exp(-1.0 / (riseSeconds * kPipeRate)));
        drain = bootDrop * attack;
        const double touchSpeed = Clamp(std::pow(1.0 / s, 0.80), 0.15, 1.75);
        kick = static_cast<float>(kickNominal * touchSpeed);
    }
    void SetRelease(double slowness) {
        const double fallSeconds = releaseSeconds * Clamp(slowness, 0.35, 4.0);
        release = static_cast<float>(1.0 - std::exp(-1.0 / (fallSeconds * kPipeRate)));
    }
    void SetBreath(float scale) { noise = noiseNominal * scale; }

    void Clear() {
        for (int i = 0; i < loopLength; ++i) loop[i] = 0.0f;
        loopAt = 0;
        allpassIn = allpassOut = lowpassOut = 0.0f;
        tongue = 1.0f;
        speed = massFlow = shallot = 0.0f;
        foot = wind = fill = 0.0f;
        quiet = 0;
    }

    void Tick() {
        // As a flue's (FluePipe::Tick).
        if (wind == 0.0f && foot < 1.0e-25f) foot = 0.0f;
        if (Abs(wind - fill) < 1.0e-20f) fill = wind;
        // A reed is judged by what it radiates. A bass resonator turns
        // nearly all of its wave back, so the wave inside outlasts the sound
        // by seconds: a 16' trumpet was kept 7.6 s, its sound 116 dB down.
        // The tongue, swinging on in an empty boot, makes none.
        const float level = radiates * (Abs(lowpassOut) + Abs(allpassOut)) + foot;
        quiet = level < FluePipe::kPipeSilent ? quiet + 1 : 0;
    }
    bool Silent() const { return wind == 0.0f && quiet > 16; }

    // Adds `frames` samples of the pipe's sound to left and right.
    void Process(float* left, float* right, int frames) {
        float sound[64];
        for (int done = 0; done < frames;) {
            const int n = Min(frames - done, 64);
            Run(sound, n);
            for (int i = 0; i < n; ++i) {
                left[done + i] += gainLeft * sound[i];
                right[done + i] += gainRight * sound[i];
            }
            done += n;
        }
    }

    // `frames` samples of the pipe's sound, before its gains.
    void Run(float* out, int frames) {
        float* const loopBuffer = loop;
        float* pLoop = loopBuffer + loopAt;
        float* const pEnd = loopBuffer + loopLength;
        float apIn = allpassIn, apOut = allpassOut, lpOut = lowpassOut;
        float y = tongue, v = speed, inMass = massFlow, p = shallot, pressure = foot, filled = fill;
        std::uint32_t seed = random;
        const float target = wind, rise = attack, fall = release, kickC = kick;
        for (int i = 0; i < frames; ++i) {
            const float x = *pLoop;
            const float ap = allpass * (x - apOut) + apIn;
            apIn = x;
            apOut = ap;
            const float arrived = ap;
            lpOut += lowpass * (ap - lpOut);
            const float returning = loopGain * lpOut;

            // The shallot's pressure is p0 + impedance * flow, with the wave
            // coming back and the cone's mass (trapezoidal) in p0, and the
            // flow is opening * sqrt(boot - shallot): a quadratic in the flow.
            const float held = inMass + mass * p;
            const float p0 = impedance * (2.0f * returning - held);
            const float open = y > 0.0f ? y * y / (y + curve) : 0.0f;
            const float white = NextNoise(seed);
            const float k = flow * open * (1.0f + noise * white);
            const float across = pressure - p0;
            const float kz = k * k * impedance;
            const float size = 0.5f * (Sqrt(kz * kz + 4.0f * k * k * Abs(across)) - kz);
            const float through = across < 0.0f ? -size : size;
            p = p0 + impedance * through;
            // The boot fills through the toe hole and empties through the tongue.
            const float gap = target - pressure;
            pressure += gap * (gap > 0.0f ? rise : fall) - drain * through;
            if (pressure < 0.0f) pressure = 0.0f;
            const float fillGap = target - filled;
            filled += fillGap * (fillGap > 0.0f ? rise : fall);
            const float span = 0.80f * target - filled;
            const float onset = (fillGap > 0.0f && span > 0.0f) ? kickC * span : 0.0f;
            inMass = held + mass * p;
            *pLoop = p - returning;
            if (++pLoop == pEnd) pLoop = loopBuffer;

            // Smooth curved-shallot rolling contact over [-kRoll, +kRoll] with
            // time-centered velocity damping avoids discrete integer-sample
            // period locking and keeps pitch independent of sample phase.
            const float pullFree = stiffness * (1.0f - y) - stiffness * (closing * (pressure - p) + onset);
            const float vFree = (v * freeNum + pullFree) / freeDen;
            if (y < kRoll) {
                const float spanRoll = kRoll - y;
                const float d = y <= -kRoll ? -y : spanRoll * spanRoll * kInvFourRoll;
                const float cAll = damping + d * landingLoss;
                const float vHit = (v * (1.0f - 0.5f * cAll) + pullFree + d * landing) / (1.0f + 0.5f * cAll);
                v = vHit > vFree ? vHit : vFree;
            } else {
                v = vFree;
            }
            y += v;

            // What the open end does not reflect, it radiates.
            out[i] = arrived - lpOut;
        }
        loopAt = static_cast<int>(pLoop - loopBuffer);
        allpassIn = apIn;
        allpassOut = apOut;
        lowpassOut = lpOut;
        tongue = y;
        speed = v;
        massFlow = inMass;
        shallot = p;
        foot = pressure;
        fill = filled;
        random = seed;
    }

#if defined(OREGANA_FOUR)
    // Pre-packed 4-pipe parameter block stored in the Engine (8-byte safe, unaligned).
    struct ReedPack {
        ReedPipe* pipe[4] {};
        float* loopBuffer[4] {};
        int loopSize[4] {};
        PackedFloat4 allpass, lowpass, loopGain, mass, impedance, stiffness;
        PackedFloat4 damping, freeNum, freeDen, closing, landing, landingLoss;
        PackedFloat4 flow, curve, noise, drain, kick, attack, release;
    };

#if defined(__GNUC__) || defined(__clang__)
    __attribute__((noinline))
#endif
    static void PackFour(ReedPack& pack, ReedPipe* const pipe[4]) {
#define OREGANA_PACK_FIELD(field) \
    Store4(pack.field, Float4 { pipe[0]->field, pipe[1]->field, pipe[2]->field, pipe[3]->field })
        for (int l = 0; l < 4; ++l) {
            pack.pipe[l] = pipe[l];
            pack.loopBuffer[l] = pipe[l]->loop;
            pack.loopSize[l] = pipe[l]->loopLength;
        }
        OREGANA_PACK_FIELD(allpass);
        OREGANA_PACK_FIELD(lowpass);
        OREGANA_PACK_FIELD(loopGain);
        OREGANA_PACK_FIELD(mass);
        OREGANA_PACK_FIELD(impedance);
        OREGANA_PACK_FIELD(stiffness);
        OREGANA_PACK_FIELD(damping);
        OREGANA_PACK_FIELD(freeNum);
        OREGANA_PACK_FIELD(freeDen);
        OREGANA_PACK_FIELD(closing);
        OREGANA_PACK_FIELD(landing);
        OREGANA_PACK_FIELD(landingLoss);
        OREGANA_PACK_FIELD(flow);
        OREGANA_PACK_FIELD(curve);
        OREGANA_PACK_FIELD(noise);
        OREGANA_PACK_FIELD(drain);
        OREGANA_PACK_FIELD(kick);
        OREGANA_PACK_FIELD(attack);
        OREGANA_PACK_FIELD(release);
#undef OREGANA_PACK_FIELD
    }

    static void RunFourPacked(const ReedPack& pack, float* const out[4], int frames) {
        const Float4 allpassC = Load4(pack.allpass), lowpassC = Load4(pack.lowpass), loopGainC = Load4(pack.loopGain);
        const Float4 massC = Load4(pack.mass), impedanceC = Load4(pack.impedance), stiffnessC = Load4(pack.stiffness);
        const Float4 dampingC = Load4(pack.damping), freeNumC = Load4(pack.freeNum), freeDenC = Load4(pack.freeDen);
        const Float4 closingC = Load4(pack.closing), landingC = Load4(pack.landing), landingLossC = Load4(pack.landingLoss);
        const Float4 flowC = Load4(pack.flow), curveC = Load4(pack.curve), noiseC = Load4(pack.noise), drainC = Load4(pack.drain);
        const Float4 kickC = Load4(pack.kick), rise = Load4(pack.attack), fall = Load4(pack.release);
        ReedPipe* const p0Pipe = pack.pipe[0];
        ReedPipe* const p1Pipe = pack.pipe[1];
        ReedPipe* const p2Pipe = pack.pipe[2];
        ReedPipe* const p3Pipe = pack.pipe[3];
        const Float4 target { p0Pipe->wind, p1Pipe->wind, p2Pipe->wind, p3Pipe->wind };
        Float4 filled { p0Pipe->fill, p1Pipe->fill, p2Pipe->fill, p3Pipe->fill };

        Float4 apIn = Load4(&p0Pipe->allpassIn), apOut = Load4(&p1Pipe->allpassIn);
        Float4 lpOut = Load4(&p2Pipe->allpassIn), y = Load4(&p3Pipe->allpassIn);
        Transpose4x4(apIn, apOut, lpOut, y);

        Float4 v = Load4(&p0Pipe->speed), inMass = Load4(&p1Pipe->speed);
        Float4 p = Load4(&p2Pipe->speed), pressure = Load4(&p3Pipe->speed);
        Transpose4x4(v, inMass, p, pressure);

        const Float4 zero = Four(0.0f), half = Four(0.5f), eightTenths = Four(0.80f), one = Four(1.0f);
        const Float4 roll = Four(kRoll), invFourRoll = Four(kInvFourRoll);
        float* const pBase[4] = { pack.loopBuffer[0], pack.loopBuffer[1], pack.loopBuffer[2], pack.loopBuffer[3] };
        float* pLoop[4] = {
            pBase[0] + p0Pipe->loopAt, pBase[1] + p1Pipe->loopAt, pBase[2] + p2Pipe->loopAt, pBase[3] + p3Pipe->loopAt
        };
        float* const pEnd[4] = {
            pBase[0] + pack.loopSize[0], pBase[1] + pack.loopSize[1], pBase[2] + pack.loopSize[2], pBase[3] + pack.loopSize[3]
        };
        UInt4 seed = { p0Pipe->random, p1Pipe->random, p2Pipe->random, p3Pipe->random };

        const auto step = [&]()
#if defined(__GNUC__) || defined(__clang__)
                              __attribute__((always_inline))
#endif
            -> Float4 {
            const Float4 x = { *pLoop[0], *pLoop[1], *pLoop[2], *pLoop[3] };
            const Float4 ap = allpassC * (x - apOut) + apIn;
            apIn = x;
            apOut = ap;
            lpOut += lowpassC * (ap - lpOut);
            const Float4 returning = loopGainC * lpOut;

            const Float4 held = inMass + massC * p;
            const Float4 p0 = impedanceC * (2.0f * returning - held);
            const Float4 open = Where(y > zero, y * y / (y + curveC));
            const Float4 white = NextNoise(seed);
            const Float4 k = flowC * open * (1.0f + noiseC * white);
            const Float4 across = pressure - p0;
            const Float4 kz = k * k * impedanceC;
            const Float4 size = 0.5f * (Sqrt(kz * kz + 4.0f * k * k * Abs(across)) - kz);
            const Float4 through = Choose(across < zero, -size, size);
            p = p0 + impedanceC * through;
            const Float4 gap = target - pressure;
            pressure += gap * Choose(gap > zero, rise, fall) - drainC * through;
            pressure = Unless(pressure < zero, pressure);
            const Float4 fillGap = target - filled;
            filled += fillGap * Choose(fillGap > zero, rise, fall);
            const Float4 spanRoll = eightTenths * target - filled;
            const Float4 onset = Where(fillGap > zero, Where(spanRoll > zero, kickC * spanRoll));
            inMass = held + massC * p;
            const Float4 leaving = p - returning;
            for (int l = 0; l < 4; ++l) {
                *pLoop[l] = leaving[l];
                if (++pLoop[l] == pEnd[l]) pLoop[l] = pBase[l];
            }

            const Float4 pullFree = stiffnessC * (one - y) - stiffnessC * (closingC * (pressure - p) + onset);
            const Float4 vFree = (v * freeNumC + pullFree) / freeDenC;
            if (AnyLane(y < roll)) {
                const Float4 span = roll - y;
                const Float4 d = Choose(y >= roll, zero, Choose(y <= -roll, -y, span * span * invFourRoll));
                const Float4 cAll = dampingC + d * landingLossC;
                const Float4 vHit = (v * (one - half * cAll) + pullFree + d * landingC) / (one + half * cAll);
                v = Choose(vHit > vFree, vHit, vFree);
            } else {
                v = vFree;
            }
            y += v;

            return ap - lpOut;
        };

        int i = 0;
        for (; i + 4 <= frames; i += 4) {
            Float4 r0 = step();
            Float4 r1 = step();
            Float4 r2 = step();
            Float4 r3 = step();
            Transpose4x4(r0, r1, r2, r3);
            Store4(out[0] + i, r0);
            Store4(out[1] + i, r1);
            Store4(out[2] + i, r2);
            Store4(out[3] + i, r3);
        }
        for (; i < frames; ++i) {
            const Float4 radiated = step();
            out[0][i] = radiated[0];
            out[1][i] = radiated[1];
            out[2][i] = radiated[2];
            out[3][i] = radiated[3];
        }

        Transpose4x4(apIn, apOut, lpOut, y);
        Store4(&p0Pipe->allpassIn, apIn);
        Store4(&p1Pipe->allpassIn, apOut);
        Store4(&p2Pipe->allpassIn, lpOut);
        Store4(&p3Pipe->allpassIn, y);

        Transpose4x4(v, inMass, p, pressure);
        Store4(&p0Pipe->speed, v);
        Store4(&p1Pipe->speed, inMass);
        Store4(&p2Pipe->speed, p);
        Store4(&p3Pipe->speed, pressure);

        for (int l = 0; l < 4; ++l) {
            ReedPipe& r = *pack.pipe[l];
            r.loopAt = static_cast<int>(pLoop[l] - pBase[l]);
            r.fill = filled[l];
            r.random = seed[l];
        }
    }

    // Run for four pipes at once, lane for lane the same arithmetic.
    static void RunFour(ReedPipe* const pipe[4], float* const out[4], int frames) {
        ReedPack pack;
        PackFour(pack, pipe);
        RunFourPacked(pack, out, frames);
    }
#endif
};

static_assert(offsetof(ReedPipe, foot) - offsetof(ReedPipe, allpassIn) == 7 * sizeof(float),
              "ReedPipe per-sample state must be contiguous");

} // namespace oregana
