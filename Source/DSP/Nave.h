#pragma once

// The church the organ stands in, heard from the nave and crossing.
//
// Cruciform two-box model (Nave + Transept/Crossing) parameterized from real
// ecclesiastical proportions and the golden ratio (phi ~ 1.618). First come
// geometric early reflections from both volumes: Box 1 (nave walls, back wall,
// pew floor, high vault) plus Box 2 (transept end walls, taller crossing dome
// or lantern, and four crossing stone piers) so large cathedrals bloom smoothly
// from onset and sounds originating "from the top" (an organ loft under the
// vault) carry natural overhead canopy cues. Per-source stereo differential
// buffers keep left/right localization synced across Distance.
//
// Early reflections pass through a mono-compatible three-stage common stone
// Schroeder diffuser (zero comb filtering at Width = 0 or L+R sum) followed by
// an orthogonal M/S >220 Hz Side diffuser (zero crossover notches).
//
// Then the coupled reverberant field: two 16-line Hadamard networks (16 Nave
// lines + 16 longer Transept/Crossing lines interleaved around golden-ratio
// prime lengths) mixed via exact double-bent non-Walsh sign masks and coupled
// through an energy-preserving Givens crossing aperture for natural two-slope
// decay. Each line includes Decay, Decay Bass (<250 Hz), surface Decay Treble,
// ISO 9613-1 air absorption, controllable drift modulation, low-frequency stereo
// coherence (<150 Hz), and clickless Freeze.

#include "DspMath.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace oregana {

class Nave {
public:
    enum Architecture { kArchChapel, kArchChurch, kArchBasilica, kArchCathedral, kArchitectures };

    struct BuildingSpec {
        double naveWide, naveLong, naveHigh, fromWall;
        double tranWide, tranLong, crossHigh, crossAt;
        double loftHeight, refDist;
        double floorFactor, wallFactor, vaultFactor;
        double scatterGain, coupling, tranDecayRatio;
    };

    // Real ecclesiastical & golden-ratio (1.618) cruciform proportions.
    // kArchChurch preserves Oregana's 14 m wide x 18 m vault benchmark gallery.
    static constexpr std::array<BuildingSpec, kArchitectures> kArchitecturesSpec { {
        { 7.5,  12.14,  9.0, 1.4, 10.5,  4.6, 10.2, 0.62,  4.6,  8.5,   0.42, 0.88, 0.72, 0.46, 0.14, 1.04 },
        { 14.0, 22.65, 18.0, 2.0, 22.65, 8.65, 22.0, 0.62,  8.0, 19.242, 0.45, 0.90, 0.74, 0.50, 0.22, 1.10 },
        { 17.5, 34.0,  24.0, 2.6, 34.0, 13.0, 31.5, 0.62, 10.5, 22.0,   0.40, 0.91, 0.78, 0.54, 0.30, 1.16 },
        { 22.0, 52.0,  35.6, 3.2, 46.0, 16.5, 44.5, 0.62, 13.5, 30.0,   0.38, 0.92, 0.80, 0.58, 0.36, 1.22 },
    } };

    // 32 lines split across Nave (0..15) and Transept/Crossing (16..31) with
    // golden-ratio interleaved primes. Tiered power-of-2 buffers keep Size 2
    // dense (size^0.65 above Size 1) while halving RAM below 1.9 MiB.
    static constexpr int kBoxLines = 16, kLines = 32;
    static constexpr std::array<int, kLines> kLineSamples { {
        2017, 2239, 2477, 2741, 3037, 3361, 3727, 4129,
        4567, 5059, 5623, 6229, 6907, 7643, 8461, 9371,
        2131, 2371, 2633, 2927, 3251, 3613, 4019, 4463,
        4957, 5479, 6053, 6689, 7393, 8171, 9029, 9973 } };
    static constexpr int kShortCap = 8192, kLongCap = 16384;
    static constexpr double kLineSizeExponent = 0.65;
    // Nominal 0.1 ms peak drift at 0.07..0.5 Hz (scaled by modDepth / 0.25).
    static constexpr double kDriftSamples = 4.8, kSlowestDrift = 0.07, kFastestDrift = 0.5;
    static constexpr int kTurn = 32;

    // Common mono-safe 3-stage early stone/pew diffuser + orthogonal >160 Hz Side diffuser.
    static constexpr std::array<int, 3> kEarlyCommonSamples { { 59, 127, 239 } };
    static constexpr int kEarlySidePreDelay = 139;
    static constexpr std::array<int, 2> kEarlySideSamples { { 173, 331 } };
    static constexpr std::array<float, 2> kEarlySideGains { { -0.52f, 0.52f } };
    static constexpr int kEarlyDiffSize = 512;
    static constexpr double kEarlySideCorner = 160.0, kOverShelfCorner = 5200.0;

    // Independent 4-stage L/R input diffusers into the two 16-line boxes.
    static constexpr std::array<int, 4> kDiffuserSamplesL { { 101, 179, 283, 397 } };
    static constexpr std::array<int, 4> kDiffuserSamplesR { { 109, 193, 293, 419 } };
    static constexpr int kDiffuserSize = 512;
    static constexpr float kDiffuserGain = 0.6f;

    // Architectural & coupling constants kept public for Churech calibration.
    static constexpr double kNaveWide = 14.0, kToListener = 18.0, kFromWall = 2.0, kSoundSpeed = 343.0;
    static constexpr double kSeatMost = 5.5;
    static constexpr int kSources = 4;
    static constexpr int kMostOrder = 3, kMostReflections = 64;
    static constexpr double kHeadSamples = 30.0;
    static constexpr int kEarlySize = 16384;
    static constexpr int kAhead = 16;
    static constexpr double kEarlyCorner = 12000.0, kLateCorner = 4600.0, kLateArchCorner = 9200.0;
    static constexpr double kBassCorner = 250.0, kTrebleFit = 4000.0, kTrebleShare = 0.17;
    static constexpr double kAirRt4k = 40.0, kAirRtTop = 1.3;
    static constexpr double kSideBassCorner = 150.0;
    static constexpr float kSideBassMix = 0.45f;
    static constexpr double kRefDist = 19.242, kNearSoft = 3.5;
    static constexpr float kEarlyGain = 0.140f, kLateGain = 0.118f;
    static constexpr float kLateDirectFeed = 0.94f, kLateEarlyFeed = 0.08f;
    static constexpr int kFadeSamples = 2400;

    static double LineScale(double size) { return size <= 1.0 ? size : std::pow(size, kLineSizeExponent); }

    void SetSampleRate(double hz) {
        const double safeHz = Clamp(hz, 22050.0, 192000.0);
        if (Abs(safeHz - sampleRate) < 0.1) return;
        sampleRate = safeHz;
        rateRatio = sampleRate / kPipeRate;
    }

    void SetArchitecture(int arch) {
        const int clamped = Clamp(arch, 0, kArchitectures - 1);
        architecture = clamped;
        useArchScale = true;
    }

    void SetCharacter(double newModDepth, double newEarlyScale, double newLateScale, double newElevation, bool newFreeze) {
        modDepth = Clamp(newModDepth, 0.0, 1.0);
        earlyScaleAsked = static_cast<float>(Clamp(newEarlyScale, 0.0, 2.5));
        lateScaleAsked = static_cast<float>(Clamp(newLateScale, 0.0, 2.5));
        elevationAsked = static_cast<float>(Clamp(newElevation, 0.0, 1.0) * 0.32);
        freezeAsked = newFreeze;
    }

