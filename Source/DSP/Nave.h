#pragma once

// The church the organ stands in, heard from the nave.
//
// First the reflections a listener gets before the church as a whole
// answers, by image sources up to the third order: the organ on a west
// gallery 8 m up and 2 m from the wall behind it, a nave 14 m wide and 18 m
// to the vault, the listener seated on its axis 18 m from the organ. A
// reflection from one side reaches the far ear later and weaker. The organ's
// divisions do not stand in one place: each can be stood nearer or higher
// than the first (Stand), and its reflections then come by its own paths,
// after its own direct sound. Then the
// reverberant field, a feedback delay network of thirty-two lines (Jot and
// Chaigne, AES 90, 1991) mixed by a Hadamard matrix, so that every line
// feeds every other alike and the echoes multiply from the first pass. Each
// line loses what its length costs of the decay asked for: Decay in the
// middle of the range, Decay Bass times that below 250 Hz, and less from
// 2 kHz up, where the air of a large space takes it, down to Decay Treble
// of it at the very top. Four Schroeder allpasses make the field dense from
// its start, and each line's length drifts a tenth of a millisecond at its
// own slow rate, so no mode of the network stands still under a held chord.
// The two ears' fields are read from two halves of the lines.
//
// Church Size stretches every path: the reflections' delays and the lines'
// lengths. A new size or width is faded in over 50 ms from the same stored
// sound, read at the new distances, so turning the knob neither clicks nor
// bends the pitch of what is ringing; the decay's changes glide in 5 ms.
//
// It is a voiced church of the size a three-manual organ stands in, not a
// model of any one building: the reflections follow from the dimensions
// above, the decay is the player's, and the surfaces' reflection factors are
// estimates (0.9 stone, 0.7 a ribbed vault, 0.6 a floor of pews). It runs at
// the pipes' rate. It is a large object (4 MB of delay lines).

#include "DspMath.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace oregana {

class Nave {
public:
    // Line lengths at Church Size 1, spread geometrically over 42 to 331 ms,
    // each a prime number of samples: 4.5 s of delay in all, a mode every
    // 0.22 Hz. At Size 2 the longest is 0.66 s, which the buffer holds.
    static constexpr int kLines = 32;
    static constexpr std::array<int, kLines> kLineSamples { {
        2017, 2161, 2309, 2467, 2633, 2819, 3011, 3217, 3449, 3673, 3929, 4201, 4493, 4799, 5147, 5479,
        5857, 6263, 6689, 7151, 7643, 8171, 8731, 9337, 10007, 10663, 11399, 12197, 13033, 13921, 14879, 15901 } };
    static constexpr int kLineSize = 32768;
    // The drift: 0.1 ms each way, at 0.07 to 0.5 Hz spread over the lines.
    // A tail's pitch moves by under half a cent.
    static constexpr double kDriftSamples = 4.8, kSlowestDrift = 0.07, kFastestDrift = 0.5;
    static constexpr int kTurn = 32;   // samples between the drift's and the decay's steps
    static constexpr std::array<int, 4> kDiffuserSamples { { 101, 179, 283, 397 } };
    static constexpr int kDiffuserSize = 512;
    static constexpr float kDiffuserGain = 0.6f;

