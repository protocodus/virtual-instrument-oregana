#pragma once

// The wind: one reservoir for each division, and the tremulants.
//
// A reservoir is a weighted bellows plate over a wind chest. The weight sets
// the pressure, the pipes take wind by the square root of it (each pipe's
// foot is an orifice), the trunk between them resists, and the plate has to
// move before more wind comes. So the pressure sags a little while many
// pipes speak, dips for a moment when a chord is struck and bounces back
// through a few cycles of the plate on its cushion of air: the "live wind"
// of an organ on bellows, and with it every pipe's pitch and level
// (FluePipe.h, ReedPipe.h).
//
//   flow:      inertia   d(flow)/dt     = 1 - pressure - resistance * flow
//   pressure:  cushion   d(pressure)/dt = flow - demand * sqrt(pressure)
//
// with pressure against the nominal and flow against what the division takes
// at full organ. The three constants are given as what a player hears: the
// sag at full organ, the dip a full chord's attack makes, and the bounce's
// frequency. They are voiced to the accounts of such winding (a sag of a few
// per cent, a bounce of four to seven a second), not measured on any organ.
//
// A tremulant spills wind from the division's trunk once a beat (Tremulant).

#include "DspMath.h"

#include <cmath>

namespace oregana {

struct WindSpec {
    double sag;        // the pressure lost at full organ, held
    double dip;        // and at the attack of a chord that takes that much, before the plate moves
    double bounceHz;   // the plate's bounce on its air
};

struct Reservoir {
    double flow = 0.0, pressure = 1.0;
    double inertia = 0.0, cushion = 0.0, resistance = 0.0;   // per step

    void Design(const WindSpec& spec, double stepSeconds) {
        const double w = kTwoPi * spec.bounceHz;
        inertia = stepSeconds * w / spec.dip;
        cushion = stepSeconds * w * spec.dip;
        resistance = spec.sag;
    }
    void Reset() {
        flow = 0.0;
        pressure = 1.0;
    }
    // One step with the wind the pipes take, `demand` (1 at full organ), and
    // what a tremulant lets out, `spilled`, on the same scale.
    void Step(double demand, double spilled = 0.0) {
        flow += inertia * (1.0 - pressure - resistance * flow);
        pressure += cushion * (flow - demand * Sqrt(Max(pressure, 0.0)) - spilled);
        pressure = Clamp(pressure, 0.0, 2.0);
    }
};

// A tremulant (tremblant fort, a vent perdu): a motor on the division's
// trunk that the wind itself drives, and whose pallet lets a pulse of that
// wind escape once a beat. What reaches the pipes is the winding's answer to
// those pulses, not a shape laid on the pressure. As C. Pykett describes such
// tremulants ("Tremulant simulation in digital organs"), the winding has to
// ring for a tremulant to tell, and the builder tunes its rate to that ring:
// the trunk here answers as its division's reservoir does (Reservoir, the
// plate on its air), so a beat goes deepest where its rate meets the
// reservoir's bounce and its depth follows its rate. The motor runs on the
// trunk's own wind, slower as that falls: its own dips and a chord's attack
// pull each beat about, and the pulse it spills is the wind it has. Depth is
// the pallet's opening, set so that at kRatedHz, with no pipe speaking, the
// pressure falls by `depth` of itself once a beat; Rate is the motor's weight
// and spring. Its constants are those accounts', not measurements.
struct Tremulant {
    static constexpr double kRatedHz = 5.5;   // the Rate control's default

    Reservoir trunk;                     // the division's winding, as the spills move it
    double cosine = 1.0, sine = 0.0;     // where the motor is in its beat
    double turn = 0.0;                   // radians a step, on the nominal wind
    double running = 0.0, start = 0.0;
    double spill = 0.0;                  // its pulse's height at a depth of 1, against full organ
    double spilled = 0.0;                // what it let out this step
    bool beat = false;                   // the pallet has just shut: once a beat

    // The motor takes a quarter of a second to find its beat and to stop.
    // The pulse 0.5 (1 - cos) has a fundamental of half its height, and the
    // trunk turns a flow at w into a pressure of |K2 (jw + K1 R) / (K1 K2 -
    // w^2 + jw K1 R)| (Reservoir's equations, linear, no pipe speaking).
    void Design(const WindSpec& spec, double hz, double stepSeconds) {
        trunk.Design(spec, stepSeconds);
        turn = kTwoPi * hz * stepSeconds;
        start = 1.0 - std::exp(-stepSeconds / 0.25);
        const double bounce = kTwoPi * spec.bounceHz, w = kTwoPi * kRatedHz;
        const double k1 = bounce / spec.dip, k2 = bounce * spec.dip, damping = k1 * spec.sag;
        const double answer = k2 * std::hypot(w, damping) / std::hypot(k1 * k2 - w * w, w * damping);
        spill = 1.0 / answer;
    }
    void Reset() {
        trunk.Reset();
        cosine = 1.0;
        sine = 0.0;
        running = spilled = 0.0;
    }
    // One step of the trunk, with the wind the pipes take (`demand`, 1 at
    // full organ) and what the motor lets out; taken before `reservoir`, its
    // division's, takes the same step. Off, the trunk rings down to the
    // reservoir and is then the reservoir to the bit.
    void Step(bool on, double depth, double demand, const Reservoir& reservoir, double supply = 1.0) {
        running += start * ((on && supply > 0.0 ? 1.0 : 0.0) - running);
        beat = false;
        if (running < 1.0e-6) {
            running = spilled = 0.0;
            cosine = 1.0;
            sine = 0.0;
            if (Abs(trunk.pressure - reservoir.pressure) < 1.0e-9 && Abs(trunk.flow - reservoir.flow) < 1.0e-9) {
                trunk.flow = reservoir.flow;
                trunk.pressure = reservoir.pressure;
            }
            trunk.Step(demand);
            return;
        }
        // The motor is driven by the supply head as well as the division's
        // reservoir pressure, so lowering wind pressure slows it and cutting the
        // wind lets it stop instead of spilling wind on a dead organ.
        const double wind = Sqrt(Max(supply * trunk.pressure, 0.0));
        spilled = running * depth * spill * wind * 0.5 * (1.0 - cosine);
        trunk.Step(demand, spilled);
        // The motor turns as fast as its wind fills it. The step is under
        // 0.04 radians, where these series are exact to 1e-12.
        const double angle = turn * wind, a2 = angle * angle;
        const double turnCos = 1.0 - a2 * (0.5 - a2 * (1.0 / 24.0)), turnSin = angle * (1.0 - a2 * (1.0 / 6.0 - a2 * (1.0 / 120.0)));
        const double c = cosine * turnCos - sine * turnSin, s = sine * turnCos + cosine * turnSin;
        const double norm = 1.5 - 0.5 * (c * c + s * s);
        beat = sine < 0.0 && s >= 0.0;
        cosine = c * norm;
        sine = s * norm;
    }
};

} // namespace oregana