    void Set(double rt60, double bass, double treble, double size, double width, double seat = 0.0) {
        const BuildingSpec& spec = kArchitecturesSpec[static_cast<std::size_t>(architecture)];
        const double sr = sampleRate;
        const double k = std::tan(kPi * kBassCorner / sr), fit = std::tan(kPi * kTrebleFit / sr);
        const double lineScale = LineScale(size);

        const double surfFactor = Clamp(treble * 1.15, 0.05, 0.96);
        const double tranFactor = std::sqrt(spec.tranDecayRatio);
        const double rtNave = Max(0.2, rt60 / tranFactor);
        const double rtTran = Max(0.2, rt60 * tranFactor);
        const double modOver = Max(0.0, (modDepth - 0.25) / 0.75);
        const double modScale = modDepth <= 0.25 ? (modDepth * 4.0) : (1.0 + 2.0 * modOver + 3.0 * modOver * modOver);
        const double speedScale = 1.0 + 0.5 * modOver;

        for (std::size_t i = 0; i < kLines; ++i) {
            const double lineRt = i < kBoxLines ? rtNave : rtTran;
            const double surfTopRt = lineRt * surfFactor;
            const double surf4kRt = lineRt / (1.0 + kTrebleShare * (1.0 / surfFactor - 1.0));
            const double eff4kRt = 1.0 / (1.0 / surf4kRt + 1.0 / kAirRt4k);
            const double effTopRt = 1.0 / (1.0 / surfTopRt + 1.0 / kAirRtTop);
            const double safe4kRt = Min(eff4kRt, lineRt * 0.98);
            const double safeTopRt = Min(effTopRt, safe4kRt * 0.75);

            const double lenSec = lineScale * kLineSamples[i] / kPipeRate;
            const double cost = -3.0 * lenSec / lineRt, mid = std::pow(10.0, cost);
            const double low = std::sqrt(std::pow(10.0, cost * (1.0 / bass - 1.0)));
            const double a0 = 1.0 + k / low;
            const double top = std::pow(10.0, -3.0 * lenSec * (1.0 / safeTopRt - 1.0 / lineRt));
            const double at = std::pow(10.0, -3.0 * lenSec * (1.0 / safe4kRt - 1.0 / lineRt));
            const double num = Max(at * at - top * top, 1.0e-12);
            const double den = Max(1.0 - at * at, 1.0e-12);
            const double corner = fit * std::sqrt(num / den);
            const double designed[kLossTerms] = { mid * (1.0 + k * low) / a0, mid * (k * low - 1.0) / a0, (k / low - 1.0) / a0,
                                                  (top + corner) / (1.0 + corner), (corner - top) / (1.0 + corner),
                                                  (corner - 1.0) / (1.0 + corner) };
            for (std::size_t term = 0; term < kLossTerms; ++term) wanted[term][i] = static_cast<float>(designed[term]);

            const double share = static_cast<double>((i * 7u) % kLines) / (kLines - 1);
            const double step = kTwoPi * (kSlowestDrift + share * (kFastestDrift - kSlowestDrift)) * speedScale * kTurn / sr;
            turnCos[i] = std::cos(step);
            turnSin[i] = std::sin(step);
        }

        const double lateHz = useArchScale ? kLateArchCorner : kLateCorner;
        earlyPole = static_cast<float>(std::exp(-kTwoPi * kEarlyCorner / sr));
        latePole = static_cast<float>(std::exp(-kTwoPi * lateHz / sr));
        sideBassPole = static_cast<float>(std::exp(-kTwoPi * kSideBassCorner / sr));
        sideScatPole = static_cast<float>(std::exp(-kTwoPi * kEarlySideCorner / sr));
        overShelfPole = static_cast<float>(std::exp(-kTwoPi * kOverShelfCorner / sr));
        widthGlide = static_cast<float>(1.0 / (0.01 * sr));
        fadeStep = static_cast<float>(1.0 / (0.05 * sr));
        freezeStep = static_cast<float>(1.0 / (0.04 * sr));
        quietLimit = static_cast<int>(kLongCap);

        const double w = Clamp(width, 0.0, 1.0);
        const double rho = std::pow(1.0 - w, 1.5);
        const double sizeEnergyNorm = std::sqrt(lineScale / size);
        const double m0 = std::sqrt(0.5 * (1.0 + rho));
        const double s0 = std::sqrt(0.5 * (1.0 - rho));
        const double lostSideShare = 1.0 - (1.0 - kSideBassMix) * (1.0 - kSideBassMix);
        midBassBoostAsked = static_cast<float>(0.5 * (std::sqrt(1.0 + lostSideShare * (s0 * s0) / (m0 * m0)) - 1.0));
        lateMidAsked = static_cast<float>(sizeEnergyNorm * m0);
        lateSideAsked = static_cast<float>(sizeEnergyNorm * s0);
        lateInWidthAsked = static_cast<float>(w);

        // Orthogonal zero-comb early width: th = 0.82 * w keeps M and S flat with constant stereo power.
        const double th = 0.82 * w;
        earlyMidShelfAsked = static_cast<float>(1.0 - std::cos(th));
        earlySideNatAsked = static_cast<float>(w);
        earlySideScatAsked = static_cast<float>(std::sin(th));
        earlyScatAsked = static_cast<float>(spec.scatterGain);
        coupCosAsked = static_cast<float>(std::cos(spec.coupling));
        coupSinAsked = static_cast<float>(std::sin(spec.coupling));
        driftPeak = static_cast<float>(modScale * kDriftSamples * rateRatio);
        headSpan = kHeadSamples * rateRatio;

        earlySidePreDelayLag = Max(1, static_cast<int>(kEarlySidePreDelay * rateRatio + 0.5));
        for (std::size_t kStage = 0; kStage < kEarlyCommonSamples.size(); ++kStage)
            earlyCommonLag[kStage] = Max(1, static_cast<int>(kEarlyCommonSamples[kStage] * rateRatio + 0.5));
        for (std::size_t kStage = 0; kStage < kEarlySideSamples.size(); ++kStage)
            earlySideLag[kStage] = Max(1, static_cast<int>(kEarlySideSamples[kStage] * rateRatio + 0.5));
        for (std::size_t kStage = 0; kStage < 4; ++kStage) {
            diffuserLagL[kStage] = Max(1, static_cast<int>(kDiffuserSamplesL[kStage] * rateRatio + 0.5));
            diffuserLagR[kStage] = Max(1, static_cast<int>(kDiffuserSamplesR[kStage] * rateRatio + 0.5));
        }

        const double maxSeat = useArchScale ? Max(2.5, 0.5 * spec.naveWide - 1.5) : kSeatMost;
        asked = { size, w, Clamp(seat, -maxSeat, maxSeat), architecture, useArchScale, rateRatio, placesRev };

        if (resting) {
            loss = wanted;
            Build(shapes[1], asked);
            fade = 1.0f;
            lateMid = lateMidAsked;
            lateSide = lateSideAsked;
            lateInWidth = lateInWidthAsked;
            midBassBoost = midBassBoostAsked;
            earlyMidShelf = earlyMidShelfAsked;
            earlySideNat = earlySideNatAsked;
            earlySideScat = earlySideScatAsked;
            earlyScat = earlyScatAsked;
            earlyScale = earlyScaleAsked;
            lateScale = lateScaleAsked;
            elevation = elevationAsked;
            coupCos = coupCosAsked;
            coupSin = coupSinAsked;
            freezeMix = freezeAsked ? 1.0f : 0.0f;
        }
    }

    void Stand(int source, double forward, double higher) {
        const BuildingSpec& spec = kArchitecturesSpec[static_cast<std::size_t>(architecture)];
        const Place next { Max(forward, 0.5 - spec.fromWall), higher };
        Place& cur = places[static_cast<std::size_t>(source)];
        if (next.forward != cur.forward || next.higher != cur.higher) {
            cur = next;
            ++placesRev;
            asked.placesRev = placesRev;
        }
    }

