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
// And each division at its own depth in the case: the Positiv at the front,
// the Hauptwerk behind the front pipes, the Pedal in its towers and the
// Recit's box at the back. Estimates of a case some three metres deep, kept
// small for the Hauptwerk, which a player hears his own keys through.
constexpr double kDivisionMetres[kDivisions] = { 0.5, 0.0, 3.0, 1.5 };
// And at its own height against the Hauptwerk: the Positiv below it at the
// gallery's rail, the Recit's box above and behind. Estimates too. The
// church reflects each division from where it stands (Nave::Stand); its wall
// is 2 m behind the Hauptwerk, so the Recit's box is taken to stand against it.
constexpr double kDivisionHigher[kDivisions] = { 0.0, -2.0, 2.5, 0.0 };

// Neighbouring flue pipes hear each other (Mitnahme). The pipes of one key
// stand along its channel, rank behind rank, kRowMetres apart and no nearer
// than they can stand (their radii and kClearance). Each hears the nearest
// speaking flue in front and behind, up to two rows off, by FluePipe's
// `telling` over the distance, as late as sound takes to cross it. Close and
// nearly in tune they lock to one pitch in antiphase and nearly cancel, and
// near that they pull each other. The strength is derived and not fitted:
// two of these pipes alone, at 740 Hz, locked within 2.8 Hz either way at
// 5 cm, 1.7 Hz at 10 cm and 0.4 Hz at 20 and 40 cm, where 6 Hz across at the
// nearest and "marginal" at 40 cm were measured on pipes of 720 Hz (Fischer,
// in Sawicki, Abel and Schoell, arXiv:1708.06571); in the organ the Octave
// and the Gamba of one key lock within about 3 cents (PipesHearEachOther).
// Antiphase is what Abel, Bergweiler and Gerhard-Multhaupt measured side by
// side (JASA 119, 2006): that measurement sets the sign. Assumed: both pipes' waves are as strong
// (they stand on one wind), the neighbour's pressure enters the mouth as a
// wave of its own size, reeds neither hear nor are heard (their tongues hold
// their pitch), and a beating rank stands apart, as builders place one so
// that it is not drawn into tune.
// A locked pair is heard as two sources a row apart, so each row's sound is
// that much later than the row in front: the rows fold back every fourth so
// that the row buses stay short, which keeps a row's step to its neighbours.
constexpr double kClearance = 0.02;
constexpr int kHearRows = 2;
constexpr int kLongestHearing = 48;    // samples: a ring's length less a hearing block

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
constexpr int kKeyOfC = kOrganC - kFirstNote;
// The pressure's slow unrest: 0.3 % RMS as modelled (Wind at 0.5), noise
// through two one-poles at 1 Hz, whose output is 0.0186 RMS.
constexpr double kUnrest = 0.16, kUnrestPole = 0.0042;
// A channel's pipes share one pallet: with every stop drawn each gets 3 %
// less pressure than a stop alone, falling with the square of the wind taken.
constexpr float kRobbing = 0.03f;

// The wind's supply. The blower fills a reservoir whose weights hold the
// pressure for as long as there is wind in it; the pipes and the leaks take
// it out again. With the blower switched off the organ plays on what is in
// store, a second and a half of full organ or some ten seconds of one stop,
// and then the plate comes down on its last quarter and the pressure with it:
// every pipe goes flat and weakens, the flues stop at a little over half the
// pressure and the reeds last. Switched on
// again, the blower takes about a second to come up to speed and fill it.
// On the bellows a calcant treads: the wind is always there, its pressure
// falls a little as each bellows sinks (the more the livelier the wind), and
// when one is nearly down the next takes over with a thud and a dip the
// reservoirs bounce on. He treads only while the organ is played. All of it
// is estimated from accounts of such winding, none of it measured.
constexpr double kSpinUp = kStepSeconds / 1.2, kSpinDown = kStepSeconds / 1.5;
constexpr double kFill = 3.0 * kStepSeconds, kLeak = kStepSeconds / 12.0, kDrain = kStepSeconds / 1.5;
constexpr double kKnee = 0.25;
constexpr double kBellowsLeak = 0.02 * kStepSeconds, kBellowsDrain = 0.117 * kStepSeconds;
constexpr double kChangeover = 0.3, kWedge = 0.04, kChangeDip = 0.03;

// A stop's slider takes 0.15 s to cross, each a little more or less. Its
// holes over the pipes' feet are an orifice in series with each foot's own,
// twice its area when the stop is full out: the foot gets 1 / (1 + (r / x)^2)
// of the channel's pressure with the slider x of the way, against 1 / (1 + r^2)
// full out, r = 0.5. So a stop half drawn leaves its pipes 0.63 of the wind.
constexpr double kSliderSeconds = 0.15;
constexpr float kSliderNarrow = 0.5f;

// The crescendo roller's whole travel takes 0.8 s however fast the wheel is
// thrown, and a stage it has added goes off again 0.4 of a stage lower than
// it came on: a wheel resting on a stage's edge does not chatter.
constexpr double kRollerStep = kStepSeconds / 0.8, kStageBack = 0.4;

// A coupler's lost motion at Action 1: the key it pulls starts 16 ms behind
// an ordinary stroke, half as much to half as much again from key to key,
// and in step with how slowly the key is pressed (an estimate of a worn
// tracker action; a tight one has a millisecond or two).
constexpr double kCouplerSeconds = 0.016;

// The swell box: shut, its walls and shutters take 8 dB of the bass and 24 dB
// of the treble, split at 500 Hz (an estimate of a well-built box, not a
// measurement); the shutters move with the pedal, in decibels.
constexpr float kBoxPole = 0.0634f;
constexpr double kBoxLowDb = -8.0, kBoxHighDb = -24.0;
constexpr float kGainStep = 0.001f;   // output gains glide in about 20 ms

