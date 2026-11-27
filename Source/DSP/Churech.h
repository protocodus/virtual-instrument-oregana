#pragma once

// Churech: Oregana's cruciform church as an effect for any sound.
//
// Shares its acoustic room engine with the Oregana pipe organ (Nave.h):
//   Type       four golden-ratio cruciform churches (Chapel, Church, Basilica,
//              Cathedral), each with authentic nave + transept/crossing volume.
//   Loft       vertical origin from 1.6 m floor up to the west organ loft
//              under the vault ("from the top" canopy presence).
//   Distance   gain-compensated depth positioning along the nave (keeps insert
//              loudness balanced while shifting early/late ratio clearly).
//   Seat       listener lateral seat across the nave, shaping the direct
//              sound (mono-safe ILD and head-shadow) and all early reflections
//              symmetrically.
//   Mod / Early / Freeze / Pre-delay / Cuts / Sidechain Duck / Safety Limiter.
//
// Runs natively at 44.1 kHz and 48 kHz with 0 samples of wet path latency;
// above 48 kHz resamples only the church send/return through RateBridge while
// keeping the dry path completely untouched and zero-latency.
//
// JUCE-free, C++17, -fno-builtin / -fno-exceptions clean, nothing allocated
// after construction. Holds a Nave (~1.8 MiB): keep it on the heap.

#include "DspMath.h"
#include "Nave.h"
#include "RateBridge.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace oregana {

class Churech {
public:
    enum Type { kChapel, kChurch, kBasilica, kCathedral, kTypes };

    struct Building { double size, decay; };
    static constexpr std::array<Building, kTypes> kBuildings { { { 0.5, 1.6 }, { 0.8, 2.8 }, { 1.25, 4.5 }, { 2.0, 7.5 } } };

    struct Controls {
        int type = kChurch;
        double decay = 1.0;           // times the building's, 0.5 .. 2
        double decayBass = 1.3;       // the Nave's, 0.5 .. 2
        double decayTreble = 0.2;     // the Nave's, 0.1 .. 0.6
        double distance = 0.0;        // 0 nearest .. 1 furthest place
        double seat = 0.0;            // -1 left wall .. 1 right wall
        double width = 0.8;           // 0 .. 1
        double preDelayMs = 0.0;      // 0 .. 200 ms
        double lowCutHz = 20.0;       // 20 .. 1000 Hz
        double highCutHz = 20000.0;   // 1000 .. 20000 Hz
        double duck = 0.0;            // 0 .. 1
        double mix = 0.3;             // 0 dry .. 1 wet
        double outputDb = 0.0;        // -24 .. 12 dB
        double loft = 1.0;            // 0 floor (1.6 m) .. 1 organ loft ("from the top")
        double modulation = 0.25;     // 0 .. 1 drift depth
        double earlyLevel = 1.0;      // 0 .. 2 early reflections gain
        bool freeze = false;          // hold current reverberant field infinitely
    };

    static constexpr int kPlaces = Nave::kSources;
    static constexpr double kMostPreDelayMs = 200.0;

    Churech() {
        Set(Controls {});
        nave.Clear();
        Prepare(48000);
    }