    // The church, m: see the head of the file.
    static constexpr double kNaveWide = 14.0, kToListener = 18.0, kFromWall = 2.0, kSoundSpeed = 343.0;
    // How far above the listener's ears the organ and its images in the
    // floor and the vault stand, how many surfaces each has met, and what
    // those leave of the sound: the organ itself, its image in the floor, in
    // the vault, in the vault by way of the floor and in the floor by way of
    // the vault.
    // `lift`: which way that rise goes for a source standing higher.
    struct Height { double rise; int order; double factor; double lift; };
    static constexpr std::array<Height, 5> kHeights { { { 6.8, 0, 1.0, 1.0 }, { 9.2, 1, 0.6, 1.0 }, { 26.8, 1, 0.7, -1.0 },
                                                        { 42.8, 2, 0.42, 1.0 }, { 29.2, 2, 0.42, -1.0 } } };
    // The sources heard apart: the organ's divisions.
    static constexpr int kSources = 4;
    static constexpr double kWallFactor = 0.9;
    static constexpr int kMostOrder = 3, kMostReflections = 40;
    // A reflection from the side reaches the far ear up to 0.63 ms late, a
    // head's width at the speed of sound.
    static constexpr double kHeadSamples = 30.0;
    static constexpr int kEarlySize = 16384;
    static constexpr int kAhead = 16;   // samples: under the soonest reflection's delay in the smallest church
    // The air and the people take the treble off a reflection and off what
    // enters the reverberant field: one-poles at 5 and 6 kHz, voiced.
    static constexpr double kEarlyCorner = 5000.0, kLateCorner = 6000.0;
    // Where the decay's bass begins, and where its treble is fitted: at
    // 4 kHz the field has lost a sixth of what Decay Treble takes at the
    // top, so that as shipped it rings 0.6 of the Decay there and a third of
    // it at 8 kHz, about what the air of a large church leaves.
    static constexpr double kBassCorner = 250.0, kTrebleFit = 4000.0, kTrebleShare = 0.17;
    // The reflections come to 2.2 times a plenum chord's own level. The
    // field at a decay of 4.5 s and Size 1 is set to 4.8 times it: what
    // 19.2 m from the organ gives in a church where the two are equal at
    // 4 m, an estimate. Both are then scaled alike so that together they are
    // as strong as the chord itself, and Room mixes the organ and the church
    // at one loudness (Oregana.Engine's NaveLevel). A longer decay or a
    // smaller church makes the field louder, as it does in a building.
    static constexpr float kEarlyGain = 0.2f, kLateGain = 0.13f;
    static constexpr int kFadeSamples = 2400;

    // `rt60` is the time the field takes to fall 60 dB in the middle of the
    // range; below kBassCorner it takes `bass` times that, and at the very
    // top `treble` of it. `size` stretches the church against the one above,
    // and `width` is how far apart the two ears hear: 0 one field for both,
    // 1 two fields with nothing in common, as ears (or microphones) a head
    // apart have above a few hundred hertz.
    void Set(double rt60, double bass, double treble, double size, double width) {
        const double k = std::tan(kPi * kBassCorner / kPipeRate), fit = std::tan(kPi * kTrebleFit / kPipeRate);
        for (std::size_t i = 0; i < kLines; ++i) {
            const double cost = -3.0 * size * kLineSamples[i] / (kPipeRate * rt60), mid = std::pow(10.0, cost);
            // A first-order shelf: `low` squared at 0 Hz, 1 at the top.
            const double low = std::sqrt(std::pow(10.0, cost * (1.0 / bass - 1.0)));
            const double a0 = 1.0 + k / low;
            // And one that is 1 at 0 Hz and `top` at the top, its corner put
            // where it gives the line's share of the loss at kTrebleFit: a
            // long line's corner is lower than a short one's, and the field
            // decays alike in all of them there too.
            const double top = std::pow(10.0, cost * (1.0 / treble - 1.0)), at = std::pow(top, kTrebleShare);
            const double corner = fit * std::sqrt((at * at - top * top) / (1.0 - at * at));
            const double designed[kLossTerms] = { mid * (1.0 + k * low) / a0, mid * (k * low - 1.0) / a0, (k / low - 1.0) / a0,
                                                  (top + corner) / (1.0 + corner), (corner - top) / (1.0 + corner),
                                                  (corner - 1.0) / (1.0 + corner) };
            for (std::size_t term = 0; term < kLossTerms; ++term) wanted[term][i] = static_cast<float>(designed[term]);
            // The lines' drift rates interleave across their lengths, so
            // neighbours do not move together.
            const double share = static_cast<double>((i * 7u) % kLines) / (kLines - 1);
            const double step = kTwoPi * (kSlowestDrift + share * (kFastestDrift - kSlowestDrift)) * kTurn / kPipeRate;
            turnCos[i] = std::cos(step);
            turnSin[i] = std::sin(step);
        }
        earlyPole = static_cast<float>(std::exp(-kTwoPi * kEarlyCorner / kPipeRate));
        latePole = static_cast<float>(std::exp(-kTwoPi * kLateCorner / kPipeRate));
        const double rho = 1.0 - Clamp(width, 0.0, 1.0);
        lateMid = static_cast<float>(std::sqrt(0.5 * (1.0 + rho)));
        lateSide = static_cast<float>(std::sqrt(0.5 * (1.0 - rho)));
        asked = { size, Clamp(width, 0.0, 1.0) };
        // A silent church has nothing to fade or glide.
        if (resting) {
            loss = wanted;
            Build(shapes[1], asked);
            fade = 1.0f;
        }
    }