    void Clear() {
        for (std::size_t i = 0; i < kLines; ++i) {
            airIn[i] = airOut[i] = shelfOut[i] = freezeApState[i] = freezeApA[i] = 0.0f;
            freezeApC[i] = 1.0f;
            for (std::size_t term = 0; term < kLossTerms; ++term) loss[term][i] = wanted[term][i];
            driftCos[i] = std::cos(2.399963229728653 * static_cast<double>(i));
            driftSin[i] = std::sin(2.399963229728653 * static_cast<double>(i));
            offset[i] = static_cast<float>(driftPeak * driftSin[i]);
            offsetStep[i] = 0.0f;
        }
        for (auto& ch : diffuserBuffer)
            for (auto& b : ch) b.fill(0.0f);
        for (auto& b : earlyCommonBuf)
            for (auto& sub : b) sub.fill(0.0f);
        for (auto& b : earlySideBuf) b.fill(0.0f);
        earlySidePreBuf.fill(0.0f);
        for (auto& b : earlyBuffer) b.fill(0.0f);
        for (auto& b : earlyDiffBuffer) b.fill(0.0f);
        earlyQuiet.fill(kEarlySize);
        earlyDiffQuiet.fill(kEarlySize);
        reflected = {};
        overhead = {};
        write = 0;
        shortFilled = longFilled = false;
        untilTurn = 0;
        stEarlyL = stEarlyR = stOverL = stOverR = stMidLow = stLateL = stLateR = stMidBass = stSideBass = 0.0f;
        for (auto& ear : directBuffer) ear.fill(0.0f);
        Build(shapes[1], asked);
        fade = 1.0f;
        lateMid = lateMidAsked;
        lateSide = lateSideAsked;
        lateInWidth = lateInWidthAsked;
        midBassBoost = midBassBoostAsked;
        earlyMidShelf = earlyMidShelfAsked;
        earlySideNat = earlySideNatAsked;
        earlySideScat = earlySideScatAsked;
        earlyScat = earlyScatAsked;
        earlyScale = earlyScaleAsked;
        lateScale = lateScaleAsked;
        elevation = elevationAsked;
        coupCos = coupCosAsked;
        coupSin = coupSinAsked;
        freezeMix = freezeAsked ? 1.0f : 0.0f;
        resting = true;
        quietFor = 0;
    }

