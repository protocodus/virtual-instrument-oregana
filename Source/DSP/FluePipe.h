#pragma once

// A flue pipe: a sheet of air (the jet) blown from the flue across the mouth
// at the upper lip, and the air column it drives.
//
// The air column is a travelling-wave loop, once round the pipe per pass: a
// delay, the open end's reflection (a one-pole low-pass, the radiation loss
// of an opening of the pipe's radius), the mouth's end correction (an
// allpass: a mass of air whose correction shrinks with frequency, so the
// upper resonances lie sharp of the harmonics, the more the wider the pipe)
// and the walls' loss. A stopped pipe's loop changes sign each pass, so it
// sounds an octave below its length and resonates on odd harmonics only. A
// chimney flute's stopper carries a narrow open tube: the wave meets the
// change of bore there (a scattering junction), most of it turns back as from
// a stopper, and the rest runs up the chimney and returns from its open top,
// which opens the pipe near the chimney's own resonances and lifts the
// harmonics that fall there. A harmonic flute is an open pipe of twice the
// length with a small hole at its middle, blown to sound its octave: the
// octave and its harmonics have no pressure at the hole and pass it
// untouched, and the long pipe's own note and its odd harmonics leak out
// there, so the pipe cannot fall back to them (Cavaille-Coll's flute
// harmonique; Fletcher and Rossing, section 17.4).
//
// The jet is deflected by the wave returning to the mouth, carries that
// deflection across the cut-up in its transit time (a delay that follows the
// wind: transit time ~ 1 / sqrt(foot pressure)), and at the lip blows into or
// out of the pipe by a saturating law (the flow of a jet with a bell-shaped
// velocity profile against a lip offset from its centre, Fletcher and
// Rossing, The Physics of Musical Instruments, section 16.3). That flow drives
// the loop. Nothing else makes the tone: the pipe speaks when the wind lets
// the loop's gain pass one, settles where the jet saturates and the lip sheds
// vortices (a loss that grows with the mouth's flow, Verge et al., JASA 101,
// 1997, and Dequand et al., JASA 113, 2003: it is why a pipe that speaks fast
// does not grow as loud as its starting gain promises), goes flat as the wind
// sags, and rings down on its own resonances when the pallet closes.
// Turbulence in the jet is the wind noise; the puff of the wind's arrival is
// the chiff.
//
// A pipe is designed from its voicing and its note (Design), then tuned and
// regulated by measurement (PipeRegulation.h), as a voicer does.

#include "DspMath.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace oregana {

// What the pipe maker and the voicer chose for a rank. Dimensions are typical
// of each kind of stop, not measurements of any one organ.
struct FlueVoicing {
    bool stopped = false;
    double scale = 0.0;      // diameter, halftones wider than Toepfer's Normalmensur (155.5 mm at 8' C, halving on the 17th pipe)
    double q = 40.0;         // Q of the first resonance
    double jetTransit = 0.33;   // the jet's transit across the cut-up, periods of the note
    double drive = 1.8;      // the jet's swing at the lip in steady speech, jet half-widths
    double offset = 0.25;    // the lip's offset from the jet's centre at rest, jet half-widths
    double stream = 0.0;     // outward deflection carried by steady mouth outflow, jet half-widths
    double noise = 0.06;     // turbulence, jet half-widths
    double chiff = 0.5;      // the wind's first puff into the pipe
    double shedding = 1.5;   // the loss to vortices shed at the lip in steady speech, against the linear losses
    double corner = 1.0;     // the radiating opening's size against the pipe's own (smaller: a higher corner)
    double speech = 1.0;     // how slowly the foot fills
    double chimney = 0.0;    // a stopped pipe's chimney, its diameter against the pipe's (0: none)
    double chimneyLength = 0.3;   // and its length against the pipe's
    double harmonic = 0.0;   // an open pipe of double length: the share of the pressure at its middle the hole lets out (0: an ordinary pipe)
};