    // Source `source` stands `forward` metres nearer the listener and
    // `higher` metres above the first source's place. Before Clear. None
    // stands behind the wall: one asked further back than half a metre
    // before it stands there.
    void Stand(int source, double forward, double higher) {
        places[static_cast<std::size_t>(source)] = { Max(forward, 0.5 - kFromWall), higher };
    }

    void Clear() {
        for (std::size_t i = 0; i < kLines; ++i) {
            lineBuffer[i].fill(0.0f);
            airIn[i] = airOut[i] = shelfOut[i] = 0.0f;
            for (std::size_t term = 0; term < kLossTerms; ++term) loss[term][i] = wanted[term][i];
            // Each line starts its drift at its own phase, the golden angle apart.
            driftCos[i] = std::cos(2.399963229728653 * static_cast<double>(i));
            driftSin[i] = std::sin(2.399963229728653 * static_cast<double>(i));
            offset[i] = static_cast<float>(kDriftSamples * driftSin[i]);
            offsetStep[i] = 0.0f;
        }
        for (auto& buffer : diffuserBuffer) buffer.fill(0.0f);
        for (auto& buffer : earlyBuffer) buffer.fill(0.0f);
        earlyQuiet.fill(kEarlySize);
        reflected = {};
        write = 0;
        untilTurn = 0;
        earlyLeft = earlyRight = late = 0.0f;
        Build(shapes[1], asked);
        fade = 1.0f;
        resting = true;
        quietFor = 0;
    }