    void Process(float left, float right, float& roomLeft, float& roomRight,
                 const float* others = nullptr, const float* othersDiff = nullptr) {
        if (resting) {
            if (left == 0.0f && right == 0.0f) {
                roomLeft = roomRight = 0.0f;
                return;
            }
            resting = false;
        }
        if (untilTurn == 0) {
            Turn();
            untilTurn = kTurn;
        }
        --untilTurn;

        const std::size_t ahead = static_cast<std::size_t>(write & (kAhead - 1));
        if (ahead == 0) {
            if (fade >= 1.0f && (asked.size != shapes[1].size || asked.width != shapes[1].width || asked.seat != shapes[1].seat
                                 || asked.arch != shapes[1].arch || asked.useArch != shapes[1].useArch
                                 || asked.rateRatio != shapes[1].rateRatio || asked.placesRev != shapes[1].placesRev)) {
                shapes[0] = shapes[1];
                Build(shapes[1], asked);
                fade = 0.0f;
            }
            Reflect(shapes[1], reflected[0], overhead[0]);
            if (fade < 1.0f) Reflect(shapes[0], reflected[1], overhead[1]);
        }
        const bool fading = fade < 1.0f;
        const float freezeTarget = freezeAsked ? 1.0f : 0.0f;
        freezeMix += freezeStep * (freezeTarget - freezeMix);
        if (Abs(freezeTarget - freezeMix) < freezeStep) freezeMix = freezeTarget;
        const float freezeKeep = freezeMix;
        const float dryFeedScale = 1.0f - freezeKeep;

#if defined(OREGANA_FOUR)
        Float4 out[kLines / 4];
        for (std::size_t i = 0; i < kLines; i += 4) {
            const Float4 drifted = Lanes(offset, i) + Lanes(offsetStep, i);
            Keep(offset, i, drifted);
            Float4 heard = Lines(i, drifted, shapes[1].length, freezeKeep);
            if (fading) {
                const Float4 before = Lines(i, drifted, shapes[0].length, freezeKeep);
                heard = before + fade * (heard - before);
            }
            const Float4 airBefore = Lanes(airOut, i);
            const Float4 air = Lanes(loss[kT0], i) * heard + Lanes(loss[kT1], i) * Lanes(airIn, i) - Lanes(loss[kTa], i) * airBefore;
            const Float4 shelved = Lanes(loss[kB0], i) * air + Lanes(loss[kB1], i) * airBefore - Lanes(loss[kA1], i) * Lanes(shelfOut, i);
            Keep(airIn, i, heard);
            Keep(airOut, i, air);
            Keep(shelfOut, i, shelved);
            if (freezeKeep > 0.0f) {
                const Float4 fa = Lanes(freezeApA, i), fc = Lanes(freezeApC, i), fz = Lanes(freezeApState, i);
                const Float4 fy = fa * heard + fc * fz;
                const Float4 fzNext = fc * heard - fa * fz;
                Keep(freezeApState, i, Unless(Abs(fzNext) < Four(1.1754944e-38f), fzNext));
                out[i >> 2] = shelved + freezeKeep * (fy - shelved);
            } else {
                Keep(freezeApState, i, heard);
                out[i >> 2] = shelved;
            }
        }
#else
        std::array<float, kLines> out;
        for (std::size_t i = 0; i < kLines; ++i) {
            offset[i] += offsetStep[i];
            float heard = Line(i, shapes[1].length[i], freezeKeep);
            if (fading) {
                const float before = Line(i, shapes[0].length[i], freezeKeep);
                heard = before + fade * (heard - before);
            }
            const float air = loss[kT0][i] * heard + loss[kT1][i] * airIn[i] - loss[kTa][i] * airOut[i];
            const float shelved = loss[kB0][i] * air + loss[kB1][i] * airOut[i] - loss[kA1][i] * shelfOut[i];
            airIn[i] = heard;
            airOut[i] = air;
            shelfOut[i] = shelved;
            if (freezeKeep > 0.0f) {
                const float fa = freezeApA[i], fc = freezeApC[i], fz = freezeApState[i];
                const float fy = fa * heard + fc * fz;
                freezeApState[i] = Settled(fc * heard - fa * fz);
                out[i] = shelved + freezeKeep * (fy - shelved);
            } else {
                freezeApState[i] = heard;
                out[i] = shelved;
            }
        }
#endif

        const float inL = dryFeedScale * left, inR = dryFeedScale * right;
        const float input = 0.5f * (inL + inR), inputDiff = 0.5f * (inL - inR);
        float first = input, firstDiff = inputDiff;
        for (std::size_t source = 1; source < kSources; ++source) {
            const float from = others != nullptr ? dryFeedScale * others[source - 1] : 0.0f;
            const float fromDiff = othersDiff != nullptr ? dryFeedScale * othersDiff[source - 1] : 0.0f;
            first -= from;
            firstDiff -= fromDiff;
            Hear(source, from, fromDiff);
        }
        Hear(0, first, firstDiff);

        float refL = reflected[0][0][ahead], refR = reflected[0][1][ahead];
        float ovrL = overhead[0][0][ahead], ovrR = overhead[0][1][ahead];
        if (fading) {
            refL = reflected[1][0][ahead] + fade * (refL - reflected[1][0][ahead]);
            refR = reflected[1][1][ahead] + fade * (refR - reflected[1][1][ahead]);
            ovrL = overhead[1][0][ahead] + fade * (ovrL - overhead[1][0][ahead]);
            ovrR = overhead[1][1][ahead] + fade * (ovrR - overhead[1][1][ahead]);
            fade += fadeStep;
        }

        // Apply gentle overhead high-frequency presence to vault/crossing canopy reflections ("from the top").
        elevation += widthGlide * (elevationAsked - elevation);
        stOverL = Settled(ovrL + overShelfPole * (stOverL - ovrL));
        stOverR = Settled(ovrR + overShelfPole * (stOverR - ovrR));
        const float combL = refL + ovrL + elevation * (ovrL - stOverL);
        const float combR = refR + ovrR + elevation * (ovrR - stOverR);

        stEarlyL = Settled(combL + earlyPole * (stEarlyL - combL));
        stEarlyR = Settled(combR + earlyPole * (stEarlyR - combR));

        // Common mono-safe 3-stage stone/statuary Schroeder allpass (0 dB flat, zero mono comb).
        earlyScat += widthGlide * (earlyScatAsked - earlyScat);
        float cL = stEarlyL, cR = stEarlyR;
        for (std::size_t st = 0; st < kEarlyCommonSamples.size(); ++st) {
            const int lag = earlyCommonLag[st];
            auto& bL = earlyCommonBuf[0][st];
            const float dL = bL[static_cast<std::size_t>((write - lag) & (kEarlyDiffSize - 1))];
            const float vL = cL - earlyScat * dL;
            bL[static_cast<std::size_t>(write & (kEarlyDiffSize - 1))] = Settled(vL);
            cL = dL + earlyScat * vL;

            auto& bR = earlyCommonBuf[1][st];
            const float dR = bR[static_cast<std::size_t>((write - lag) & (kEarlyDiffSize - 1))];
            const float vR = cR - earlyScat * dR;
            bR[static_cast<std::size_t>(write & (kEarlyDiffSize - 1))] = Settled(vR);
            cR = dR + earlyScat * vR;
        }

        // Orthogonal M/S Side-only >160 Hz scattering: zero crossover notch and zero mono-sum comb.
        // Pre-delay + alternating-sign allpasses ensure zero-lag <mOut, scatSide> == 0 (L/R mirror symmetric).
        const float eMid = 0.5f * (cL + cR), eSide = 0.5f * (cL - cR);
        stMidLow = Settled(eMid + sideScatPole * (stMidLow - eMid));
        const float eMidHi = eMid - stMidLow;
        earlySidePreBuf[static_cast<std::size_t>(write & (kEarlyDiffSize - 1))] = Settled(eMidHi);
        float scatSide = earlySidePreBuf[static_cast<std::size_t>((write - earlySidePreDelayLag) & (kEarlyDiffSize - 1))];
        for (std::size_t st = 0; st < kEarlySideSamples.size(); ++st) {
            auto& bS = earlySideBuf[st];
            const float gS = kEarlySideGains[st];
            const float dS = bS[static_cast<std::size_t>((write - earlySideLag[st]) & (kEarlyDiffSize - 1))];
            const float vS = scatSide - gS * dS;
            bS[static_cast<std::size_t>(write & (kEarlyDiffSize - 1))] = Settled(vS);
            scatSide = dS + gS * vS;
        }

        earlyMidShelf += widthGlide * (earlyMidShelfAsked - earlyMidShelf);
        earlySideNat += widthGlide * (earlySideNatAsked - earlySideNat);
        earlySideScat += widthGlide * (earlySideScatAsked - earlySideScat);
        earlyScale += widthGlide * (earlyScaleAsked - earlyScale);
        lateScale += widthGlide * (lateScaleAsked - lateScale);
        lateInWidth += widthGlide * (lateInWidthAsked - lateInWidth);
        coupCos += widthGlide * (coupCosAsked - coupCos);
        coupSin += widthGlide * (coupSinAsked - coupSin);

        const float mOut = eMid - earlyMidShelf * eMidHi;
        const float sOut = earlySideNat * eSide + earlySideScat * scatSide;
        const float earEarlyL = mOut + sOut, earEarlyR = mOut - sOut;

        // Width-aware stereo feed so pure-Side input collapses cleanly at Width = 0.
        const float rawFeedL = kLateDirectFeed * inL + kLateEarlyFeed * earEarlyL;
        const float rawFeedR = kLateDirectFeed * inR + kLateEarlyFeed * earEarlyR;
        const float rawFeedM = 0.5f * (rawFeedL + rawFeedR), rawFeedS = 0.5f * (rawFeedL - rawFeedR);
        const float feedL = rawFeedM + lateInWidth * rawFeedS;
        const float feedR = rawFeedM - lateInWidth * rawFeedS;

        stLateL = Settled(feedL + latePole * (stLateL - feedL));
        stLateR = Settled(feedR + latePole * (stLateR - feedR));
        float spreadL = stLateL, spreadR = stLateR;
        for (std::size_t st = 0; st < 4; ++st) {
            auto& bL = diffuserBuffer[0][st];
            const float dL = bL[static_cast<std::size_t>((write - diffuserLagL[st]) & (kDiffuserSize - 1))];
            const float vL = spreadL + kDiffuserGain * dL;
            bL[static_cast<std::size_t>(write & (kDiffuserSize - 1))] = Settled(vL);
            spreadL = dL - kDiffuserGain * vL;

            auto& bR = diffuserBuffer[1][st];
            const float dR = bR[static_cast<std::size_t>((write - diffuserLagR[st]) & (kDiffuserSize - 1))];
            const float vR = spreadR + kDiffuserGain * dR;
            bR[static_cast<std::size_t>(write & (kDiffuserSize - 1))] = Settled(vR);
            spreadR = dR - kDiffuserGain * vR;
        }

        // Read balanced even/odd taps across coupled Nave (0..15) and Transept/Crossing (16..31) boxes.
        float even = 0.0f, odd = 0.0f;
#if defined(OREGANA_FOUR)
        for (std::size_t g = 0; g < kLines / 4; ++g) {
            const std::size_t base = g * 4;
            even += ((kOutputSigns >> (base + 0)) & 1u) ? -out[g][0] : out[g][0];
            odd  += ((kOutputSigns >> (base + 1)) & 1u) ? -out[g][1] : out[g][1];
            even += ((kOutputSigns >> (base + 2)) & 1u) ? -out[g][2] : out[g][2];
            odd  += ((kOutputSigns >> (base + 3)) & 1u) ? -out[g][3] : out[g][3];
        }
        MixBox(out, 0);
        MixBox(out, 4);

        // Energy-preserving Givens crossing aperture coupling between Nave and Transept boxes.
        Float4 fb[kLines / 4];
        for (std::size_t i = 0; i < kLines; i += 4) {
            const std::size_t g = i >> 2;
            const UInt4 fbTurned = UInt4 { (kFeedbackSigns >> i) & 1u, (kFeedbackSigns >> (i + 1)) & 1u,
                                           (kFeedbackSigns >> (i + 2)) & 1u, (kFeedbackSigns >> (i + 3)) & 1u } << 31;
            fb[g] = (Float4)((UInt4)(kMixGain16 * out[g]) ^ fbTurned);
        }
        for (std::size_t g = 0; g < 4; ++g) {
            const Float4 u = fb[g], v = fb[g + 4];
            fb[g]     = coupCos * u - coupSin * v;
            fb[g + 4] = coupSin * u + coupCos * v;
        }

        bool quiet = inL == 0.0f && inR == 0.0f && freezeKeep == 0.0f;
        Int4 sounding = { 0, 0, 0, 0 };
        const Float4 spNave = 1.08f * Float4 { spreadL, spreadR, spreadL, spreadR };
        const Float4 spTran = 0.90f * Float4 { spreadL, spreadR, spreadL, spreadR };
        for (std::size_t i = 0; i < kLines; i += 4) {
            const std::size_t g = i >> 2;
            const UInt4 inTurned = UInt4 { (kInputSigns >> i) & 1u, (kInputSigns >> (i + 1)) & 1u,
                                           (kInputSigns >> (i + 2)) & 1u, (kInputSigns >> (i + 3)) & 1u } << 31;
            const Float4& sp = i < kBoxLines ? spNave : spTran;
            const Float4 mixed = fb[g] + (Float4)((UInt4)sp ^ inTurned);
            const Float4 in = Unless(Abs(mixed) < Four(1.1754944e-38f), mixed);
            const int cap = LineCap(i);
            const auto writeAt = static_cast<std::size_t>(write & (cap - 1));
            for (std::size_t lane = 0; lane < 4; ++lane) {
                float* const buf = LinePtr(i + lane);
                buf[writeAt] = in[lane];
                if (writeAt == 0) buf[static_cast<std::size_t>(cap)] = in[lane];
            }
            sounding |= in != Four(0.0f);
        }
        quiet = quiet && (sounding[0] | sounding[1] | sounding[2] | sounding[3]) == 0;
#else
        for (std::size_t i = 0; i < kLines; i += 2) {
            even += ((kOutputSigns >> i) & 1u) ? -out[i] : out[i];
            odd  += ((kOutputSigns >> (i + 1)) & 1u) ? -out[i + 1] : out[i + 1];
        }
        MixBox(out, 0);
        MixBox(out, kBoxLines);

        std::array<float, kLines> fb;
        for (std::size_t i = 0; i < kLines; ++i)
            fb[i] = (((kFeedbackSigns >> i) & 1u) ? -kMixGain16 : kMixGain16) * out[i];
        for (std::size_t i = 0; i < kBoxLines; ++i) {
            const float u = fb[i], v = fb[i + kBoxLines];
            fb[i]             = coupCos * u - coupSin * v;
            fb[i + kBoxLines] = coupSin * u + coupCos * v;
        }

        bool quiet = inL == 0.0f && inR == 0.0f && freezeKeep == 0.0f;
        for (std::size_t i = 0; i < kLines; ++i) {
            const float baseSp = (i & 1u) ? spreadR : spreadL;
            const float sp = (i < kBoxLines ? 1.08f : 0.90f) * baseSp;
            const float in = Settled(fb[i] + (((kInputSigns >> i) & 1u) ? -sp : sp));
            const int cap = LineCap(i);
            const auto writeAt = static_cast<std::size_t>(write & (cap - 1));
            float* const buf = LinePtr(i);
            buf[writeAt] = in;
            if (writeAt == 0) buf[static_cast<std::size_t>(cap)] = in;
            quiet = quiet && in == 0.0f;
        }
#endif

        ++write;
        if (write >= kShortCap) shortFilled = true;
        if (write >= kLongCap) {
            write = 0;
            longFilled = true;
        }

        lateMid += widthGlide * (lateMidAsked - lateMid);
        lateSide += widthGlide * (lateSideAsked - lateSide);
        midBassBoost += widthGlide * (midBassBoostAsked - midBassBoost);
        const float lateGainScaled = kLateGain * lateScale;
        const float rawMid = lateGainScaled * lateMid * (even + odd);
        const float rawApart = lateGainScaled * lateSide * (even - odd);
        stMidBass = Settled(rawMid + sideBassPole * (stMidBass - rawMid));
        stSideBass = Settled(rawApart + sideBassPole * (stSideBass - rawApart));
        const float mid = rawMid + midBassBoost * stMidBass;
        const float apart = rawApart - kSideBassMix * stSideBass;
        const float earlyGainScaled = kEarlyGain * earlyScale;
        roomLeft = earlyGainScaled * earEarlyL + mid + apart;
        roomRight = earlyGainScaled * earEarlyR + mid - apart;

        quiet = quiet && Abs(earEarlyL) < 1.1754944e-38f && Abs(earEarlyR) < 1.1754944e-38f && Abs(stOverL) < 1.1754944e-38f
                      && Abs(stOverR) < 1.1754944e-38f && Abs(stMidLow) < 1.1754944e-38f && Abs(scatSide) < 1.1754944e-38f;
        quietFor = quiet ? quietFor + 1 : 0;
        if (quietFor >= quietLimit) {
            resting = true;
            quietFor = 0;
            airIn.fill(0.0f);
            airOut.fill(0.0f);
            shelfOut.fill(0.0f);
            freezeApState.fill(0.0f);
            freezeApA.fill(0.0f);
            freezeApC.fill(1.0f);
            for (auto& ch : diffuserBuffer)
                for (auto& b : ch) b.fill(0.0f);
            for (auto& b : earlyCommonBuf)
                for (auto& sub : b) sub.fill(0.0f);
            for (auto& b : earlySideBuf) b.fill(0.0f);
        earlySidePreBuf.fill(0.0f);
            stEarlyL = stEarlyR = stOverL = stOverR = stMidLow = stLateL = stLateR = stMidBass = stSideBass = 0.0f;
        }
    }