// The jet's flow into the pipe against its displacement at the lip, a
// rational tanh: exact slope at 0, within 2.4 % everywhere, flat beyond 3.
inline float JetShape(float x) {
    if (x > 3.0f) return 1.0f;
    if (x < -3.0f) return -1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// The fundamental of JetShape(swing * sin + offset), by quadrature.
inline double JetFundamental(double swing, double offset) {
    double sum = 0.0;
    const int steps = 256;
    for (int i = 0; i < steps; ++i) {
        const double phase = kTwoPi * (i + 0.5) / steps;
        sum += JetShape(static_cast<float>(swing * std::sin(phase) + offset)) * std::sin(phase);
    }
    return 2.0 * sum / steps;
}

// A pipe's inner diameter in metres at a sounding frequency.
inline double PipeDiameter(double scaleHalftones, double hz) {
    const double halftones = 12.0 * std::log2(hz / 65.406);
    return 0.1555 * std::pow(2.0, (scaleHalftones - halftones) / 16.0);
}

namespace detail {

// y += g (x - y) at w radians per sample: its gain and phase lag.
inline void OnePole(double g, double w, double& gain, double& lag) {
    const double p = 1.0 - g;
    gain = g / std::sqrt(1.0 - 2.0 * p * std::cos(w) + p * p);
    lag = std::atan2(p * std::sin(w), 1.0 - p * std::cos(w));
}

// y = x - x1 + r y1 at w: its gain and phase lead.
inline void DcBlock(double r, double w, double& gain, double& lead) {
    gain = std::sqrt(2.0 - 2.0 * std::cos(w)) / std::sqrt(1.0 - 2.0 * r * std::cos(w) + r * r);
    lead = std::atan2(std::sin(w), 1.0 - std::cos(w)) - std::atan2(r * std::sin(w), 1.0 - r * std::cos(w));
}

// The first-order allpass (c + z^-1) / (1 + c z^-1) whose phase delay at w
// is `delay` samples.
inline double AllpassFor(double delay, double w) {
    return std::sin(0.5 * w * (1.0 - delay)) / std::sin(0.5 * w * (1.0 + delay));
}

// A read `delay` samples back between two neighbours, at w: gain and lag.
inline void Interpolated(double delay, double w, double& gain, double& lag) {
    const double whole = std::floor(delay), f = delay - whole;
    const double re = 1.0 - f + f * std::cos(w), im = f * std::sin(w);
    gain = std::sqrt(re * re + im * im);
    lag = whole * w + std::atan2(im, re);
}

} // namespace detail

// The jet's turbulence is not white: between kBreathFloor and kBreathCorner
// times the note it is kBreathLow times as strong, and from there it falls 6
// dB an octave to the white level near the fourteenth harmonic. Under the
// floor it is white again: slower turbulence than that made a small pipe's
// level wander (a 2960 Hz pipe by 1.4 dB), which no recorded pipe did. Calibrated against two recorded
// organs (Docs/research/benchmark-ilmenau-2026-11-13.md): between their
// harmonics a principal and a flute carry most noise about the first two and
// 15 to 20 dB less by the twelfth, where white turbulence left the model 25
// dB short at the bottom and right at the top.
inline constexpr double kBreathLow = 20.0;
inline constexpr double kBreathCorner = 0.7, kBreathFloor = 0.15;

struct FluePipe {
    // The loop.
    int loopLength = 1;
    float allpass = 0.0f, lowpass = 1.0f, loopGain = 0.0f, dcPole = 0.0f;
    // The jet.
    int jetLength = 4;
    float jetNominal = 1.0f, jetLongest = 1.0f;   // transit at nominal wind and the longest kept, samples
    float jetLowpass = 1.0f, jetGain = 0.0f, offset = 0.0f, source = 0.0f, noise = 0.0f, puff = 0.0f;
    float biasPole = 0.0f, biasGain = 0.0f, biasCap = 0.0f;   // acoustic-streaming deflection of the jet in steady speech
    float breathPole = 0.0f, breathFloor = 0.0f, breathLow = 0.0f;   // the turbulence's band and its strength there (kBreathLow)
    float shed = 0.0f;
    // The pipe beyond a junction (a chimney, or a harmonic pipe's upper
    // half): its round trip, what the junction passes each way, and its open top.
    int upperLength = 0;
    float upFromBelow = 0.0f, upFromAbove = 0.0f, downFromBelow = 1.0f, downFromAbove = 0.0f;
    float upperLowpass = 1.0f, upperGain = 0.0f, upperAllpass = 0.0f, chimneyArea = 0.0f;
    // The foot and the radiation.
    float attack = 1.0f, release = 1.0f, radiation = 0.0f, directMouth = 0.0f;
    float gainLeft = 0.0f, gainRight = 0.0f;
    double air = 0.0;   // the wind it takes at nominal pressure, against an 8' C principal
    double attackSeconds = 0.008;
    double palletSeconds = 0.009, footSeconds = 0.008, releaseSeconds = 0.008;
    float puffNominal = 0.0f, noiseNominal = 0.0f;
    // What a neighbour a metre away hears of this pipe, against its own wave,
    // for each unit of its radiation filter's output (Run, `told`).
    float telling = 0.0f;

    float* loop = nullptr;
    float* jet = nullptr;
    float* upper = nullptr;
    int loopAt = 0, jetAt = 0, upperAt = 0;   // where the loop and the upper pipe are written next
    // The air's warmth as the player bends it (the pitch wheel): the loop is
    // `bendNow` samples longer than it was made, gliding a sample at a time.
    // `loopPass` is the samples a unit of stretch adds (0: a loop too short
    // to bend), `upperShare` the upper pipe's against the loop's.
    int loopRoom = 1, upperRoom = 0;
    float loopPass = 0.0f, upperShare = 0.0f;
    float bendStep = 0.0f, bendTarget = 0.0f, jetStep = 0.0f, wind = 0.0f;
    // Sixteen dynamic per-sample states kept contiguous for 4-lane state transpose.
    float allpassIn = 0.0f, allpassOut = 0.0f, lowpassOut = 0.0f, dcIn = 0.0f;
    float dcOut = 0.0f, jetOut = 0.0f, jetBias = 0.0f, breathOut = 0.0f;
    float breathSlow = 0.0f, radiationIn = 0.0f, radiationOut = 0.0f, foot = 0.0f;
    float filledFrom = 0.0f, puffNow = 0.0f, jetTransit = 1.0f, bendNow = 0.0f;
    float upperOut = 0.0f, upperAllpassIn = 0.0f, upperAllpassOut = 0.0f;
    std::uint32_t random = 1u;
    int quiet = 0;

    static constexpr int kTick = 32;   // samples between Tick() calls
    static constexpr float kPipeSilent = 1.0e-5f;
    // The most the air's bend stretches or shortens a pipe: 10 C either way,
    // 29.5 cents.
    static constexpr double kBendMost = 0.0173;
    static constexpr float kLongestJet = 3.0f;   // the transit kept as the wind falls, against nominal

    // `resonance` is the tuner's correction: the loop's first resonance
    // against the note. Buffers needed: loopRoom, jetLength + 1 and
    // upperRoom floats.
    void Design(const FlueVoicing& v, double hz, double resonance) {
        const double w = kTwoPi * hz / kPipeRate, wr = w * resonance;
        // A period under ten samples cannot carry a hard-blown jet: above
        // 5 kHz the loop breaks up or falls silent where the swing passes
        // 2.5 half-widths or the lip sheds (ProbePipe, 5.3 to 8.4 kHz, at
        // drives 1.8 to 4). The top octave is voiced gently instead, which
        // costs harmonics above 10 kHz only.
        const double top = Clamp(std::log2(hz / 4186.0) * 2.0, 0.0, 1.0);
        const double drive = v.drive + top * (Min(v.drive, 2.2) - v.drive);
        const double lip = v.offset * drive / v.drive;
        const double stream = v.stream * (1.0 - top);
        const double shedding = v.shedding * (1.0 - top);
        const double radius = 0.5 * PipeDiameter(v.scale, hz);
        const double corner = Min(v.corner * kSoundSpeed / (kTwoPi * radius), 0.45 * kPipeRate);
        const double lp = 1.0 - std::exp(-kTwoPi * corner / kPipeRate);
        // The steady flow through an open pipe is the loop's resonance at 0
        // Hz; a blocker a decade under the note takes it out.
        const double dc = 1.0 - kTwoPi * 0.1 * hz / kPipeRate;
        // The loop's delay, gain and lags are taken at the note, where the
        // pipe sounds once it is tuned, not at its own resonance: the loop's
        // length steps by whole samples as the tuner moves it, and only at
        // the frequency the allpass was made for does the step pass unheard
        // (a chimney flute, which sounds a minor third above its resonance,
        // jumped 6 cents there).
        double lpGain, lpLag, dcGain, dcLead;
        detail::OnePole(lp, w, lpGain, lpLag);
        detail::DcBlock(dc, w, dcGain, dcLead);
        // One pass is a period of an open pipe and half a period of a stopped one.
        const double pass = (v.stopped ? kPi : kTwoPi) / wr;
        // Bound the dispersive end correction carried by the first-order
        // allpass (Ingerslev and Frobenius, 1947): an open pipe already carries
        // its top end's radiation reactance in `lowpass` (0.08 * pass ~ 0.5 / w),
        // whereas a stopped pipe's mouth is its only open end (0.85 / w).
        const double mouth = Max(1.1, Min(2.0 * 2.9 * radius / kSoundSpeed * kPipeRate, v.stopped ? 0.85 / w : 0.08 * pass));
        const double rest = pass - (lpLag - dcLead) / w;
        loopLength = Max(1, static_cast<int>(std::lround(rest - mouth)));
        allpass = static_cast<float>(detail::AllpassFor(rest - loopLength, w));
        // The bend reads between the loop's samples, four at a time, from
        // one newer than the shortest loop to two older than the longest.
        const double bendPass = 1.04 * pass;
        const int bendMost = static_cast<int>(bendPass * kBendMost);
        loopRoom = loopLength + bendMost + 4;
        loopPass = loopLength >= bendMost + 3 ? static_cast<float>(bendPass) : 0.0f;
        upperRoom = 0;
        upperShare = 0.0f;
        lowpass = static_cast<float>(lp);
        dcPole = static_cast<float>(dc);
        // The first resonance's Q is the loss of one period: one pass of an
        // open pipe, two of a stopped one.
        const double perPeriod = 1.0 - kPi / v.q;
        const double walls = Min((v.stopped ? std::sqrt(perPeriod) : perPeriod) / (lpGain * dcGain), 0.9999);
        loopGain = static_cast<float>(v.stopped ? -walls : walls);

        // The jet inverts, crosses the cut-up and lags as a low-pass does
        // (it carries slow deflections and smooths fast ones), and the three
        // together bring the returning wave back in phase with itself at the
        // note. A one-pole can lag by less than (pi - w) / 2.
        double transit = v.jetTransit * kTwoPi / w;
        double lag = Clamp(kPi - w * transit, 0.15, 0.9 * 0.5 * (kPi - w));
        double readGain = 1.0, readLag = 0.0;
        for (int i = 0; i < 4; ++i) {
            transit = Max(0.0, transit);
            detail::Interpolated(transit, w, readGain, readLag);
            transit += (kPi - lag - readLag) / w;
        }
        detail::Interpolated(transit, w, readGain, readLag);
        const double jetPole = std::sin(lag) / std::sin(w + lag);
        double jetLpGain, jetLpLag;
        detail::OnePole(1.0 - jetPole, w, jetLpGain, jetLpLag);
        jetNominal = static_cast<float>(transit);
        jetLongest = kLongestJet * jetNominal + 1.0f;
        jetLength = static_cast<int>(jetLongest) + 3;
        jetLowpass = static_cast<float>(1.0 - jetPole);

        // At the note the loop returns g of what leaves the mouth, in phase.
        // With the mouth's flow at 1 in steady speech the returning wave is
        // g / (1 + g), and the jet must supply what the pass loses of it,
        // (1 - g) / g, and what the lip sheds, `shedding` times as much: the
        // fundamental of r |r| is 8 / (3 pi) of its amplitude squared. The
        // jet starts from a small geometric rest offset `lip` (so its slope at
        // rest starts oscillation easily) and steady mouth outflow pushes it
        // outward by `stream` (acoustic streaming, Fletcher 1976 / Verge 1997)
        // as oscillation builds. The source is sized at that steady offset
        // `lip + stream`.
        const double g = walls * lpGain * dcGain;
        const double fundamental = JetFundamental(drive, lip + stream);
        const double returning = g / (1.0 + g), lost = (1.0 - g) / g;
        source = static_cast<float>(returning * lost * (1.0 + shedding) / fundamental);
        shed = static_cast<float>(shedding * lost * 3.0 * kPi / (8.0 * returning));
        jetGain = static_cast<float>(drive / (returning * jetLpGain * readGain));
        offset = static_cast<float>(lip);
        biasPole = static_cast<float>(1.0 - std::exp(-kTwoPi * Min(hz / 15.0, 200.0) / kPipeRate));
        biasGain = static_cast<float>((kPi / (2.0 * drive)) * stream);
        biasCap = static_cast<float>(1.5 * drive);
        directMouth = v.stopped ? 1.0f : 0.0f;
        noiseNominal = static_cast<float>(v.noise);
        noise = noiseNominal;
        breathPole = static_cast<float>(1.0 - std::exp(-kTwoPi * kBreathCorner * hz / kPipeRate));
        breathFloor = static_cast<float>(1.0 - std::exp(-kTwoPi * kBreathFloor * hz / kPipeRate));
        // The top octave's short loops cannot carry it either: at 7.4 kHz it
        // threw a pipe to the octave below.
        breathLow = static_cast<float>(kBreathLow * (1.0 - top));
        puffNominal = static_cast<float>(1.2 * v.chiff);

        // Beyond a junction. Neither a chimney nor a harmonic pipe's upper
        // half is the tuner's to move: the builder cut them for the note.
        if (v.stopped && v.chimney > 0.0) {
            // A wave meeting a bore of s times the area turns back by
            // (1 - s) / (1 + s). The chimney's top radiates as an opening of
            // its own radius, and its open-end lag shortens the physical tube.
            const double area = v.chimney * v.chimney;
            const double bore = (1.0 - area) / (1.0 + area);
            upFromBelow = static_cast<float>(1.0 + bore);
            upFromAbove = static_cast<float>(-bore);
            downFromBelow = static_cast<float>(bore);
            downFromAbove = static_cast<float>(1.0 - bore);
            const double opening = Min(kSoundSpeed / (kTwoPi * radius * v.chimney), 0.45 * kPipeRate);
            const double upLp = 1.0 - std::exp(-kTwoPi * opening / kPipeRate);
            double upLpGain, upLpLag;
            detail::OnePole(upLp, w, upLpGain, upLpLag);
            // Its length there and back is a whole number of samples and
            // an allpass for the rest: cut to whole samples alone, it stepped
            // the pipe's pitch by up to 4 cents as the organ was retuned.
            const double acousticChimney = v.chimneyLength * kPi / w;
            const double chimney = Max(1.2, acousticChimney - upLpLag / w);
            upperLength = Max(1, static_cast<int>(std::lround(chimney - 1.0)));
            SetUpperBend(acousticChimney, pass);
            upperAllpass = static_cast<float>(detail::AllpassFor(Max(chimney - upperLength, 0.05), w));
            upperLowpass = static_cast<float>(upLp);
            upperGain = static_cast<float>(Min(std::sqrt(perPeriod) / upLpGain, 0.9999));
            chimneyArea = static_cast<float>(area);
        } else if (!v.stopped && v.harmonic > 0.0) {
            // A hole that lets a share h of the pressure out turns a wave
            // back by -h and passes 1 - h (a shunt on a line). The loop above
            // is the lower half, and its far end's turn of sign is taken here.
            // The upper half is a period there and back with its top's lag:
            // a whole number of samples and the lag of a one-pole made to
            // fill the rest.
            const double h = v.harmonic;
            upFromBelow = static_cast<float>(1.0 - h);
            upFromAbove = static_cast<float>(-h);
            downFromBelow = static_cast<float>(h);
            downFromAbove = static_cast<float>(-(1.0 - h));
            double topGain, topLag;
            detail::OnePole(lp, w, topGain, topLag);
            const int whole = Max(2, static_cast<int>(std::floor((kTwoPi - topLag) / w)));
            const double fill = Clamp(kTwoPi - whole * w, 0.02, 0.9 * 0.5 * (kPi - w));
            // The line's allpass at 0 is one sample of the delay.
            upperLength = whole - 1;
            SetUpperBend(kTwoPi / w, pass);
            upperAllpass = 0.0f;
            const double topPole = std::sin(fill) / std::sin(w + fill);
            detail::OnePole(1.0 - topPole, w, topGain, topLag);
            upperLowpass = static_cast<float>(1.0 - topPole);
            upperGain = static_cast<float>(Min(perPeriod / topGain, 0.9999));
            chimneyArea = 0.0f;
        } else {
            upperLength = 0;
            chimneyArea = 0.0f;
        }

        // The foot fills through the pallet and the toe, the faster the
        // smaller and wider the pipe, and empties through the flue. Narrower
        // string ranks (v.scale < 0) fill and excite more slowly by Töpfer's
        // diameter ratio, while wider stopped flutes speak promptly.
        const double narrow = v.scale < 0.0 ? std::pow(2.0, Clamp(-v.scale, 0.0, 14.0) / 16.0) : 1.0;
        palletSeconds = Min(0.0090, 0.0007 + 2.0 / hz);
        footSeconds = v.speech * narrow * Min(0.052, 0.0008 + 1.3 / hz);
        attackSeconds = palletSeconds + footSeconds;
        releaseSeconds = 0.004 + 0.5 / hz;
        SetTouch(1.0);
        SetRelease(1.0);
        radiation = static_cast<float>(std::exp(-kTwoPi * corner / kPipeRate));
        // Wind through the flue: its area goes with the diameter squared.
        air = 4.0 * radius * radius / (0.1555 * 0.1555);
        // The mouth is a small source: a flow Q changing at dQ/dt makes a
        // pressure rho / (4 pi d) dQ/dt at d metres. With the flow as the
        // pipe's area times its wave over rho c, that is r^2 / (4 d c) of the
        // wave's rate of change, here per sample and for d = 1. That holds
        // while the opening is small against the wavelength, as far as the
        // radiation filter differentiates, so the neighbour hears that
        // filter's output: scaled here to the same figure below its corner,
        // and level above it, where a rate of change would grow without end.
        telling = static_cast<float>(radius * radius * kPipeRate / (4.0 * kSoundSpeed) * (1.0 - radiation) / radiation);
    }

    // How fast the key opens the pallet: 1 is an ordinary stroke (velocity 80).
    void SetTouch(double slowness) {
        const double s = Clamp(slowness, 0.2, 8.0);
        const double riseSeconds = palletSeconds * s + footSeconds * (0.35 + 0.65 * s);
        attack = static_cast<float>(1.0 - std::exp(-1.0 / (riseSeconds * kPipeRate)));
        const double speed = Clamp(std::pow(1.0 / s, 1.15), 0.06, 2.25);
        puff = static_cast<float>(puffNominal * speed);
    }
    void SetRelease(double slowness) {
        const double fallSeconds = releaseSeconds * Clamp(slowness, 0.35, 4.0);
        release = static_cast<float>(1.0 - std::exp(-1.0 / (fallSeconds * kPipeRate)));
    }
    void SetBreath(float scale) { noise = noiseNominal * scale; }

    void Clear() {
        for (int i = 0; i < loopRoom; ++i) loop[i] = 0.0f;
        for (int i = 0; i <= jetLength; ++i) jet[i] = 0.0f;
        for (int i = 0; i < upperRoom; ++i) upper[i] = 0.0f;
        loopAt = jetAt = upperAt = 0;
        allpassIn = allpassOut = lowpassOut = dcIn = dcOut = jetOut = jetBias = radiationIn = radiationOut = upperOut = breathOut = breathSlow = 0.0f;
        upperAllpassIn = upperAllpassOut = 0.0f;
        foot = wind = filledFrom = puffNow = 0.0f;
        jetTransit = jetLongest;
        jetStep = 0.0f;
        bendNow = bendStep = bendTarget = 0.0f;
        quiet = 0;
    }

    // The air's bend as it stands, for a pipe that begins to speak: `stretch`
    // is how much longer the air makes every length, against 1.
    void SetBend(float stretch) {
        bendNow = bendTarget = loopPass * stretch;
        bendStep = 0.0f;
    }

    // Every kTick samples: the jet's transit follows the foot pressure, no
    // faster than the jet itself moves.
    void Tick(float stretch = 0.0f) {
        // The loop glides to the air's bend over the tick, and rests on it
        // exactly once it has stood still for one: at rest it is 0, and the
        // pipe is stepped as it was made.
        const float bent = loopPass * stretch;
        if (bent == bendTarget) {
            bendNow = bent;
            bendStep = 0.0f;
        } else {
            bendStep = (bent - bendNow) * (1.0f / kTick);
        }
        bendTarget = bent;
        // A foot all but empty is empty. Left to fall it passes, long before
        // the pipe has rung out, through numbers too small for a processor
        // to keep at speed (denormals): a host that does not flush them paid
        // twice over for every ringing pipe (full organ let go, 0.5 s on: 15 %
        // of a core against 7 %). For the same reason the puff takes the gap
        // only while the foot fills, and so never squares a falling foot.
        if (wind == 0.0f && foot < 1.0e-25f) foot = 0.0f;
        const float pressure = Max(foot, 1.0f / (kLongestJet * kLongestJet));
        // The jet's transit keeps its share of the period (at rest, times 1).
        const float nominal = jetNominal * (1.0f + (loopPass > 0.0f ? stretch : 0.0f));
        const float target = Min(nominal / Sqrt(pressure), jetLongest);
        jetStep = Clamp((target - jetTransit) * (1.0f / kTick), -0.5f, 0.5f);
        const float level = Abs(lowpassOut) + Abs(allpassOut) + Abs(radiationOut) + foot;
        // A released pipe is let go when it has rung down to kPipeSilent, some
        // 105 dB under its own speech at the output (owner, 2026-11-13: 100 dB
        // is past hearing; it rang 20 dB further), and costs nothing from then.
        quiet = level < kPipeSilent ? quiet + 1 : 0;
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

    // `frames` samples of the pipe's sound, before its gains. A pipe with
    // neighbours near enough to matter is given what it hears of them at its
    // mouth, `heard`, and writes what they will hear of it, `told` (at a
    // metre: the listener divides by its distance).
    void Run(float* out, int frames, const float* heard = nullptr, float* told = nullptr) {
        float* const loopBuffer = loop;
        float* const jetBuffer = jet;
        float* const upperBuffer = upper;
        const int upperTrip = upperLength, room = loopRoom, upperSize = upperRoom;
        int at = loopAt, jetWrite = jetAt, upperWrite = upperAt;
        int from = at - loopLength, upperRead = upperWrite - upperTrip;
        if (from < 0) from += room;
        if (upperRead < 0) upperRead += upperSize;
        float bend = bendNow;
        const float bendBy = bendStep, share = upperShare;
        const bool bent = bend != 0.0f || bendBy != 0.0f;
        float upperLp = upperOut, upperApIn = upperAllpassIn, upperApOut = upperAllpassOut;
        float apIn = allpassIn, apOut = allpassOut, lpOut = lowpassOut, blockIn = dcIn, blockOut = dcOut;
        float jetLp = jetOut, bias = jetBias, radIn = radiationIn, radOut = radiationOut, pressure = foot, transit = jetTransit, breath = breathOut, slow = breathSlow;
        float filled = filledFrom, pushing = puffNow;
        const float keep = 1.0f - release;
        std::uint32_t seed = random;
        const float target = wind, rise = attack, fall = release;
        const float jetSize = static_cast<float>(jetLength);
        for (int i = 0; i < frames; ++i) {
            const float gap = target - pressure;
            pressure += gap * (gap > 0.0f ? rise : fall);
            if (!(gap > 0.0f)) filled = pressure;

            const float x = bent ? Bent(loopBuffer, room, at, loopLength, bend) : loopBuffer[from];
            const float ap = allpass * (x - apOut) + apIn;
            apIn = x;
            apOut = ap;
            float turned = ap, topFlow = 0.0f;
            if (upperTrip > 0) {
                const float stored = bent ? Bent(upperBuffer, upperSize, upperWrite, upperTrip, share * bend) : upperBuffer[upperRead];
                const float back = upperAllpass * (stored - upperApOut) + upperApIn;
                upperApIn = stored;
                upperApOut = back;
                const float intoUpper = upFromBelow * ap + upFromAbove * back;
                upperLp += upperLowpass * (intoUpper - upperLp);
                const float up = -upperGain * upperLp;
                upperBuffer[upperWrite] = up;
                if (++upperWrite == upperSize) upperWrite = 0;
                if (++upperRead == upperSize) upperRead = 0;
                topFlow = chimneyArea * (intoUpper - up);
                turned = downFromBelow * ap + downFromAbove * back;
            }
            lpOut += lowpass * (turned - lpOut);
            const float passed = loopGain * lpOut;
            const float returned = passed - blockIn + dcPole * blockOut;
            blockIn = passed;
            blockOut = returned;

            jetBuffer[jetWrite] = returned;
            if (jetWrite == 0) jetBuffer[jetLength] = returned;
            float read = static_cast<float>(jetWrite) - transit;
            if (read < 0.0f) read += jetSize;
            // A read just under 0 rounds up to the buffer's length as a float.
            int older = static_cast<int>(read);
            if (older >= jetLength) older = 0;
            const float between = read - static_cast<float>(older);
            const float* const jp = jetBuffer + older;
            const float deflection = jp[0] + between * (jp[1] - jp[0]);
            if (++jetWrite == jetLength) jetWrite = 0;
            jetLp += jetLowpass * (deflection - jetLp);

            const float white = NextNoise(seed);
            breath += breathPole * (white - breath);
            slow += breathFloor * (white - slow);
            const float turbulence = noise * (white + breathLow * (breath - slow));
            // The wind's arrival pushes a puff of air into the pipe, largest
            // midway through the foot's filling, and counted from where the
            // filling began: counted from an empty foot, a key struck again
            // within 20 ms, its foot still part full, began its puff with a
            // step (a click as large as the pipe's whole sound at c1). Wind
            // taken away in mid-puff (a note under 10 ms) lets the puff die
            // as the foot empties: cut off, it ended with the same step.
            if (gap > 0.0f) pushing = gap * (pressure - filled);
            else pushing = pushing > 1.0e-20f ? pushing * keep : 0.0f;
            const float jetSwing = jetGain * jetLp;
            const float jetRect = Abs(jetSwing);
            bias += biasPole * ((jetRect < biasCap ? jetRect : biasCap) * biasGain - bias);
            const float blown = -source * pressure * JetShape(jetSwing + offset + bias + turbulence) + puff * pushing;
            float leaving = returned + blown - shed * Abs(returned) * returned;
            // The pressure a neighbour makes at the mouth enters as a wave.
            if (heard != nullptr) leaving += heard[i];
            loopBuffer[at] = leaving;
            if (++at == room) at = 0;
            if (++from == room) from = 0;
            bend += bendBy;

            // The mouth's flow radiates as its rate of change up to the
            // corner where the opening is a wavelength round, evenly above.
            // On stopped pipes, the mouth radiates the jet's direct volume
            // flow at even-harmonic antiresonances (Fletcher & Rossing, eq.
            // 16.6); on a chimney flute, the open chimney top radiates too.
            float flow = (returned + leaving) + directMouth * blown;
            if (upperTrip > 0) flow += topFlow;
            radOut = radiation * (radOut + flow - radIn);
            radIn = flow;
            if (told != nullptr) told[i] = telling * radOut;
            out[i] = radOut;
            transit += jetStep;
        }
        loopAt = at;
        jetAt = jetWrite;
        upperAt = upperWrite;
        bendNow = bend;
        upperOut = upperLp;
        upperAllpassIn = upperApIn;
        upperAllpassOut = upperApOut;
        allpassIn = apIn;
        allpassOut = apOut;
        lowpassOut = lpOut;
        dcIn = blockIn;
        dcOut = blockOut;
        jetOut = jetLp;
        jetBias = bias;
        breathOut = breath;
        breathSlow = slow;
        radiationIn = radIn;
        radiationOut = radOut;
        foot = pressure;
        filledFrom = filled;
        puffNow = pushing;
        jetTransit = Clamp(transit, 0.0f, jetLongest);
        random = seed;
    }

#if defined(OREGANA_FOUR)
    // Pre-packed 4-pipe parameter block stored in the Engine (8-byte safe, unaligned).
    struct FluePack {
        FluePipe* pipe[4] {};
        float* loopBuffer[4] {};
        float* jetBuffer[4] {};
        float* upperBuffer[4] {};
        int jetCount[4] {}, loopSize[4] {}, upperTrip[4] {}, room[4] {}, upperSize[4] {};
        PackedFloat4 allpass, lowpass, loopGain, dcPole, jetLowpass, jetGain;
        PackedFloat4 offset, source, noise, biasPole, biasGain, biasCap;
        PackedFloat4 directMouth, puff, shed, radiation, keep, jetSize;
        PackedFloat4 breathPole, breathFloor, breathLow, attack, release;
        PackedFloat4 upperShare, upFromBelow, upFromAbove, downFromBelow, downFromAbove;
        PackedFloat4 upperLowpass, upperGain, upperAllpass, chimneyArea, telling;
    };

#if defined(__GNUC__) || defined(__clang__)
    __attribute__((noinline))
#endif
    static void PackFour(FluePack& pack, FluePipe* const pipe[4]) {
#define OREGANA_PACK_FIELD(field) \
    Store4(pack.field, Float4 { pipe[0]->field, pipe[1]->field, pipe[2]->field, pipe[3]->field })
        for (int l = 0; l < 4; ++l) {
            pack.pipe[l] = pipe[l];
            pack.loopBuffer[l] = pipe[l]->loop;
            pack.jetBuffer[l] = pipe[l]->jet;
            pack.upperBuffer[l] = pipe[l]->upper;
            pack.jetCount[l] = pipe[l]->jetLength;
            pack.loopSize[l] = pipe[l]->loopLength;
            pack.upperTrip[l] = pipe[l]->upperLength;
            pack.room[l] = pipe[l]->loopRoom;
            pack.upperSize[l] = pipe[l]->upperRoom;
        }
        OREGANA_PACK_FIELD(allpass);
        OREGANA_PACK_FIELD(lowpass);
        OREGANA_PACK_FIELD(loopGain);
        OREGANA_PACK_FIELD(dcPole);
        OREGANA_PACK_FIELD(jetLowpass);
        OREGANA_PACK_FIELD(jetGain);
        OREGANA_PACK_FIELD(offset);
        OREGANA_PACK_FIELD(source);
        OREGANA_PACK_FIELD(noise);
        OREGANA_PACK_FIELD(biasPole);
        OREGANA_PACK_FIELD(biasGain);
        OREGANA_PACK_FIELD(biasCap);
        OREGANA_PACK_FIELD(directMouth);
        OREGANA_PACK_FIELD(puff);
        OREGANA_PACK_FIELD(shed);
        OREGANA_PACK_FIELD(radiation);
        const Float4 fall = { pipe[0]->release, pipe[1]->release, pipe[2]->release, pipe[3]->release };
        Store4(pack.release, fall);
        Store4(pack.keep, Four(1.0f) - fall);
        Store4(pack.jetSize, Float4 { static_cast<float>(pipe[0]->jetLength), static_cast<float>(pipe[1]->jetLength),
                                      static_cast<float>(pipe[2]->jetLength), static_cast<float>(pipe[3]->jetLength) });
        OREGANA_PACK_FIELD(breathPole);
        OREGANA_PACK_FIELD(breathFloor);
        OREGANA_PACK_FIELD(breathLow);
        OREGANA_PACK_FIELD(attack);
        OREGANA_PACK_FIELD(upperShare);
        OREGANA_PACK_FIELD(upFromBelow);
        OREGANA_PACK_FIELD(upFromAbove);
        OREGANA_PACK_FIELD(downFromBelow);
        OREGANA_PACK_FIELD(downFromAbove);
        OREGANA_PACK_FIELD(upperLowpass);
        OREGANA_PACK_FIELD(upperGain);
        OREGANA_PACK_FIELD(upperAllpass);
        OREGANA_PACK_FIELD(chimneyArea);
        OREGANA_PACK_FIELD(telling);
#undef OREGANA_PACK_FIELD
    }

    template <bool Upper, bool Hears>
    static void RunFourPacked(const FluePack& pack, float* const out[4], int frames, const float* const heard[4] = nullptr,
                              float* const told[4] = nullptr) {
        bool bent = false;
        for (int l = 0; l < 4; ++l) bent = bent || pack.pipe[l]->bendNow != 0.0f || pack.pipe[l]->bendStep != 0.0f;
        if (bent) RunFourPackedAs<Upper, Hears, true>(pack, out, frames, heard, told);
        else RunFourPackedAs<Upper, Hears, false>(pack, out, frames, heard, told);
    }

    // Run for four pipes at once, lane for lane the same arithmetic. `Upper`:
    // all four have a pipe beyond a junction, else none has. `Hears`: all
    // four hear neighbours and tell them, else none does.
    template <bool Upper, bool Hears>
    static void RunFour(FluePipe* const pipe[4], float* const out[4], int frames, const float* const heard[4] = nullptr,
                        float* const told[4] = nullptr) {
        FluePack pack;
        PackFour(pack, pipe);
        RunFourPacked<Upper, Hears>(pack, out, frames, heard, told);
    }

    // `Bends`: the loops are read between their samples, where the air's
    // bend has them; else none is bent, and each is read where it was made.
    template <bool Upper, bool Hears, bool Bends>
    static void RunFourPackedAs(const FluePack& pack, float* const out[4], int frames, const float* const heard[4],
                                float* const told[4]) {
        FluePipe* const p0 = pack.pipe[0];
        FluePipe* const p1 = pack.pipe[1];
        FluePipe* const p2 = pack.pipe[2];
        FluePipe* const p3 = pack.pipe[3];

        const Float4 target { p0->wind, p1->wind, p2->wind, p3->wind };
        const Float4 stepC { p0->jetStep, p1->jetStep, p2->jetStep, p3->jetStep };

        Float4 apIn = Load4(&p0->allpassIn), apOut = Load4(&p1->allpassIn);
        Float4 lpOut = Load4(&p2->allpassIn), blockIn = Load4(&p3->allpassIn);
        Transpose4x4(apIn, apOut, lpOut, blockIn);

        Float4 blockOut = Load4(&p0->dcOut), jetLp = Load4(&p1->dcOut);
        Float4 bias = Load4(&p2->dcOut), breath = Load4(&p3->dcOut);
        Transpose4x4(blockOut, jetLp, bias, breath);

        Float4 slow = Load4(&p0->breathSlow), radIn = Load4(&p1->breathSlow);
        Float4 radOut = Load4(&p2->breathSlow), pressure = Load4(&p3->breathSlow);
        Transpose4x4(slow, radIn, radOut, pressure);

        Float4 filled = Load4(&p0->filledFrom), pushing = Load4(&p1->filledFrom);
        Float4 transit = Load4(&p2->filledFrom), bend = Load4(&p3->filledFrom);
        Transpose4x4(filled, pushing, transit, bend);

        const Float4 bendBy = [&]() {
            if constexpr (Bends) return Float4 { p0->bendStep, p1->bendStep, p2->bendStep, p3->bendStep };
            else return Four(0.0f);
        }();
        const Float4 share = [&]() {
            if constexpr (Upper && Bends) return Load4(pack.upperShare);
            else return Four(0.0f);
        }();
        Float4 upperLp = [&]() {
            if constexpr (Upper) return Float4 { p0->upperOut, p1->upperOut, p2->upperOut, p3->upperOut };
            else return Four(0.0f);
        }();
        Float4 upperApIn = [&]() {
            if constexpr (Upper) return Float4 { p0->upperAllpassIn, p1->upperAllpassIn, p2->upperAllpassIn, p3->upperAllpassIn };
            else return Four(0.0f);
        }();
        Float4 upperApOut = [&]() {
            if constexpr (Upper) return Float4 { p0->upperAllpassOut, p1->upperAllpassOut, p2->upperAllpassOut, p3->upperAllpassOut };
            else return Four(0.0f);
        }();

        const Float4 zero = Four(0.0f), one = Four(1.0f);
        int at[4], from[4], upperWrite[4], upperRead[4];
        float* const loopBase[4] = { pack.loopBuffer[0], pack.loopBuffer[1], pack.loopBuffer[2], pack.loopBuffer[3] };
        float* const loopEnd[4] = {
            loopBase[0] + pack.room[0], loopBase[1] + pack.room[1], loopBase[2] + pack.room[2], loopBase[3] + pack.room[3]
        };
        float* loopWritePtr[4];
        float* loopReadPtr[4];
        for (int l = 0; l < 4; ++l) {
            at[l] = pack.pipe[l]->loopAt;
            from[l] = at[l] - pack.loopSize[l];
            if (from[l] < 0) from[l] += pack.room[l];
            loopWritePtr[l] = loopBase[l] + at[l];
            loopReadPtr[l] = loopBase[l] + from[l];
            if constexpr (Upper) {
                upperWrite[l] = pack.pipe[l]->upperAt;
                upperRead[l] = upperWrite[l] - pack.upperTrip[l];
                if (upperRead[l] < 0) upperRead[l] += pack.upperSize[l];
            }
        }
        float* upBase[4] {};
        float* upEnd[4] {};
        float* upWritePtr[4] {};
        float* upReadPtr[4] {};
        if constexpr (Upper) {
            for (int l = 0; l < 4; ++l) {
                upBase[l] = pack.upperBuffer[l];
                upEnd[l] = upBase[l] + pack.upperSize[l];
                if constexpr (!Bends) {
                    upWritePtr[l] = upBase[l] + upperWrite[l];
                    upReadPtr[l] = upBase[l] + upperRead[l];
                }
            }
        }

        Int4 jetWrite = { p0->jetAt, p1->jetAt, p2->jetAt, p3->jetAt };
        const Int4 jetCount = { pack.jetCount[0], pack.jetCount[1], pack.jetCount[2], pack.jetCount[3] };
        const Float4 jetSize = Load4(pack.jetSize);
        UInt4 seed = { p0->random, p1->random, p2->random, p3->random };

        const Float4 allpassC = Load4(pack.allpass), lowpassC = Load4(pack.lowpass), loopGainC = Load4(pack.loopGain);
        const Float4 dcPoleC = Load4(pack.dcPole), jetLowpassC = Load4(pack.jetLowpass), jetGainC = Load4(pack.jetGain);
        const Float4 offsetC = Load4(pack.offset), sourceC = Load4(pack.source), noiseC = Load4(pack.noise);
        const Float4 biasPoleC = Load4(pack.biasPole), biasGainC = Load4(pack.biasGain), biasCapC = Load4(pack.biasCap);
        const Float4 directMouthC = Load4(pack.directMouth), puffC = Load4(pack.puff), shedC = Load4(pack.shed);
        const Float4 radiationC = Load4(pack.radiation), keep = Load4(pack.keep);
        const Float4 rise = Load4(pack.attack), fall = Load4(pack.release);
        const Float4 breathPoleC = Load4(pack.breathPole), breathFloorC = Load4(pack.breathFloor), breathLowC = Load4(pack.breathLow);
        const Float4 chimneyAreaC = [&]() {
            if constexpr (Upper) return Load4(pack.chimneyArea);
            else return Four(0.0f);
        }();
        const Float4 upFromBelowC = [&]() {
            if constexpr (Upper) return Load4(pack.upFromBelow);
            else return Four(0.0f);
        }();
        const Float4 upFromAboveC = [&]() {
            if constexpr (Upper) return Load4(pack.upFromAbove);
            else return Four(0.0f);
        }();
        const Float4 downFromBelowC = [&]() {
            if constexpr (Upper) return Load4(pack.downFromBelow);
            else return Four(0.0f);
        }();
        const Float4 downFromAboveC = [&]() {
            if constexpr (Upper) return Load4(pack.downFromAbove);
            else return Four(0.0f);
        }();
        const Float4 upperLowpassC = [&]() {
            if constexpr (Upper) return Load4(pack.upperLowpass);
            else return Four(0.0f);
        }();
        const Float4 upperGainC = [&]() {
            if constexpr (Upper) return Load4(pack.upperGain);
            else return Four(0.0f);
        }();
        const Float4 upperAllpassC = [&]() {
            if constexpr (Upper) return Load4(pack.upperAllpass);
            else return Four(0.0f);
        }();
        const Float4 tellingC = [&]() {
            if constexpr (Hears) return Load4(pack.telling);
            else return Four(0.0f);
        }();

        const auto step = [&](int sampleIndex)
#if defined(__GNUC__) || defined(__clang__)
                              __attribute__((always_inline))
#endif
            -> Float4 {
            const Float4 gap = target - pressure;
            const Int4 filling = gap > zero;
            pressure += gap * Choose(filling, rise, fall);
            filled = Choose(filling, filled, pressure);
            pushing = Choose(filling, gap * (pressure - filled), Where(pushing > Four(1.0e-20f), pushing * keep));

            Float4 x;
            if constexpr (Bends) {
                x = BentFour(pack.loopBuffer, pack.room, at, pack.loopSize, bend);
            } else {
                x = Float4 { *loopReadPtr[0], *loopReadPtr[1], *loopReadPtr[2], *loopReadPtr[3] };
            }
            const Float4 ap = allpassC * (x - apOut) + apIn;
            apIn = x;
            apOut = ap;
            Float4 turned = ap, topFlow = zero;
            if constexpr (Upper) {
                Float4 stored;
                if constexpr (Bends) {
                    stored = BentFour(pack.upperBuffer, pack.upperSize, upperWrite, pack.upperTrip, share * bend);
                } else {
                    stored = Float4 { *upReadPtr[0], *upReadPtr[1], *upReadPtr[2], *upReadPtr[3] };
                }
                const Float4 back = upperAllpassC * (stored - upperApOut) + upperApIn;
                upperApIn = stored;
                upperApOut = back;
                const Float4 intoUpper = upFromBelowC * ap + upFromAboveC * back;
                upperLp += upperLowpassC * (intoUpper - upperLp);
                const Float4 up = -upperGainC * upperLp;
                if constexpr (Bends) {
                    for (int l = 0; l < 4; ++l) {
                        pack.upperBuffer[l][upperWrite[l]] = up[l];
                        if (++upperWrite[l] == pack.upperSize[l]) upperWrite[l] = 0;
                        if (++upperRead[l] == pack.upperSize[l]) upperRead[l] = 0;
                    }
                } else {
                    for (int l = 0; l < 4; ++l) {
                        *upWritePtr[l] = up[l];
                        if (++upWritePtr[l] == upEnd[l]) upWritePtr[l] = upBase[l];
                        if (++upReadPtr[l] == upEnd[l]) upReadPtr[l] = upBase[l];
                    }
                }
                topFlow = chimneyAreaC * (intoUpper - up);
                turned = downFromBelowC * ap + downFromAboveC * back;
            }
            lpOut += lowpassC * (turned - lpOut);
            const Float4 passed = loopGainC * lpOut;
            const Float4 returned = passed - blockIn + dcPoleC * blockOut;
            blockIn = passed;
            blockOut = returned;

            for (int l = 0; l < 4; ++l) {
                float* const jb = pack.jetBuffer[l];
                const int jw = jetWrite[l];
                jb[jw] = returned[l];
                if (jw == 0) jb[pack.jetCount[l]] = returned[l];
            }
            Float4 read = ToFloat(jetWrite) - transit;
            read += Where(read < zero, jetSize);
            // A read just under 0 rounds up to the buffer's length as a float.
            Int4 older = Truncated(read);
            older &= ~(older >= jetCount);
            const Float4 between = read - ToFloat(older);
            const float* const jp0 = pack.jetBuffer[0] + older[0];
            const float* const jp1 = pack.jetBuffer[1] + older[1];
            const float* const jp2 = pack.jetBuffer[2] + older[2];
            const float* const jp3 = pack.jetBuffer[3] + older[3];
            const Float4 early { jp0[0], jp1[0], jp2[0], jp3[0] };
            const Float4 late { jp0[1], jp1[1], jp2[1], jp3[1] };
            const Float4 deflection = early + between * (late - early);
            jetWrite += 1;
            jetWrite &= ~(jetWrite == jetCount);
            transit += stepC;
            const Float4 white = NextNoise(seed);
            jetLp += jetLowpassC * (deflection - jetLp);

            breath += breathPoleC * (white - breath);
            slow += breathFloorC * (white - slow);
            const Float4 turbulence = noiseC * (white + breathLowC * (breath - slow));
            const Float4 jetSwing = jetGainC * jetLp;
            const Float4 jetRect = Abs(jetSwing);
            bias += biasPoleC * (Choose(jetRect < biasCapC, jetRect, biasCapC) * biasGainC - bias);
            const Float4 swing = jetSwing + offsetC + bias + turbulence;
            const Float4 swing2 = swing * swing;
            const Float4 shape = Choose(swing > Four(3.0f), one,
                                        Choose(swing < Four(-3.0f), -one, swing * (27.0f + swing2) / (27.0f + 9.0f * swing2)));
            const Float4 blown = -sourceC * pressure * shape + puffC * pushing;
            Float4 leaving = returned + blown - shedC * Abs(returned) * returned;
            if constexpr (Hears) leaving += Float4 { heard[0][sampleIndex], heard[1][sampleIndex], heard[2][sampleIndex], heard[3][sampleIndex] };
            if constexpr (Bends) {
                for (int l = 0; l < 4; ++l) {
                    pack.loopBuffer[l][at[l]] = leaving[l];
                    if (++at[l] == pack.room[l]) at[l] = 0;
                    if (++from[l] == pack.room[l]) from[l] = 0;
                }
                bend += bendBy;
            } else {
                for (int l = 0; l < 4; ++l) {
                    *loopWritePtr[l] = leaving[l];
                    if (++loopWritePtr[l] == loopEnd[l]) loopWritePtr[l] = loopBase[l];
                    if (++loopReadPtr[l] == loopEnd[l]) loopReadPtr[l] = loopBase[l];
                }
            }

            Float4 flow = (returned + leaving) + directMouthC * blown;
            if constexpr (Upper) flow += topFlow;
            radOut = radiationC * (radOut + flow - radIn);
            radIn = flow;
            return radOut;
        };

        int i = 0;
        for (; i + 4 <= frames; i += 4) {
            Float4 r0 = step(i);
            Float4 r1 = step(i + 1);
            Float4 r2 = step(i + 2);
            Float4 r3 = step(i + 3);
            if constexpr (Hears) {
                Float4 t0 = tellingC * r0;
                Float4 t1 = tellingC * r1;
                Float4 t2 = tellingC * r2;
                Float4 t3 = tellingC * r3;
                Transpose4x4(t0, t1, t2, t3);
                Store4(told[0] + i, t0);
                Store4(told[1] + i, t1);
                Store4(told[2] + i, t2);
                Store4(told[3] + i, t3);
            }
            Transpose4x4(r0, r1, r2, r3);
            Store4(out[0] + i, r0);
            Store4(out[1] + i, r1);
            Store4(out[2] + i, r2);
            Store4(out[3] + i, r3);
        }
        for (; i < frames; ++i) {
            const Float4 r = step(i);
            if constexpr (Hears) {
                const Float4 tells = tellingC * r;
                told[0][i] = tells[0];
                told[1][i] = tells[1];
                told[2][i] = tells[2];
                told[3][i] = tells[3];
            }
            out[0][i] = r[0];
            out[1][i] = r[1];
            out[2][i] = r[2];
            out[3][i] = r[3];
        }

        Transpose4x4(apIn, apOut, lpOut, blockIn);
        Store4(&p0->allpassIn, apIn);
        Store4(&p1->allpassIn, apOut);
        Store4(&p2->allpassIn, lpOut);
        Store4(&p3->allpassIn, blockIn);

        Transpose4x4(blockOut, jetLp, bias, breath);
        Store4(&p0->dcOut, blockOut);
        Store4(&p1->dcOut, jetLp);
        Store4(&p2->dcOut, bias);
        Store4(&p3->dcOut, breath);

        Transpose4x4(slow, radIn, radOut, pressure);
        Store4(&p0->breathSlow, slow);
        Store4(&p1->breathSlow, radIn);
        Store4(&p2->breathSlow, radOut);
        Store4(&p3->breathSlow, pressure);

        Transpose4x4(filled, pushing, transit, bend);
        Store4(&p0->filledFrom, filled);
        Store4(&p1->filledFrom, pushing);
        Store4(&p2->filledFrom, transit);
        Store4(&p3->filledFrom, bend);

        for (int l = 0; l < 4; ++l) {
            FluePipe& p = *pack.pipe[l];
            p.loopAt = Bends ? at[l] : static_cast<int>(loopWritePtr[l] - loopBase[l]);
            p.jetAt = jetWrite[l];
            if constexpr (Upper) {
                p.upperAt = Bends ? upperWrite[l] : static_cast<int>(upWritePtr[l] - upBase[l]);
                p.upperOut = upperLp[l];
                p.upperAllpassIn = upperApIn[l];
                p.upperAllpassOut = upperApOut[l];
            }
            p.jetTransit = Clamp(p.jetTransit, 0.0f, p.jetLongest);
            p.random = seed[l];
        }
    }
#endif

private:
    // A buffer's four samples about a delay of `length + whole` samples
    // behind the one written next at `write`: one newer, that one, and two
    // older.
    static void Taps(const float* buffer, int room, int write, int length, int whole, float tap[4]) {
        int i = write - length - whole - 2;
        if (i < 0) i += room;
        if (i + 3 < room) {
            tap[3] = buffer[i];
            tap[2] = buffer[i + 1];
            tap[1] = buffer[i + 2];
            tap[0] = buffer[i + 3];
            return;
        }
        tap[3] = buffer[i];
        if (++i == room) i = 0;
        tap[2] = buffer[i];
        if (++i == room) i = 0;
        tap[1] = buffer[i];
        if (++i == room) i = 0;
        tap[0] = buffer[i];
    }

    // The loop as it was `length + bend` samples ago, through the cubic that
    // passes through its four nearest samples (Lagrange): with no bend, the
    // sample itself. At 4 kHz and half way between samples it
    // loses 0.15 % a pass, where a pipe's walls take some 5 %.
    static float Bent(const float* buffer, int room, int write, int length, float bend) {
        int whole = static_cast<int>(bend);
        if (static_cast<float>(whole) > bend) --whole;
        const float d = bend - static_cast<float>(whole);
        float tap[4];
        Taps(buffer, room, write, length, whole, tap);
        const float newer = d * (d - 1.0f) * (d - 2.0f) * (-1.0f / 6.0f), here = (d + 1.0f) * (d - 1.0f) * (d - 2.0f) * 0.5f;
        const float older = (d + 1.0f) * d * (d - 2.0f) * -0.5f, oldest = (d + 1.0f) * d * (d - 1.0f) * (1.0f / 6.0f);
        return newer * tap[0] + here * tap[1] + older * tap[2] + oldest * tap[3];
    }

#if defined(OREGANA_FOUR)
    static Float4 BentFour(float* const buffer[4], const int room[4], const int write[4], const int length[4], Float4 bend) {
        Int4 whole = Truncated(bend);
        whole += ToFloat(whole) > bend;
        const Float4 d = bend - ToFloat(whole);
        float tap[4][4];
        for (int l = 0; l < 4; ++l) Taps(buffer[l], room[l], write[l], length[l], whole[l], tap[l]);
        const Float4 newer = d * (d - 1.0f) * (d - 2.0f) * (-1.0f / 6.0f), here = (d + 1.0f) * (d - 1.0f) * (d - 2.0f) * 0.5f;
        const Float4 older = (d + 1.0f) * d * (d - 2.0f) * -0.5f, oldest = (d + 1.0f) * d * (d - 1.0f) * (1.0f / 6.0f);
        return newer * Float4 { tap[0][0], tap[1][0], tap[2][0], tap[3][0] } + here * Float4 { tap[0][1], tap[1][1], tap[2][1], tap[3][1] }
            + older * Float4 { tap[0][2], tap[1][2], tap[2][2], tap[3][2] } + oldest * Float4 { tap[0][3], tap[1][3], tap[2][3], tap[3][3] };
    }
#endif

    // The upper pipe's buffer and its share of the bend: `trip` is its
    // samples there and back, `pass` the loop's.
    void SetUpperBend(double trip, double pass) {
        const int most = static_cast<int>(trip * kBendMost);
        upperRoom = upperLength + most + 4;
        upperShare = loopPass > 0.0f && upperLength >= most + 3 ? static_cast<float>(trip / pass) : 0.0f;
    }
};

static_assert(offsetof(FluePipe, dcIn) - offsetof(FluePipe, allpassIn) == 3 * sizeof(float)
                  && offsetof(FluePipe, breathOut) - offsetof(FluePipe, dcOut) == 3 * sizeof(float)
                  && offsetof(FluePipe, foot) - offsetof(FluePipe, breathSlow) == 3 * sizeof(float)
                  && offsetof(FluePipe, bendNow) - offsetof(FluePipe, filledFrom) == 3 * sizeof(float),
              "FluePipe per-sample state blocks must be contiguous");

} // namespace oregana