    // Takes the organ's sound and returns what the church adds for each ear.
    // `others`, if given, is what of it comes from each source after the
    // first (kSources - 1 of them, both ears' mean); the rest is the first's.
    void Process(float left, float right, float& roomLeft, float& roomRight, const float* others = nullptr) {
        // At rest every buffer and state is zero, and silence in gives
        // silence out and leaves them so.
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
        // The reflections are worked out kAhead samples at a time: none comes
        // sooner than that after its sound, so all they need is stored. A new
        // shape comes in at such a step.
        const std::size_t ahead = static_cast<std::size_t>(write & (kAhead - 1));
        if (ahead == 0) {
            if (fade >= 1.0f && (asked.size != shapes[1].size || asked.width != shapes[1].width)) {
                shapes[0] = shapes[1];
                Build(shapes[1], asked);
                fade = 0.0f;
            }
            Reflect(shapes[1], reflected[0]);
            if (fade < 1.0f) Reflect(shapes[0], reflected[1]);
        }
        const bool fading = fade < 1.0f;

        std::array<float, kLines> out;
#if defined(OREGANA_FOUR)
        // Four lines to an instruction, each lane a line's own arithmetic.
        for (std::size_t i = 0; i < kLines; i += 4) {
            const Float4 drifted = Lanes(offset, i) + Lanes(offsetStep, i);
            Keep(offset, i, drifted);
            Float4 heard = Lines(i, drifted, shapes[1].length);
            if (fading) {
                const Float4 before = Lines(i, drifted, shapes[0].length);
                heard = before + fade * (heard - before);
            }
            const Float4 airBefore = Lanes(airOut, i);
            const Float4 air = Lanes(loss[kT0], i) * heard + Lanes(loss[kT1], i) * Lanes(airIn, i) - Lanes(loss[kTa], i) * airBefore;
            const Float4 shelved = Lanes(loss[kB0], i) * air + Lanes(loss[kB1], i) * airBefore - Lanes(loss[kA1], i) * Lanes(shelfOut, i);
            Keep(airIn, i, heard);
            Keep(airOut, i, air);
            Keep(shelfOut, i, shelved);
            Keep(out, i, shelved);
        }
#else
        for (std::size_t i = 0; i < kLines; ++i) {
            offset[i] += offsetStep[i];
            float heard = Line(i, shapes[1].length[i]);
            if (fading) {
                const float before = Line(i, shapes[0].length[i]);
                heard = before + fade * (heard - before);
            }
            const float air = loss[kT0][i] * heard + loss[kT1][i] * airIn[i] - loss[kTa][i] * airOut[i];
            out[i] = loss[kB0][i] * air + loss[kB1][i] * airOut[i] - loss[kA1][i] * shelfOut[i];
            airIn[i] = heard;
            airOut[i] = air;
            shelfOut[i] = out[i];
        }
#endif

        const float input = 0.5f * (left + right);
        float first = input;
        for (std::size_t source = 1; source < kSources; ++source) {
            const float from = others != nullptr ? others[source - 1] : 0.0f;
            first -= from;
            Hear(source, from);
        }
        Hear(0, first);
        float reflectedLeft = reflected[0][0][ahead], reflectedRight = reflected[0][1][ahead];
        if (fading) {
            const float beforeLeft = reflected[1][0][ahead], beforeRight = reflected[1][1][ahead];
            reflectedLeft = beforeLeft + fade * (reflectedLeft - beforeLeft);
            reflectedRight = beforeRight + fade * (reflectedRight - beforeRight);
            fade += 1.0f / kFadeSamples;
        }
        earlyLeft = reflectedLeft + earlyPole * (earlyLeft - reflectedLeft);
        earlyRight = reflectedRight + earlyPole * (earlyRight - reflectedRight);

        late = input + latePole * (late - input);
        float spread = late;
        for (std::size_t i = 0; i < kDiffuserSamples.size(); ++i) {
            auto& buffer = diffuserBuffer[i];
            const float delayed = buffer[static_cast<std::size_t>((write - kDiffuserSamples[i]) & (kDiffuserSize - 1))];
            const float v = spread + kDiffuserGain * delayed;
            buffer[static_cast<std::size_t>(write & (kDiffuserSize - 1))] = Settled(v);
            spread = delayed - kDiffuserGain * v;
        }

        // The left ear reads the even lines and the right ear the odd ones.
        float even = 0.0f, odd = 0.0f;
        for (std::size_t i = 0; i < kLines; i += 2) {
            even += out[i];
            odd += out[i + 1];
        }
        Mix(out);
        bool quiet = left == 0.0f && right == 0.0f;
        const auto writeAt = static_cast<std::size_t>(write & (kLineSize - 1));
#if defined(OREGANA_FOUR)
        Int4 sounding = { 0, 0, 0, 0 };
        for (std::size_t i = 0; i < kLines; i += 4) {
            // A line that takes the sound turned over: its sign bit flipped.
            const UInt4 turned = UInt4 { (kInputSigns >> i) & 1u, (kInputSigns >> (i + 1)) & 1u, (kInputSigns >> (i + 2)) & 1u,
                                         (kInputSigns >> (i + 3)) & 1u } << 31;
            const Float4 mixed = kMixGain * Lanes(out, i) + (Float4)((UInt4)Four(spread) ^ turned);
            const Float4 in = Choose(Abs(mixed) < Four(1.1754944e-38f), Four(0.0f), mixed);
            lineBuffer[i][writeAt] = in[0];
            lineBuffer[i + 1][writeAt] = in[1];
            lineBuffer[i + 2][writeAt] = in[2];
            lineBuffer[i + 3][writeAt] = in[3];
            sounding |= in != Four(0.0f);
        }
        quiet = quiet && (sounding[0] | sounding[1] | sounding[2] | sounding[3]) == 0;
#else
        for (std::size_t i = 0; i < kLines; ++i) {
            const float in = Settled(kMixGain * out[i] + (((kInputSigns >> i) & 1u) ? -spread : spread));
            lineBuffer[i][writeAt] = in;
            quiet = quiet && in == 0.0f;
        }
#endif
        write = (write + 1) & (kLineSize - 1);

        const float mid = kLateGain * lateMid * (even + odd), apart = kLateGain * lateSide * (even - odd);
        roomLeft = kEarlyGain * earlyLeft + mid + apart;
        roomRight = kEarlyGain * earlyRight + mid - apart;

        // Silence in and silence written to every line for as long as the
        // longest buffer takes to wrap: every buffer holds only zeros.
        quietFor = quiet ? quietFor + 1 : 0;
        if (quietFor >= kLineSize) {
            resting = true;
            quietFor = 0;
            airIn.fill(0.0f);
            airOut.fill(0.0f);
            shelfOut.fill(0.0f);
            earlyLeft = earlyRight = late = 0.0f;
        }
    }

private:
    static constexpr float kMixGain = 0.17677669f;   // 1 / sqrt(kLines): the Hadamard matrix loses nothing
    // The sound enters half the lines turned over, in no order the matrix
    // would gather into one line: a set bit is a line that takes it so.
    static constexpr std::uint32_t kInputSigns = 0xA3C596E1u;