    void Direct(float& left, float& right) {
        const auto at = static_cast<std::size_t>(directAt);
        directBuffer[0][at] = left;
        directBuffer[1][at] = right;
        directAt = (directAt + 1) & (kDirectSize - 1);
        const bool fading = fade < 1.0f;
        if (!fading && shapes[1].seat == 0.0) return;
        float* const ears[2] = { &left, &right };
        for (std::size_t ear = 0; ear < 2; ++ear) {
            const auto heard = [&](const Shape& shape) {
                return shape.directGain[ear] * directBuffer[ear][(at - static_cast<std::size_t>(shape.directLate[ear])) & (kDirectSize - 1)];
            };
            float now = heard(shapes[1]);
            if (fading) {
                const float before = heard(shapes[0]);
                now = before + fade * (now - before);
            }
            *ears[ear] = now;
        }
    }

private:
    static constexpr int kDirectSize = 128;
    static constexpr float kMixGain16 = 0.25f;   // 1 / sqrt(16): orthogonal per-box Hadamard
    // Balanced double-bent sign masks: Nave (0x1BE4 / 0x03CF) and Transept (0xA3CA / 0x1BD8).
    static constexpr std::uint32_t kInputSigns    = 0xA3C596E1u;
    static constexpr std::uint32_t kFeedbackSigns = 0xA3CA1BE4u;
    static constexpr std::uint32_t kOutputSigns   = 0x1BD803CFu;

    struct Reflection { int left, right; float leftGain, rightGain; std::uint8_t overhead; };
    struct Shape {
        double size = 0.0, width = 0.0, seat = 0.0, rateRatio = 1.0;
        int arch = kArchChurch;
        bool useArch = false;
        std::uint32_t placesRev = 0;
        std::array<int, 2> directLate {};
        std::array<float, 2> directGain { { 1.0f, 1.0f } };
        std::array<std::array<Reflection, kMostReflections>, kSources> reflections {};
        int count = 0;
        std::array<float, kLines> length {};
    };
    struct Asked {
        double size = 1.0, width = 1.0, seat = 0.0;
        int arch = kArchChurch;
        bool useArch = false;
        double rateRatio = 1.0;
        std::uint32_t placesRev = 0;
    };
    struct Place { double forward = 0.0, higher = 0.0; };
    enum : std::size_t { kB0, kB1, kA1, kT0, kT1, kTa, kLossTerms };

    static float Settled(float x) { return Abs(x) < 1.1754944e-38f ? 0.0f : x; }
    static constexpr int LineCap(std::size_t i) { return (i & 8u) ? kLongCap : kShortCap; }
    float* LinePtr(std::size_t i) {
        if (i < kBoxLines) return (i & 8u) ? naveLong[i & 7u].data() : naveShort[i & 7u].data();
        return (i & 8u) ? tranLong[i & 7u].data() : tranShort[i & 7u].data();
    }
    const float* LinePtr(std::size_t i) const {
        if (i < kBoxLines) return (i & 8u) ? naveLong[i & 7u].data() : naveShort[i & 7u].data();
        return (i & 8u) ? tranLong[i & 7u].data() : tranShort[i & 7u].data();
    }