    void Prepare(int hostRate) {
        rate = Max(22050, hostRate);
        nativeRate = rate == 44100 || rate == 48000;
        churchRate = nativeRate ? rate : static_cast<int>(kPipeRate);
        nave.SetSampleRate(static_cast<double>(churchRate));
        if (!nativeRate) bridge.Prepare(static_cast<double>(rate));
        else bridge.Reset();

        hostGlide = static_cast<float>(1.0 / (kGlideSeconds * rate));
        churchGlide = static_cast<float>(1.0 / (kGlideSeconds * churchRate));
        delayFadeSpan = Max(1, static_cast<int>(0.05 * churchRate + 0.5));
        duckAttack = static_cast<float>(1.0 - std::exp(-1.0 / (0.004 * rate)));
        duckReleaseFast = static_cast<float>(1.0 - std::exp(-1.0 / (0.09 * rate)));
        duckReleaseSlow = static_cast<float>(1.0 - std::exp(-1.0 / (0.42 * rate)));
        duckSustainCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (0.35 * rate)));
        duckGlide = static_cast<float>(1.0 / (0.01 * rate));
        meterDecay = static_cast<float>(std::exp(-1.0 / (0.28 * rate)));
        dryShadowPole = static_cast<float>(std::exp(-kTwoPi * 5500.0 / rate));
        limAttack = static_cast<float>(1.0 - std::exp(-1.0 / (0.00015 * rate)));
        limRelease = static_cast<float>(1.0 - std::exp(-1.0 / (0.070 * rate)));
        dcPole = static_cast<float>(std::exp(-kTwoPi * 5.0 / churchRate));
        roomSet = false;
        Clear();
    }

    void Set(const Controls& c) {
        const int clampedType = Clamp(c.type, 0, kTypes - 1);
        const Building& building = kBuildings[static_cast<std::size_t>(clampedType)];
        const Nave::BuildingSpec& spec = Nave::kArchitecturesSpec[static_cast<std::size_t>(clampedType)];
        if (!roomSet || clampedType != lastType) nave.SetArchitecture(clampedType);

        const double listenerY = 0.78 * spec.naveLong;
        const double nearY = Max(spec.fromWall + 2.0, listenerY - 3.8);
        const double farY = spec.fromWall + 1.5;
        const double loft = Clamp(c.loft, 0.0, 1.0);
        const double sourceZ = 1.6 + loft * (spec.loftHeight - 1.6);
        const double higherP = sourceZ - spec.loftHeight;
        const bool needStand = !roomSet || clampedType != lastType || Abs(loft - lastStandLoft) >= 0.015;
        if (needStand) {
            for (int p = 0; p < kPlaces; ++p) {
                const double frac = static_cast<double>(p) / (kPlaces - 1);
                const double yP = nearY - (nearY - farY) * frac;
                nave.Stand(p, yP - spec.fromWall, higherP);
            }
            lastStandLoft = loft;
        }

        const double d = Clamp(c.distance, 0.0, 1.0);
        const double earlyDistScale = 1.0 - 0.55 * d;
        const double lateDistScale = 1.0 + 0.20 * d;
        const double earlyKnob = Clamp(c.earlyLevel, 0.0, 2.0);
        const double mod = Clamp(c.modulation, 0.0, 1.0);
        freezeActive = c.freeze;

        const bool needChar = !roomSet || mod != lastMod || earlyKnob != lastEarlyKnob || d != lastDist
                              || loft != lastLoft || freezeActive != lastFreeze;
        if (needChar) {
            nave.SetCharacter(mod, earlyKnob * earlyDistScale, lateDistScale, loft, freezeActive);
        }

        const double decay = Clamp(c.decay, 0.5, 2.0);
        const double decayBass = Clamp(c.decayBass, 0.5, 2.0);
        const double decayTreble = Clamp(c.decayTreble, 0.1, 0.6);
        const double maxSeat = Max(2.5, 0.5 * spec.naveWide - 1.5);
        const double seatMeters = Clamp(c.seat, -1.0, 1.0) * maxSeat;
        const double w = Clamp(c.width, 0.0, 1.0);
        const bool needNaveSet = !roomSet || clampedType != lastType || decay != lastDecay || decayBass != lastBass
                                 || decayTreble != lastTreble || w != lastWidth || seatMeters != lastSeat
                                 || mod != lastMod || (resting && (needStand || needChar));
        if (needNaveSet) {
            nave.Set(building.decay * decay, decayBass, decayTreble, building.size, w, seatMeters);
        }

        lastType = clampedType;
        lastDecay = decay;
        lastBass = decayBass;
        lastTreble = decayTreble;
        lastWidth = w;
        lastSeat = seatMeters;
        lastMod = mod;
        lastEarlyKnob = earlyKnob;
        lastDist = d;
        lastLoft = loft;
        lastFreeze = freezeActive;
        roomSet = true;

        const double at = d * (kPlaces - 1);
        const int nearIdx = Min(static_cast<int>(at), kPlaces - 2);
        const double fracFar = at - nearIdx;
        placeAsked.fill(0.0f);
        placeAsked[static_cast<std::size_t>(nearIdx)] = static_cast<float>(1.0 - fracFar);
        placeAsked[static_cast<std::size_t>(nearIdx + 1)] = static_cast<float>(fracFar);

        // Gain-compensated depth curve + pure unity dry at Mix = 0% (engaged smoothly by Mix = 15%).
        const double out = std::pow(10.0, Clamp(c.outputDb, -24.0, 12.0) / 20.0);
        const double mix = Clamp(c.mix, 0.0, 1.0);
        const double wetBlend = Clamp(mix / 0.15, 0.0, 1.0);
        const double fromSound = 1.0 / (1.0 + 0.35 * d * wetBlend);
        const double wetDistBoost = 1.0 + 0.45 * d;
        dryAsked = mix >= 1.0 ? 0.0f : static_cast<float>(out * fromSound * std::cos(0.5 * kPi * mix));
        wetAsked = mix <= 0.0 ? 0.0f : static_cast<float>(out * wetDistBoost * std::sin(0.5 * kPi * mix));

        // Mono-safe direct lateral seat cues (far-ear level attenuation up to -3.1 dB + gentle head-shadow LPF).
        const double activeY = nearY - (nearY - farY) * d;
        const double aheadMean = Max(1.0, listenerY - activeY);
        const double topMean = sourceZ - 1.2;
        const double seatFar = w * Abs(seatMeters) / std::sqrt(seatMeters * seatMeters + aheadMean * aheadMean + topMean * topMean);
        const std::size_t farEar = seatMeters > 0.0 ? 1 : 0;
        drySeatGainAsked = { { 1.0f, 1.0f } };
        dryShadowAsked = { { 0.0f, 0.0f } };
        if (Abs(seatMeters) > 1.0e-6 && w > 1.0e-6 && wetBlend > 0.0) {
            drySeatGainAsked[farEar] = static_cast<float>(Max(0.70, 1.0 - 0.35 * seatFar * wetBlend));
            dryShadowAsked[farEar] = static_cast<float>(Clamp(0.45 * seatFar * wetBlend, 0.0, 0.38));
        }

        preDelayAsked = static_cast<int>(Clamp(c.preDelayMs, 0.0, kMostPreDelayMs) * 0.001 * churchRate + 0.5);
        lowCutAsked = static_cast<float>(Clamp(c.lowCutHz, 20.0, 1000.0));
        highCutAsked = static_cast<float>(Clamp(c.highCutHz, 1000.0, 20000.0));
        lowCutMixAsked = lowCutAsked <= 20.1f ? 0.0f : 1.0f;
        highCutMixAsked = highCutAsked >= 19950.0f ? 0.0f : 1.0f;
        duckAsked = static_cast<float>(Clamp(c.duck, 0.0, 1.0));
        if (resting) Settle();
    }

    void Clear() {
        nave.Clear();
        bridge.Reset();
        for (auto& ch : preDelay) ch.fill(0.0f);
        delayAt = tailGuard = 0;
        lowState = highState = {};
        dcPrevIn.fill(0.0f);
        dcPrevOut.fill(0.0f);
        dryShadowState.fill(0.0f);
        duckEnv = duckSustain = 0.0f;
        untilHostTurn = 0;
        untilChurchTurn = 0;
        quietFor = 0;
        meterEnv[0] = meterEnv[1] = meterDuckEnv = limEnv = 0.0f;
        resting = true;
        Settle();
    }

    void Process(const float* inLeft, const float* inRight, float* outLeft, float* outRight, int frames,
                 const float* scLeft = nullptr, const float* scRight = nullptr) {
        while (frames > 0) {
            const int n = Min(frames, kBlock);
            if (nativeRate) {
                Church(inLeft, inRight, wetBuffer[0].data(), wetBuffer[1].data(), n);
            } else {
                const float* const inPtr[2] = { inLeft, inRight };
                float* const wetPtr[2] = { wetBuffer[0].data(), wetBuffer[1].data() };
                bridge.Render(inPtr, wetPtr, 2, n,
                              [this](const float* const* from, float* const* to, int m) { Church(from[0], from[1], to[0], to[1], m); });
            }

            for (int i = 0; i < n; ++i) {
                if (untilHostTurn == 0) {
                    HostTurn();
                    untilHostTurn = kTurn;
                }
                --untilHostTurn;

                const float l = inLeft[i], r = inRight[i];
                const float dl = scLeft != nullptr ? scLeft[i] : l;
                const float dr = scRight != nullptr ? scRight[i] : r;
                const float scMag = 0.5f * (Abs(dl) + Abs(dr)) + 0.5f * Max(Abs(dl), Abs(dr));
                duckSustain = Settled(duckSustain + duckSustainCoeff * (scMag - duckSustain));
                const float holdRatio = Clamp(duckSustain / (duckEnv + 1.0e-5f), 0.0f, 1.0f);
                const float rel = duckReleaseFast + holdRatio * (duckReleaseSlow - duckReleaseFast);
                duckEnv = Settled(duckEnv + (scMag > duckEnv ? duckAttack : rel) * (scMag - duckEnv));
                duckGain += Step(duckTarget - duckGain, duckGlide);

                dry += Step(dryAsked - dry, hostGlide);
                wetGain += Step(wetAsked - wetGain, hostGlide);
                drySeatGain[0] += Step(drySeatGainAsked[0] - drySeatGain[0], hostGlide);
                drySeatGain[1] += Step(drySeatGainAsked[1] - drySeatGain[1], hostGlide);
                dryShadow[0] += Step(dryShadowAsked[0] - dryShadow[0], hostGlide);
                dryShadow[1] += Step(dryShadowAsked[1] - dryShadow[1], hostGlide);

                const float dryLowL = dryShadowState[0] = Settled(l + dryShadowPole * (dryShadowState[0] - l));
                const float dryLowR = dryShadowState[1] = Settled(r + dryShadowPole * (dryShadowState[1] - r));
                float seatedL = l, seatedR = r;
                if (drySeatGain[0] != 1.0f || drySeatGain[1] != 1.0f || dryShadow[0] != 0.0f || dryShadow[1] != 0.0f) {
                    seatedL = (l + dryShadow[0] * (dryLowL - l)) * drySeatGain[0];
                    seatedR = (r + dryShadow[1] * (dryLowR - r)) * drySeatGain[1];
                }

                const float wd = wetGain * duckGain;
                const float wetL = SoftLimit(wd * wetBuffer[0][static_cast<std::size_t>(i)]);
                const float wetR = SoftLimit(wd * wetBuffer[1][static_cast<std::size_t>(i)]);
                const float rawL = dry * seatedL + wetL;
                const float rawR = dry * seatedR + wetR;
                const float stereoPeak = Max(Abs(rawL), Abs(rawR));
                const float limCoeff = stereoPeak > limEnv ? limAttack : limRelease;
                limEnv = Settled(limEnv + limCoeff * (stereoPeak - limEnv));
                const float limGain = limEnv > kLimThresh ? (kLimThresh / limEnv) : 1.0f;
                outLeft[i] = Settled(SafetyCeiling(rawL * limGain));
                outRight[i] = Settled(SafetyCeiling(rawR * limGain));

                const float aL = Abs(outLeft[i]), aR = Abs(outRight[i]);
                meterEnv[0] = aL > meterEnv[0] ? aL : Settled(meterEnv[0] * meterDecay);
                meterEnv[1] = aR > meterEnv[1] ? aR : Settled(meterEnv[1] * meterDecay);
                const float duckLoss = 1.0f - duckGain;
                meterDuckEnv = duckLoss > meterDuckEnv ? duckLoss : Settled(meterDuckEnv * meterDecay);
            }

            inLeft += n;
            inRight += n;
            outLeft += n;
            outRight += n;
            if (scLeft != nullptr) scLeft += n;
            if (scRight != nullptr) scRight += n;
            frames -= n;
        }
    }

    int ChurchLateSamples() const { return nativeRate ? 0 : bridge.LatencySamples(); }
    int Rate() const { return rate; }

    // Normalized telemetry getters (0..1) evaluated safely off the per-sample inner loop.
    float MeterOutLeft() const { return LevelToMeter(meterEnv[0]); }
    float MeterOutRight() const { return LevelToMeter(meterEnv[1]); }
    float MeterDuckGr() const {
        const float minGain = Max(0.05f, 1.0f - meterDuckEnv);
        const float grDb = -20.0f * std::log10(minGain);
        return Clamp(grDb / kDuckDepthDb, 0.0f, 1.0f);
    }

