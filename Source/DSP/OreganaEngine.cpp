#include "OreganaEngine.h"

#include "PipeRegulation.h"

#include <cmath>
#include <cstddef>
#include <initializer_list>

namespace oregana {

namespace {

// The Hauptwerk's Octave 8' at c1, alone and at the pipes, sounds at this
// level (RMS, both channels' power together): a full-organ chord then peaks
// a few decibels under the output stage's ceiling.
constexpr float kMaster = 0.012f;

// A pipe is heard from where it stands: the ranks of a division stand in rows
// 12 cm apart over the chest, back from the front pipes, and no pipe quite in
// its row (up to 1.5 m off, the large ones on conveyances). Sound takes 2.9 ms
// a metre, so the pipes of a chord speak over some 12 ms and never in step:
// begun together, pipes whose harmonics coincide would add as one pulse, 8 dB
// above what a real organ's give.
constexpr double kRowMetres = 0.12, kOffRowMetres = 1.5;

// The flattest a pipe is ever made against its regulated pitch (a1 at 415 Hz
// in meantone, a cold church and the worst drift come to 0.907), and what its
// delay lines are given room for.
constexpr double kLowestRatio = 0.89, kRoomRatio = 0.88;
constexpr int kRoomSpare = 4;

constexpr double kStepSeconds = FluePipe::kTick / kPipeRate;

// Each division's winding (Wind.h): the Pedal's large reservoir is the
// slowest and sags most, the Positiv's small one the quickest. Voiced.
constexpr WindSpec kWinding[kDivisions] = {
    { 0.025, 0.07, 5.0 }, { 0.02, 0.06, 6.5 }, { 0.025, 0.06, 5.5 }, { 0.035, 0.09, 3.8 } };
// What "full organ" takes: every stop of the division on a chord of eight
// keys from C up for a manual, and on an octave from C for the Pedal.
constexpr int kFullChord[] = { 0, 7, 12, 16, 19, 24, 28, 31 };
constexpr int kFullPedal[] = { 0, 12 };
// The pressure's slow unrest: 0.3 % RMS as modelled (Wind at 0.5), noise
// through two one-poles at 1 Hz, whose output is 0.0186 RMS.
constexpr double kUnrest = 0.16, kUnrestPole = 0.0042;
// A channel's pipes share one pallet: with every stop drawn each gets 3 %
// less pressure than a stop alone, falling with the square of the wind taken.
constexpr float kRobbing = 0.03f;

// The swell box: shut, its walls and shutters take 8 dB of the bass and 24 dB
// of the treble, split at 500 Hz (an estimate of a well-built box, not a
// measurement); the shutters move with the pedal, in decibels.
constexpr float kBoxPole = 0.0634f;
constexpr double kBoxLowDb = -8.0, kBoxHighDb = -24.0;
constexpr float kGainStep = 0.001f;   // output gains glide in about 20 ms

// The noises at Action and Blower 1: a key's knock peaks near -38 dBFS and
// the wind noise sits at -50 dBFS RMS, at the pipes (estimates).
constexpr double kKnockLevel = 0.012, kBlowerLevel = 0.22;
constexpr float kPresenceRise = 0.00222f, kPresenceFall = 0.000333f;   // 0.3 s and 2 s

// The church's decay towards the treble (Nave.h): a fifth of the time at 24 kHz.
constexpr double kTrebleShare = 0.2;

constexpr double kDeadShare = 0.0625, kCipherShare = 0.04, kRunningShare = 0.083;

constexpr std::array<int, kRankCount + 1> RankFirsts() {
    std::array<int, kRankCount + 1> first {};
    for (int r = 0; r < kRankCount; ++r)
        first[static_cast<std::size_t>(r + 1)] = first[static_cast<std::size_t>(r)] + kDivisionKeys[RankDivision(r)];
    return first;
}
constexpr std::array<int, kRankCount + 1> kRankFirst = RankFirsts();

constexpr std::array<int, kDivisions + 1> DivisionRanks() {
    std::array<int, kDivisions + 1> first {};
    for (int r = 0; r < kRankCount; ++r) first[static_cast<std::size_t>(RankDivision(r)) + 1] = r + 1;
    return first;
}
// A division's ranks are kRanks[kDivisionRank[d]] up to kDivisionRank[d + 1].
constexpr std::array<int, kDivisions + 1> kDivisionRank = DivisionRanks();

// A pipe's or a key's own share of an imperfection: the same every time.
std::uint32_t Hash(std::uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}
// 0..1, evenly.
double Unit(int place, int what) {
    const std::uint32_t h = Hash(static_cast<std::uint32_t>(place) * 0x9E3779B9u + static_cast<std::uint32_t>(what) * 0x85EBCA6Bu + 1u);
    return (h >> 8) * (1.0 / 16777216.0);
}
// -1..1, mostly near 0.
double Spread(int place, int what) { return Unit(place, what) + Unit(place, what + 64) - 1.0; }
enum What { kDrift = 1, kLevel, kSpeech, kChiff, kNoise, kDead, kCipher, kCipherLeak, kRunning, kRunningLeak, kOffRow };

// Where a pipe stands across the organ's front, -1 left .. 1 right: the keys
// alternate between the two sides of a chest, the largest pipes outermost.
// The Hauptwerk fills the case, the Positiv and the Recit its middle, and the
// Pedal stands in the two towers.
double Stand(int division, int key) {
    const double side = (key & 1) ? 1.0 : -1.0;
    const double bass = 1.0 - static_cast<double>(key) / (kDivisionKeys[division] - 1);
    switch (division) {
    case kPositiv: return side * (0.1 + 0.4 * bass);
    case kRecit: return side * (0.1 + 0.5 * bass);
    case kPedal: return side * (0.75 + 0.25 * bass);
    default: return side * (0.15 + 0.85 * bass);
    }
}

double Sanitized(int index, double value) {
    const ControlInfo& c = kControlInfo[static_cast<std::size_t>(index)];
    if (!(value == value)) return c.initial;
    value = Clamp(value, c.min, c.max);
    return c.steps > 0 ? std::floor(value + 0.5) : value;
}

} // namespace

Engine::Engine() {
    slots.resize(static_cast<std::size_t>(kPipeCount));
    int flueCount = 0, reedCount = 0;
    std::size_t room = 0;
    for (int rank = 0; rank < kRankCount; ++rank) {
        const int division = RankDivision(rank);
        for (int key = 0; key < kDivisionKeys[division]; ++key) {
            const int index = kRankFirst[static_cast<std::size_t>(rank)] + key;
            Slot& s = slots[static_cast<std::size_t>(index)];
            s.rank = static_cast<std::uint8_t>(rank);
            s.key = static_cast<std::uint8_t>(key);
            s.division = static_cast<std::uint8_t>(division);
            const PipePlan plan = PlanPipe(rank, key);
            if (!plan.exists) continue;
            s.reed = plan.reed;
            const double metres = kRowMetres * (rank - kDivisionRank[static_cast<std::size_t>(division)]) + kOffRowMetres * Unit(index, kOffRow);
            s.delay = static_cast<std::uint16_t>(std::lround(metres / kSoundSpeed * kPipeRate));
            const double tuning = kPipeRegulation[index].tuning;
            if (plan.reed) {
                ReedPipe pipe;
                pipe.Design(plan.tongue, plan.hz * kRoomRatio, tuning);
                s.loopRoom = pipe.loopLength + kRoomSpare;
                s.index = static_cast<std::int16_t>(reedCount++);
            } else {
                FluePipe pipe;
                pipe.Design(plan.flue, plan.hz * kRoomRatio, tuning);
                s.loopRoom = pipe.loopLength + kRoomSpare;
                s.jetRoom = pipe.jetLength + kRoomSpare;
                s.upperRoom = pipe.upperLength > 0 ? pipe.upperLength + kRoomSpare : 0;
                s.index = static_cast<std::int16_t>(flueCount++);
            }
            room += static_cast<std::size_t>(s.loopRoom + s.jetRoom + s.upperRoom);
        }
    }
    arena.assign(room, 0.0f);
    flues.resize(static_cast<std::size_t>(flueCount));
    reeds.resize(static_cast<std::size_t>(reedCount));
    active.reserve(static_cast<std::size_t>(kPipeCount));
    pending.reserve(static_cast<std::size_t>(4 * kPipeCount));
    float* next = arena.data();
    for (Slot& s : slots) {
        if (s.index < 0) continue;
        s.buffer = next;
        next += s.loopRoom + s.jetRoom + s.upperRoom;
        if (s.reed) {
            reeds[static_cast<std::size_t>(s.index)].loop = s.buffer;
        } else {
            FluePipe& pipe = flues[static_cast<std::size_t>(s.index)];
            pipe.loop = s.buffer;
            pipe.jet = s.buffer + s.loopRoom;
            pipe.upper = s.buffer + s.loopRoom + s.jetRoom;
        }
    }

    for (int i = 0; i < kControlCount; ++i) controls[i] = Sanitized(i, controls[i]);
    for (int index = 0; index < kPipeCount; ++index)
        if (slots[static_cast<std::size_t>(index)].index >= 0) Design(index);
    for (int d = 0; d < kDivisions; ++d) {
        for (int rank = kDivisionRank[static_cast<std::size_t>(d)]; rank < kDivisionRank[static_cast<std::size_t>(d) + 1]; ++rank)
            for (int key = 0; key < kDivisionKeys[d]; ++key)
                fullAir[static_cast<std::size_t>(d)][static_cast<std::size_t>(key)]
                    += slots[static_cast<std::size_t>(kRankFirst[static_cast<std::size_t>(rank)] + key)].air;
        if (d == kPedal) for (int key : kFullPedal) fullWind[static_cast<std::size_t>(d)] += fullAir[static_cast<std::size_t>(d)][static_cast<std::size_t>(key)];
        else for (int key : kFullChord) fullWind[static_cast<std::size_t>(d)] += fullAir[static_cast<std::size_t>(d)][static_cast<std::size_t>(key)];
        reservoirs[static_cast<std::size_t>(d)].Design(kWinding[d], kStepSeconds);
    }
    output.Prepare(kPipeRate);
    Apply(controls, true);
    Reset();
}

void Engine::Reset() {
    for (std::size_t i = 0; i < slots.size(); ++i) {
        Slot& s = slots[i];
        s.active = false;
        s.offered = s.share = 0.0f;
        if (s.index < 0) continue;
        // Each pipe's turbulence is its own.
        const std::uint32_t seed = Hash(static_cast<std::uint32_t>(i) + 0x51ED270Bu) | 1u;
        if (s.reed) {
            reeds[static_cast<std::size_t>(s.index)].Clear();
            reeds[static_cast<std::size_t>(s.index)].random = seed;
        } else {
            flues[static_cast<std::size_t>(s.index)].Clear();
            flues[static_cast<std::size_t>(s.index)].random = seed;
        }
    }
    active.clear();
    pending.clear();
    now = 0;
    for (std::size_t d = 0; d < kDivisions; ++d) {
        played[d].fill(false);
        open[d].fill(false);
        hang[d].fill(0.0f);
        share[d].fill(0.0f);
        robbed[d].fill(1.0f);
        reservoirs[d].Reset();
        demand[d] = unrest[d] = unrestSlow[d] = 0.0;
        pressure[d] = 1.0f;
        for (auto& channel : bus[d]) channel.fill(0.0f);
    }
    touch.fill(1.0f);
    for (Tremulant& t : tremulants) t.Reset();
    unrestRandom = 0x2545F491u;
    untilTick = 0;
    boxLow[0] = boxLow[1] = 0.0f;
    boxLowGain = boxLowTarget;
    boxHighGain = boxHighTarget;
    dry = dryTarget;
    wet = wetTarget;
    presence = blower = 0.0f;
    knocks.Clear();
    windNoise.Clear();
    nave.Clear();
    output.Reset();
    diag = Diagnostics {};
    for (double& p : diag.pressure) p = 1.0;
}

void Engine::SetControls(const Controls& next) {
    const Controls before = controls;
    for (int i = 0; i < kControlCount; ++i) controls[i] = Sanitized(i, next[i]);
    Apply(before, false);
}

void Engine::Apply(const Controls& before, bool all) {
    const auto changed = [&](int index) { return all || controls[index] != before[index]; };

    if (!all && (changed(kPitch) || changed(kTemperament) || changed(kTemperature) || changed(kTuning) || changed(kVoicing))) {
        ++epoch;
        stale = kPipeCount;
    }

    bool registration = all || changed(kCrescendo) || changed(kDeadPipes) || changed(kRunnings) || changed(kCiphers);
    int knobs = 0;
    for (int i = 0; i < kStopCount + kCouplerCount; ++i) {
        if (!changed(i)) continue;
        registration = true;
        // A combination's knobs go together: three knocks tell as many.
        if (!all && controls[kAction] > 0.0 && knobs++ < 3) knocks.Strike(kStopKnob, kKnockLevel * controls[kAction], 0.0);
    }
    if (changed(kCrescendo)) crescendo = controls[kCrescendo];
    if (changed(kSwell)) SetSwell(controls[kSwell]);
    if (!all && changed(kCiphers)) {
        // A pallet that is no longer one to hang closes.
        for (int d = 0; d < kDivisions; ++d)
            for (int key = 0; key < kDivisionKeys[d]; ++key)
                if (Unit(d * 64 + key, kCipher) >= kCipherShare * controls[kCiphers])
                    hang[static_cast<std::size_t>(d)][static_cast<std::size_t>(key)] = 0.0f;
    }

    if (changed(kTremulantRate))
        for (Tremulant& t : tremulants) t.Design(controls[kTremulantRate], kStepSeconds);
    if (changed(kDecay)) nave.Set(controls[kDecay], kTrebleShare);
    if (changed(kWidth)) {
        nave.SetWidth(controls[kWidth]);
        for (int d = 0; d < kDivisions; ++d)
            for (int key = 0; key < kDivisionKeys[d]; ++key) {
                const double angle = 0.25 * kPi * (1.0 + 0.7 * controls[kWidth] * Stand(d, key));
                placeLeft[static_cast<std::size_t>(d)][static_cast<std::size_t>(key)] = static_cast<float>(std::cos(angle));
                placeRight[static_cast<std::size_t>(d)][static_cast<std::size_t>(key)] = static_cast<float>(std::sin(angle));
            }
        for (int index : active) Place(slots[static_cast<std::size_t>(index)]);
    }
    if (changed(kVolume) || changed(kRoom)) {
        // The listener walks down the nave: the organ's own sound gives way
        // to the church's, at one loudness.
        const double volume = std::pow(10.0, controls[kVolume] / 20.0);
        dryTarget = static_cast<float>(volume * std::sqrt(1.0 - controls[kRoom]));
        wetTarget = static_cast<float>(volume * std::sqrt(controls[kRoom]));
    }
    if (registration) Register();
}

void Engine::SetSwell(double position) {
    swell = position == position ? Clamp(position, 0.0, 1.0) : 1.0;
    boxLowTarget = static_cast<float>(std::pow(10.0, kBoxLowDb * (1.0 - swell) / 20.0));
    boxHighTarget = static_cast<float>(std::pow(10.0, kBoxHighDb * (1.0 - swell) / 20.0));
}

void Engine::SetCrescendo(double position) {
    const double before = crescendo;
    crescendo = position == position ? Clamp(position, 0.0, 1.0) : 0.0;
    if (std::lround(crescendo * kCrescendoStages) != std::lround(before * kCrescendoStages)) Register();
}

// What is drawn, by hand and by the roller, and what every key then gives.
void Engine::Register() {
    for (int i = 0; i < kStopCount; ++i) stopDrawn[static_cast<std::size_t>(i)] = controls[i] >= 0.5;
    for (int i = 0; i < kCouplerCount; ++i) coupled[static_cast<std::size_t>(i)] = controls[kCouplerPosHw + i] >= 0.5;
    const int stage = static_cast<int>(std::lround(crescendo * kCrescendoStages));
    for (int i = 0; i < stage; ++i) {
        if (kCrescendoOrder[i] < kStopCount) stopDrawn[static_cast<std::size_t>(kCrescendoOrder[i])] = true;
        else coupled[static_cast<std::size_t>(kCrescendoOrder[i] - kCouplerPosHw)] = true;
    }
    for (int i = 0; i < kStopCount; ++i)
        for (int r = 0; r < kStops[i].ranks; ++r)
            rankDrawn[static_cast<std::size_t>(kStops[i].firstRank + r)] = stopDrawn[static_cast<std::size_t>(i)];
    for (int key = 0; key < kManualKeys; ++key) UpdateKey(key);
}

// Which pallets key `key` holds open across the divisions: its own, and
// through the couplers, which pull one another's keys as a tracker action's do.
void Engine::UpdateKey(int key) {
    const auto k = static_cast<std::size_t>(key);
    const bool pedal = key < kPedalKeys && played[kPedal][k];
    bool pulled[kDivisions];
    pulled[kHauptwerk] = played[kHauptwerk][k] || (coupled[3] && pedal);
    pulled[kPositiv] = played[kPositiv][k] || (coupled[0] && pulled[kHauptwerk]) || (coupled[4] && pedal);
    pulled[kRecit] = played[kRecit][k] || (coupled[1] && pulled[kHauptwerk]) || (coupled[2] && pulled[kPositiv]) || (coupled[5] && pedal);
    pulled[kPedal] = pedal;
    for (int d = 0; d < kDivisions; ++d) {
        if (key >= kDivisionKeys[d] || pulled[d] == open[static_cast<std::size_t>(d)][k]) continue;
        open[static_cast<std::size_t>(d)][k] = pulled[d];
        // A pallet that hangs is pulled free by its key, and hangs again
        // when the key lets it go.
        const int place = d * 64 + key;
        const bool hangs = !pulled[d] && Unit(place, kCipher) < kCipherShare * controls[kCiphers];
        hang[static_cast<std::size_t>(d)][k] = hangs ? static_cast<float>(0.35 + 0.55 * Unit(place, kCipherLeak)) : 0.0f;
    }
    for (int d = 0; d < kDivisions; ++d)
        for (int near : { key - 2, key, key + 2 })
            if (near >= 0 && near < kDivisionKeys[d]) UpdateChannel(d, near);
}

// The wind one key's channel holds, and which of its pipes take it.
void Engine::UpdateChannel(int division, int key) {
    const auto d = static_cast<std::size_t>(division), k = static_cast<std::size_t>(key);
    float wind = open[d][k] ? 1.0f : hang[d][k];
    if (!open[d][k] && controls[kRunnings] > 0.0) {
        for (int near : { key - 2, key + 2 }) {
            if (near < 0 || near >= kDivisionKeys[division] || !open[d][static_cast<std::size_t>(near)]) continue;
            const int place = division * 64 + Min(key, near);
            if (Unit(place, kRunning) < kRunningShare * controls[kRunnings])
                wind = Max(wind, static_cast<float>(0.35 + 0.3 * Unit(place, kRunningLeak)));
        }
    }
    share[d][k] = wind;

    const double dead = kDeadShare * controls[kDeadPipes];
    const int firstRank = kDivisionRank[d], endRank = kDivisionRank[d + 1];
    const auto speaks = [&](int rank, int index) {
        return rankDrawn[static_cast<std::size_t>(rank)] && slots[static_cast<std::size_t>(index)].index >= 0
            && !(dead > 0.0 && Unit(index, kDead) < dead);
    };
    float taken = 0.0f;
    for (int rank = firstRank; rank < endRank; ++rank) {
        const int index = kRankFirst[static_cast<std::size_t>(rank)] + key;
        if (speaks(rank, index)) taken += slots[static_cast<std::size_t>(index)].air;
    }
    const float load = taken / fullAir[d][k];
    robbed[d][k] = 1.0f / (1.0f + kRobbing * load * load);
    for (int rank = firstRank; rank < endRank; ++rank) {
        const int index = kRankFirst[static_cast<std::size_t>(rank)] + key;
        if (slots[static_cast<std::size_t>(index)].index >= 0) Offer(index, speaks(rank, index) ? wind : 0.0f);
    }
}

// A pallet gives a pipe its share of the wind, or takes it away: heard when
// the sound has come from where the pipe stands.
void Engine::Offer(int index, float given) {
    Slot& s = slots[static_cast<std::size_t>(index)];
    if (given == s.offered) return;
    s.offered = given;
    if (pending.size() == pending.capacity()) {
        Take(index, given);
        return;
    }
    // Kept in the order the wind arrives, so that pipes begin in that order
    // however the frames are asked for.
    const std::uint64_t due = now + s.delay;
    auto at = pending.end();
    while (at != pending.begin() && (at - 1)->due > due) --at;
    pending.insert(at, { index, due, given });
}

// The wind reaches the pipe. True if the pipe was silent and now begins: it
// has then joined the speaking pipes, and its wind is on. A pipe already
// speaking or ringing takes its new share at the next tick.
bool Engine::Take(int index, float given) {
    Slot& s = slots[static_cast<std::size_t>(index)];
    const bool starting = s.share == 0.0f && given > 0.0f;
    s.share = given;
    if (starting && !s.reed) flues[static_cast<std::size_t>(s.index)].SetTouch(touch[s.key]);
    if (s.active || !starting) return false;
    if (s.epoch != epoch) Design(index);
    const float wind = given * pressure[s.division] * robbed[s.division][s.key];
    if (s.reed) {
        ReedPipe& pipe = reeds[static_cast<std::size_t>(s.index)];
        pipe.Clear();
        pipe.wind = wind;
    } else {
        FluePipe& pipe = flues[static_cast<std::size_t>(s.index)];
        pipe.Clear();
        pipe.wind = wind;
    }
    Place(s);
    s.active = true;
    active.push_back(index);
    return true;
}

// A pipe's pitch against its regulated one, cents: the tuner's temperament
// and pitch, its own drift, and for a flue the air's temperature. A reed's
// tongue sets its pitch and is no warmer or cooler for the air; the small
// pull of its resonator, which is, is left out.
double Engine::Cents(const Slot& s) const {
    const int index = kRankFirst[s.rank] + s.key;
    const auto& temperament = kTemperaments[static_cast<std::size_t>(controls[kTemperament])];
    double cents = 1200.0 * std::log2(controls[kPitch] / 440.0) + temperament.cents[s.key % 12u]
        + controls[kTuning] * (s.reed ? 20.0 : 10.0) * Spread(index, kDrift);
    if (!s.reed) cents += 2.95 * controls[kTemperature];
    return cents;
}

double Engine::PipeHz(int rank, int key) const {
    if (rank < 0 || rank >= kRankCount || key < 0 || key >= kDivisionKeys[RankDivision(rank)]) return 0.0;
    const Slot& s = slots[static_cast<std::size_t>(kRankFirst[static_cast<std::size_t>(rank)] + key)];
    return s.index < 0 ? 0.0 : PlanPipe(rank, key).hz * std::pow(2.0, Cents(s) / 1200.0);
}

// Makes a pipe as the controls now have it: the builder's plan, the tuner's
// regulation, and the pipe's own share of each imperfection.
void Engine::Design(int index) {
    Slot& s = slots[static_cast<std::size_t>(index)];
    PipePlan plan = PlanPipe(s.rank, s.key);
    const PipeRegulated& regulated = kPipeRegulation[index];
    const double voicing = controls[kVoicing];
    const double speech = std::pow(2.0, 0.6 * voicing * Spread(index, kSpeech));
    const double noise = 1.0 + 0.5 * voicing * Spread(index, kNoise);
    double ratio = std::pow(2.0, Cents(s) / 1200.0);
    ratio = Max(ratio, kLowestRatio);
    if (s.reed) {
        plan.tongue.speech *= speech;
        plan.tongue.noise *= noise;
        ReedPipe& pipe = reeds[static_cast<std::size_t>(s.index)];
        pipe.Design(plan.tongue, plan.hz * ratio, regulated.tuning);
        s.air = static_cast<float>(pipe.air);
    } else {
        plan.flue.speech *= speech;
        plan.flue.noise *= noise;
        plan.flue.chiff *= 1.0 + 0.6 * voicing * Spread(index, kChiff);
        FluePipe& pipe = flues[static_cast<std::size_t>(s.index)];
        pipe.Design(plan.flue, plan.hz * ratio, regulated.tuning);
        s.air = static_cast<float>(pipe.air);
    }
    s.gain = kMaster * static_cast<float>(plan.level * std::pow(10.0, 3.0 * voicing * Spread(index, kLevel) / 20.0)) / regulated.rms;
    s.epoch = epoch;
}

void Engine::Place(Slot& s) {
    const float left = s.gain * placeLeft[s.division][s.key], right = s.gain * placeRight[s.division][s.key];
    if (s.reed) {
        reeds[static_cast<std::size_t>(s.index)].gainLeft = left;
        reeds[static_cast<std::size_t>(s.index)].gainRight = right;
    } else {
        flues[static_cast<std::size_t>(s.index)].gainLeft = left;
        flues[static_cast<std::size_t>(s.index)].gainRight = right;
    }
}

void Engine::Midi(const std::uint8_t* bytes, int size) {
    if (size < 3) return;
    const int status = bytes[0] & 0xF0, channel = bytes[0] & 0x0F;
    const int data = bytes[1] & 0x7F, value = bytes[2] & 0x7F;
    const int division = channel < kDivisions ? channel : kHauptwerk;
    if (status == 0x90 && value > 0) {
        KeyDown(division, data - kFirstNote, value);
    } else if (status == 0x80 || status == 0x90) {
        KeyUp(division, data - kFirstNote);
    } else if (status == 0xB0) {
        if (data == 1) SetCrescendo(value / 127.0);
        else if (data == 11) SetSwell(value / 127.0);
        else if (data == 120) AllSoundOff();
        else if (data == 123) AllKeysUp();
    }
}

void Engine::KeyDown(int division, int key, int velocity) {
    if (division < 0 || division >= kDivisions || key < 0 || key >= kDivisionKeys[division]) return;
    const auto d = static_cast<std::size_t>(division), k = static_cast<std::size_t>(key);
    if (played[d][k]) return;
    played[d][k] = true;
    // A hard stroke opens the pallet in half the time of an ordinary one, a
    // soft one in half as much again.
    const double stroke = Clamp(velocity, 1, 127) / 127.0;
    touch[k] = static_cast<float>(1.5 - stroke);
    if (controls[kAction] > 0.0)
        knocks.Strike(kKeyDown, kKnockLevel * controls[kAction] * (0.4 + stroke), controls[kWidth] * Stand(division, key));
    UpdateKey(key);
}

void Engine::KeyUp(int division, int key) {
    if (division < 0 || division >= kDivisions || key < 0 || key >= kDivisionKeys[division]) return;
    const auto d = static_cast<std::size_t>(division), k = static_cast<std::size_t>(key);
    if (!played[d][k]) return;
    played[d][k] = false;
    if (controls[kAction] > 0.0) knocks.Strike(kKeyUp, kKnockLevel * controls[kAction], controls[kWidth] * Stand(division, key));
    UpdateKey(key);
}

void Engine::AllKeysUp() {
    for (auto& division : played) division.fill(false);
    for (int key = 0; key < kManualKeys; ++key) UpdateKey(key);
}

void Engine::AllSoundOff() {
    AllKeysUp();
    for (auto& division : hang) division.fill(0.0f);
    for (int d = 0; d < kDivisions; ++d)
        for (int key = 0; key < kDivisionKeys[d]; ++key) UpdateChannel(d, key);
}

// Every kTick samples: the wind, each speaking pipe's share of it, and the
// pipes that have rung out.
void Engine::Tick() {
    const double live = 2.0 * controls[kWind], depth = 0.01 * controls[kTremulantDepth];
    for (std::size_t d = 0; d < kDivisions; ++d) {
        reservoirs[d].Step(demand[d] / fullWind[d]);
        unrestSlow[d] += kUnrestPole * (NextNoise(unrestRandom) - unrestSlow[d]);
        unrest[d] += kUnrestPole * (unrestSlow[d] - unrest[d]);
        double p = 1.0 + live * (reservoirs[d].pressure - 1.0 + kUnrest * unrest[d]);
        if (d == kPositiv) p *= tremulants[0].Step(controls[kTremulantPos] >= 0.5, depth);
        if (d == kRecit) p *= tremulants[1].Step(controls[kTremulantRec] >= 0.5, depth);
        pressure[d] = static_cast<float>(Max(p, 0.0));
        diag.pressure[d] = pressure[d];
        diag.demand[d] = demand[d] / fullWind[d];
        demand[d] = 0.0;
    }

    for (std::size_t i = 0; i < active.size();) {
        Slot& s = slots[static_cast<std::size_t>(active[i])];
        const float wind = s.share * pressure[s.division] * robbed[s.division][s.key];
        bool silent;
        if (s.reed) {
            ReedPipe& pipe = reeds[static_cast<std::size_t>(s.index)];
            pipe.wind = wind;
            pipe.Tick();
            demand[s.division] += pipe.air * Sqrt(static_cast<double>(pipe.foot));
            silent = pipe.Silent();
        } else {
            FluePipe& pipe = flues[static_cast<std::size_t>(s.index)];
            pipe.wind = wind;
            pipe.Tick();
            demand[s.division] += pipe.air * Sqrt(static_cast<double>(pipe.foot));
            silent = pipe.Silent();
        }
        if (silent) {
            s.active = false;
            active[i] = active.back();
            active.pop_back();
        } else {
            ++i;
        }
    }
    const std::size_t speaking = active.size();
    diag.pipes = static_cast<int>(speaking);

    // The blower is heard while the organ is played and a little after.
    presence += (speaking > 0 ? kPresenceRise * (1.0f - presence) : -kPresenceFall * presence);
    if (speaking == 0 && presence < 1.0e-4f) presence = 0.0f;
    blower = static_cast<float>(kBlowerLevel * controls[kBlower]) * presence;
    for (float& low : boxLow)
        if (Abs(low) < 1.0e-20f) low = 0.0f;

    // Pipes made for an earlier tuning are remade a few at a time while
    // they are silent; one that is asked to speak first is remade then.
    for (int made = 0, seen = 0; stale > 0 && made < 2 && seen < 256; ++seen, --stale) {
        Slot& s = slots[static_cast<std::size_t>(redesignAt)];
        if (s.index >= 0 && !s.active && s.epoch != epoch) {
            Design(redesignAt);
            ++made;
        }
        if (++redesignAt == kPipeCount) redesignAt = 0;
    }
}

void Engine::RenderChunk(float* left, float* right, int frames) {
    for (auto& division : bus)
        for (auto& channel : division)
            for (int i = 0; i < frames; ++i) channel[static_cast<std::size_t>(i)] = 0.0f;
    // The pipes already speaking, then those whose wind arrives within
    // these frames, each from its own sample on.
    const auto process = [&](const Slot& s, int from) {
        float* const busLeft = bus[s.division][0].data() + from;
        float* const busRight = bus[s.division][1].data() + from;
        if (s.reed) reeds[static_cast<std::size_t>(s.index)].Process(busLeft, busRight, frames - from);
        else flues[static_cast<std::size_t>(s.index)].Process(busLeft, busRight, frames - from);
    };
    for (std::size_t i = 0, count = active.size(); i < count; ++i) process(slots[static_cast<std::size_t>(active[i])], 0);
    std::size_t arrived = 0;
    for (; arrived < pending.size() && pending[arrived].due < now + static_cast<std::uint64_t>(frames); ++arrived) {
        const Pending arriving = pending[arrived];
        if (!Take(arriving.slot, arriving.share)) continue;
        process(slots[static_cast<std::size_t>(arriving.slot)], static_cast<int>(arriving.due - now));
    }
    pending.erase(pending.begin(), pending.begin() + static_cast<std::ptrdiff_t>(arrived));
    now += static_cast<std::uint64_t>(frames);
    for (int i = 0; i < frames; ++i) {
        const auto at = static_cast<std::size_t>(i);
        float sound[2];
        boxLowGain += kGainStep * (boxLowTarget - boxLowGain);
        boxHighGain += kGainStep * (boxHighTarget - boxHighGain);
        for (std::size_t c = 0; c < 2; ++c) {
            const float enclosed = bus[kRecit][c][at];
            boxLow[c] += kBoxPole * (enclosed - boxLow[c]);
            sound[c] = bus[kHauptwerk][c][at] + bus[kPositiv][c][at] + bus[kPedal][c][at]
                + boxLowGain * boxLow[c] + boxHighGain * (enclosed - boxLow[c]);
        }
        knocks.Process(sound[0], sound[1]);
        if (blower > 0.0f) windNoise.Process(blower, sound[0], sound[1]);
        float roomLeft, roomRight;
        nave.Process(sound[0], sound[1], roomLeft, roomRight);
        dry += kGainStep * (dryTarget - dry);
        wet += kGainStep * (wetTarget - wet);
        std::array<float, 2> frame { dry * sound[0] + wet * roomLeft, dry * sound[1] + wet * roomRight };
        output.Process(frame);
        left[i] = frame[0];
        right[i] = frame[1];
    }
}

void Engine::Render(float* left, float* right, int frames) {
    while (frames > 0) {
        if (untilTick == 0) {
            Tick();
            untilTick = kChunk;
        }
        const int chunk = Min(frames, untilTick);
        RenderChunk(left, right, chunk);
        left += chunk;
        right += chunk;
        frames -= chunk;
        untilTick -= chunk;
    }
}

} // namespace oregana