    void Build(Shape& shape, const Asked& to) const {
        const BuildingSpec& spec = kArchitecturesSpec[static_cast<std::size_t>(to.arch)];
        shape.size = to.size;
        shape.width = to.width;
        shape.seat = to.seat;
        shape.arch = to.arch;
        shape.useArch = to.useArch;
        shape.rateRatio = to.rateRatio;
        shape.placesRev = to.placesRev;

        const double baseAhead = to.useArch ? (0.78 * spec.naveLong - spec.fromWall) : kToListener;
        const double ahead = Max(1.0, baseAhead - places[0].forward);
        const double top = Max(0.3, (spec.loftHeight - 1.2) + places[0].higher);
        const double hSpan = kHeadSamples * to.rateRatio;
        const double far = to.width * Abs(to.seat) / std::sqrt(to.seat * to.seat + ahead * ahead + top * top);
        const std::size_t farEar = to.seat > 0.0 ? 1 : 0;
        shape.directLate = {};
        shape.directGain = { { 1.0f, 1.0f } };
        shape.directLate[farEar] = static_cast<int>(hSpan * far + 0.5);
        shape.directGain[farEar] = static_cast<float>(1.0 - far);

        for (std::size_t source = 0; source < kSources; ++source)
            BuildSource(shape.reflections[source], shape.count, places[source], to, spec);
        const double lineScale = LineScale(to.size) * to.rateRatio;
        for (std::size_t i = 0; i < kLines; ++i) shape.length[i] = static_cast<float>(lineScale * kLineSamples[i]);
    }

    static void BuildSource(std::array<Reflection, kMostReflections>& reflections, int& count, const Place& place,
                            const Asked& asked, const BuildingSpec& spec) {
        count = 0;
        const double sr = kPipeRate * asked.rateRatio;
        const double hSpan = kHeadSamples * asked.rateRatio;
        const double listenerY = asked.useArch ? (0.78 * spec.naveLong) : (kFromWall + kToListener);
        const double srcY = Max(0.5, spec.fromWall + place.forward);
        const double srcZ = Max(1.3, spec.loftHeight + place.higher);
        const double earZ = 1.2;
        const double ahead = Max(1.0, listenerY - srcY);
        const double top = srcZ - earZ;
        const double direct = std::sqrt(asked.seat * asked.seat + ahead * ahead + top * top);

        const struct Vert { double rise; int order; double factor; double lift; std::uint8_t overhead; } verts[5] = {
            { srcZ - earZ,                    0, 1.0,                               1.0, 0 },
            { srcZ + earZ,                    1, spec.floorFactor,                  1.0, 0 },
            { 2.0 * spec.naveHigh - srcZ - earZ, 1, spec.vaultFactor,              -1.0, 1 },
            { 2.0 * spec.naveHigh + srcZ - earZ, 2, spec.floorFactor * spec.vaultFactor,  1.0, 1 },
            { 2.0 * spec.naveHigh - srcZ + earZ, 2, spec.floorFactor * spec.vaultFactor, -1.0, 0 },
        };

        auto push = [&](double across, double along, double rise, double atten, std::uint8_t isOver) {
            if (count >= kMostReflections) return;
            const double path = std::sqrt(across * across + along * along + rise * rise);
            if (path <= direct + 0.05) return;
            const double earlySizeScale = asked.useArch ? 1.0 : asked.size;
            const double samples = (path - direct) * earlySizeScale / kSoundSpeed * sr;
            const double aside = asked.width * across / path, far = aside < 0.0 ? -aside : aside;
            const int near = Max(kAhead, static_cast<int>(samples + 0.5));
            const int late = Max(kAhead, static_cast<int>(samples + hSpan * far + 0.5));
            if (late >= kEarlySize - kAhead) return;
            double lateTaper = 1.0;
            if (asked.useArch) {
                const double tMs = samples * 1000.0 / sr;
                if (tMs > 160.0) {
                    const double u = Clamp((tMs - 160.0) / 140.0, 0.0, 1.0);
                    lateTaper = 0.25 + 0.75 * 0.5 * (1.0 + std::cos(kPi * u));
                }
            }
            const double level = (spec.refDist + kNearSoft) / (path + kNearSoft) * atten * lateTaper;
            const auto strong = static_cast<float>(level), weak = static_cast<float>(level * (1.0 - far));
            reflections[static_cast<std::size_t>(count++)] =
                aside < 0.0 ? Reflection { near, late, strong, weak, isOver } : Reflection { late, near, weak, strong, isOver };
        };

        // Box 1: Main Nave (side walls, west wall behind gallery, pew floor, nave vault).
        for (int side = -2; side <= 2; ++side)
            for (int behind = 0; behind <= 1; ++behind)
                for (const Vert& v : verts) {
                    const int walls = (side < 0 ? -side : side) + behind, order = walls + v.order;
                    if (order == 0 || order > kMostOrder) continue;
                    const double across = spec.naveWide * side - asked.seat;
                    const double along = ahead + 2.0 * srcY * behind;
                    push(across, along, v.rise, std::pow(spec.wallFactor, walls) * v.factor, v.overhead);
                }

        // Box 2A: Transept end walls (72% crossing aperture factor).
        for (int side : { -1, 1 }) {
            const double across = spec.tranWide * side - asked.seat;
            const double aperture = 0.72;
            push(across, ahead, srcZ - earZ, aperture * spec.wallFactor, 0);
            push(across, ahead, srcZ + earZ, aperture * spec.wallFactor * spec.floorFactor, 0);
            push(across, ahead, 2.0 * spec.crossHigh - srcZ - earZ, aperture * spec.wallFactor * spec.vaultFactor, 1);
        }

        // Box 2B: Taller Crossing Lantern Vault overhead return ("from the top" canopy).
        push(-asked.seat, ahead, 2.0 * spec.crossHigh - srcZ - earZ, 1.15 * spec.vaultFactor, 1);
        push(-asked.seat, ahead + 2.0 * srcY, 2.0 * spec.crossHigh - srcZ - earZ,
             spec.wallFactor * spec.vaultFactor, 1);

        // Box 2C: Four crossing stone piers (adds natural column/statue scatter density in 15-60 ms).
        const double crossY = spec.crossAt * spec.naveLong;
        const double pierX = 0.5 * spec.naveWide;
        for (int sx : { -1, 1 }) {
            for (int sy : { -1, 1 }) {
                const double px = sx * pierX, py = crossY + sy * 0.5 * spec.tranLong;
                const double d1 = std::hypot(px, py - srcY);
                const double d2 = std::hypot(asked.seat - px, listenerY - py);
                const double totalHoriz = d1 + d2;
                const double bearingAcross = (px - asked.seat) * (totalHoriz / Max(d2, 0.5));
                const double pierAtten = 0.35 * spec.wallFactor;
                push(bearingAcross, std::sqrt(Max(1.0, totalHoriz * totalHoriz - bearingAcross * bearingAcross)),
                     srcZ - earZ, pierAtten, 0);
            }
        }
    }

    void Hear(std::size_t source, float sound, float diff) {
        const auto at = static_cast<std::size_t>(write & (kEarlySize - 1));
        earlyBuffer[source][at] = sound;
        earlyDiffBuffer[source][at] = diff;
        if (at < kAhead) {
            earlyBuffer[source][at + kEarlySize] = sound;
            earlyDiffBuffer[source][at + kEarlySize] = diff;
        }
        earlyQuiet[source] = sound != 0.0f ? 0 : Min(earlyQuiet[source] + 1, kEarlySize);
        earlyDiffQuiet[source] = diff != 0.0f ? 0 : Min(earlyDiffQuiet[source] + 1, kEarlySize);
    }

#if defined(OREGANA_FOUR)
    static Float4 Lanes(const std::array<float, kLines>& from, std::size_t i) {
        Float4 lanes;
        __builtin_memcpy(&lanes, from.data() + i, sizeof lanes);
        return lanes;
    }
    static void Keep(std::array<float, kLines>& to, std::size_t i, Float4 lanes) { __builtin_memcpy(to.data() + i, &lanes, sizeof lanes); }

