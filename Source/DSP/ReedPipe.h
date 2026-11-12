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
    float stiffness = 0.0f, damping = 0.0f, closing = 0.0f, landing = 0.0f, landingLoss = 0.0f;
    float flow = 0.0f, curve = 0.1f, noise = 0.0f, drain = 0.0f;
    float attack = 1.0f, release = 1.0f;
    float gainLeft = 0.0f, gainRight = 0.0f;
    double air = 0.0;

    float* loop = nullptr;
    int loopAt = 0;
    float allpassIn = 0.0f, allpassOut = 0.0f, lowpassOut = 0.0f;
    float tongue = 1.0f, speed = 0.0f;       // opening against rest, and its change per sample
    float massFlow = 0.0f, shallot = 0.0f;   // flow into the cone's mass, and the shallot's pressure
    float foot = 0.0f, wind = 0.0f;
    std::uint32_t random = 1u;
    int quiet = 0;

    // `tuning` is the tuner's correction, the tongue's and the resonator's
    // frequencies together against the note: the pipe built that much smaller.
    // Buffer needed: loopLength floats.
    void Design(const ReedVoicing& v, double hz, double tuning) {
        const double w = kTwoPi * hz * tuning * v.length / kPipeRate;
        const double radius = 0.5 * PipeDiameter(v.scale, hz);
        const double corner = Min(kSoundSpeed / (kTwoPi * radius), 0.45 * kPipeRate);
        const double lp = 1.0 - std::exp(-kTwoPi * corner / kPipeRate);
        double lpGain, lpLag;
        detail::OnePole(lp, w, lpGain, lpLag);
        // There and back takes half a period of a cylinder's first resonance.
        // A cone's lies at tan(kL) = -k x0 for a tip x0 beyond the shallot,
        // near a whole period.
        double pass = kPi / w;
        if (v.cone) {
            double u = kPi;
            for (int i = 0; i < 20; ++i) u = kPi - std::atan(u * v.apex);
            pass = 2.0 * u / w;
            mass = static_cast<float>(1.0 / (v.apex * pass));
        } else {
            mass = 0.0f;
        }
        impedance = 1.0f / (1.0f + mass);
        const double rest = pass - lpLag / w;
        loopLength = Max(1, static_cast<int>(std::lround(rest - 1.1)));
        allpass = static_cast<float>(detail::AllpassFor(rest - loopLength, w));
        lowpass = static_cast<float>(lp);
        const double perPeriod = 1.0 - kPi / v.q;
        const double walls = Min((v.cone ? perPeriod : std::sqrt(perPeriod)) / lpGain, 0.9999);
        loopGain = static_cast<float>(-walls);

        const double wt = kTwoPi * hz * tuning * v.tongue / kPipeRate;
        stiffness = static_cast<float>(wt * wt);
        damping = static_cast<float>(wt / v.tongueQ);
        closing = static_cast<float>(v.closing);
        // The shallot's face is some thirty times stiffer than the tongue,
        // as far as the step allows, and takes up most of a landing.
        const double hard = Min(30.0 * wt * wt, 1.0);
        landing = static_cast<float>(hard);
        landingLoss = static_cast<float>(0.6 * std::sqrt(hard));
        flow = static_cast<float>(v.flow);
        curve = static_cast<float>(v.curve);
        noise = static_cast<float>(v.noise);
        attack = static_cast<float>(1.0 - std::exp(-1.0 / (v.speech * (0.0015 + 0.3 / hz) * kPipeRate)));
        drain = static_cast<float>(v.boot) * attack;
        release = static_cast<float>(1.0 - std::exp(-1.0 / ((0.004 + 0.5 / hz) * kPipeRate)));
        air = 2.0 * radius * radius / (0.1555 * 0.1555);
    }

    void SetTouch(double) {}

    void Clear() {
        for (int i = 0; i < loopLength; ++i) loop[i] = 0.0f;
        loopAt = 0;
        allpassIn = allpassOut = lowpassOut = 0.0f;
        tongue = 1.0f;
        speed = massFlow = shallot = 0.0f;
        foot = wind = 0.0f;
        quiet = 0;
    }

    void Tick() {
        const float level = Abs(lowpassOut) + Abs(allpassOut) + Abs(speed) + foot;
        quiet = level < 1.0e-6f ? quiet + 1 : 0;
    }
    bool Silent() const { return wind == 0.0f && quiet > 16; }

    void Process(float* left, float* right, int frames) {
        float* const loopBuffer = loop;
        int at = loopAt;
        float apIn = allpassIn, apOut = allpassOut, lpOut = lowpassOut;
        float y = tongue, v = speed, inMass = massFlow, p = shallot, pressure = foot;
        std::uint32_t seed = random;
        const float target = wind, rise = attack, fall = release;
        for (int i = 0; i < frames; ++i) {
            const float x = loopBuffer[at];
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
            const float k = flow * open * (1.0f + noise * NextNoise(seed));
            const float across = pressure - p0;
            const float kz = k * k * impedance;
            const float size = 0.5f * (Sqrt(kz * kz + 4.0f * k * k * Abs(across)) - kz);
            const float through = across < 0.0f ? -size : size;
            p = p0 + impedance * through;
            // The boot fills through the toe hole and empties through the tongue.
            const float gap = target - pressure;
            pressure += gap * (gap > 0.0f ? rise : fall) - drain * through;
            if (pressure < 0.0f) pressure = 0.0f;
            inMass = held + mass * p;
            loopBuffer[at] = p - returning;
            if (++at == loopLength) at = 0;

            float pull = stiffness * (1.0f - y) - damping * v - stiffness * closing * (pressure - p);
            if (y < 0.0f) pull -= landing * y + landingLoss * v;
            v += pull;
            y += v;

            // What the open end does not reflect, it radiates.
            const float radiated = arrived - lpOut;
            left[i] += gainLeft * radiated;
            right[i] += gainRight * radiated;
        }
        loopAt = at;
        allpassIn = apIn;
        allpassOut = apOut;
        lowpassOut = lpOut;
        tongue = y;
        speed = v;
        massFlow = inMass;
        shallot = p;
        foot = pressure;
        random = seed;
    }
};

} // namespace oregana