private:
    static constexpr int kBlock = 64;
    static constexpr int kPreDelaySize = 16384;
    static constexpr int kTurn = 32;
    static constexpr double kGlideSeconds = 0.02;
    static constexpr int kQuiet = 2400;
    static constexpr float kFloor = 1.0e-7f;
    static constexpr float kDuckFloorDb = -36.0f, kDuckRangeDb = 36.0f, kDuckDepthDb = 24.0f;
    static constexpr float kCornerGlide = 0.25f;
    static constexpr float kLimitKnee = 0.8912509f, kLimitCeiling = 0.99f;         // -1.0 dBFS wet knee
    static constexpr float kLimThresh = 0.9660509f;                                  // -0.3 dBFS stereo-linked peak limiter
    static constexpr float kHeadroomKnee = 0.995f, kHeadroomCeiling = 0.999f;        // single-sample impulse safety guard

    struct Pass { float low = 0.0f, band = 0.0f; };
    struct Filter { float g = 0.0f, k = 1.41421356f; };

    static float Step(float gap, float most) { return Abs(gap) <= most ? gap : (gap > 0.0f ? most : -most); }
    static float Settled(float x) { return Abs(x) < 1.1754944e-38f ? 0.0f : x; }
    static float RationalLimit(float x, float knee, float ceiling) {
        const float a = Abs(x);
        if (a <= knee) return x;
        const float head = ceiling - knee;
        const float d = (a - knee) / head;
        const float y = knee + head * (d / (1.0f + d));
        return x >= 0.0f ? y : -y;
    }
    static float SoftLimit(float x) { return RationalLimit(x, kLimitKnee, kLimitCeiling); }
    static float SafetyCeiling(float x) { return RationalLimit(x, kHeadroomKnee, kHeadroomCeiling); }
    static float LevelToMeter(float amp) {
        if (amp <= 1.0e-4f) return 0.0f;
        const float db = 20.0f * std::log10(amp);
        return Clamp((db + 60.0f) / 60.0f, 0.0f, 1.0f);
    }

    void Settle() {
        dry = dryAsked;
        wetGain = wetAsked;
        drySeatGain = drySeatGainAsked;
        dryShadow = dryShadowAsked;
        place = placeAsked;
        delay = delayBefore = preDelayAsked;
        delayFade = delayFadeSpan;
        lowCut = lowCutAsked;
        highCut = highCutAsked;
        lowCutMix = lowCutMixAsked;
        highCutMix = highCutMixAsked;
        Design(lowCutFilter, lowCut);
        Design(highCutFilter, highCut);
        duckGain = duckTarget = 1.0f;
        duckSustain = 0.0f;
    }

    void Design(Filter& f, float hz) const {
        f.g = static_cast<float>(std::tan(kPi * Clamp(static_cast<double>(hz), 10.0, 0.45 * churchRate) / churchRate));
    }

    static float Run(const Filter& f, Pass& s, float x, float& high) {
        const float a1 = 1.0f / (1.0f + f.g * (f.g + f.k)), a2 = f.g * a1, a3 = f.g * a2;
        const float v3 = x - s.low;
        const float v1 = a1 * s.band + a2 * v3;
        const float v2 = s.low + a2 * s.band + a3 * v3;
        s.band = Settled(2.0f * v1 - s.band);
        s.low = Settled(2.0f * v2 - s.low);
        high = x - f.k * v1 - v2;
        return v2;
    }

    void HostTurn() {
        const float levelDb = 20.0f * std::log10(duckEnv + 1.0e-12f);
        const float over = Clamp((levelDb - kDuckFloorDb) / kDuckRangeDb, 0.0f, 1.0f);
        duckTarget = std::pow(10.0f, -duckAsked * over * kDuckDepthDb / 20.0f);
    }

    void ChurchTurn() {
        lowCut *= std::pow(lowCutAsked / lowCut, kCornerGlide);
        highCut *= std::pow(highCutAsked / highCut, kCornerGlide);
        Design(lowCutFilter, lowCut);
        Design(highCutFilter, highCut);
        if (delayFade >= delayFadeSpan && preDelayAsked != delay) {
            delayBefore = delay;
            delay = preDelayAsked;
            delayFade = 0;
        }
    }

    void Church(const float* inLeft, const float* inRight, float* outLeft, float* outRight, int frames) {
        for (int i = 0; i < frames; ++i) {
            const float inL = inLeft[i], inR = inRight[i];
            if (resting) {
                if (inL == 0.0f && inR == 0.0f) {
                    outLeft[i] = outRight[i] = 0.0f;
                    continue;
                }
                resting = false;
                quietFor = 0;
            }
            if (untilChurchTurn == 0) {
                ChurchTurn();
                untilChurchTurn = kTurn;
            }
            --untilChurchTurn;

            const float cleanL = Settled(inL - dcPrevIn[0] + dcPole * dcPrevOut[0]);
            const float cleanR = Settled(inR - dcPrevIn[1] + dcPole * dcPrevOut[1]);
            dcPrevIn[0] = inL;
            dcPrevOut[0] = cleanL;
            dcPrevIn[1] = inR;
            dcPrevOut[1] = cleanR;

            const auto at = static_cast<std::size_t>(delayAt);
            preDelay[0][at] = cleanL;
            preDelay[1][at] = cleanR;
            const auto readAt = (at - static_cast<std::size_t>(delay)) & (kPreDelaySize - 1);
            float heardL = preDelay[0][readAt], heardR = preDelay[1][readAt];
            if (delayFade < delayFadeSpan) {
                const auto beforeAt = (at - static_cast<std::size_t>(delayBefore)) & (kPreDelaySize - 1);
                const float alpha = static_cast<float>(delayFade) / static_cast<float>(delayFadeSpan);
                heardL = preDelay[0][beforeAt] + alpha * (heardL - preDelay[0][beforeAt]);
                heardR = preDelay[1][beforeAt] + alpha * (heardR - preDelay[1][beforeAt]);
                ++delayFade;
            }
            delayAt = (delayAt + 1) & (kPreDelaySize - 1);

            const float heard = 0.5f * (heardL + heardR);
            const float heardDiff = 0.5f * (heardL - heardR);
            float others[kPlaces - 1], othersDiff[kPlaces - 1];
            for (std::size_t p = 0; p < kPlaces; ++p) {
                place[p] += Step(placeAsked[p] - place[p], churchGlide);
                if (p > 0) {
                    others[p - 1] = place[p] * heard;
                    othersDiff[p - 1] = place[p] * heardDiff;
                }
            }
            float roomLeft, roomRight;
            nave.Process(heardL, heardR, roomLeft, roomRight, others, othersDiff);

            lowCutMix += Step(lowCutMixAsked - lowCutMix, churchGlide);
            highCutMix += Step(highCutMixAsked - highCutMix, churchGlide);
            float ears[2] = { roomLeft, roomRight };
            for (std::size_t ear = 0; ear < 2; ++ear) {
                if (lowCutMix > 0.0f) {
                    float high;
                    Run(lowCutFilter, lowState[ear], ears[ear], high);
                    ears[ear] = ears[ear] + lowCutMix * (high - ears[ear]);
                } else {
                    lowState[ear] = {};
                }
                if (highCutMix > 0.0f) {
                    float high;
                    const float lp = Run(highCutFilter, highState[ear], ears[ear], high);
                    ears[ear] = ears[ear] + highCutMix * (lp - ears[ear]);
                } else {
                    highState[ear] = {};
                }
            }
            outLeft[i] = ears[0];
            outRight[i] = ears[1];
            if (inL != 0.0f || inR != 0.0f) tailGuard = Max(delay, delayBefore) + static_cast<int>(0.12 * churchRate);
            else if (tailGuard > 0) --tailGuard;
            const bool isQuiet = (!freezeActive || nave.IsResting()) && tailGuard == 0 && inL == 0.0f && inR == 0.0f && Abs(ears[0]) < kFloor && Abs(ears[1]) < kFloor;
            quietFor = isQuiet ? quietFor + 1 : 0;
            if (quietFor >= kQuiet) Rest();
        }
    }

    void Rest() {
        nave.Clear();
        for (auto& ch : preDelay) ch.fill(0.0f);
        lowState = highState = {};
        dcPrevIn.fill(0.0f);
        dcPrevOut.fill(0.0f);
        dryShadowState.fill(0.0f);
        limEnv = 0.0f;
        tailGuard = 0;
        resting = true;
        Settle();
    }

    Nave nave;
    RateBridge<2> bridge;
    int rate = 48000, churchRate = 48000;
    bool nativeRate = true, freezeActive = false;
    std::array<std::array<float, kBlock>, 2> wetBuffer {};
    std::array<std::array<float, kPreDelaySize>, 2> preDelay {};
    int delayAt = 0, delay = 0, delayBefore = 0, delayFade = kFadeSpanDefault, delayFadeSpan = kFadeSpanDefault;
    int preDelayAsked = 0, tailGuard = 0;
    static constexpr int kFadeSpanDefault = Nave::kFadeSamples;
    std::array<float, kPlaces> place {}, placeAsked {};
    std::array<Pass, 2> lowState {}, highState {};
    Filter lowCutFilter, highCutFilter;
    float lowCut = 20.0f, highCut = 20000.0f, lowCutAsked = 20.0f, highCutAsked = 20000.0f;
    float lowCutMix = 0.0f, lowCutMixAsked = 0.0f, highCutMix = 0.0f, highCutMixAsked = 0.0f;
    float duckEnv = 0.0f, duckSustain = 0.0f, duckGain = 1.0f, duckTarget = 1.0f, duckAsked = 0.0f;
    float duckAttack = 0.005f, duckReleaseFast = 0.0002f, duckReleaseSlow = 0.00005f, duckSustainCoeff = 0.00006f, duckGlide = 1.0f / 480.0f;
    float meterEnv[2] { 0.0f, 0.0f }, meterDuckEnv = 0.0f, meterDecay = 0.9999f;
    float limEnv = 0.0f, limAttack = 0.13f, limRelease = 0.0003f;
    bool roomSet = false, lastFreeze = false;
    int lastType = -1;
    double lastStandLoft = -1.0, lastDecay = 0.0, lastBass = 0.0, lastTreble = 0.0;
    double lastWidth = -1.0, lastSeat = 0.0, lastMod = -1.0, lastEarlyKnob = -1.0, lastDist = -1.0, lastLoft = -1.0;
    float dry = 1.0f, wetGain = 0.0f, dryAsked = 1.0f, wetAsked = 0.0f;
    float hostGlide = 0.0f, churchGlide = 0.0f;
    float dryShadowPole = 0.0f, dcPole = 0.9993f;
    std::array<float, 2> dcPrevIn {}, dcPrevOut {};
    std::array<float, 2> drySeatGain { { 1.0f, 1.0f } }, drySeatGainAsked { { 1.0f, 1.0f } };
    std::array<float, 2> dryShadow { { 0.0f, 0.0f } }, dryShadowAsked { { 0.0f, 0.0f } }, dryShadowState { { 0.0f, 0.0f } };
    int untilHostTurn = 0, untilChurchTurn = 0, quietFor = 0;
    bool resting = true;
};

} // namespace oregana
