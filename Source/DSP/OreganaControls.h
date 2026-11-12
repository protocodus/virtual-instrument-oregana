#pragma once

// Oregana's player controls, shared by every front end: the property or
// parameter id, the range in physical units, the default and the mapping from
// a normalised 0..1 position. Front ends store and automate the normalised
// positions; the engine takes physical values (SetControls).
//
// The first kStopCount controls are the stop knobs, in kStops' order
// (Stops.h), each with its stop's id and name. The rest follow in the order
// below. Never rename or reuse an id, and append new controls at the end:
// sessions and patches recall by them.

#include "DspMath.h"
#include "Stops.h"
#include "Temperament.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace oregana {

enum Control : int {
    kFirstStop = 0,                 // kStopCount stop knobs: 0 in, 1 drawn
    kCouplerPosHw = kStopCount,     // Positiv to Hauptwerk (II/I)
    kCouplerRecHw,                  // Recit to Hauptwerk (III/I)
    kCouplerRecPos,                 // Recit to Positiv (III/II)
    kCouplerHwPed,                  // Hauptwerk to Pedal (I/P)
    kCouplerPosPed,                 // Positiv to Pedal (II/P)
    kCouplerRecPed,                 // Recit to Pedal (III/P)
    kTremulantPos,                  // the Positiv's tremulant
    kTremulantRec,                  // the Recit's tremulant
    kTremulantRate,                 // beats a second, both
    kTremulantDepth,                // how far each beat lets the wind fall, per cent of the pressure
    kSwell,                         // the Recit's swell box: 0 shut .. 1 open (also MIDI CC11)
    kCrescendo,                     // the crescendo roller: 0 only what is drawn .. 1 full organ (also MIDI CC1)
    kTemperament,                   // kTemperaments (Temperament.h)
    kPitch,                         // a1 with the church at 20 C, Hz
    kTuning,                        // how far the pipes have drifted since the tuner left: 0 none .. 1 badly (Imperfections, OreganaEngine.h)
    kTemperature,                   // the church's air against the 20 C the organ was tuned at, C: flues follow it, reeds do not
    kVoicing,                       // how unevenly the pipes of a rank are voiced: 0 all alike .. 1 roughly
    kWind,                          // how live the wind is: 0 a steady supply .. 0.5 as modelled (Wind.h) .. 1 twice as unsteady
    kBlower,                        // the blower's hum and the chests' hiss, against an estimate
    kAction,                        // the keys', pallets' and stop knobs' knocks, against an estimate
    kDeadPipes,                     // pipes that no longer speak: 0 none .. 1 one in sixteen
    kCiphers,                       // pallets that hang open when their key is let go: 0 none .. 1 one key in twenty-five
    kRunnings,                      // wind leaking from a key's channel into its neighbour's: 0 none .. 1 one channel in twelve
    kVolume,                        // output level, dB
    kRoom,                          // where the listener is: 0 at the pipes (no church) .. 1 far down the nave (only the church)
    kDecay,                         // the church's reverberation time at low frequencies, s
    kWidth,                         // 0 one ear .. 1 the organ's breadth and the church around both ears
    kControlCount
};

constexpr int kCouplerCount = 6;

enum class Curve { kLinear, kLog, kStepped };

struct ControlInfo {
    const char* id;        // stable parameter / property id
    const char* name;
    double min, max, initial;
    int steps;             // > 0 for a switch: values 0 .. steps - 1
    Curve curve;
    const char* unit;
};