// The noises at Action and Blower 1: a key's knock peaks near -38 dBFS and
// the wind noise sits at -50 dBFS RMS, at the pipes (estimates).
constexpr double kKnockLevel = 0.012, kBlowerLevel = 0.22;
// A slider's rub is at its loudest with three or more stops moving, quiet
// beside their knobs' knocks, and the shutters knock as a thrown pedal lands
// them: a tenth of the travel in one step is a full knock (estimates).
constexpr double kRubLevel = 1.0, kShutterThrow = 0.1;
constexpr float kPresenceRise = 0.00222f, kPresenceFall = 0.000333f;   // 0.3 s and 2 s

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
enum What { kDrift = 1, kLevel, kSpeech, kChiff, kNoise, kDead, kCipher, kCipherLeak, kRunning, kRunningLeak, kOffRow, kSlack, kSlider };
// A pipe's and a key's place for those: by rank or division and note, so that
// neither changes its character if the compass ever does.
int PipePlace(int rank, int key) { return rank * 128 + kFirstNote + key; }
int KeyPlace(int division, int key) { return division * 128 + kFirstNote + key; }

// Where a pipe stands across the organ's front, -1 left .. 1 right: the keys
// alternate between the two sides of a chest, the largest pipes outermost.
// The Hauptwerk fills the case, the Positiv and the Recit its middle, and the
// Pedal stands in the two towers.
double Stand(int division, int key) {
    const int note = kFirstNote + key;
    const double side = (note & 1) ? 1.0 : -1.0;
    // The keys beyond an organ's own compass stand with its outermost.
    const double bass = 1.0 - Clamp((note - kOrganC) / (division == kPedal ? 31.0 : 60.0), 0.0, 1.0);
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
    static_assert(Nave::kSources == kDivisions, "the church hears each division apart");
    for (int d = 0; d < kDivisions; ++d) nave.Stand(d, kDivisionMetres[kHauptwerk] - kDivisionMetres[d], kDivisionHigher[d]);
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
            s.row = static_cast<std::uint8_t>(rank - kDivisionRank[static_cast<std::size_t>(division)]);
            constexpr int fold[4] = { 0, 1, 2, 1 };
            s.outDelay = static_cast<std::uint8_t>(plan.reed ? 0 : kRowSamples * fold[s.row & 3]);
            s.apart = kRanks[rank].cents != 0.0f;
            s.radius = static_cast<float>(0.5 * PipeDiameter(plan.flue.scale, plan.hz));
            const double metres = kDivisionMetres[division] + kRowMetres * (rank - kDivisionRank[static_cast<std::size_t>(division)])
                + kOffRowMetres * Unit(PipePlace(rank, key), kOffRow);
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
    scratch.assign(static_cast<std::size_t>(kPipeCount) * kChunk, 0.0f);
    hearScratch.assign(static_cast<std::size_t>(kPipeCount) * kHearBlock, 0.0f);
    toldRing.assign(flues.size() * kRingSize, 0.0f);
    for (auto& list : flueList) list.reserve(flues.size() + kSpares);
    for (auto& list : flueSound) list.reserve(flues.size() + kSpares);
    for (auto& list : flueAt) list.reserve(flues.size() + kSpares);
    reedList.reserve(reeds.size() + kSpares);
    reedSound.reserve(reeds.size() + kSpares);
    {
        FlueVoicing plain, chimneyed;
        chimneyed.stopped = true;
        chimneyed.chimney = 0.3;
        std::size_t spareRoom = 0;
        for (std::size_t upper = 0; upper < 2; ++upper)
            for (FluePipe& pipe : spareFlue[upper]) {
                pipe.Design(upper ? chimneyed : plain, 440.0, 1.0);
                spareRoom += static_cast<std::size_t>(pipe.loopLength + pipe.jetLength + pipe.upperLength);
            }
        for (ReedPipe& pipe : spareReed) {
            pipe.Design(ReedVoicing {}, 440.0, 1.0);
            spareRoom += static_cast<std::size_t>(pipe.loopLength);
        }
        spareArena.assign(spareRoom, 0.0f);
        float* spot = spareArena.data();
        for (auto& kind : spareFlue)
            for (FluePipe& pipe : kind) {
                pipe.loop = spot;
                pipe.jet = spot + pipe.loopLength;
                pipe.upper = pipe.jet + pipe.jetLength;
                spot = pipe.upper + pipe.upperLength;
                pipe.Clear();
            }
        for (ReedPipe& pipe : spareReed) {
            pipe.loop = spot;
            spot += pipe.loopLength;
            pipe.Clear();
        }
    }
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
        if (d == kPedal) for (int key : kFullPedal) fullWind[static_cast<std::size_t>(d)] += fullAir[static_cast<std::size_t>(d)][static_cast<std::size_t>(kKeyOfC + key)];
        else for (int key : kFullChord) fullWind[static_cast<std::size_t>(d)] += fullAir[static_cast<std::size_t>(d)][static_cast<std::size_t>(kKeyOfC + key)];
        reservoirs[static_cast<std::size_t>(d)].Design(kWinding[d], kStepSeconds);
    }
    for (int i = 0; i < kStopCount; ++i)
        sliderStep[static_cast<std::size_t>(i)] = static_cast<float>(kStepSeconds / (kSliderSeconds * (0.7 + 0.6 * Unit(i, kSlider))));
    output.Prepare(kPipeRate);
    Apply(controls, true);
    Reset();
}

