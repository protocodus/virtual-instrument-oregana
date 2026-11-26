#pragma once

// Churech: Oregana's church as an effect for any sound.
//
// The church is the organ's own (Nave.h), not a copy: whatever is improved
// there is heard in both. What this adds is what an effect needs around it:
//
//   Type      four buildings, each one Church Size and a Decay, from a
//             chapel to a cathedral. Decay then scales that building's.
//   Distance  where the sound stands in the nave, 4 to 19.5 m from the
//             listener at Size 1. The church hears four places apart
//             (Nave::Stand), and the sound is shared between the two places
//             either side of the one asked for, so its reflections come by
//             those places' paths. Its own sound falls as one over the
//             distance, against the nearest place.
//   Seat, Width, Decay Bass and Decay Treble are the Nave's.
//   Pre-delay before the church, Low Cut and High Cut after it, Duck (the
//             church's answer drawn down while the sound plays), and Mix
//             and Output.
//
// The church runs at its own rate, 48 kHz. Off that rate its input and its
// answer pass through RateBridge, so the sound itself (the dry path) is
// never resampled or delayed: the device has no latency, and the church
// answers a few samples later, inside its own reflections' delays.
//
// JUCE-free, C++17, nothing allocated after construction. It holds a Nave
// (4 MB): keep it on the heap.

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

    // Voiced buildings, not measured ones: Church Size against the Nave's
    // 14 m wide nave, and a mid-range decay in the span such buildings are
    // reported to have (a chapel under 2 s, a large cathedral 6 to 10 s).
    struct Building { double size, decay; };
    static constexpr std::array<Building, kTypes> kBuildings { { { 0.5, 1.6 }, { 0.8, 2.8 }, { 1.25, 4.5 }, { 2.0, 7.5 } } };

    // The controls in their own units. Front ends map their ranges onto
    // these; the Rack device's Lua says the same ranges.
    struct Controls {
        int type = kChurch;
        double decay = 1.0;           // times the building's, 0.5 .. 2
        double decayBass = 1.3;       // the Nave's, 0.5 .. 2
        double decayTreble = 0.2;     // the Nave's, 0.1 .. 0.6
        double distance = 0.0;        // 0 nearest .. 1 furthest place
        double seat = 0.0;            // -1 by the left wall .. 1 by the right
        double width = 0.8;           // the Nave's, 0 .. 1
        double preDelayMs = 0.0;      // 0 .. kMostPreDelayMs
        double lowCutHz = 20.0;       // 20 .. 1000
        double highCutHz = 20000.0;   // 1000 .. 20000
        double duck = 0.0;            // 0 .. 1
        double mix = 0.3;             // 0 dry .. 1 only the church
        double outputDb = 0.0;        // -24 .. 12
    };

    static constexpr int kPlaces = Nave::kSources;
    // How far forward of the Nave's first place each stands, m, and how far
    // below its gallery: a voice or an instrument on the floor, 1.6 m up,
    // where the organ stands 8 m up.
    static constexpr std::array<double, kPlaces> kPlaceForward { { 14.0, 9.0, 4.0, -1.5 } };
    static constexpr double kPlaceHigher = 1.6 - 8.0;
    static constexpr double kMostPreDelayMs = 200.0;

    Churech() {
        for (int p = 0; p < kPlaces; ++p) nave.Stand(p, kPlaceForward[static_cast<std::size_t>(p)], kPlaceHigher);
        Set(Controls {});
        nave.Clear();
        Prepare(48000);
    }

    // Off the audio thread, before playing.
    void Prepare(int hostRate) {
        rate = hostRate;
        bridge.Prepare(static_cast<double>(hostRate));
        hostGlide = static_cast<float>(1.0 / (kGlideSeconds * hostRate));
        Clear();
    }

    void Set(const Controls& c) {
        const Building& building = kBuildings[static_cast<std::size_t>(Clamp(c.type, 0, kTypes - 1))];
        nave.Set(building.decay * Clamp(c.decay, 0.5, 2.0), Clamp(c.decayBass, 0.5, 2.0), Clamp(c.decayTreble, 0.1, 0.6), building.size,
                 Clamp(c.width, 0.0, 1.0), Clamp(c.seat, -1.0, 1.0) * Nave::kSeatMost);
        // The two places either side of the one asked for share the sound.
        const double at = Clamp(c.distance, 0.0, 1.0) * (kPlaces - 1);
        const int near = Min(static_cast<int>(at), kPlaces - 2);
        const double far = at - near;
        placeAsked.fill(0.0f);
        placeAsked[static_cast<std::size_t>(near)] = static_cast<float>(1.0 - far);
        placeAsked[static_cast<std::size_t>(near + 1)] = static_cast<float>(far);
        // The sound's own level falls as one over its distance, against the
        // nearest place: the reflections keep theirs (Nave.h's levels are
        // against each place's own direct sound), and so does the field.
        const double metres = (1.0 - far) * PlaceMetres(near) + far * PlaceMetres(near + 1);
        const double fromSound = PlaceMetres(0) / metres;
        const double out = std::pow(10.0, Clamp(c.outputDb, -24.0, 12.0) / 20.0);
        const double mix = Clamp(c.mix, 0.0, 1.0);
        // Equal power: halfway, the sound and the church are each 3 dB down.
        dryAsked = static_cast<float>(out * fromSound * std::cos(0.5 * kPi * mix));
        wetAsked = static_cast<float>(out * std::sin(0.5 * kPi * mix));
        preDelayAsked = static_cast<int>(Clamp(c.preDelayMs, 0.0, kMostPreDelayMs) * 0.001 * kPipeRate + 0.5);
        lowCutAsked = static_cast<float>(Clamp(c.lowCutHz, 20.0, 1000.0));
        highCutAsked = static_cast<float>(Clamp(c.highCutHz, 1000.0, 20000.0));
        duckAsked = static_cast<float>(Clamp(c.duck, 0.0, 1.0));
        if (resting) Settle();
    }

    void Clear() {
        nave.Clear();
        bridge.Reset();
        preDelay.fill(0.0f);
        delayAt = 0;
        lowState = highState = {};
        follower = 0.0f;
        untilTurn = 0;
        quietFor = 0;
        resting = true;
        Settle();
    }

    // `in` and `out`: a left and a right buffer of `frames`; out may be in.
    void Process(const float* inLeft, const float* inRight, float* outLeft, float* outRight, int frames) {
        while (frames > 0) {
            const int n = Min(frames, kBlock);
            const float* const in[2] = { inLeft, inRight };
            float* const wet[2] = { wetBuffer[0].data(), wetBuffer[1].data() };
            bridge.Render(in, wet, 2, n, [this](const float* const* from, float* const* to, int m) { Church(from[0], from[1], to[0], to[1], m); });
            for (int i = 0; i < n; ++i) {
                dry += Step(dryAsked - dry, hostGlide);
                wetGain += Step(wetAsked - wetGain, hostGlide);
                const float l = inLeft[i], r = inRight[i];
                outLeft[i] = Settled(dry * l + wetGain * wetBuffer[0][static_cast<std::size_t>(i)]);
                outRight[i] = Settled(dry * r + wetGain * wetBuffer[1][static_cast<std::size_t>(i)]);
            }
            inLeft += n;
            inRight += n;
            outLeft += n;
            outRight += n;
            frames -= n;
        }
    }

    // Host samples between the sound and the church taking it, beyond its
    // own delays: none at 48 kHz.
    int ChurchLateSamples() const { return bridge.LatencySamples(); }
    int Rate() const { return rate; }

