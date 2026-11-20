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
// The jet leaves the flue straight while the air in the mouth moves with the
// sound, so the wave returning to the mouth deflects it by the air's
// displacement there (Fletcher and Rossing, The Physics of Musical
// Instruments, section 16.3). It carries that deflection across the cut-up in
// its transit time (a delay that follows the wind: transit time ~ 1 /
// sqrt(foot pressure)), and at the lip blows into or out of the pipe by a
// saturating law (the flow of a jet with a bell-shaped velocity profile
// against a lip offset from its centre, ibid.). The jet's flow pushes the air
// in the mouth, so what drives the loop is the rate at which that flow
// changes, the jet-drive source of Verge et al. (JASA 101, 1997), Fabre,
// Hirschberg and Wijnands (Acustica 86, 2000) and Auvray, Fabre and Lagree
// (JASA 131, 2012): it weights each harmonic of the jet's flow by its number,
// which gives a jet standing off the lip the strong octave every recorded
// principal has (Docs/research/physical-gaps-2026-11-18.md), where a flow
// driving the loop made odd harmonics. The rate of change gives back the
// quarter period the displacement takes, so at the note the loop is the
// flow's. Nothing else makes the tone:
// the pipe speaks when the wind lets the loop's gain pass one, settles where
// the jet saturates and the lip sheds vortices (a loss that grows with the
// mouth's flow, Verge et al., JASA 101, 1997, and Dequand et al., JASA 113,
// 2003: it is why a pipe that speaks fast does not grow as loud as its
// starting gain promises), goes flat as the wind sags, and rings down on its
// own resonances when the pallet closes. Turbulence in the jet is the wind
// noise; the puff of the wind's arrival is the chiff.
//
// A pipe is designed from its voicing and its note (Design), then tuned and
// regulated by measurement (PipeRegulation.h), as a voicer does.

#include "DspMath.h"

#include <cmath>
#include <cstdint>