    struct Reflection { int left, right; float leftGain, rightGain; };   // samples after the direct sound and level against it, each ear
    // The church at one size and width: where its reflections fall and how
    // long its lines are.
    struct Shape {
        double size = 0.0, width = 0.0;
        std::array<std::array<Reflection, kMostReflections>, kSources> reflections {};
        int count = 0;   // of each source's: every source meets the same surfaces
        std::array<float, kLines> length {};
    };
    struct Asked { double size = 1.0, width = 1.0; };
    struct Place { double forward = 0.0, higher = 0.0; };
    // A line's two shelves: the bass's, with the line's gain in it, and the air's.
    // A line's loss each pass: the bass shelf (b0, b1, a1) and the air's (t0, t1, ta).
    enum : std::size_t { kB0, kB1, kA1, kT0, kT1, kTa, kLossTerms };

    // What a buffer stores: silence below the smallest normal float.
    // Subnormals would otherwise circulate for ever, and the room never rest.
    static float Settled(float x) { return Abs(x) < 1.1754944e-38f ? 0.0f : x; }

    // The images of the organ in the side walls (up to two each way), the
    // wall behind it, the floor and the vault, by the path each one's sound
    // travels against the direct one's.
    void Build(Shape& shape, const Asked& to) const {
        shape.size = to.size;
        shape.width = to.width;
        for (std::size_t source = 0; source < kSources; ++source) Build(shape.reflections[source], shape.count, places[source], to);
        for (std::size_t i = 0; i < kLines; ++i) shape.length[i] = static_cast<float>(to.size * kLineSamples[i]);
    }

    static void Build(std::array<Reflection, kMostReflections>& reflections, int& count, const Place& place, const Asked& asked) {
        count = 0;
        const double ahead = kToListener - place.forward, top = kHeights[0].rise + place.higher;
        const double direct = std::sqrt(ahead * ahead + top * top);
        for (int side = -2; side <= 2; ++side)
            for (int behind = 0; behind <= 1; ++behind)
                for (const Height& height : kHeights) {
                    const int walls = (side < 0 ? -side : side) + behind, order = walls + height.order;
                    if (order == 0 || order > kMostOrder) continue;
                    const double across = kNaveWide * side, along = ahead + 2.0 * (kFromWall + place.forward) * behind;
                    const double rise = height.rise + height.lift * place.higher;
                    const double path = std::sqrt(across * across + along * along + rise * rise);
                    const double level = direct / path * std::pow(kWallFactor, walls) * height.factor;
                    const double samples = (path - direct) * asked.size / kSoundSpeed * kPipeRate;
                    // How far to one side it arrives from: the far ear hears
                    // it later and weaker.
                    const double aside = asked.width * across / path, far = aside < 0.0 ? -aside : aside;
                    const int near = Max(kAhead, static_cast<int>(samples + 0.5));
                    const int late = Max(kAhead, static_cast<int>(samples + kHeadSamples * far + 0.5));
                    const auto strong = static_cast<float>(level), weak = static_cast<float>(level * (1.0 - far));
                    reflections[static_cast<std::size_t>(count++)] =
                        aside < 0.0 ? Reflection { near, late, strong, weak } : Reflection { late, near, weak, strong };
                }
    }