private:
    static constexpr int kBlock = 64;
    static constexpr int kPreDelaySize = 16384;   // over kMostPreDelayMs at 48 kHz
    static constexpr int kTurn = 32;              // church samples between the filters' and the duck's steps
    static constexpr double kGlideSeconds = 0.02;
    static constexpr float kChurchGlide = static_cast<float>(1.0 / (kGlideSeconds * kPipeRate));
    // A pre-delay's new time is faded in over the Nave's 50 ms, from the same
    // stored sound, so that turning it neither clicks nor bends the pitch.
    static constexpr int kDelayFade = Nave::kFadeSamples;
    // 50 ms of no sound in and the church's answer under -140 dB: it has
    // rung out (Rest).
    static constexpr int kQuiet = 2400;
    static constexpr float kFloor = 1.0e-7f;
    // The duck follows the sound's peak, at once up and in 0.4 s down, and
    // draws the church down by Duck's share of 24 dB as the sound rises from
    // -40 to -16 dBFS; the draw itself moves in 10 ms. Voiced, as a
    // broadcast engineer's ducker is.
    static constexpr float kDuckRelease = 1.0f - 1.0f / (0.4f * static_cast<float>(kPipeRate));
    static constexpr float kDuckFloorDb = -40.0f, kDuckDepthDb = 24.0f;
    static constexpr float kDuckGlide = 1.0f / (0.01f * static_cast<float>(kPipeRate));
    // The cut filters' corners glide a quarter of the way to the asked one
    // each turn, in octaves.
    static constexpr float kCornerGlide = 0.25f;

    struct Pass { float low = 0.0f, band = 0.0f; };          // a state-variable filter's two integrators
    struct Filter { float g = 0.0f, k = 1.41421356f; };      // Butterworth: 1 / Q = sqrt(2)

    static double PlaceMetres(int place) {
        const double ahead = Nave::kToListener - kPlaceForward[static_cast<std::size_t>(place)];
        const double rise = Nave::kHeights[0].rise + kPlaceHigher;
        return std::sqrt(ahead * ahead + rise * rise);
    }
    static float Step(float gap, float most) { return Abs(gap) <= most ? gap : (gap > 0.0f ? most : -most); }
    static float Settled(float x) { return Abs(x) < 1.1754944e-38f ? 0.0f : x; }

    // Everything at what was asked, with nothing to glide: before the first
    // sound, and after Clear.
    void Settle() {
        dry = dryAsked;
        wetGain = wetAsked;
        place = placeAsked;
        delay = delayBefore = preDelayAsked;
        delayFade = kDelayFade;
        lowCut = lowCutAsked;
        highCut = highCutAsked;
        Design(lowCutFilter, lowCut);
        Design(highCutFilter, highCut);
        duckGain = duckTarget = 1.0f;
    }

    void Design(Filter& f, float hz) const { f.g = static_cast<float>(std::tan(kPi * hz / kPipeRate)); }

    // A state-variable filter (Simper's trapezoidal form), which stays put
    // while its corner moves. Returns the low-pass; `high` is the high-pass.
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

    void Turn() {
        // In octaves, so a wide turn glides evenly.
        lowCut *= std::pow(lowCutAsked / lowCut, kCornerGlide);
        highCut *= std::pow(highCutAsked / highCut, kCornerGlide);
        Design(lowCutFilter, lowCut);
        Design(highCutFilter, highCut);
        const float level = 20.0f * std::log10(follower + 1.0e-12f);
        duckTarget = std::pow(10.0f, -duckAsked * Clamp(level - kDuckFloorDb, 0.0f, kDuckDepthDb) / 20.0f);
        if (delayFade >= kDelayFade && preDelayAsked != delay) {
            delayBefore = delay;
            delay = preDelayAsked;
            delayFade = 0;
        }
    }

    // `frames` samples of the church at its own rate.
    void Church(const float* inLeft, const float* inRight, float* outLeft, float* outRight, int frames) {
        for (int i = 0; i < frames; ++i) {
            const float sound = 0.5f * (inLeft[i] + inRight[i]);
            if (resting) {
                if (sound == 0.0f) {
                    outLeft[i] = outRight[i] = 0.0f;
                    continue;
                }
                resting = false;
                quietFor = 0;
            }
            if (untilTurn == 0) {
                Turn();
                untilTurn = kTurn;
            }
            --untilTurn;

            const float magnitude = Abs(sound);
            follower = magnitude > follower ? magnitude : Settled(kDuckRelease * follower);
            duckGain += Step(duckTarget - duckGain, kDuckGlide);

            const auto at = static_cast<std::size_t>(delayAt);
            preDelay[at] = sound;
            float heard = preDelay[(at - static_cast<std::size_t>(delay)) & (kPreDelaySize - 1)];
            if (delayFade < kDelayFade) {
                const float before = preDelay[(at - static_cast<std::size_t>(delayBefore)) & (kPreDelaySize - 1)];
                heard = before + (static_cast<float>(delayFade) / kDelayFade) * (heard - before);
                ++delayFade;
            }
            delayAt = (delayAt + 1) & (kPreDelaySize - 1);

            float others[kPlaces - 1];
            for (std::size_t p = 0; p < kPlaces; ++p) {
                place[p] += Step(placeAsked[p] - place[p], kChurchGlide);
                if (p > 0) others[p - 1] = place[p] * heard;
            }
            float roomLeft, roomRight;
            // Nave takes what is not given to the others as its first place's.
            nave.Process(heard, heard, roomLeft, roomRight, others);

            float ears[2] = { roomLeft, roomRight };
            for (std::size_t ear = 0; ear < 2; ++ear) {
                float high;
                Run(lowCutFilter, lowState[ear], ears[ear], high);   // its high-pass is the low cut
                ears[ear] = Run(highCutFilter, highState[ear], high, high) * duckGain;
            }
            outLeft[i] = ears[0];
            outRight[i] = ears[1];
            quietFor = sound == 0.0f && Abs(ears[0]) < kFloor && Abs(ears[1]) < kFloor ? quietFor + 1 : 0;
            if (quietFor >= kQuiet) Rest();
        }
    }

    // The church has rung out below kFloor with no sound in. Left alone the
    // Nave would go on until its tail fell under the smallest float, over a
    // minute in a cathedral; it is emptied instead, and what glides is put
    // where it is going.
    void Rest() {
        nave.Clear();
        lowState = highState = {};
        follower = 0.0f;
        resting = true;
        Settle();
    }

    Nave nave;
    RateBridge<2> bridge;
    int rate = 48000;
    std::array<std::array<float, kBlock>, 2> wetBuffer {};
    std::array<float, kPreDelaySize> preDelay {};
    int delayAt = 0, delay = 0, delayBefore = 0, delayFade = kDelayFade, preDelayAsked = 0;
    std::array<float, kPlaces> place {}, placeAsked {};
    std::array<Pass, 2> lowState {}, highState {};
    Filter lowCutFilter, highCutFilter;
    float lowCut = 20.0f, highCut = 20000.0f, lowCutAsked = 20.0f, highCutAsked = 20000.0f;
    float follower = 0.0f, duckGain = 1.0f, duckTarget = 1.0f, duckAsked = 0.0f;
    float dry = 1.0f, wetGain = 0.0f, dryAsked = 1.0f, wetAsked = 0.0f, hostGlide = 0.0f;
    int untilTurn = 0, quietFor = 0;
    bool resting = true;   // rung out (Rest): Set takes a change at once
};

} // namespace oregana
