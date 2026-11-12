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
// A tremulant is a small sprung valve on the trunk that lets wind out in
// pulses: here the pressure it leaves, dipping `depth` of the way once a
// beat.

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
    // One step with the wind the pipes take: `demand` is 1 at full organ.
    void Step(double demand) {
        flow += inertia * (1.0 - pressure - resistance * flow);
        pressure += cushion * (flow - demand * Sqrt(Max(pressure, 0.0)));
        pressure = Clamp(pressure, 0.0, 2.0);
    }
};

struct Tremulant {
    double cosine = 1.0, sine = 0.0;   // where the valve is in its beat
    double turnCos = 1.0, turnSin = 0.0;
    double running = 0.0, start = 0.0;
    bool beat = false;   // the valve has just shut: once a beat

    // The valve takes a quarter of a second to find its beat and to stop.
    void Design(double hz, double stepSeconds) {
        const double step = kTwoPi * hz * stepSeconds;
        turnCos = std::cos(step);
        turnSin = std::sin(step);
        start = 1.0 - std::exp(-stepSeconds / 0.25);
    }
    void Reset() {
        cosine = 1.0;
        sine = 0.0;
        running = 0.0;
    }
    // The share of the pressure it leaves this step.
    double Step(bool on, double depth) {
        running += start * ((on ? 1.0 : 0.0) - running);
        beat = false;
        if (running < 1.0e-6) {
            running = 0.0;
            cosine = 1.0;
            sine = 0.0;
            return 1.0;
        }
        const double c = cosine * turnCos - sine * turnSin, s = sine * turnCos + cosine * turnSin;
        const double norm = 1.5 - 0.5 * (c * c + s * s);
        beat = sine < 0.0 && s >= 0.0;
        cosine = c * norm;
        sine = s * norm;
        return 1.0 - running * depth * 0.5 * (1.0 - cosine);
    }
};

} // namespace oregana