    // A source's sound goes into its own store; one that has been silent
    // for the store's whole length holds only zeros and is passed over.
    void Hear(std::size_t source, float sound) {
        const auto at = static_cast<std::size_t>(write & (kEarlySize - 1));
        earlyBuffer[source][at] = sound;
        // The store's first samples are kept again after its last, so that
        // kAhead samples from any place in it lie in a row.
        if (at < kAhead) earlyBuffer[source][at + kEarlySize] = sound;
        earlyQuiet[source] = sound != 0.0f ? 0 : Min(earlyQuiet[source] + 1, kEarlySize);
    }

#if defined(OREGANA_FOUR)
    static Float4 Lanes(const std::array<float, kLines>& from, std::size_t i) {
        Float4 lanes;
        __builtin_memcpy(&lanes, from.data() + i, sizeof lanes);
        return lanes;
    }
    static void Keep(std::array<float, kLines>& to, std::size_t i, Float4 lanes) { __builtin_memcpy(to.data() + i, &lanes, sizeof lanes); }

    // Line for lines i to i + 3.
    Float4 Lines(std::size_t i, Float4 drifted, const std::array<float, kLines>& length) const {
        Float4 read = Four(static_cast<float>(write)) - Lanes(length, i) - drifted;
        read = Choose(read < Four(0.0f), read + Four(static_cast<float>(kLineSize)), read);
        const Int4 at = Truncated(read);
        const Float4 between = read - ToFloat(at);
        const Int4 first = at & (kLineSize - 1);
        std::int32_t from[4];
        __builtin_memcpy(from, &first, sizeof from);
        float stored[8];
        for (std::size_t lane = 0; lane < 4; ++lane) {
            const float* const buffer = lineBuffer[i + lane].data();
            stored[lane] = buffer[from[lane]];
            stored[lane + 4] = buffer[(from[lane] + 1) & (kLineSize - 1)];
        }
        Float4 older, newer;
        __builtin_memcpy(&older, stored, sizeof older);
        __builtin_memcpy(&newer, stored + 4, sizeof newer);
        return older + between * (newer - older);
    }
#endif

    float Line(std::size_t i, float length) const {
        float read = static_cast<float>(write) - length - offset[i];
        if (read < 0.0f) read += static_cast<float>(kLineSize);
        const int at = static_cast<int>(read);
        const float between = read - static_cast<float>(at);
        const auto& buffer = lineBuffer[i];
        const float older = buffer[static_cast<std::size_t>(at & (kLineSize - 1))];
        const float newer = buffer[static_cast<std::size_t>((at + 1) & (kLineSize - 1))];
        return older + between * (newer - older);
    }

    // The next kAhead samples of a shape's reflections, each ear's.
    void Reflect(const Shape& shape, std::array<std::array<float, kAhead>, 2>& ears) const {
        ears[0].fill(0.0f);
        ears[1].fill(0.0f);
        for (std::size_t source = 0; source < kSources; ++source) {
            if (earlyQuiet[source] >= kEarlySize) continue;
            const float* const buffer = earlyBuffer[source].data();
            for (int i = 0; i < shape.count; ++i) {
                const Reflection& r = shape.reflections[source][static_cast<std::size_t>(i)];
                Add(ears[0].data(), r.leftGain, buffer + ((write - r.left) & (kEarlySize - 1)));
                Add(ears[1].data(), r.rightGain, buffer + ((write - r.right) & (kEarlySize - 1)));
            }
        }
    }