    Float4 Lines(std::size_t i, Float4 drifted, const std::array<float, kLines>& length, float freezeKeep) const {
        const int cap = LineCap(i);
        const int wAt = write & (cap - 1);
        const bool filled = cap == kShortCap ? shortFilled : longFilled;
        const Float4 len = Lanes(length, i);
        Float4 read;
        if (freezeKeep <= 0.0f) {
            read = Four(static_cast<float>(wAt)) - len - drifted;
        } else {
            const Float4 rounded = ToFloat(Truncated(len - Four(0.5f)));
            const Float4 effLen = rounded + Four(1.0f - freezeKeep) * (len - rounded + drifted);
            read = Four(static_cast<float>(wAt)) - effLen;
        }
        read += Where(read < Four(0.0f), Four(static_cast<float>(cap)));
        read += Where(read < Four(0.0f), Four(static_cast<float>(cap)));
        const Int4 at = Truncated(read);
        const Float4 between = read - ToFloat(at);
        const Int4 first = at & (cap - 1);
        std::int32_t from[4];
        __builtin_memcpy(from, &first, sizeof from);
        float stored[8];
        for (std::size_t lane = 0; lane < 4; ++lane) {
            const float* const buffer = LinePtr(i + lane);
            stored[lane] = buffer[from[lane]];
            stored[lane + 4] = buffer[from[lane] + 1];
        }
        if (!filled) {
            for (int lane = 0; lane < 4; ++lane) {
                if (from[lane] >= wAt) stored[lane] = 0.0f;
                if (((from[lane] + 1) & (cap - 1)) >= wAt) stored[lane + 4] = 0.0f;
            }
        }
        Float4 older, newer;
        __builtin_memcpy(&older, stored, sizeof older);
        __builtin_memcpy(&newer, stored + 4, sizeof newer);
        return older + between * (newer - older);
    }
#endif

    float Line(std::size_t i, float length, float freezeKeep) const {
        const int cap = LineCap(i);
        const int wAt = write & (cap - 1);
        const bool filled = cap == kShortCap ? shortFilled : longFilled;
        float read;
        if (freezeKeep <= 0.0f) {
            read = static_cast<float>(wAt) - length - offset[i];
        } else {
            const float rounded = static_cast<float>(static_cast<int>(length - 0.5f));
            const float effLen = rounded + (1.0f - freezeKeep) * (length - rounded + offset[i]);
            read = static_cast<float>(wAt) - effLen;
        }
        while (read < 0.0f) read += static_cast<float>(cap);
        const int at = static_cast<int>(read);
        const float between = read - static_cast<float>(at);
        const float* const buffer = LinePtr(i);
        const auto first = static_cast<std::size_t>(at & (cap - 1));
        const float older = filled || static_cast<int>(first) < wAt ? buffer[first] : 0.0f;
        const float newer = filled || ((at + 1) & (cap - 1)) < wAt ? buffer[first + 1] : 0.0f;
        return older + between * (newer - older);
    }

    void Reflect(const Shape& shape, std::array<std::array<float, kAhead>, 2>& walls,
                 std::array<std::array<float, kAhead>, 2>& over) const {
#if defined(OREGANA_FOUR)
        Float4 l0 = Four(0.0f), l1 = Four(0.0f), l2 = Four(0.0f), l3 = Four(0.0f);
        Float4 r0 = Four(0.0f), r1 = Four(0.0f), r2 = Four(0.0f), r3 = Four(0.0f);
        Float4 ol0 = Four(0.0f), ol1 = Four(0.0f), ol2 = Four(0.0f), ol3 = Four(0.0f);
        Float4 or0 = Four(0.0f), or1 = Four(0.0f), or2 = Four(0.0f), or3 = Four(0.0f);
        for (std::size_t source = 0; source < kSources; ++source) {
            if (earlyQuiet[source] < kEarlySize) {
                const float* const buffer = earlyBuffer[source].data();
                for (int i = 0; i < shape.count; ++i) {
                    const Reflection& r = shape.reflections[source][static_cast<std::size_t>(i)];
                    const float* const fl = buffer + ((write - r.left) & (kEarlySize - 1));
                    const float* const fr = buffer + ((write - r.right) & (kEarlySize - 1));
                    if (r.overhead) {
                        ol0 += r.leftGain * Load4(fl);      ol1 += r.leftGain * Load4(fl + 4);
                        ol2 += r.leftGain * Load4(fl + 8);  ol3 += r.leftGain * Load4(fl + 12);
                        or0 += r.rightGain * Load4(fr);     or1 += r.rightGain * Load4(fr + 4);
                        or2 += r.rightGain * Load4(fr + 8); or3 += r.rightGain * Load4(fr + 12);
                    } else {
                        l0 += r.leftGain * Load4(fl);       l1 += r.leftGain * Load4(fl + 4);
                        l2 += r.leftGain * Load4(fl + 8);   l3 += r.leftGain * Load4(fl + 12);
                        r0 += r.rightGain * Load4(fr);      r1 += r.rightGain * Load4(fr + 4);
                        r2 += r.rightGain * Load4(fr + 8);  r3 += r.rightGain * Load4(fr + 12);
                    }
                }
            }
            if (earlyDiffQuiet[source] < kEarlySize) {
                const float* const dbuf = earlyDiffBuffer[source].data();
                const float dScale = static_cast<float>(0.42 * shape.width);
                for (int i = 0; i < shape.count; ++i) {
                    const Reflection& r = shape.reflections[source][static_cast<std::size_t>(i)];
                    const float* const fl = dbuf + ((write - r.left) & (kEarlySize - 1));
                    const float* const fr = dbuf + ((write - r.right) & (kEarlySize - 1));
                    const float alt = (i & 1) ? -dScale : dScale;
                    const float gl = alt * r.leftGain, gr = -alt * r.rightGain;
                    if (r.overhead) {
                        ol0 += gl * Load4(fl);     ol1 += gl * Load4(fl + 4);
                        ol2 += gl * Load4(fl + 8); ol3 += gl * Load4(fl + 12);
                        or0 += gr * Load4(fr);     or1 += gr * Load4(fr + 4);
                        or2 += gr * Load4(fr + 8); or3 += gr * Load4(fr + 12);
                    } else {
                        l0 += gl * Load4(fl);      l1 += gl * Load4(fl + 4);
                        l2 += gl * Load4(fl + 8);  l3 += gl * Load4(fl + 12);
                        r0 += gr * Load4(fr);      r1 += gr * Load4(fr + 4);
                        r2 += gr * Load4(fr + 8);  r3 += gr * Load4(fr + 12);
                    }
                }
            }
        }
        Store4(walls[0].data(), l0);      Store4(walls[0].data() + 4, l1);
        Store4(walls[0].data() + 8, l2);  Store4(walls[0].data() + 12, l3);
        Store4(walls[1].data(), r0);      Store4(walls[1].data() + 4, r1);
        Store4(walls[1].data() + 8, r2);  Store4(walls[1].data() + 12, r3);
        Store4(over[0].data(), ol0);      Store4(over[0].data() + 4, ol1);
        Store4(over[0].data() + 8, ol2);  Store4(over[0].data() + 12, ol3);
        Store4(over[1].data(), or0);      Store4(over[1].data() + 4, or1);
        Store4(over[1].data() + 8, or2);  Store4(over[1].data() + 12, or3);
#else
        walls[0].fill(0.0f); walls[1].fill(0.0f);
        over[0].fill(0.0f);  over[1].fill(0.0f);
        for (std::size_t source = 0; source < kSources; ++source) {
            if (earlyQuiet[source] < kEarlySize) {
                const float* const buffer = earlyBuffer[source].data();
                for (int i = 0; i < shape.count; ++i) {
                    const Reflection& r = shape.reflections[source][static_cast<std::size_t>(i)];
                    auto& target = r.overhead ? over : walls;
                    Add(target[0].data(), r.leftGain, buffer + ((write - r.left) & (kEarlySize - 1)));
                    Add(target[1].data(), r.rightGain, buffer + ((write - r.right) & (kEarlySize - 1)));
                }
            }
            if (earlyDiffQuiet[source] < kEarlySize) {
                const float* const dbuf = earlyDiffBuffer[source].data();
                const float dScale = static_cast<float>(0.42 * shape.width);
                for (int i = 0; i < shape.count; ++i) {
                    const Reflection& r = shape.reflections[source][static_cast<std::size_t>(i)];
                    const float alt = (i & 1) ? -dScale : dScale;
                    auto& target = r.overhead ? over : walls;
                    Add(target[0].data(), alt * r.leftGain, dbuf + ((write - r.left) & (kEarlySize - 1)));
                    Add(target[1].data(), -alt * r.rightGain, dbuf + ((write - r.right) & (kEarlySize - 1)));
                }
            }
        }
#endif
    }

#if defined(OREGANA_FOUR)
    // Unscaled 16-point fast Walsh-Hadamard transform on a 4-Float4 block (`start .. start + 3`).
    static void MixBox(Float4 (&fours)[kLines / 4], std::size_t start) {
        for (std::size_t n = start; n < start + 4; ++n) {
            const Float4 p = fours[n];
            const float sum0 = p[0] + p[1], diff0 = p[0] - p[1], sum1 = p[2] + p[3], diff1 = p[2] - p[3];
            fours[n] = Float4 { sum0 + sum1, diff0 + diff1, sum0 - sum1, diff0 - diff1 };
        }
        for (std::size_t span = 1; span < 4; span *= 2)
            for (std::size_t first = start; first < start + 4; first += 2 * span)
                for (std::size_t n = first; n < first + span; ++n) {
                    const Float4 a = fours[n], b = fours[n + span];
                    fours[n] = a + b;
                    fours[n + span] = a - b;
                }
    }
#else
    static void Add(float* sum, float gain, const float* from) {
        for (int i = 0; i < kAhead; ++i) sum[i] += gain * from[i];
    }