void Engine::Reset() {
    for (std::size_t i = 0; i < slots.size(); ++i) {
        Slot& s = slots[i];
        s.active = s.resting = s.coupled = false;
        s.offered = s.share = 0.0f;
        s.lastDue = 0;
        if (s.index < 0) continue;
        // Each pipe's turbulence is its own.
        const std::uint32_t seed = Hash(static_cast<std::uint32_t>(PipePlace(s.rank, s.key)) + 0x51ED270Bu) | 1u;
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
        for (auto& rows : rowBus[d])
            for (auto& channel : rows) channel.fill(0.0f);
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
    presence = hum = hiss = rub = 0.0f;
    knocks.Clear();
    windNoise.Clear();
    nave.Clear();
    output.Reset();
    diag = Diagnostics {};
    for (double& p : diag.pressure) p = 1.0;

    // The organ as the controls have it, and as if they had for a while: the
    // roller where it is turned to, the sliders across, the wind up or gone.
    roller = crescendo;
    stage = static_cast<int>(roller * kCrescendoStages);
    diag.stage = stage;
    Register();
    for (int i = 0; i < kStopCount; ++i) SetSlider(i, sliderTarget[static_cast<std::size_t>(i)]);
    const int supply = static_cast<int>(controls[kWindSupply]);
    spin = supply == kWindBlower ? 1.0 : 0.0;
    stock = supply == kWindOff ? 0.0 : 1.0;
    diag.supply = stock;
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

    bool registration = all || changed(kDeadPipes) || changed(kRunnings) || changed(kCiphers);
    int knobs = 0;
    for (int i = 0; i < kStopCount + kCouplerCount; ++i) {
        if (!changed(i)) continue;
        registration = true;
        // A knob knocks as it is drawn or put in, not while it is held part
        // of the way. A combination's knobs go together: three knocks tell
        // as many.
        if (!all && (controls[i] >= 0.5) != (before[i] >= 0.5) && controls[kAction] > 0.0 && knobs++ < 3)
            knocks.Strike(kStopKnob, kKnockLevel * controls[kAction], 0.0);
    }
    if (changed(kCrescendo)) crescendo = controls[kCrescendo];
    if (changed(kSwell)) SetSwell(controls[kSwell]);
    if (!all && changed(kCiphers)) {
        // A pallet that is no longer one to hang closes.
        for (int d = 0; d < kDivisions; ++d)
            for (int key = 0; key < kDivisionKeys[d]; ++key)
                if (Unit(KeyPlace(d, key), kCipher) >= kCipherShare * controls[kCiphers])
                    hang[static_cast<std::size_t>(d)][static_cast<std::size_t>(key)] = 0.0f;
    }

    if (changed(kTremulantRate))
        for (Tremulant& t : tremulants) t.Design(controls[kTremulantRate], kStepSeconds);
    if (changed(kDecay) || changed(kDecayBass) || changed(kDecayTreble) || changed(kRoomSize) || changed(kWidth))
        nave.Set(controls[kDecay], controls[kDecayBass], controls[kDecayTreble], controls[kRoomSize], controls[kWidth]);
    if (changed(kWidth)) {
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
    const double before = swell;
    swell = position == position ? Clamp(position, 0.0, 1.0) : 1.0;
    // The shutters meet their frame at either end of the pedal's travel, as
    // hard as the pedal was moving.
    if (swell != before && (swell == 0.0 || swell == 1.0) && controls[kAction] > 0.0)
        knocks.Strike(kShutters, kKnockLevel * controls[kAction] * Min(1.0, Abs(swell - before) / kShutterThrow), 0.0);
    boxLowTarget = static_cast<float>(std::pow(10.0, kBoxLowDb * (1.0 - swell) / 20.0));
    boxHighTarget = static_cast<float>(std::pow(10.0, kBoxHighDb * (1.0 - swell) / 20.0));
}

void Engine::SetCrescendo(double position) {
    crescendo = position == position ? Clamp(position, 0.0, 1.0) : 0.0;
}

// What the knobs and the roller want drawn, and what every key then gives.
// The sliders follow in their own time (Tick).
void Engine::Register() {
    for (int i = 0; i < kStopCount; ++i) sliderTarget[static_cast<std::size_t>(i)] = static_cast<float>(controls[i]);
    for (int i = 0; i < kCouplerCount; ++i) coupled[static_cast<std::size_t>(i)] = controls[kCouplerPosHw + i] >= 0.5;
    for (int i = 0; i < stage; ++i) {
        if (kCrescendoOrder[i] < kStopCount) sliderTarget[static_cast<std::size_t>(kCrescendoOrder[i])] = 1.0f;
        else coupled[static_cast<std::size_t>(kCrescendoOrder[i] - kCouplerPosHw)] = true;
    }
    for (int key = 0; key < kManualKeys; ++key) UpdateKey(key);
}

// A stop's slider at `position`: the wind it leaves its pipes, and, as it
// opens or shuts, which pipes of the open channels take wind at all.
void Engine::SetSlider(int stop, float position) {
    const auto i = static_cast<std::size_t>(stop);
    const bool was = slider[i] > 0.0f, is = position > 0.0f;
    slider[i] = position;
    const float narrow = is ? kSliderNarrow / position : 0.0f;
    const float through = is ? (1.0f + kSliderNarrow * kSliderNarrow) / (1.0f + narrow * narrow) : 0.0f;
    for (int r = 0; r < kStops[stop].ranks; ++r) {
        opening[static_cast<std::size_t>(kStops[stop].firstRank + r)] = through;
        rankOpen[static_cast<std::size_t>(kStops[stop].firstRank + r)] = is;
    }
    if (was != is)
        for (int key = 0; key < kDivisionKeys[kStops[stop].division]; ++key) UpdateChannel(kStops[stop].division, key);
}

// Which pallets key `key` holds open across the divisions: its own, and
// through the couplers, which pull one another's keys as a tracker action's
// do. A key just `struck` pulls the coupled ones late, by each coupler's lost
// motion on the way. `own` is the division whose key moved, whose knock
// the caller has struck.
void Engine::UpdateKey(int key, bool struck, int own) {
    const auto k = static_cast<std::size_t>(key);
    const bool pedal = key < kPedalKeys && played[kPedal][k];
    bool pulled[kDivisions];
    pulled[kHauptwerk] = played[kHauptwerk][k] || (coupled[3] && pedal);
    pulled[kPositiv] = played[kPositiv][k] || (coupled[0] && pulled[kHauptwerk]) || (coupled[4] && pedal);
    pulled[kRecit] = played[kRecit][k] || (coupled[1] && pulled[kHauptwerk]) || (coupled[2] && pulled[kPositiv]) || (coupled[5] && pedal);
    pulled[kPedal] = pedal;

    int late[kDivisions] = {};
    if (struck && controls[kAction] > 0.0) {
        const auto lost = [&](int coupler) {
            return static_cast<int>(kCouplerSeconds * kPipeRate * controls[kAction]
                                    * (0.5 + Unit(coupler * 128 + kFirstNote + key, kSlack)) * touch[k]);
        };
        // By the quickest way the pull comes: a key's own division is not late.
        const int never = 1 << 20;
        const auto soonest = [&](int division, std::initializer_list<int> ways) {
            int least = played[static_cast<std::size_t>(division)][k] ? 0 : never;
            for (int way : ways) least = Min(least, way);
            late[division] = least == never ? 0 : least;
        };
        soonest(kHauptwerk, { coupled[3] && pedal ? lost(3) : never });
        soonest(kPositiv, { coupled[0] && pulled[kHauptwerk] ? late[kHauptwerk] + lost(0) : never, coupled[4] && pedal ? lost(4) : never });
        soonest(kRecit, { coupled[1] && pulled[kHauptwerk] ? late[kHauptwerk] + lost(1) : never,
                          coupled[2] && pulled[kPositiv] ? late[kPositiv] + lost(2) : never, coupled[5] && pedal ? lost(5) : never });
    }

    bool opened[kDivisions] = {};
    for (int d = 0; d < kDivisions; ++d) {
        if (key >= kDivisionKeys[d] || pulled[d] == open[static_cast<std::size_t>(d)][k]) continue;
        open[static_cast<std::size_t>(d)][k] = opened[d] = pulled[d];
        // A pallet that hangs is pulled free by its key, and hangs again
        // when the key lets it go.
        const int place = KeyPlace(d, key);
        const bool hangs = !pulled[d] && Unit(place, kCipher) < kCipherShare * controls[kCiphers];
        hang[static_cast<std::size_t>(d)][k] = hangs ? static_cast<float>(0.35 + 0.55 * Unit(place, kCipherLeak)) : 0.0f;
        // A pallet a coupler moves knocks at its own chest, when the lost
        // motion is taken up; the key's own is in the key's knock.
        if (d != own && controls[kAction] > 0.0)
            knocks.Strike(pulled[d] ? kPalletPulled : kPalletShut, kKnockLevel * controls[kAction] * (pulled[d] ? 1.9 - touch[k] : 1.0),
                          controls[kWidth] * Stand(d, key), late[d]);
    }
    for (int d = 0; d < kDivisions; ++d)
        for (int near : { key - 2, key, key + 2 })
            if (near >= 0 && near < kDivisionKeys[d]) UpdateChannel(d, near, near == key && opened[d] ? late[d] : 0);
}

// The wind one key's channel holds, and which of its pipes take it.
void Engine::UpdateChannel(int division, int key, int late) {
    const auto d = static_cast<std::size_t>(division), k = static_cast<std::size_t>(key);
    float wind = open[d][k] ? 1.0f : hang[d][k];
    if (!open[d][k] && controls[kRunnings] > 0.0) {
        for (int near : { key - 2, key + 2 }) {
            if (near < 0 || near >= kDivisionKeys[division] || !open[d][static_cast<std::size_t>(near)]) continue;
            const int place = KeyPlace(division, Min(key, near));
            if (Unit(place, kRunning) < kRunningShare * controls[kRunnings])
                wind = Max(wind, static_cast<float>(0.35 + 0.3 * Unit(place, kRunningLeak)));
        }
    }
    share[d][k] = wind;

    const double dead = kDeadShare * controls[kDeadPipes];
    const int firstRank = kDivisionRank[d], endRank = kDivisionRank[d + 1];
    const auto speaks = [&](int rank, int index) {
        return rankOpen[static_cast<std::size_t>(rank)] && slots[static_cast<std::size_t>(index)].index >= 0
            && !(dead > 0.0 && Unit(PipePlace(rank, key), kDead) < dead);
    };
    float taken = 0.0f;
    for (int rank = firstRank; rank < endRank; ++rank) {
        const int index = kRankFirst[static_cast<std::size_t>(rank)] + key;
        if (!speaks(rank, index)) continue;
        // A silent pipe made for an earlier tuning is remade here and not
        // when its wind arrives: the wind it takes is part of the load.
        Slot& s = slots[static_cast<std::size_t>(index)];
        if (!s.active && s.epoch != epoch) Design(index);
        taken += s.air;
    }
    const float load = taken / fullAir[d][k];
    robbed[d][k] = 1.0f / (1.0f + kRobbing * load * load);
    for (int rank = firstRank; rank < endRank; ++rank) {
        const int index = kRankFirst[static_cast<std::size_t>(rank)] + key;
        if (slots[static_cast<std::size_t>(index)].index >= 0) Offer(index, speaks(rank, index) ? wind : 0.0f, late);
    }
}

// A pallet gives a pipe its share of the wind, or takes it away: heard when
// the sound has come from where the pipe stands, `late` samples later for a
// pallet a coupler pulls.
void Engine::Offer(int index, float given, int late) {
    Slot& s = slots[static_cast<std::size_t>(index)];
    if (given == s.offered) return;
    s.offered = given;
    if (pending.size() == pending.capacity()) {
        Take(index, given);
        return;
    }
    // Kept in the order the wind arrives, so that pipes begin in that order
    // however the frames are asked for.
    // Wind is never taken away before it was given: a key let go within its
    // coupler's lost motion would otherwise leave the pipe sounding.
    const std::uint64_t due = Max(now + s.delay + static_cast<std::uint64_t>(late), s.lastDue);
    s.lastDue = due;
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
    // Made first: a pipe's making sets its speech for an ordinary touch.
    if (starting && !s.active && s.epoch != epoch) Design(index);
    if (starting && !s.reed) flues[static_cast<std::size_t>(s.index)].SetTouch(touch[s.key]);
    if (s.active || !starting) return false;
    const float wind = given * opening[s.rank] * pressure[s.division] * robbed[s.division][s.key];
    if (s.reed) {
        ReedPipe& pipe = reeds[static_cast<std::size_t>(s.index)];
        pipe.Clear();
        pipe.wind = wind;
    } else {
        FluePipe& pipe = flues[static_cast<std::size_t>(s.index)];
        pipe.Clear();
        pipe.wind = wind;
        s.coupled = false;
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
    const auto& temperament = kTemperaments[static_cast<std::size_t>(controls[kTemperament])];
    double cents = 1200.0 * std::log2(controls[kPitch] / 440.0) + temperament.cents[static_cast<unsigned>(kFirstNote + s.key) % 12u]
        + controls[kTuning] * (s.reed ? 20.0 : 10.0) * Spread(PipePlace(s.rank, s.key), kDrift);
    if (!s.reed) cents += 2.95 * controls[kTemperature];
    return cents;
}

double Engine::PipeHz(int rank, int note) const {
    const int key = note - kFirstNote;
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
    const int place = PipePlace(s.rank, s.key);
    const double voicing = controls[kVoicing];
    const double speech = std::pow(2.0, 0.6 * voicing * Spread(place, kSpeech));
    const double noise = 1.0 + 0.5 * voicing * Spread(place, kNoise);
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
        plan.flue.chiff *= 1.0 + 0.6 * voicing * Spread(place, kChiff);
        FluePipe& pipe = flues[static_cast<std::size_t>(s.index)];
        pipe.Design(plan.flue, plan.hz * ratio, regulated.tuning);
        s.air = static_cast<float>(pipe.air);
    }
    s.gain = kMaster * static_cast<float>(plan.level * std::pow(10.0, 3.0 * voicing * Spread(place, kLevel) / 20.0)) / regulated.rms;
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
        KeyDown(division, data, value);
    } else if (status == 0x80 || status == 0x90) {
        KeyUp(division, data);
    } else if (status == 0xB0) {
        if (data == 1) SetCrescendo(value / 127.0);
        else if (data == 11) SetSwell(value / 127.0);
        else if (data == 120) AllSoundOff();
        else if (data == 123) AllKeysUp();
    }
}

void Engine::KeyDown(int division, int note, int velocity) {
    const int key = note - kFirstNote;
    if (division < 0 || division >= kDivisions || key < 0 || key >= kDivisionKeys[division]) return;
    const auto d = static_cast<std::size_t>(division), k = static_cast<std::size_t>(key);
    if (played[d][k]) return;
    played[d][k] = true;
    // A hard stroke opens the pallet in half the time of an ordinary one, a
    // soft one in half as much again.
    const double stroke = Clamp(velocity, 1, 127) / 127.0;
    touch[k] = static_cast<float>(1.5 - stroke);
    if (controls[kAction] > 0.0)
        knocks.Strike(division == kPedal ? kPedalDown : kKeyDown, kKnockLevel * controls[kAction] * (0.4 + stroke),
                      controls[kWidth] * Stand(division, key));
    UpdateKey(key, true, division);
}

void Engine::KeyUp(int division, int note) {
    const int key = note - kFirstNote;
    if (division < 0 || division >= kDivisions || key < 0 || key >= kDivisionKeys[division]) return;
    const auto d = static_cast<std::size_t>(division), k = static_cast<std::size_t>(key);
    if (!played[d][k]) return;
    played[d][k] = false;
    if (controls[kAction] > 0.0)
        knocks.Strike(division == kPedal ? kPedalUp : kKeyUp, kKnockLevel * controls[kAction], controls[kWidth] * Stand(division, key));
    UpdateKey(key, false, division);
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

// Every kTick samples: the roller and the sliders, the wind, each speaking
// pipe's share of it, and the pipes that have rung out.
void Engine::Tick() {
    if (roller != crescendo) {
        const double gap = crescendo - roller;
        roller = Abs(gap) <= kRollerStep ? crescendo : roller + (gap > 0.0 ? kRollerStep : -kRollerStep);
        const double at = roller * kCrescendoStages;
        int next = stage;
        while (next < kCrescendoStages && at >= next + 1) ++next;
        while (next > 0 && at < next - kStageBack) --next;
        if (next != stage) {
            stage = diag.stage = next;
            Register();
        }
    }
    int moving = 0;
    for (int i = 0; i < kStopCount; ++i) {
        const float at = slider[static_cast<std::size_t>(i)], target = sliderTarget[static_cast<std::size_t>(i)];
        const float step = sliderStep[static_cast<std::size_t>(i)];
        if (at == target) continue;
        SetSlider(i, at < target ? Min(at + step, target) : Max(at - step, target));
        ++moving;
    }
    rub = static_cast<float>(kRubLevel * controls[kAction] * Min(moving, 3) / 3.0);

    const double live = 2.0 * controls[kWind], depth = 0.01 * controls[kTremulantDepth];

    // The supply (kSpinUp and below): what it holds for every division.
    const int supply = static_cast<int>(controls[kWindSupply]);
    double taken = 0.0, full = 0.0;
    for (std::size_t d = 0; d < kDivisions; ++d) {
        taken += demand[d];
        full += fullWind[d];
    }
    const double load = taken / full;
    spin += supply == kWindBlower ? kSpinUp * (1.0 - spin) : -kSpinDown * spin;
    if (spin < 1.0e-6) spin = 0.0;
    if (supply == kWindBellows) {
        if (load > 0.0) stock -= kBellowsLeak + kBellowsDrain * load;
        if (stock < kChangeover) {
            stock = 1.0;
            // The fresh bellows starts where the spent one's droop left the
            // wind, less the dip, and the reservoirs bring it back.
            for (Reservoir& reservoir : reservoirs) reservoir.pressure -= kWedge * (1.0 - kChangeover) + kChangeDip;
            if (controls[kBlower] > 0.0) knocks.Strike(kBellowsTurn, kKnockLevel * controls[kBlower], 0.0);
        }
    } else {
        stock = Clamp(stock + kFill * spin * spin * (1.0 - stock) - kLeak - kDrain * load, 0.0, 1.0);
    }
    double head = Min(1.0, stock / kKnee);
    if (supply == kWindBellows) head *= 1.0 - kWedge * live * (1.0 - stock);
    head *= controls[kPressure];
    diag.supply = head;

    for (std::size_t d = 0; d < kDivisions; ++d) {
        reservoirs[d].Step(demand[d] / fullWind[d]);
        unrestSlow[d] += kUnrestPole * (NextNoise(unrestRandom) - unrestSlow[d]);
        unrest[d] += kUnrestPole * (unrestSlow[d] - unrest[d]);
        double p = head * (1.0 + live * (reservoirs[d].pressure - 1.0 + kUnrest * unrest[d]));
        if (d == kPositiv) p *= tremulants[0].Step(controls[kTremulantPos] >= 0.5, depth);
        if (d == kRecit) p *= tremulants[1].Step(controls[kTremulantRec] >= 0.5, depth);
        // The valve's thump, from the middle of its division: as loud as the
        // beat is deep, a full one at 10 %, and heard with the wind's noise,
        // while the organ is played and a little after.
        if (d == kPositiv || d == kRecit) {
            const Tremulant& t = tremulants[d == kPositiv ? 0 : 1];
            if (t.beat && controls[kBlower] > 0.0 && presence > 0.0f)
                knocks.Strike(kTremulantBeat, kKnockLevel * controls[kBlower] * presence * t.running * Min(1.0, 10.0 * depth), 0.0);
        }
        pressure[d] = static_cast<float>(Max(p, 0.0));
        diag.pressure[d] = pressure[d];
        diag.demand[d] = demand[d] / fullWind[d];
        demand[d] = 0.0;
    }

    std::size_t speaking = 0;
    for (std::size_t i = 0; i < active.size();) {
        Slot& s = slots[static_cast<std::size_t>(active[i])];
        const float wind = s.share * opening[s.rank] * pressure[s.division] * robbed[s.division][s.key];
        bool silent = s.resting && wind == 0.0f;
        if (!silent) {
            s.resting = false;
            if (s.reed) {
                ReedPipe& pipe = reeds[static_cast<std::size_t>(s.index)];
                pipe.wind = wind;
                pipe.Tick();
                demand[s.division] += pipe.air * Sqrt(static_cast<double>(pipe.foot));
                silent = pipe.Silent();
                if (silent && s.share > 0.0f) pipe.Clear();
            } else {
                FluePipe& pipe = flues[static_cast<std::size_t>(s.index)];
                pipe.wind = wind;
                pipe.Tick();
                demand[s.division] += pipe.air * Sqrt(static_cast<double>(pipe.foot));
                silent = pipe.Silent();
                if (silent && s.share > 0.0f) pipe.Clear();
            }
        }
        // A silent pipe whose key is still held (the wind has gone) waits
        // for it, cleared; any other has rung out.
        s.resting = silent && s.share > 0.0f;
        if (silent && !s.resting) {
            s.active = false;
            active[i] = active.back();
            active.pop_back();
        } else {
            if (!s.resting) ++speaking;
            ++i;
        }
    }
    diag.pipes = static_cast<int>(speaking);

    // Who hears whom, now that the speaking pipes are known.
    for (const int index : active) {
        Slot& s = slots[static_cast<std::size_t>(index)];
        if (s.reed) continue;
        const bool was = s.coupled;
        s.coupled = false;
        s.hears[0] = s.hears[1] = -1;
        // A pipe without wind has no jet to be drawn along: it rings out
        // alone, and comes to rest.
        if (s.resting || s.apart || flues[static_cast<std::size_t>(s.index)].wind <= 0.0f) continue;
        const int firstRank = kDivisionRank[s.division], endRank = kDivisionRank[static_cast<std::size_t>(s.division) + 1];
        for (int side = 0; side < 2; ++side) {
            for (int rows = 1; rows <= kHearRows; ++rows) {
                const int rank = s.rank + (side == 0 ? -rows : rows);
                if (rank < firstRank || rank >= endRank) break;
                const Slot& near = slots[static_cast<std::size_t>(kRankFirst[static_cast<std::size_t>(rank)] + s.key)];
                if (near.index < 0 || near.reed || !near.active || near.resting || near.apart
                    || flues[static_cast<std::size_t>(near.index)].wind <= 0.0f)
                    continue;
                const double metres = Max(kRowMetres * rows, static_cast<double>(s.radius + near.radius) + kClearance);
                const int after = Max(kRowSamples, static_cast<int>(metres / kSoundSpeed * kPipeRate + 0.5));
                if (after <= kLongestHearing) {
                    s.hears[side] = near.index;
                    s.hearGain[side] = static_cast<float>(-1.0 / metres);
                    s.hearDelay[side] = static_cast<std::uint8_t>(after);
                    s.coupled = true;
                }
                break;
            }
        }
        // What it told before anyone listened is not known.
        if (s.coupled && !was)
            for (int i = 0; i < kRingSize; ++i) toldRing[static_cast<std::size_t>(s.index) * kRingSize + static_cast<std::size_t>(i)] = 0.0f;
    }

    // The wind is heard while the organ is played and a little after: the
    // blower's hum as it turns, the chests' hiss as they hold pressure.
    // The blower is heard coming up to speed and running down as well.
    const bool turning = supply == kWindBlower ? spin < 0.98 : spin > 0.02;
    presence += (speaking > 0 || turning ? kPresenceRise * (1.0f - presence) : -kPresenceFall * presence);
    if (speaking == 0 && !turning && presence < 1.0e-4f) presence = 0.0f;
    windNoise.SetSpin(spin);
    const float noise = static_cast<float>(kBlowerLevel * controls[kBlower]) * presence;
    hum = noise * static_cast<float>(spin);
    hiss = noise * static_cast<float>(Min(head, 1.5));
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
    // these frames, each from its own sample on. Each pipe's sound is made
    // first, four pipes of a kind at a time where there are four, and then
    // added to its division in the list's order.
    const auto add = [&](const Slot& s, const float* sound, int from) {
        float* busLeft = bus[s.division][0].data() + from;
        float* busRight = bus[s.division][1].data() + from;
        const float gainLeft = s.reed ? reeds[static_cast<std::size_t>(s.index)].gainLeft : flues[static_cast<std::size_t>(s.index)].gainLeft;
        const float gainRight = s.reed ? reeds[static_cast<std::size_t>(s.index)].gainRight : flues[static_cast<std::size_t>(s.index)].gainRight;
        // A row behind the front one is heard its distance later, through
        // its own bus: every sample of that bus is then summed in one call
        // and in the list's order, however the frames are cut.
        if (s.outDelay > 0) {
            auto& rows = rowBus[s.division][s.outDelay / kRowSamples - 1];
            busLeft = rows[0].data() + from + s.outDelay;
            busRight = rows[1].data() + from + s.outDelay;
        }
        for (int i = 0; i < frames - from; ++i) {
            busLeft[i] += gainLeft * sound[i];
            busRight[i] += gainRight * sound[i];
        }
    };
    // Open flues, flues with a pipe beyond a junction, either hearing
    // neighbours or not, and reeds.
    for (auto& list : flueList) list.clear();
    for (auto& list : flueSound) list.clear();
    for (auto& list : flueAt) list.clear();
    reedList.clear();
    reedSound.clear();
    const std::size_t count = active.size();
    for (std::size_t j = 0; j < count; ++j) {
        const Slot& s = slots[static_cast<std::size_t>(active[j])];
        if (s.resting) continue;
        float* const sound = scratch.data() + j * kChunk;
        if (s.reed) {
            reedList.push_back(&reeds[static_cast<std::size_t>(s.index)]);
            reedSound.push_back(sound);
        } else {
            FluePipe& pipe = flues[static_cast<std::size_t>(s.index)];
            const std::size_t kind = (pipe.upperLength > 0 ? 1 : 0) + (s.coupled ? 2 : 0);
            flueList[kind].push_back(&pipe);
            flueSound[kind].push_back(sound);
            if (s.coupled) flueAt[kind - 2].push_back(j);
        }
    }
#if defined(OREGANA_FOUR)
    // Two or three left over of a kind are made a four with silent pipes: a
    // lane's arithmetic is its pipe's own, so theirs is as it would be alone.
    for (std::size_t kind = 0; kind < 4; ++kind)
        for (std::size_t spare = 0; flueList[kind].size() % 4 >= 2; ++spare) {
            flueList[kind].push_back(&spareFlue[kind & 1][spare]);
            flueSound[kind].push_back(spareSound[spare].data());
            if (kind >= 2) flueAt[kind - 2].push_back(count + spare);
        }
    for (std::size_t spare = 0; reedList.size() % 4 >= 2; ++spare) {
        reedList.push_back(&spareReed[spare]);
        reedSound.push_back(spareSound[spare].data());
    }
#endif
    for (std::size_t kind = 0; kind < 2; ++kind) {
        std::size_t done = 0;
#if defined(OREGANA_FOUR)
        for (; done + 4 <= flueList[kind].size(); done += 4) {
            if (kind == 0) FluePipe::RunFour<false, false>(flueList[kind].data() + done, flueSound[kind].data() + done, frames);
            else FluePipe::RunFour<true, false>(flueList[kind].data() + done, flueSound[kind].data() + done, frames);
        }
#endif
        for (; done < flueList[kind].size(); ++done) flueList[kind][done]->Run(flueSound[kind][done], frames);
    }
    // The hearing flues go a hearing block at a time: no neighbour is nearer
    // than a block of samples, so all that one hears in a block was told
    // before it began, however the pipes are ordered or the frames are cut.
    // A tick begins on a multiple of kChunk samples, so a chunk's place in a
    // ring never wraps, and a ring's first block is kept again after its
    // last, so nor does a block that is read.
    if (!flueAt[0].empty() || !flueAt[1].empty()) {
        for (int sub = 0; sub < frames; sub += kHearBlock) {
            const int n = Min(kHearBlock, frames - sub);
            const std::size_t at = static_cast<std::size_t>((now + static_cast<std::uint64_t>(sub)) & (kRing - 1));
            for (const auto& places : flueAt) {
                for (const std::size_t j : places) {
                    if (j >= count) continue;   // a spare
                    const Slot& s = slots[static_cast<std::size_t>(active[j])];
                    float* const heard = hearScratch.data() + j * kHearBlock;
                    for (int i = 0; i < kHearBlock; ++i) heard[i] = 0.0f;
                    for (int side = 0; side < 2; ++side) {
                        if (s.hears[side] < 0) continue;
                        const float* const told = toldRing.data() + static_cast<std::size_t>(s.hears[side]) * kRingSize
                            + ((at + kRing - s.hearDelay[side]) & (kRing - 1));
                        const float gain = s.hearGain[side];
#if defined(OREGANA_FOUR)
                        for (int i = 0; i < kHearBlock; i += 4) {
                            Float4 sum, from;
                            __builtin_memcpy(&sum, heard + i, sizeof sum);
                            __builtin_memcpy(&from, told + i, sizeof from);
                            sum += gain * from;
                            __builtin_memcpy(heard + i, &sum, sizeof sum);
                        }
#else
                        for (int i = 0; i < kHearBlock; ++i) heard[i] += gain * told[i];
#endif
                    }
                }
            }
            for (std::size_t kind = 2; kind < 4; ++kind) {
                const auto& places = flueAt[kind - 2];
                const auto heardOf = [&](std::size_t m) {
                    return places[m] >= count ? spareHeard.data() : hearScratch.data() + places[m] * kHearBlock;
                };
                const auto toldOf = [&](std::size_t m) {
                    if (places[m] >= count) return spareTold[places[m] - count].data();
                    return toldRing.data() + static_cast<std::size_t>(slots[static_cast<std::size_t>(active[places[m]])].index) * kRingSize + at;
                };
                std::size_t done = 0;
#if defined(OREGANA_FOUR)
                for (; done + 4 <= places.size(); done += 4) {
                    float* sound[4];
                    const float* heard[4];
                    float* told[4];
                    for (std::size_t l = 0; l < 4; ++l) {
                        sound[l] = flueSound[kind][done + l] + sub;
                        heard[l] = heardOf(done + l);
                        told[l] = toldOf(done + l);
                    }
                    if (kind == 2) FluePipe::RunFour<false, true>(flueList[kind].data() + done, sound, n, heard, told);
                    else FluePipe::RunFour<true, true>(flueList[kind].data() + done, sound, n, heard, told);
                }
#endif
                for (; done < places.size(); ++done)
                    flueList[kind][done]->Run(flueSound[kind][done] + sub, n, heardOf(done), toldOf(done));
                if (at < kHearBlock)
                    for (std::size_t m = 0; m < places.size(); ++m) {
                        if (places[m] >= count) continue;
                        float* const told = toldOf(m);
                        for (int i = 0; i < n; ++i) told[kRing + i] = told[i];
                    }
            }
        }
    }
    std::size_t done = 0;
#if defined(OREGANA_FOUR)
    for (; done + 4 <= reedList.size(); done += 4) ReedPipe::RunFour(reedList.data() + done, reedSound.data() + done, frames);
#endif
    for (; done < reedList.size(); ++done) reedList[done]->Run(reedSound[done], frames);
    for (std::size_t j = 0; j < count; ++j) {
        const Slot& s = slots[static_cast<std::size_t>(active[j])];
        if (!s.resting) add(s, scratch.data() + j * kChunk, 0);
    }
    std::size_t arrived = 0;
    for (; arrived < pending.size() && pending[arrived].due < now + static_cast<std::uint64_t>(frames); ++arrived) {
        const Pending arriving = pending[arrived];
        if (!Take(arriving.slot, arriving.share)) continue;
        const Slot& s = slots[static_cast<std::size_t>(arriving.slot)];
        const int from = static_cast<int>(arriving.due - now);
        if (s.reed) reeds[static_cast<std::size_t>(s.index)].Run(scratch.data(), frames - from);
        else flues[static_cast<std::size_t>(s.index)].Run(scratch.data(), frames - from);
        add(s, scratch.data(), from);
    }
    pending.erase(pending.begin(), pending.begin() + static_cast<std::ptrdiff_t>(arrived));
    now += static_cast<std::uint64_t>(frames);
    for (std::size_t d = 0; d < kDivisions; ++d) {
        for (auto& rows : rowBus[d]) {
            for (std::size_t c = 0; c < 2; ++c) {
                float* const row = rows[c].data();
                for (int i = 0; i < frames; ++i) bus[d][c][static_cast<std::size_t>(i)] += row[i];
                const int kept = static_cast<int>(rows[c].size()) - frames;
                for (int i = 0; i < kept; ++i) row[i] = row[i + frames];
                for (int i = kept; i < kept + frames; ++i) row[i] = 0.0f;
            }
        }
    }
    for (int i = 0; i < frames; ++i) {
        const auto at = static_cast<std::size_t>(i);
        float sound[2];
        boxLowGain += kGainStep * (boxLowTarget - boxLowGain);
        boxHighGain += kGainStep * (boxHighTarget - boxHighGain);
        // What the church hears from the divisions after the first, in
        // their order: the Positiv, the Recit through its box, the Pedal.
        float apart[kDivisions - 1] = {};
        for (std::size_t c = 0; c < 2; ++c) {
            const float enclosed = bus[kRecit][c][at];
            boxLow[c] += kBoxPole * (enclosed - boxLow[c]);
            sound[c] = bus[kHauptwerk][c][at] + bus[kPositiv][c][at] + bus[kPedal][c][at]
                + boxLowGain * boxLow[c] + boxHighGain * (enclosed - boxLow[c]);
            apart[kPositiv - 1] += 0.5f * bus[kPositiv][c][at];
            apart[kRecit - 1] += 0.5f * (boxLowGain * boxLow[c] + boxHighGain * (enclosed - boxLow[c]));
            apart[kPedal - 1] += 0.5f * bus[kPedal][c][at];
        }
        knocks.Process(sound[0], sound[1]);
        if (hum > 0.0f || hiss > 0.0f || rub > 0.0f) windNoise.Process(hum, hiss, rub, sound[0], sound[1]);
        float roomLeft, roomRight;
        nave.Process(sound[0], sound[1], roomLeft, roomRight, apart);
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