namespace oregana {

// What the pipe maker and the voicer chose for a rank. Dimensions are typical
// of each kind of stop, not measurements of any one organ.
struct FlueVoicing {
    bool stopped = false;
    double scale = 0.0;      // diameter, halftones wider than Toepfer's Normalmensur (155.5 mm at 8' C, halving on the 17th pipe)
    double q = 40.0;         // Q of the first resonance
    // The jet's transit across the cut-up, periods of the note. A low-pass
    // takes what the transit leaves of the jet's lag (Design): at 0.33 its
    // corner fell under the note, a pipe heard more of its jet below the note
    // than at it, and principals of 0.7 to 1.5 kHz jumped 40 cents sharp at
    // Wind Pressure 1.5 or fell into chaos. At 0.4 none did from 0.6 to 1.6,
    // and pitch and level follow the wind as 7 to 1 in cents to dB (0.9 cents
    // and 0.13 dB for 1 %), as a recorded pipe's wander does
    // (Docs/research/known-gaps-2026-11-13.md).
    double jetTransit = 0.4;
    double drive = 1.5;      // the jet's swing at the lip in steady speech, jet half-widths
    double offset = 1.2;     // the lip's offset from the jet's centre, jet half-widths
    double noise = 0.006;    // turbulence, jet half-widths
    double chiff = 0.5;      // the wind's first puff into the pipe
    double shedding = 4.0;   // the loss to vortices shed at the lip in steady speech, against the linear losses
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

// Nominal wind consumption, relative to an 8' C principal.
inline double PipeWindLoad(double radius, bool reed) {
    return (reed ? 2.0 : 4.0) * radius * radius / (0.1555 * 0.1555);
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

// The breath a pipe sounds is not white: between kBreathFloor and
// kBreathCorner times the note it is kBreathLow times as strong, and from
// there it falls 6 dB an octave to the white level near the fourteenth
// harmonic. Calibrated against two recorded organs
// (Docs/research/benchmark-ilmenau-2026-11-13.md): between their harmonics a
// principal and a flute carry most noise about the first two and 15 to 20 dB
// less by the twelfth, where white turbulence left the model 25 dB short at
// the bottom and right at the top. The pipe hears the rate of change of the
// jet's flow (Design's source), so the turbulence that moves the jet is that
// breath integrated once, and under the floor it falls away: slower
// turbulence made a small pipe's level wander (a 2960 Hz pipe by 1.4 dB),
// which no recorded pipe did.
inline constexpr double kBreathLow = 20.0;
inline constexpr double kBreathCorner = 0.7, kBreathFloor = 0.15;

struct FluePipe {
    // The loop.
    int loopLength = 1;
    float mouthAllpass = 0.0f, allpass = 0.0f, lowpass = 1.0f, loopGain = 0.0f, dcPole = 0.0f;
    // The jet.
    int jetLength = 4;
    float jetNominal = 1.0f, jetLongest = 1.0f;   // transit at nominal wind and the longest kept, samples
    float follow = 0.0f;   // the jet's hold on the air's displacement (a leaky integrator's pole)
    float jetLowpass = 1.0f, jetGain = 0.0f, offset = 0.0f, source = 0.0f, noise = 0.0f, puff = 0.0f;
    float breathPole = 0.0f, breathFloor = 0.0f, breathLow = 0.0f;   // the turbulence's band and its strength there (kBreathLow)
    float integrated = 0.0f;   // the integrated breath's share of the turbulence
    float shed = 0.0f;
    // The pipe beyond a junction (a chimney, or a harmonic pipe's upper
    // half): its round trip, what the junction passes each way, and its open top.
    int upperLength = 0;
    float upFromBelow = 0.0f, upFromAbove = 0.0f, downFromBelow = 1.0f, downFromAbove = 0.0f;
    float upperLowpass = 1.0f, upperGain = 0.0f, upperAllpass = 0.0f;
    // The foot and the radiation. The foot fills at `attack` while its
    // pallet opens and at `refill`, through its toe alone, once it has caught
    // up with its channel; it empties at `release`.
    float attack = 1.0f, refill = 1.0f, release = 1.0f, radiation = 0.0f;
    float gainLeft = 0.0f, gainRight = 0.0f;
    double releaseSeconds = 0.0;
    double air = 0.0;   // the wind it takes at nominal pressure, against an 8' C principal
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
    // to bend), `upperShare` the upper pipe's against the loop's, `jetPass`
    // the jet's transit's.
    int loopRoom = 1, upperRoom = 0;
    float loopPass = 0.0f, upperShare = 0.0f, jetPass = 0.0f;
    // A filter as it was made and what a unit of stretch moves it by (MakeWarm).
    struct Warming {
        float made = 0.0f, by = 0.0f;
        float At(float s) const { return made + by * s; }
    };
    Warming mouthWarm, allpassWarm, lowpassWarm, dcWarm, radiationWarm, jetLowpassWarm, upperLowpassWarm, upperAllpassWarm;
    float bendNow = 0.0f, bendStep = 0.0f, bendTarget = 0.0f;
    float upperOut = 0.0f, upperAllpassIn = 0.0f, upperAllpassOut = 0.0f;
    float mouthIn = 0.0f, mouthOut = 0.0f, allpassIn = 0.0f, allpassOut = 0.0f, lowpassOut = 0.0f, dcIn = 0.0f, dcOut = 0.0f;
    float jetOut = 0.0f, radiationIn = 0.0f, radiationOut = 0.0f, breathOut = 0.0f, breathSlow = 0.0f, breathSum = 0.0f;
    float displaced = 0.0f;  // the air's displacement at the flue, as the jet takes it
    float jetShare = 0.0f;   // the jet's share at the lip (JetShape), the sample before
    float jetRate = 0.0f;    // its rate of change as the compact source passes it
    float foot = 0.0f, wind = 0.0f;       // foot pressure and what the pallet offers it, against nominal
    float rising = 1.0f;                  // how fast the foot fills now: `attack` or `refill`
    float jetTransit = 1.0f, jetStep = 0.0f;
    std::uint32_t random = 1u;
    float filledFrom = 0.0f;   // the foot's pressure when its wind last began to rise
    float puffNow = 0.0f;      // the puff's drive: the gap times the pressure gained, or what is left of it
    int quiet = 0;

    static constexpr int kTick = 32;   // samples between Tick() calls
    // A foot within 2 % of its channel's pressure, about what its own wind
    // wavers by, has caught up with it: the pallet stands open, and the foot
    // follows the channel through its toe.
    static constexpr float kCaughtUp = 0.02f;
    static constexpr float kPipeSilent = 1.0e-5f;
    // The most the air's bend stretches or shortens a pipe: 10 C either way,
    // 29.5 cents.
    static constexpr double kBendMost = 0.0173;
    static constexpr float kLongestJet = 3.0f;   // the transit kept as the wind falls, against nominal
    static constexpr double kTopLip = 0.35;   // the top octave's lip offset at most, against the swing

    // `resonance` is the tuner's correction: the loop's first resonance
    // against the note. Buffers needed: loopRoom, jetLength and
    // upperRoom floats.
    void Design(const FlueVoicing& v, double hz, double resonance) {
        const double w = kTwoPi * hz / kPipeRate, wr = w * resonance;
        // A period under ten samples cannot carry a hard-blown jet: above
        // 5 kHz the loop breaks up or falls silent where the swing passes
        // 2.5 half-widths or the lip sheds hard (ProbePipe, 5.3 to 8.4 kHz,
        // at drives 1.8 to 4). The top octave is voiced gently instead, which
        // costs harmonics above 10 kHz only: it sheds half as much, and its
        // lip comes to within kTopLip of the swing, for a lip far off the jet
        // starts the pipe only by the shedding's help (a principal's 6 kHz
        // pipe never spoke). With no shedding there, the top took 30 to 50
        // ms to speak where its octave below takes 5 to 10.
        const double top = Clamp(std::log2(hz / 4186.0) * 2.0, 0.0, 1.0);
        const double drive = v.drive + top * (Min(v.drive, 2.2) - v.drive);
        const double lip = drive / v.drive * (v.offset + top * (Min(v.offset, kTopLip * v.drive) - v.offset));
        const double shedding = v.shedding * (1.0 - 0.5 * top);
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
        // The mouth's end correction, 2.9 radii for a mouth a quarter of the
        // circumference wide and a quarter of that high (Ingerslev and
        // Frobenius, 1947), there and back.
        const double mouth = Max(1.1, 2.0 * 2.9 * radius / kSoundSpeed * kPipeRate);
        const double rest = pass - (lpLag - dcLead) / w;
        loopLength = Max(1, static_cast<int>(std::lround(rest - mouth)));
        // The mouth's correction is an allpass's, which shortens as the
        // frequency rises: the pipe's upper resonances stand sharp of its
        // harmonics. It is the mouth's own, a sample short of it, and a
        // second allpass of 0.5 to 1.5 samples takes up what the tuner's
        // whole samples leave. In one allpass with that remainder the
        // correction's spread stepped wherever the tuner moved the loop a
        // sample; under the jet-drive, which hears the upper resonances, a
        // Gambe's pitch jumped 1.5 cents there and no try came within 0.5.
        mouthAllpass = static_cast<float>(detail::AllpassFor(mouth - 1.0, w));
        allpass = static_cast<float>(detail::AllpassFor(rest - (mouth - 1.0) - loopLength, w));
        lowpass = static_cast<float>(lp);
        dcPole = static_cast<float>(dc);
        // The first resonance's Q is the loss of one period: one pass of an
        // open pipe, two of a stopped one.
        const double perPeriod = 1.0 - kPi / v.q;
        const double walls = Min((v.stopped ? std::sqrt(perPeriod) : perPeriod) / (lpGain * dcGain), 0.9999);
        loopGain = static_cast<float>(v.stopped ? -walls : walls);

        // The jet drives the pipe by the rate of change of its flow at the
        // lip (the jet-drive source, at the head of this file), a compact
        // source while the mouth is small against the wavelength: it
        // differentiates up to the corner where the radiation does, and is
        // level above it, where a rate of change would grow without end. At
        // the note it leads by nearly a quarter period. The jet takes the
        // air's displacement, the returning wave integrated (leaking a decade
        // under the note, as the blocker does), which lags by nearly as much.
        // It turns the wave over, crosses the cut-up and lags as a low-pass
        // does (it carries slow deflections and smooths fast ones), which
        // brings the returning wave back in phase with itself at the note.
        radiation = static_cast<float>(std::exp(-kTwoPi * corner / kPipeRate));
        double rateGain, lead;
        detail::DcBlock(radiation, w, rateGain, lead);
        rateGain *= radiation;
        follow = static_cast<float>(dc);
        double followGain, followLag;
        detail::OnePole(1.0 - dc, w, followGain, followLag);
        followGain /= 1.0 - dc;
        const double budget = kPi + lead - followLag;
        double transit = v.jetTransit * kTwoPi / w;
        double lag = Clamp(budget - w * transit, 0.15, 0.9 * 0.5 * (kPi - w));
        double readGain = 1.0, readLag = 0.0;
        for (int i = 0; i < 4; ++i) {
            transit = Max(0.0, transit);
            detail::Interpolated(transit, w, readGain, readLag);
            transit += (budget - lag - readLag) / w;
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
        // source is sized for that, and the jet's gain for the swing asked for.
        const double g = walls * lpGain * dcGain;
        const double fundamental = JetFundamental(drive, lip);
        const double returning = g / (1.0 + g), lost = (1.0 - g) / g;
        // The rate of change passes rateGain of the note, so the source is
        // sized at the note as a flow would be.
        source = static_cast<float>(returning * lost * (1.0 + shedding) / (fundamental * rateGain));
        shed = static_cast<float>(shedding * lost * 3.0 * kPi / (8.0 * returning));
        jetGain = static_cast<float>(drive / (returning * followGain * jetLpGain * readGain));
        offset = static_cast<float>(lip);
        // The pipe hears the turbulence's rate of change, as it hears the
        // jet's: what crosses the mouth is the calibrated breath integrated
        // once below the source's corner (and leaking under kBreathFloor
        // times the note), sized at the note, so that the pipe hears the
        // breath itself.
        noise = static_cast<float>(v.noise * rateGain);
        integrated = static_cast<float>((1.0 - radiation) / radiation);
        breathPole = static_cast<float>(1.0 - std::exp(-kTwoPi * kBreathCorner * hz / kPipeRate));
        breathFloor = static_cast<float>(1.0 - std::exp(-kTwoPi * kBreathFloor * hz / kPipeRate));
        // The top octave's short loops cannot carry it either: at 7.4 kHz it
        // threw a pipe to the octave below.
        breathLow = static_cast<float>(kBreathLow * (1.0 - top));
        puff = static_cast<float>(1.2 * v.chiff);

        // Beyond a junction. Neither a chimney nor a harmonic pipe's upper
        // half is the tuner's to move: the builder cut them for the note.
        if (v.stopped && v.chimney > 0.0) {
            // A wave meeting a bore of s times the area turns back by
            // (1 - s) / (1 + s). The chimney's top radiates as an opening of
            // its own radius.
            const double area = v.chimney * v.chimney;
            const double bore = (1.0 - area) / (1.0 + area);
            upFromBelow = static_cast<float>(1.0 + bore);
            upFromAbove = static_cast<float>(-bore);
            downFromBelow = static_cast<float>(bore);
            downFromAbove = static_cast<float>(1.0 - bore);
            // Its length there and back is a whole number of samples and
            // an allpass for the rest: cut to whole samples alone, it stepped
            // the pipe's pitch by up to 4 cents as the organ was retuned.
            const double chimney = v.chimneyLength * kPi / w;
            upperLength = Max(1, static_cast<int>(std::lround(chimney - 1.0)));
            upperAllpass = static_cast<float>(detail::AllpassFor(Max(chimney - upperLength, 0.1), w));
            const double opening = Min(kSoundSpeed / (kTwoPi * radius * v.chimney), 0.45 * kPipeRate);
            upperLowpass = static_cast<float>(1.0 - std::exp(-kTwoPi * opening / kPipeRate));
            upperGain = static_cast<float>(std::sqrt(perPeriod));
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
            upperAllpass = 0.0f;
            const double topPole = std::sin(fill) / std::sin(w + fill);
            detail::OnePole(1.0 - topPole, w, topGain, topLag);
            upperLowpass = static_cast<float>(1.0 - topPole);
            upperGain = static_cast<float>(Min(perPeriod / topGain, 0.9999));
        } else {
            upperLength = 0;
        }

        // The foot fills through the pallet and the toe, the faster the
        // smaller the pipe, and empties through the flue. The pallet's
        // share is Castellengo's "short" attack (ISMA 2004): on a chest
        // whose pallet opened at chosen rates, a 475 Hz pipe's foot crossed
        // the pressure the pipe speaks at, 125 of 360 Pa, 9 ms after the
        // pallet began to open (4 ms "impulsive", 40 ms "slow") and was
        // full in 45 ms, and a one-pole crosses it at 0.43 of its time. So
        // a pipe breathes before its tone, as recorded ones do, and a hard
        // stroke, which halves it, is the impulsive attack. Which measured
        // attack is an ordinary stroke is the ear's (Docs/decisions.md).
        // Once caught up (kCaughtUp), and from the start when a stop's
        // slider brings the pipe on under a pallet already open, the foot
        // follows its channel through the toe alone: the pallet's time is
        // its opening's. Kept for every rise of the wind, it slowed a held
        // pipe's answer to its wind's bounce and bunched the crescendo
        // roller's stops into a step.
        attackSeconds = v.speech * (0.0204 + 0.35 / hz);
        refill = static_cast<float>(1.0 - std::exp(-1.0 / (v.speech * 0.35 / hz * kPipeRate)));
        releaseSeconds = 0.004 + 0.5 / hz;
        SetTouch(1.0, true);
        SetRelease(1.0);
        // Wind through the flue: its area goes with the diameter squared.
        air = PipeWindLoad(radius, false);
        // The mouth is a small source: a flow Q changing at dQ/dt makes a
        // pressure rho / (4 pi d) dQ/dt at d metres. With the flow as the
        // pipe's area times its wave over rho c, that is r^2 / (4 d c) of the
        // wave's rate of change, here per sample and for d = 1. That holds
        // while the opening is small against the wavelength, as far as the
        // radiation filter differentiates, so the neighbour hears that
        // filter's output: scaled here to the same figure below its corner,
        // and level above it, where a rate of change would grow without end.
        telling = static_cast<float>(radius * radius * kPipeRate / (4.0 * kSoundSpeed) * (1.0 - radiation) / radiation);
        MakeWarm(w);
    }

    // How fast the key opens the pallet: 1 is an ordinary stroke. Not
    // `opening`: the pipe's pallet was open already (a stop drawn under a
    // held key), and its foot fills through the toe alone.
    void SetTouch(double slowness, bool opening) {
        attack = static_cast<float>(1.0 - std::exp(-1.0 / (attackSeconds * slowness * kPipeRate)));
        rising = opening ? attack : refill;
    }
    // How fast the key lets the pallet shut, as SetTouch: the foot empties
    // that much slower or faster (an estimate, the opening's law mirrored).
    void SetRelease(double slowness) { release = static_cast<float>(1.0 - std::exp(-1.0 / (releaseSeconds * slowness * kPipeRate))); }

    void Clear() {
        for (int i = 0; i < loopRoom; ++i) loop[i] = 0.0f;
        for (int i = 0; i < jetLength; ++i) jet[i] = 0.0f;
        for (int i = 0; i < upperRoom; ++i) upper[i] = 0.0f;
        loopAt = jetAt = upperAt = 0;
        mouthIn = mouthOut = allpassIn = allpassOut = lowpassOut = dcIn = dcOut = jetOut = radiationIn = radiationOut = upperOut = breathOut = breathSlow = breathSum = 0.0f;
        displaced = jetShare = jetRate = 0.0f;
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
        Warm(stretch);
    }

    // The filters as the air's warmth has them: at rest exactly as made.
    void Warm(float stretch) {
        const float s = loopPass > 0.0f ? stretch : 0.0f;
        mouthAllpass = mouthWarm.At(s);
        allpass = allpassWarm.At(s);
        lowpass = lowpassWarm.At(s);
        dcPole = follow = dcWarm.At(s);
        radiation = radiationWarm.At(s);
        jetLowpass = jetLowpassWarm.At(s);
        upperLowpass = upperLowpassWarm.At(s);
        upperAllpass = upperAllpassWarm.At(s);
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
        Warm(stretch);
        // A foot all but empty is empty. Left to fall it passes, long before
        // the pipe has rung out, through numbers too small for a processor
        // to keep at speed (denormals): a host that does not flush them paid
        // twice over for every ringing pipe (full organ let go, 0.5 s on: 15 %
        // of a core against 7 %). For the same reason the puff takes the gap
        // only while the foot fills, and so never squares a falling foot.
        if (wind == 0.0f && foot < 1.0e-25f) foot = 0.0f;
        const float pressure = Max(foot, 1.0f / (kLongestJet * kLongestJet));
        // The jet's transit stretches with the air (MakeWarm).
        const float nominal = jetNominal + jetPass * (loopPass > 0.0f ? stretch : 0.0f);
        const float target = Min(nominal / Sqrt(pressure), jetLongest);
        jetStep = Clamp((target - jetTransit) * (1.0f / kTick), -0.5f, 0.5f);
        const float level = Abs(lowpassOut) + Abs(mouthOut) + Abs(allpassOut) + Abs(radiationOut) + foot;
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
        float lipIn = mouthIn, lipOut = mouthOut;
        float apIn = allpassIn, apOut = allpassOut, lpOut = lowpassOut, blockIn = dcIn, blockOut = dcOut;
        float jetLp = jetOut, radIn = radiationIn, radOut = radiationOut, pressure = foot, transit = jetTransit, breath = breathOut, slow = breathSlow;
        float sum = breathSum, moved = displaced;
        float shareBefore = jetShare, rate = jetRate;
        float filled = filledFrom, pushing = puffNow;
        const float keep = 1.0f - release;
        std::uint32_t seed = random;
        const float target = wind, toe = refill, fall = release;
        float rise = rising;
        const float jetSize = static_cast<float>(jetLength);
        for (int i = 0; i < frames; ++i) {
            const float gap = target - pressure;
            rise = gap > target * kCaughtUp ? rise : toe;
            pressure += gap * (gap > 0.0f ? rise : fall);
            if (!(gap > 0.0f)) filled = pressure;

            const float x = bent ? Bent(loopBuffer, room, at, loopLength, bend) : loopBuffer[from];
            const float corrected = mouthAllpass * (x - lipOut) + lipIn;
            lipIn = x;
            lipOut = corrected;
            const float ap = allpass * (corrected - apOut) + apIn;
            apIn = corrected;
            apOut = ap;
            float turned = ap;
            if (upperTrip > 0) {
                const float stored = bent ? Bent(upperBuffer, upperSize, upperWrite, upperTrip, share * bend) : upperBuffer[upperRead];
                const float back = upperAllpass * (stored - upperApOut) + upperApIn;
                upperApIn = stored;
                upperApOut = back;
                upperLp += upperLowpass * (upFromBelow * ap + upFromAbove * back - upperLp);
                upperBuffer[upperWrite] = -upperGain * upperLp;
                if (++upperWrite == upperSize) upperWrite = 0;
                if (++upperRead == upperSize) upperRead = 0;
                turned = downFromBelow * ap + downFromAbove * back;
            }
            lpOut += lowpass * (turned - lpOut);
            const float passed = loopGain * lpOut;
            const float returned = passed - blockIn + dcPole * blockOut;
            blockIn = passed;
            blockOut = returned;

            moved = returned + follow * moved;
            jetBuffer[jetWrite] = moved;
            float read = static_cast<float>(jetWrite) - transit;
            if (read < 0.0f) read += jetSize;
            // A read just under 0 rounds up to the buffer's length as a float:
            // the buffer's start. (Left at the length, with only its sample
            // wrapped, it read that many samples' slope beyond the start: a
            // click as loud as the pipe, once in hours of a wavering wind.)
            if (read >= jetSize) read = 0.0f;
            const int older = static_cast<int>(read);
            const int newer = older + 1 == jetLength ? 0 : older + 1;
            const float between = read - static_cast<float>(older);
            const float deflection = jetBuffer[older] + between * (jetBuffer[newer] - jetBuffer[older]);
            if (++jetWrite == jetLength) jetWrite = 0;
            jetLp += jetLowpass * (deflection - jetLp);

            const float white = NextNoise(seed);
            breath += breathPole * (white - breath);
            slow += breathFloor * (white - slow);
            const float shaped = white - slow + breathLow * (breath - slow);
            sum += shaped - breathFloor * sum;
            const float turbulence = noise * (shaped + integrated * sum);
            // The wind's arrival pushes a puff of air into the pipe, largest
            // midway through the foot's filling, and counted from where the
            // filling began: counted from an empty foot, a key struck again
            // within 20 ms, its foot still part full, began its puff with a
            // step (a click as large as the pipe's whole sound at c1). Wind
            // taken away in mid-puff (a note under 10 ms) lets the puff die
            // as the foot empties: cut off, it ended with the same step.
            if (gap > 0.0f) pushing = gap * (pressure - filled);
            else pushing = pushing > 1.0e-20f ? pushing * keep : 0.0f;
            // The jet's speed is taken as it stands: what its rise pushes in
            // at the onset is the puff.
            const float shape = JetShape(jetGain * jetLp + offset + turbulence);
            rate = radiation * (rate + shape - shareBefore);
            shareBefore = shape;
            const float blown = -source * pressure * rate + puff * pushing;
            // The lip sheds at most the whole of the wave that reaches it: a
            // loss turns no wave back inverted, and taken past that the
            // square law grew without end where the jet threw a loud upper
            // mode at it (a 4186 Hz pipe at twice its wind).
            const float kept = 1.0f - shed * Abs(returned);
            float leaving = returned * Max(kept, 0.0f) + blown;
            // The pressure a neighbour makes at the mouth enters as a wave.
            if (heard != nullptr) leaving += heard[i];
            loopBuffer[at] = leaving;
            if (++at == room) at = 0;
            if (++from == room) from = 0;
            bend += bendBy;

            // The mouth's flow radiates as its rate of change up to the
            // corner where the opening is a wavelength round, evenly above.
            const float flow = returned + leaving;
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
        mouthIn = lipIn;
        mouthOut = lipOut;
        allpassIn = apIn;
        allpassOut = apOut;
        lowpassOut = lpOut;
        dcIn = blockIn;
        dcOut = blockOut;
        jetOut = jetLp;
        displaced = moved;
        jetShare = shareBefore;
        jetRate = rate;
        breathOut = breath;
        breathSlow = slow;
        breathSum = sum;
        radiationIn = radIn;
        radiationOut = radOut;
        foot = pressure;
        rising = rise;
        filledFrom = filled;
        puffNow = pushing;
        jetTransit = Clamp(transit, 0.0f, jetLongest);
        random = seed;
    }

#if defined(OREGANA_FOUR)
    // Run for four pipes at once, lane for lane the same arithmetic. `Upper`:
    // all four have a pipe beyond a junction, else none has. `Hears`: all
    // four hear neighbours and tell them, else none does.
    template <bool Upper, bool Hears>
    static void RunFour(FluePipe* const pipe[4], float* const out[4], int frames, const float* const heard[4] = nullptr,
                        float* const told[4] = nullptr) {
        bool bent = false;
        for (int l = 0; l < 4; ++l) bent = bent || pipe[l]->bendNow != 0.0f || pipe[l]->bendStep != 0.0f;
        if (bent) RunFourAs<Upper, Hears, true>(pipe, out, frames, heard, told);
        else RunFourAs<Upper, Hears, false>(pipe, out, frames, heard, told);
    }

    // `Bends`: the loops are read between their samples, where the air's
    // bend has them; else none is bent, and each is read where it was made.
    template <bool Upper, bool Hears, bool Bends>
    static void RunFourAs(FluePipe* const pipe[4], float* const out[4], int frames, const float* const heard[4],
                          float* const told[4]) {
#define OREGANA_LANES(field) Float4 { pipe[0]->field, pipe[1]->field, pipe[2]->field, pipe[3]->field }
        const Float4 mouthC = OREGANA_LANES(mouthAllpass), allpassC = OREGANA_LANES(allpass);
        const Float4 lowpassC = OREGANA_LANES(lowpass), loopGainC = OREGANA_LANES(loopGain);
        const Float4 dcPoleC = OREGANA_LANES(dcPole), jetLowpassC = OREGANA_LANES(jetLowpass), jetGainC = OREGANA_LANES(jetGain);
        const Float4 offsetC = OREGANA_LANES(offset), sourceC = OREGANA_LANES(source), noiseC = OREGANA_LANES(noise);
        const Float4 puffC = OREGANA_LANES(puff), shedC = OREGANA_LANES(shed), radiationC = OREGANA_LANES(radiation);
        const Float4 upFromBelowC = OREGANA_LANES(upFromBelow), upFromAboveC = OREGANA_LANES(upFromAbove);
        const Float4 downFromBelowC = OREGANA_LANES(downFromBelow), downFromAboveC = OREGANA_LANES(downFromAbove);
        const Float4 upperLowpassC = OREGANA_LANES(upperLowpass), upperGainC = OREGANA_LANES(upperGain), upperAllpassC = OREGANA_LANES(upperAllpass);
        const Float4 target = OREGANA_LANES(wind), toe = OREGANA_LANES(refill), fall = OREGANA_LANES(release);
        Float4 rise = OREGANA_LANES(rising);
        const Float4 tellingC = OREGANA_LANES(telling);
        Float4 upperLp = OREGANA_LANES(upperOut), upperApIn = OREGANA_LANES(upperAllpassIn), upperApOut = OREGANA_LANES(upperAllpassOut);
        Float4 lipIn = OREGANA_LANES(mouthIn), lipOut = OREGANA_LANES(mouthOut);
        Float4 apIn = OREGANA_LANES(allpassIn), apOut = OREGANA_LANES(allpassOut), lpOut = OREGANA_LANES(lowpassOut);
        Float4 blockIn = OREGANA_LANES(dcIn), blockOut = OREGANA_LANES(dcOut), jetLp = OREGANA_LANES(jetOut);
        Float4 breath = OREGANA_LANES(breathOut), slow = OREGANA_LANES(breathSlow), sum = OREGANA_LANES(breathSum);
        const Float4 breathPoleC = OREGANA_LANES(breathPole), breathFloorC = OREGANA_LANES(breathFloor), breathLowC = OREGANA_LANES(breathLow);
        Float4 radIn = OREGANA_LANES(radiationIn), radOut = OREGANA_LANES(radiationOut), pressure = OREGANA_LANES(foot);
        Float4 filled = OREGANA_LANES(filledFrom), pushing = OREGANA_LANES(puffNow), shareBefore = OREGANA_LANES(jetShare);
        Float4 rate = OREGANA_LANES(jetRate), moved = OREGANA_LANES(displaced);
        const Float4 integratedC = OREGANA_LANES(integrated), followC = OREGANA_LANES(follow);
        const Float4 keep = Four(1.0f) - fall;
#undef OREGANA_LANES
        const Float4 zero = Four(0.0f), one = Four(1.0f);
        float* loopBuffer[4];
        float* jetBuffer[4];
        float* upperBuffer[4];
        int at[4], from[4], upperWrite[4], upperRead[4], loopSize[4], upperTrip[4], room[4], upperSize[4];
        for (int l = 0; l < 4; ++l) {
            loopBuffer[l] = pipe[l]->loop;
            jetBuffer[l] = pipe[l]->jet;
            upperBuffer[l] = pipe[l]->upper;
            at[l] = pipe[l]->loopAt;
            upperWrite[l] = pipe[l]->upperAt;
            loopSize[l] = pipe[l]->loopLength;
            upperTrip[l] = pipe[l]->upperLength;
            room[l] = pipe[l]->loopRoom;
            upperSize[l] = pipe[l]->upperRoom;
            from[l] = at[l] - loopSize[l];
            if (from[l] < 0) from[l] += room[l];
            upperRead[l] = upperWrite[l] - upperTrip[l];
            if (upperRead[l] < 0) upperRead[l] += upperSize[l];
        }
        Float4 bend = { pipe[0]->bendNow, pipe[1]->bendNow, pipe[2]->bendNow, pipe[3]->bendNow };
        const Float4 bendBy = { pipe[0]->bendStep, pipe[1]->bendStep, pipe[2]->bendStep, pipe[3]->bendStep };
        const Float4 share = { pipe[0]->upperShare, pipe[1]->upperShare, pipe[2]->upperShare, pipe[3]->upperShare };
        Int4 jetWrite = { pipe[0]->jetAt, pipe[1]->jetAt, pipe[2]->jetAt, pipe[3]->jetAt };
        const Int4 jetCount = { pipe[0]->jetLength, pipe[1]->jetLength, pipe[2]->jetLength, pipe[3]->jetLength };
        const Float4 jetSize = ToFloat(jetCount);
        Float4 transit = { pipe[0]->jetTransit, pipe[1]->jetTransit, pipe[2]->jetTransit, pipe[3]->jetTransit };
        const Float4 step = { pipe[0]->jetStep, pipe[1]->jetStep, pipe[2]->jetStep, pipe[3]->jetStep };
        UInt4 seed = { pipe[0]->random, pipe[1]->random, pipe[2]->random, pipe[3]->random };
        for (int i = 0; i < frames; ++i) {
            const Float4 gap = target - pressure;
            rise = Choose(gap > target * Four(kCaughtUp), rise, toe);
            const Int4 filling = gap > zero;
            pressure += gap * Choose(filling, rise, fall);
            filled = Choose(filling, filled, pressure);
            pushing = Choose(filling, gap * (pressure - filled), Where(pushing > Four(1.0e-20f), pushing * keep));

            Float4 x;
            if (Bends) x = BentFour(loopBuffer, room, at, loopSize, bend);
            else x = Float4 { loopBuffer[0][from[0]], loopBuffer[1][from[1]], loopBuffer[2][from[2]], loopBuffer[3][from[3]] };
            const Float4 corrected = mouthC * (x - lipOut) + lipIn;
            lipIn = x;
            lipOut = corrected;
            const Float4 ap = allpassC * (corrected - apOut) + apIn;
            apIn = corrected;
            apOut = ap;
            Float4 turned = ap;
            if (Upper) {
                Float4 stored;
                if (Bends) stored = BentFour(upperBuffer, upperSize, upperWrite, upperTrip, share * bend);
                else stored = Float4 { upperBuffer[0][upperRead[0]], upperBuffer[1][upperRead[1]], upperBuffer[2][upperRead[2]],
                                       upperBuffer[3][upperRead[3]] };
                const Float4 back = upperAllpassC * (stored - upperApOut) + upperApIn;
                upperApIn = stored;
                upperApOut = back;
                upperLp += upperLowpassC * (upFromBelowC * ap + upFromAboveC * back - upperLp);
                const Float4 up = -upperGainC * upperLp;
                for (int l = 0; l < 4; ++l) {
                    upperBuffer[l][upperWrite[l]] = up[l];
                    if (++upperWrite[l] == upperSize[l]) upperWrite[l] = 0;
                    if (++upperRead[l] == upperSize[l]) upperRead[l] = 0;
                }
                turned = downFromBelowC * ap + downFromAboveC * back;
            }
            lpOut += lowpassC * (turned - lpOut);
            const Float4 passed = loopGainC * lpOut;
            const Float4 returned = passed - blockIn + dcPoleC * blockOut;
            blockIn = passed;
            blockOut = returned;

            moved = returned + followC * moved;
            for (int l = 0; l < 4; ++l) jetBuffer[l][jetWrite[l]] = moved[l];
            Float4 read = ToFloat(jetWrite) - transit;
            read += Where(read < zero, jetSize);
            // A read just under 0 rounds up to the buffer's length as a float:
            // the buffer's start (Run).
            read = Unless(read >= jetSize, read);
            const Int4 older = Truncated(read);
            Int4 newer = older + 1;
            newer &= ~(newer == jetCount);
            const Float4 between = read - ToFloat(older);
            const Float4 early = { jetBuffer[0][older[0]], jetBuffer[1][older[1]], jetBuffer[2][older[2]], jetBuffer[3][older[3]] };
            const Float4 late = { jetBuffer[0][newer[0]], jetBuffer[1][newer[1]], jetBuffer[2][newer[2]], jetBuffer[3][newer[3]] };
            const Float4 deflection = early + between * (late - early);
            jetWrite += 1;
            jetWrite &= ~(jetWrite == jetCount);
            transit += step;
            const Float4 white = NextNoise(seed);
            jetLp += jetLowpassC * (deflection - jetLp);

            breath += breathPoleC * (white - breath);
            slow += breathFloorC * (white - slow);
            const Float4 shaped = white - slow + breathLowC * (breath - slow);
            sum += shaped - breathFloorC * sum;
            const Float4 turbulence = noiseC * (shaped + integratedC * sum);
            const Float4 swing = jetGainC * jetLp + offsetC + turbulence;
            const Float4 swing2 = swing * swing;
            const Float4 shape = Choose(swing > Four(3.0f), one,
                                        Choose(swing < Four(-3.0f), -one, swing * (27.0f + swing2) / (27.0f + 9.0f * swing2)));
            rate = radiationC * (rate + shape - shareBefore);
            shareBefore = shape;
            const Float4 blown = -sourceC * pressure * rate + puffC * pushing;
            const Float4 kept = one - shedC * Abs(returned);
            Float4 leaving = returned * Where(kept > zero, kept) + blown;
            if (Hears) leaving += Float4 { heard[0][i], heard[1][i], heard[2][i], heard[3][i] };
            for (int l = 0; l < 4; ++l) {
                loopBuffer[l][at[l]] = leaving[l];
                if (++at[l] == room[l]) at[l] = 0;
                if (++from[l] == room[l]) from[l] = 0;
            }
            if (Bends) bend += bendBy;

            const Float4 flow = returned + leaving;
            radOut = radiationC * (radOut + flow - radIn);
            radIn = flow;
            if (Hears) {
                const Float4 tells = tellingC * radOut;
                told[0][i] = tells[0];
                told[1][i] = tells[1];
                told[2][i] = tells[2];
                told[3][i] = tells[3];
            }
            out[0][i] = radOut[0];
            out[1][i] = radOut[1];
            out[2][i] = radOut[2];
            out[3][i] = radOut[3];
        }
        for (int l = 0; l < 4; ++l) {
            FluePipe& p = *pipe[l];
            p.loopAt = at[l];
            p.jetAt = jetWrite[l];
            p.upperAt = upperWrite[l];
            p.bendNow = bend[l];
            p.upperOut = upperLp[l];
            p.upperAllpassIn = upperApIn[l];
            p.upperAllpassOut = upperApOut[l];
            p.mouthIn = lipIn[l];
            p.mouthOut = lipOut[l];
            p.allpassIn = apIn[l];
            p.allpassOut = apOut[l];
            p.lowpassOut = lpOut[l];
            p.dcIn = blockIn[l];
            p.dcOut = blockOut[l];
            p.jetOut = jetLp[l];
            p.displaced = moved[l];
            p.jetShare = shareBefore[l];
            p.jetRate = rate[l];
            p.breathOut = breath[l];
            p.breathSlow = slow[l];
            p.breathSum = sum[l];
            p.radiationIn = radIn[l];
            p.radiationOut = radOut[l];
            p.foot = pressure[l];
            p.rising = rise[l];
            p.filledFrom = filled[l];
            p.puffNow = pushing[l];
            p.jetTransit = Clamp(transit[l], 0.0f, p.jetLongest);
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

    // The air's warmth stretches every time in the pipe alike: its lengths,
    // the mouth's correction, the corners of its walls, lip and opening, and
    // the jet's transit, for the jet's speed goes with the speed of sound
    // (both with the root of the temperature). The pitch follows the bend
    // only if all of them do: bending the delays alone left a principal's
    // 65 Hz pipe 3.3 cents short of a 29.5-cent bend, and leaving the
    // allpasses as made took a 4186 Hz pipe 1.5 cents past it. A pole moves
    // as a time constant stretched by 1 + s does, from p to p^(1 / (1 + s)),
    // by -p ln p for a unit of s. An allpass's (1 - c) / (1 + c), its time
    // constant on the bilinear map, grows by 1 + s, and with it the mouth's
    // correction, so the upper resonances move with the first. Both hold
    // while a filter is slow against the sample rate; what they leave of a
    // path's lag at the bent note, a treble pipe's, the path's own delay
    // takes up (the loop's line, the upper pipe's and the jet's transit),
    // for a delay alone stretches exactly. The pipes AirBendsTheFlues
    // sounds then follow the bend to within 0.7 cents.
    void MakeWarm(double w) {
        const auto pole = [](double p) { return p > 0.0 ? -p * std::log(p) : 0.0; };
        const auto turn = [](double c) {
            const double t = (1.0 - c) / (1.0 + c);
            return -2.0 * t / ((1.0 + t) * (1.0 + t));
        };
        const auto make = [](Warming& m, float made, double by) {
            m.made = made;
            m.by = static_cast<float>(by);
        };
        const auto at = [](const Warming& m, double s) { return static_cast<double>(m.made) + static_cast<double>(m.by) * s; };
        const auto allpassLag = [](double c, double x) { return x - 2.0 * std::atan2(c * std::sin(x), 1.0 + c * std::cos(x)); };
        const auto poleLag = [](double p, double x) { return std::atan2(p * std::sin(x), 1.0 - p * std::cos(x)); };
        const auto blockLead = [](double r, double x) {
            double gain, lead;
            detail::DcBlock(r, x, gain, lead);
            return lead;
        };
        // The samples a path's delay adds for a unit of stretch beyond its
        // own length, to keep the lag the path had at the note.
        const auto left = [w](auto lag) {
            constexpr double d = 1.0e-3;
            return (lag(-d) - lag(d)) / (2.0 * d * w);
        };

        make(mouthWarm, mouthAllpass, turn(mouthAllpass));
        make(allpassWarm, allpass, turn(allpass));
        make(lowpassWarm, lowpass, -pole(1.0 - lowpass));
        make(dcWarm, dcPole, pole(dcPole));
        const double pass = loopLength + left([&](double s) {
            const double x = w / (1.0 + s);
            return allpassLag(at(mouthWarm, s), x) + allpassLag(at(allpassWarm, s), x) + poleLag(1.0 - at(lowpassWarm, s), x) -
                   blockLead(at(dcWarm, s), x);
        });
        // The bend reads between a line's samples, four at a time, from one
        // newer than the shortest line to two older than the longest.
        const int most = static_cast<int>(pass * kBendMost);
        loopRoom = loopLength + most + 4;
        loopPass = loopLength >= most + 3 ? static_cast<float>(pass) : 0.0f;

        upperRoom = 0;
        upperShare = 0.0f;
        make(upperLowpassWarm, upperLowpass, 0.0);
        make(upperAllpassWarm, upperAllpass, 0.0);
        if (upperLength > 0) {
            Warming top, back;
            make(top, upperLowpass, -pole(1.0 - upperLowpass));
            make(back, upperAllpass, turn(upperAllpass));
            const double trip = upperLength + left([&](double s) {
                const double x = w / (1.0 + s);
                return allpassLag(at(back, s), x) + poleLag(1.0 - at(top, s), x);
            });
            const int upperMost = static_cast<int>(trip * kBendMost);
            upperRoom = upperLength + upperMost + 4;
            if (loopPass > 0.0f && upperLength >= upperMost + 3) {
                upperShare = static_cast<float>(trip / pass);
                upperLowpassWarm = top;
                upperAllpassWarm = back;
            }
        }

        // The jet's transit is read between two samples, whose lag is not
        // quite its length's at the note.
        make(radiationWarm, radiation, pole(radiation));
        make(jetLowpassWarm, jetLowpass, -pole(1.0 - jetLowpass));
        const auto read = [](double transit, double x) {
            double gain, lag;
            detail::Interpolated(transit, x, gain, lag);
            return lag;
        };
        const double transit = jetNominal, h = 1.0e-4 * w, e = 1.0e-3;
        const double filters = left([&](double s) {
            const double x = w / (1.0 + s);
            return poleLag(at(dcWarm, s), x) + poleLag(1.0 - at(jetLowpassWarm, s), x) - blockLead(at(radiationWarm, s), x);
        });
        const double readDelay = (read(transit, w + h) - read(transit, w - h)) / (2.0 * h);
        const double readSlope = (read(transit + e, w) - read(transit - e, w)) / (2.0 * e * w);
        jetPass = static_cast<float>((readDelay + filters) / readSlope);
    }

    double attackSeconds = 0.003;
};

} // namespace oregana