    static void MixBox(std::array<float, kLines>& v, std::size_t start) {
        for (std::size_t span = 1; span < kBoxLines; span *= 2)
            for (std::size_t first = start; first < start + kBoxLines; first += 2 * span)
                for (std::size_t i = first; i < first + span; ++i) {
                    const float a = v[i], b = v[i + span];
                    v[i] = a + b;
                    v[i + span] = a - b;
                }
    }
#endif

    void Turn() {
        const double apDepth = 0.22 + 0.36 * modDepth;
        for (std::size_t i = 0; i < kLines; ++i) {
            const double c = driftCos[i] * turnCos[i] - driftSin[i] * turnSin[i];
            const double s = driftSin[i] * turnCos[i] + driftCos[i] * turnSin[i];
            const double norm = 1.5 - 0.5 * (c * c + s * s);
            driftCos[i] = c * norm;
            driftSin[i] = s * norm;
            offsetStep[i] = (static_cast<float>(driftPeak * driftSin[i]) - offset[i]) * (1.0f / kTurn);
            const double apA = static_cast<double>(freezeMix) * apDepth * driftSin[i];
            const double apC = std::sqrt(Max(0.0, 1.0 - apA * apA));
            freezeApA[i] = static_cast<float>(apA);
            freezeApC[i] = static_cast<float>(apC);
            for (std::size_t term = 0; term < kLossTerms; ++term) loss[term][i] += 0.25f * (wanted[term][i] - loss[term][i]);
        }
    }

    // Tiered FDN storage: 1.50 MiB total across 32 lines (vs 4.20 MiB flat).
    std::array<std::array<float, kShortCap + 1>, 8> naveShort {}, tranShort {};
    std::array<std::array<float, kLongCap + 1>, 8> naveLong {}, tranLong {};
    std::array<std::array<float, kLines>, kLossTerms> loss {}, wanted {};
    std::array<float, kLines> airIn {}, airOut {}, shelfOut {}, freezeApState {}, freezeApA {}, freezeApC {}, offset {}, offsetStep {};
    std::array<double, kLines> driftCos {}, driftSin {}, turnCos {}, turnSin {};

    std::array<std::array<std::array<float, kDiffuserSize>, 4>, 2> diffuserBuffer {};
    std::array<std::array<std::array<float, kEarlyDiffSize>, 3>, 2> earlyCommonBuf {};
    std::array<std::array<float, kEarlyDiffSize>, 2> earlySideBuf {};
    std::array<float, kEarlyDiffSize> earlySidePreBuf {};
    std::array<std::array<float, kEarlySize + kAhead>, kSources> earlyBuffer {}, earlyDiffBuffer {};
    std::array<std::array<std::array<float, kAhead>, 2>, 2> reflected {}, overhead {};
    std::array<int, kSources> earlyQuiet {}, earlyDiffQuiet {};
    std::array<Place, kSources> places {};
    std::array<std::array<float, kDirectSize>, 2> directBuffer {};
    int directAt = 0;
    std::array<Shape, 2> shapes {};
    Asked asked {};

    int architecture = kArchChurch;
    bool useArchScale = false;
    std::uint32_t placesRev = 0;
    double sampleRate = kPipeRate, rateRatio = 1.0, modDepth = 0.25, headSpan = kHeadSamples;
    float fade = 1.0f, fadeStep = 1.0f / kFadeSamples;
    float freezeMix = 0.0f, freezeStep = 1.0f / 1920.0f;
    bool freezeAsked = false;
    int write = 0, untilTurn = 0, quietFor = 0, quietLimit = kLongCap;
    bool shortFilled = false, longFilled = false, resting = true;

    float earlyPole = 0.0f, latePole = 0.0f, sideBassPole = 0.0f, sideScatPole = 0.0f, overShelfPole = 0.0f;
    float stEarlyL = 0.0f, stEarlyR = 0.0f, stOverL = 0.0f, stOverR = 0.0f;
    float stMidLow = 0.0f, stLateL = 0.0f, stLateR = 0.0f, stMidBass = 0.0f, stSideBass = 0.0f;
    float widthGlide = 1.0f / 480.0f;
    float lateMid = 0.7071f, lateSide = 0.7071f, lateMidAsked = 0.7071f, lateSideAsked = 0.7071f;
    float lateInWidth = 1.0f, lateInWidthAsked = 1.0f;
    float midBassBoost = 0.0f, midBassBoostAsked = 0.0f;
    float earlyMidShelf = 0.0f, earlyMidShelfAsked = 0.0f;
    float earlySideNat = 1.0f, earlySideNatAsked = 1.0f;
    float earlySideScat = 0.0f, earlySideScatAsked = 0.0f;
    float earlyScat = 0.50f, earlyScatAsked = 0.50f;
    float earlyScale = 1.0f, earlyScaleAsked = 1.0f, lateScale = 1.0f, lateScaleAsked = 1.0f;
    float elevation = 0.0f, elevationAsked = 0.0f;
    float coupCos = 1.0f, coupSin = 0.0f, coupCosAsked = 1.0f, coupSinAsked = 0.0f;
    float driftPeak = static_cast<float>(kDriftSamples);
    int earlySidePreDelayLag = kEarlySidePreDelay;
    std::array<int, 3> earlyCommonLag { { 59, 127, 239 } };
    std::array<int, 2> earlySideLag { { 173, 331 } };
    std::array<int, 4> diffuserLagL { { 101, 179, 283, 397 } };
    std::array<int, 4> diffuserLagR { { 109, 193, 293, 419 } };
};

} // namespace oregana