namespace detail {

constexpr std::array<ControlInfo, kControlCount> MakeControlInfo() {
    std::array<ControlInfo, kControlCount> info {};
    for (int i = 0; i < kStopCount; ++i)
        info[static_cast<std::size_t>(i)] = { kStops[i].id, kStops[i].name, 0.0, 1.0, 0.0, 2, Curve::kStepped, "" };
    const ControlInfo rest[] = {
        { "coupler_pos_hw", "Positiv to Hauptwerk", 0.0, 1.0, 0.0, 2, Curve::kStepped, "" },
        { "coupler_rec_hw", "Recit to Hauptwerk", 0.0, 1.0, 0.0, 2, Curve::kStepped, "" },
        { "coupler_rec_pos", "Recit to Positiv", 0.0, 1.0, 0.0, 2, Curve::kStepped, "" },
        { "coupler_hw_ped", "Hauptwerk to Pedal", 0.0, 1.0, 0.0, 2, Curve::kStepped, "" },
        { "coupler_pos_ped", "Positiv to Pedal", 0.0, 1.0, 0.0, 2, Curve::kStepped, "" },
        { "coupler_rec_ped", "Recit to Pedal", 0.0, 1.0, 0.0, 2, Curve::kStepped, "" },
        { "tremulant_pos", "Tremulant Positiv", 0.0, 1.0, 0.0, 2, Curve::kStepped, "" },
        { "tremulant_rec", "Tremulant Recit", 0.0, 1.0, 0.0, 2, Curve::kStepped, "" },
        // Voiced: a tremblant doux beats five or six times a second. At 10 %
        // a flue swings by 5.5 cents and 0.8 dB (0.55 cents and 0.08 dB for
        // each 1 % of pressure, measured on the regulated pipes at 0.94 wind).
        { "tremulant_rate", "Tremulant Rate", 3.0, 8.0, 5.5, 0, Curve::kLinear, "Hz" },
        { "tremulant_depth", "Tremulant Depth", 0.0, 20.0, 10.0, 0, Curve::kLinear, "%" },
        { "swell", "Swell", 0.0, 1.0, 1.0, 0, Curve::kLinear, "" },
        { "crescendo", "Crescendo", 0.0, 1.0, 0.0, 0, Curve::kLinear, "" },
        { "temperament", "Temperament", 0.0, static_cast<double>(kTemperaments.size() - 1), 0.0,
          static_cast<int>(kTemperaments.size()), Curve::kStepped, "" },
        { "pitch", "Pitch", 415.0, 466.0, 440.0, 0, Curve::kLog, "Hz" },
        // A quarter: an organ a season after its tuning, flues within 2.5
        // cents and reeds within 5.
        { "tuning", "Tuning", 0.0, 1.0, 0.25, 0, Curve::kLinear, "" },
        { "temperature", "Temperature", -10.0, 10.0, 0.0, 0, Curve::kLinear, "C" },
        { "voicing", "Voicing", 0.0, 1.0, 0.3, 0, Curve::kLinear, "" },
        { "wind", "Wind", 0.0, 1.0, 0.5, 0, Curve::kLinear, "" },
        { "blower", "Blower", 0.0, 1.0, 0.2, 0, Curve::kLinear, "" },
        { "action", "Action", 0.0, 1.0, 0.25, 0, Curve::kLinear, "" },
        { "dead_pipes", "Dead Pipes", 0.0, 1.0, 0.0, 0, Curve::kLinear, "" },
        { "ciphers", "Ciphers", 0.0, 1.0, 0.0, 0, Curve::kLinear, "" },
        { "runnings", "Runnings", 0.0, 1.0, 0.0, 0, Curve::kLinear, "" },
        { "volume", "Volume", -24.0, 24.0, 0.0, 0, Curve::kLinear, "dB" },
        { "room", "Room", 0.0, 1.0, 0.6, 0, Curve::kLinear, "" },
        { "decay", "Decay", 1.5, 9.0, 4.5, 0, Curve::kLog, "s" },
        { "width", "Width", 0.0, 1.0, 0.8, 0, Curve::kLinear, "" },
    };
    for (int i = kStopCount; i < kControlCount; ++i) info[static_cast<std::size_t>(i)] = rest[i - kStopCount];
    return info;
}

} // namespace detail

inline constexpr std::array<ControlInfo, kControlCount> kControlInfo = detail::MakeControlInfo();

// Physical value of control `index` at normalised position n (clamped to 0..1).
inline double FromNormalized(int index, double n) {
    const ControlInfo& c = kControlInfo[static_cast<std::size_t>(index)];
    n = n == n ? Clamp(n, 0.0, 1.0) : 0.0;
    switch (c.curve) {
    case Curve::kStepped: return std::floor(n * (c.steps - 1) + 0.5);
    case Curve::kLog: return c.min * std::pow(c.max / c.min, n);
    case Curve::kLinear: break;
    }
    return c.min + (c.max - c.min) * n;
}

inline double ToNormalized(int index, double value) {
    const ControlInfo& c = kControlInfo[static_cast<std::size_t>(index)];
    value = value == value ? Clamp(value, c.min, c.max) : c.initial;
    if (c.curve == Curve::kLog) return std::log(value / c.min) / std::log(c.max / c.min);
    return (value - c.min) / (c.max - c.min);
}

// Physical values of every control, in Control order.
struct Controls {
    std::array<double, kControlCount> value {};
    constexpr Controls() {
        for (int i = 0; i < kControlCount; ++i) value[static_cast<std::size_t>(i)] = kControlInfo[static_cast<std::size_t>(i)].initial;
    }
    constexpr double& operator[](int index) { return value[static_cast<std::size_t>(index)]; }
    constexpr double operator[](int index) const { return value[static_cast<std::size_t>(index)]; }
};

} // namespace oregana