    static void Add(float* sum, float gain, const float* from) {
#if defined(OREGANA_FOUR)
        for (int i = 0; i < kAhead; i += 4) {
            Float4 to, add;
            __builtin_memcpy(&to, sum + i, sizeof to);
            __builtin_memcpy(&add, from + i, sizeof add);
            to += gain * add;
            __builtin_memcpy(sum + i, &to, sizeof to);
        }
#else
        for (int i = 0; i < kAhead; ++i) sum[i] += gain * from[i];
#endif
    }

    // The fast Walsh-Hadamard transform, unscaled.
    static void Mix(std::array<float, kLines>& v) {
#if defined(OREGANA_FOUR)
        // The first two stages within each four, then fours against fours:
        // every sum and difference the stages below make, in their order.
        Float4 fours[kLines / 4];
        for (std::size_t n = 0; n < kLines / 4; ++n) {
            const float* const p = v.data() + 4 * n;
            const float sum0 = p[0] + p[1], difference0 = p[0] - p[1], sum1 = p[2] + p[3], difference1 = p[2] - p[3];
            fours[n] = Float4 { sum0 + sum1, difference0 + difference1, sum0 - sum1, difference0 - difference1 };
        }
        for (std::size_t span = 1; span < kLines / 4; span *= 2)
            for (std::size_t first = 0; first < kLines / 4; first += 2 * span)
                for (std::size_t n = first; n < first + span; ++n) {
                    const Float4 a = fours[n], b = fours[n + span];
                    fours[n] = a + b;
                    fours[n + span] = a - b;
                }
        __builtin_memcpy(v.data(), fours, sizeof fours);
#else
        for (std::size_t span = 1; span < kLines; span *= 2)
            for (std::size_t first = 0; first < kLines; first += 2 * span)
                for (std::size_t i = first; i < first + span; ++i) {
                    const float a = v[i], b = v[i + span];
                    v[i] = a + b;
                    v[i + span] = a - b;
                }
#endif
    }

    // The drifts' phasors turn by one step, kept at length 1 by a first-order
    // correction, and each line's offset ramps to its new place over the
    // step; each line's loss moves a quarter of the way to the one asked for.
    void Turn() {
        for (std::size_t i = 0; i < kLines; ++i) {
            const double c = driftCos[i] * turnCos[i] - driftSin[i] * turnSin[i];
            const double s = driftSin[i] * turnCos[i] + driftCos[i] * turnSin[i];
            const double norm = 1.5 - 0.5 * (c * c + s * s);
            driftCos[i] = c * norm;
            driftSin[i] = s * norm;
            offsetStep[i] = (static_cast<float>(kDriftSamples * driftSin[i]) - offset[i]) * (1.0f / kTurn);
            for (std::size_t term = 0; term < kLossTerms; ++term) loss[term][i] += 0.25f * (wanted[term][i] - loss[term][i]);
        }
    }

    std::array<std::array<float, kLineSize>, kLines> lineBuffer {};
    std::array<std::array<float, kLines>, kLossTerms> loss {}, wanted {};
    std::array<float, kLines> airIn {}, airOut {}, shelfOut {}, offset {}, offsetStep {};
    std::array<double, kLines> driftCos {}, driftSin {}, turnCos {}, turnSin {};
    std::array<std::array<float, kDiffuserSize>, 4> diffuserBuffer {};
    std::array<std::array<float, kEarlySize + kAhead>, kSources> earlyBuffer {};
    std::array<std::array<std::array<float, kAhead>, 2>, 2> reflected {};   // the shape heard and the one fading out, each ear
    std::array<int, kSources> earlyQuiet {};
    std::array<Place, kSources> places {};
    // The shape fading out and the one heard; `fade` is how far the second
    // has come in, 1 when it is all there is.
    std::array<Shape, 2> shapes {};
    Asked asked;
    float fade = 1.0f;
    int write = 0, untilTurn = 0;
    float earlyPole = 0.0f, latePole = 0.0f, earlyLeft = 0.0f, earlyRight = 0.0f, late = 0.0f;
    float lateMid = 0.7071f, lateSide = 0.7071f;
    bool resting = true;
    int quietFor = 0;
};

} // namespace oregana
