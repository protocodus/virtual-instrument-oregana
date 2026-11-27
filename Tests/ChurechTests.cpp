// Churech contracts: silence stays silence, a dry mix is the sound itself,
// the church rings longer in a larger building and with a longer Decay, the
// sound's own level falls with Distance, Duck draws the church down under a
// sound, the cuts take what they cut, a changed control neither clicks nor
// jumps, and the same samples come out however the host cuts its blocks and
// at every host rate.
#include "DSP/Churech.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

namespace {

using namespace oregana;

int failures = 0;

void Expect(bool ok, const char* test, const char* what, double value) {
    std::printf("%s %s: %s (%.4g)\n", ok ? "PASS" : "FAIL", test, what, value);
    if (!ok) ++failures;
}

double Db(double x) { return 20.0 * std::log10(x + 1.0e-30); }

struct Take {
    int rate = 48000;
    std::vector<float> left, right;
    std::size_t At(double seconds) const { return Min(left.size(), static_cast<std::size_t>(seconds * rate)); }
    double Rms(double from, double to) const {
        double sum = 0.0;
        for (std::size_t i = At(from); i < At(to); ++i) sum += static_cast<double>(left[i]) * left[i] + static_cast<double>(right[i]) * right[i];
        return std::sqrt(sum / static_cast<double>(Max<std::size_t>(At(to) - At(from), 1)));
    }
    double Peak(double from, double to) const {
        float peak = 0.0f;
        for (std::size_t i = At(from); i < At(to); ++i) peak = Max(peak, Max(Abs(left[i]), Abs(right[i])));
        return peak;
    }
    // The largest step between two samples: a click stands far above a
    // tail's or a tone's own.
    double Jump(double from, double to) const {
        float most = 0.0f;
        for (std::size_t i = At(from) + 1; i < At(to); ++i)
            most = Max(most, Max(Abs(left[i] - left[i - 1]), Abs(right[i] - right[i - 1])));
        return most;
    }
};

using Source = float (*)(std::size_t i, int rate);

float Click(std::size_t i, int) { return i == 0 ? 1.0f : 0.0f; }
// Pink-ish noise for 0.5 s, then silence: a stroke that fills the church.
float Burst(std::size_t i, int rate) {
    if (i >= static_cast<std::size_t>(rate / 2)) return 0.0f;
    static std::uint32_t state = 1;
    if (i == 0) state = 1;
    return 0.25f * NextNoise(state);
}
float Tone(std::size_t i, int rate) { return 0.3f * static_cast<float>(std::sin(kTwoPi * 220.0 * static_cast<double>(i) / rate)); }

// `change`, if given, is applied at `changeAt` seconds.
Take Play(const Churech::Controls& controls, Source source, double seconds, int rate = 48000, int block = 64,
          const Churech::Controls* change = nullptr, double changeAt = 0.0) {
    auto churech = std::make_unique<Churech>();
    churech->Prepare(rate);
    churech->Set(controls);
    Take take;
    take.rate = rate;
    const auto frames = static_cast<std::size_t>(seconds * rate);
    take.left.resize(frames);
    take.right.resize(frames);
    std::vector<float> inLeft(frames), inRight(frames);
    for (std::size_t i = 0; i < frames; ++i) inLeft[i] = inRight[i] = source(i, rate);
    const auto changeFrame = static_cast<std::size_t>(changeAt * rate);
    for (std::size_t done = 0; done < frames;) {
        auto n = Min<std::size_t>(static_cast<std::size_t>(block), frames - done);
        if (change != nullptr && done <= changeFrame && changeFrame < done + n) {
            if (changeFrame > done) n = changeFrame - done;
            else churech->Set(*change);
        }
        churech->Process(&inLeft[done], &inRight[done], &take.left[done], &take.right[done], static_cast<int>(n));
        done += n;
    }
    return take;
}

// The time the tail takes to fall from -5 to -35 dB below its loudest
// 10 ms, doubled: T30 as a room's decay is read.
double DecaySeconds(const Take& take) {
    const auto window = static_cast<std::size_t>(0.01 * take.rate);
    std::vector<double> levels;
    for (std::size_t i = 0; i + window <= take.left.size(); i += window) {
        double sum = 0.0;
        for (std::size_t k = i; k < i + window; ++k) sum += static_cast<double>(take.left[k]) * take.left[k] + static_cast<double>(take.right[k]) * take.right[k];
        levels.push_back(10.0 * std::log10(sum + 1.0e-30));
    }
    std::size_t loudest = 0;
    for (std::size_t i = 0; i < levels.size(); ++i)
        if (levels[i] > levels[loudest]) loudest = i;
    std::size_t from = loudest, to = loudest;
    while (from < levels.size() && levels[from] > levels[loudest] - 5.0) ++from;
    to = from;
    while (to < levels.size() && levels[to] > levels[loudest] - 35.0) ++to;
    return 2.0 * static_cast<double>(to - from) * 0.01;
}

Churech::Controls Wet() {
    Churech::Controls c;
    c.mix = 1.0;
    return c;
}

void SilenceStaysSilent() {
    const Take take = Play(Wet(), [](std::size_t, int) { return 0.0f; }, 1.0);
    Expect(take.Peak(0.0, 1.0) == 0.0, "Silence", "no sound in gives exact silence out", take.Peak(0.0, 1.0));
    // And once a burst has rung out under -140 dB, exact silence again.
    Churech::Controls c = Wet();
    c.type = Churech::kChapel;
    const Take after = Play(c, Burst, 6.0);
    Expect(after.Peak(5.5, 6.0) == 0.0, "Silence", "a chapel rung out returns to exact silence", after.Peak(5.5, 6.0));
}

void DryIsTheSound() {
    Churech::Controls c;
    c.mix = 0.0;
    const Take take = Play(c, Tone, 0.5);
    double most = 0.0;
    for (std::size_t i = 0; i < take.left.size(); ++i) most = Max(most, static_cast<double>(Abs(take.left[i] - Tone(i, 48000))));
    Expect(most == 0.0, "Dry", "Mix 0 at the nearest place is the sound itself", most);
}

void LargerRingsLonger() {
    double previous = 0.0;
    for (int type = 0; type < Churech::kTypes; ++type) {
        Churech::Controls c = Wet();
        c.type = type;
        const double t = DecaySeconds(Play(c, Burst, 14.0));
        char what[96];
        std::snprintf(what, sizeof what, "type %d rings longer than the one before, near its building's %.1f s", type,
                      Churech::kBuildings[static_cast<std::size_t>(type)].decay);
        const double asked = Churech::kBuildings[static_cast<std::size_t>(type)].decay;
        // The air takes the treble sooner than the middle: a broadband T30
        // reads shorter than the mid-range decay asked for.
        Expect(t > previous && t > 0.4 * asked && t < 1.3 * asked, "Types", what, t);
        previous = t;
    }
    Churech::Controls shorter = Wet(), longer = Wet();
    shorter.decay = 0.5;
    longer.decay = 2.0;
    const double ts = DecaySeconds(Play(shorter, Burst, 14.0)), tl = DecaySeconds(Play(longer, Burst, 14.0));
    Expect(tl > 3.0 * ts, "Decay", "Decay x2 rings over three times as long as x0.5", tl / ts);
}

void DistanceLowersTheSound() {
    // Since 0.15.1, Mix = 0% stays bit-exact unity dry regardless of Distance;
    // the gain-compensated direct attenuation engages cleanly across 0..15% Mix.
    Churech::Controls near, far;
    near.mix = far.mix = 0.25;
    near.earlyLevel = far.earlyLevel = 0.0;
    far.distance = 1.0;
    const double drop = Db(Play(far, Tone, 0.03).Rms(0.005, 0.03) / Play(near, Tone, 0.03).Rms(0.005, 0.03));
    Expect(drop < -2.0 && drop > -3.5, "Distance", "the furthest place lowers direct insert level ~2.6 dB", drop);

    // On an insert (Mix 30%), overall track loudness stays balanced within 2.5 dB across Distance.
    Churech::Controls insNear, insFar;
    insNear.mix = insFar.mix = 0.3;
    insFar.distance = 1.0;
    const double insertDelta = Db(Play(insFar, Burst, 1.5).Rms(0.05, 0.6) / Play(insNear, Burst, 1.5).Rms(0.05, 0.6));
    Expect(Abs(insertDelta) < 2.5, "Distance", "insert Mix 30% stays balanced across Distance, dB", insertDelta);

    // And the church's answer changes arrival with the place.
    Churech::Controls wetNear = Wet(), wetFar = Wet();
    wetFar.distance = 1.0;
    const Take a = Play(wetNear, Click, 0.3), b = Play(wetFar, Click, 0.3);
    auto first = [](const Take& t) {
        for (std::size_t i = 0; i < t.left.size(); ++i)
            if (Abs(t.left[i]) + Abs(t.right[i]) > 1.0e-4f) return static_cast<double>(i) / t.rate;
        return 1.0;
    };
    Expect(first(b) != first(a), "Distance", "the first reflection moves with the place", first(b) - first(a));
}

void DuckDrawsTheChurchDown() {
    Churech::Controls open = Wet(), ducked = Wet();
    ducked.duck = 1.0;
    const double under = Db(Play(ducked, Tone, 2.0).Rms(1.0, 2.0) / Play(open, Tone, 2.0).Rms(1.0, 2.0));
    Expect(under < -12.0, "Duck", "a -10 dBFS tone draws the church down over 12 dB", under);
    const Take a = Play(open, Burst, 4.0), b = Play(ducked, Burst, 4.0);
    const double after = Db(b.Rms(2.0, 3.0) / a.Rms(2.0, 3.0));
    Expect(after > -1.0, "Duck", "the tail comes back once the sound stops", after);
}

void CutsTakeWhatTheyCut() {
    Churech::Controls open = Wet(), cut = Wet();
    cut.lowCutHz = 1000.0;
    cut.highCutHz = 1000.0;
    const double taken = Db(Play(cut, Burst, 2.0).Rms(0.0, 2.0) / Play(open, Burst, 2.0).Rms(0.0, 2.0));
    Expect(taken < -6.0, "Cuts", "both cuts at 1 kHz leave a narrow band", taken);
}

void ChangesDoNotClick() {
    // A ringing cathedral turned to a chapel, moved to the far place, the
    // pre-delay thrown and the mix and the cuts thrown, at once.
    Churech::Controls from = Wet();
    from.type = Churech::kCathedral;
    Churech::Controls to = from;
    to.type = Churech::kChapel;
    to.distance = 1.0;
    to.seat = 1.0;
    to.preDelayMs = 200.0;
    to.mix = 0.5;
    to.lowCutHz = 400.0;
    to.highCutHz = 2000.0;
    to.outputDb = -6.0;
    const Take still = Play(from, Burst, 2.0);
    const Take turned = Play(from, Burst, 2.0, 48000, 64, &to, 1.0);
    const double ringing = still.Jump(0.9, 1.0), turning = turned.Jump(1.0, 1.1);
    Expect(turning < 2.0 * ringing, "Changes", "every control thrown at once steps no more than the tail itself", turning / ringing);
}

void BlocksAndRates() {
    Churech::Controls c = Wet();
    c.mix = 0.6;
    c.duck = 0.5;
    c.distance = 0.4;
    const Take a = Play(c, Burst, 1.5, 48000, 64), b = Play(c, Burst, 1.5, 48000, 17);
    double most = 0.0;
    for (std::size_t i = 0; i < a.left.size(); ++i) most = Max(most, static_cast<double>(Abs(a.left[i] - b.left[i]) + Abs(a.right[i] - b.right[i])));
    Expect(most == 0.0, "Blocks", "64- and 17-sample blocks give the same samples", most);
    const double reference = Play(c, Burst, 1.5, 48000).Rms(0.2, 1.5);
    for (int rate : { 44100, 88200, 96000, 192000 }) {
        const Take t = Play(c, Burst, 1.5, rate);
        char what[64];
        std::snprintf(what, sizeof what, "at %d Hz the church is within 1 dB of 48 kHz's", rate);
        Expect(Abs(Db(t.Rms(0.2, 1.5) / reference)) < 1.0, "Rates", what, Db(t.Rms(0.2, 1.5) / reference));
    }
}

void TrueStereoPreservesImage() {
    const auto renderClick = [](bool leftOnly) {
        auto churech = std::make_unique<Churech>();
        churech->Prepare(48000);
        churech->Set(Wet());
        Take take;
        take.rate = 48000;
        const std::size_t frames = 96000;
        take.left.resize(frames);
        take.right.resize(frames);
        std::vector<float> inLeft(frames), inRight(frames);
        (leftOnly ? inLeft[0] : inRight[0]) = 1.0f;
        for (std::size_t done = 0; done < frames; done += 64)
            churech->Process(&inLeft[done], &inRight[done], &take.left[done], &take.right[done], 64);
        return take;
    };
    const Take fromLeft = renderClick(true), fromRight = renderClick(false);
    const auto sumSq = [](const std::vector<float>& v, std::size_t a, std::size_t b) {
        double s = 0.0;
        for (std::size_t i = a; i < b; ++i) s += static_cast<double>(v[i]) * v[i];
        return s;
    };
    const std::size_t earlyEnd = static_cast<std::size_t>(0.08 * 48000);
    const double leftBiasDb = 10.0 * std::log10((sumSq(fromLeft.left, 0, earlyEnd) + 1.0e-30) / (sumSq(fromLeft.right, 0, earlyEnd) + 1.0e-30));
    const double rightBiasDb = 10.0 * std::log10((sumSq(fromRight.right, 0, earlyEnd) + 1.0e-30) / (sumSq(fromRight.left, 0, earlyEnd) + 1.0e-30));
    Expect(leftBiasDb > 2.0 && rightBiasDb > 2.0, "TrueStereo", "early reflections preserve left/right source separation, dB", Min(leftBiasDb, rightBiasDb));

    const std::size_t lateStart = static_cast<std::size_t>(0.2 * 48000), lateEnd = static_cast<std::size_t>(1.5 * 48000);
    double dot = 0.0, eL = 0.0, eR = 0.0;
    for (std::size_t i = lateStart; i < lateEnd; ++i) {
        const double l = fromLeft.left[i], r = fromRight.left[i];
        dot += l * r;
        eL += l * l;
        eR += r * r;
    }
    const double lateCorr = Abs(dot / Sqrt(eL * eR + 1.0e-30));
    Expect(lateCorr < 0.25, "TrueStereo", "late tails from left and right inputs are decorrelated", lateCorr);
}

void EarlyReflectionsAreWideAndDense() {
    Churech::Controls c = Wet();
    c.type = Churech::kCathedral;
    const Take take = Play(c, Click, 0.3);
    const std::size_t earlyEnd = static_cast<std::size_t>(0.08 * 48000);
    double dot = 0.0, eL = 0.0, eR = 0.0;
    for (std::size_t i = 0; i < earlyEnd; ++i) {
        const double l = take.left[i], r = take.right[i];
        dot += l * r;
        eL += l * l;
        eR += r * r;
    }
    const double earlyCorr = dot / Sqrt(eL * eR + 1.0e-30);
    Expect(earlyCorr < 0.5, "EarlyDensity", "centered cathedral early reflections are stereo, not mono-collapsed", earlyCorr);
    const double gapRelDb = Db(take.Rms(0.015, 0.05) / (take.Rms(0.08, 0.2) + 1.0e-30));
    Expect(gapRelDb > -10.0, "EarlyDensity", "15-50 ms cathedral early window has smooth buildup vs 80-200 ms, dB", gapRelDb);
}

void LoftAndSeat() {
    // Loft (1.0 organ loft under the vault vs 0.0 floor) changes first-reflection geometry and overhead presence.
    Churech::Controls floorC = Wet(), loftC = Wet();
    floorC.loft = 0.0;
    loftC.loft = 1.0;
    const Take tf = Play(floorC, Click, 0.25), tl = Play(loftC, Click, 0.25);
    double diff = 0.0;
    for (std::size_t i = 0; i < tf.left.size(); ++i) diff += Abs(tf.left[i] - tl.left[i]);
    Expect(diff > 0.05, "Loft", "organ loft reflections differ markedly from floor source", diff);

    // Seat moves direct sound on active inserts (mono-safe ILD + head-shadow), while Mix = 0% stays unity dry.
    Churech::Controls dryCenter, dryRight;
    dryCenter.mix = dryRight.mix = 0.20;
    dryCenter.earlyLevel = dryRight.earlyLevel = 0.0;
    dryRight.seat = 1.0;
    const Take tr = Play(dryRight, Tone, 0.03);
    const double dryRatio = Db(tr.Rms(0.005, 0.03) / Play(dryCenter, Tone, 0.03).Rms(0.005, 0.03));
    double earDiff = 0.0;
    for (std::size_t i = tr.At(0.005); i < tr.At(0.03); ++i) earDiff += Abs(tr.left[i] - tr.right[i]);
    Expect(earDiff > 1.0 && Abs(dryRatio) < 4.0, "SeatDirect", "off-axis seat shapes direct left/right level and shadow", earDiff);
}

void FreezeSidechainSafetyAndMemory() {
    // Freeze sustains energy indefinitely without decaying, then releases smoothly when turned off.
    auto ch = std::make_unique<Churech>();
    ch->Prepare(48000);
    Churech::Controls c = Wet();
    c.type = Churech::kChapel;
    c.freeze = false;
    ch->Set(c);
    const int block = 64;
    std::vector<float> zero(block, 0.0f), burst(48000, 0.0f), ol(block), orr(block);
    std::uint32_t rng = 9;
    for (int i = 0; i < 12000; ++i) burst[static_cast<std::size_t>(i)] = 0.25f * NextNoise(rng);
    for (int pos = 0; pos < 12000; pos += block) ch->Process(&burst[static_cast<std::size_t>(pos)], &burst[static_cast<std::size_t>(pos)], ol.data(), orr.data(), block);
    c.freeze = true;
    ch->Set(c);
    auto measureRms = [&](int frames) {
        double p = 0.0;
        for (int pos = 0; pos < frames; pos += block) {
            ch->Process(zero.data(), zero.data(), ol.data(), orr.data(), block);
            for (int i = 0; i < block; ++i) p += static_cast<double>(ol[static_cast<std::size_t>(i)]) * ol[static_cast<std::size_t>(i)] + static_cast<double>(orr[static_cast<std::size_t>(i)]) * orr[static_cast<std::size_t>(i)];
        }
        return std::sqrt(p / (2.0 * frames));
    };
    (void)measureRms(24000); // settle into freeze
    const double earlyF = measureRms(24000);
    (void)measureRms(48000 * 3);
    const double lateF = measureRms(24000);
    Expect(earlyF > 1.0e-3 && Abs(Db(lateF / earlyF)) < 0.2, "Freeze", "frozen tail holds level across 3+ seconds in a chapel, dB drift", Db(lateF / earlyF));

    // External sidechain ducks a steady dry+wet tone while main input does not trigger it.
    ch->Prepare(48000);
    Churech::Controls scC = Wet();
    scC.duck = 1.0;
    ch->Set(scC);
    std::vector<float> quietIn(block, 0.02f), loudSc(block, 0.7f);
    for (int k = 0; k < 400; ++k) ch->Process(quietIn.data(), quietIn.data(), ol.data(), orr.data(), block, zero.data(), zero.data());
    double openScRms = 0.0;
    for (int k = 0; k < 50; ++k) {
        ch->Process(quietIn.data(), quietIn.data(), ol.data(), orr.data(), block, zero.data(), zero.data());
        for (int i = 0; i < block; ++i) openScRms += ol[static_cast<std::size_t>(i)] * ol[static_cast<std::size_t>(i)];
    }
    for (int k = 0; k < 400; ++k) ch->Process(quietIn.data(), quietIn.data(), ol.data(), orr.data(), block, loudSc.data(), loudSc.data());
    double duckedScRms = 0.0;
    for (int k = 0; k < 50; ++k) {
        ch->Process(quietIn.data(), quietIn.data(), ol.data(), orr.data(), block, loudSc.data(), loudSc.data());
        for (int i = 0; i < block; ++i) duckedScRms += ol[static_cast<std::size_t>(i)] * ol[static_cast<std::size_t>(i)];
    }
    const double scGr = Db(std::sqrt(duckedScRms / (openScRms + 1.0e-30)));
    Expect(scGr < -15.0 && ch->MeterDuckGr() > 0.5f, "SidechainDuck", "external sidechain draws reverb down cleanly, dB", scGr);

    // Output soft safety limiter + zero-latency 44.1 kHz + compact RAM (< 2.5 MiB).
    ch->Prepare(44100);
    Expect(ch->ChurchLateSamples() == 0, "NativeRate441", "44.1 kHz runs natively with zero latency, samples", ch->ChurchLateSamples());
    Churech::Controls hot = Wet();
    hot.type = Churech::kCathedral;
    hot.decay = 2.0;
    hot.decayBass = 2.0;
    hot.outputDb = 12.0;
    ch->Set(hot);
    std::vector<float> hotIn(block);
    float peakOut = 0.0f;
    for (int k = 0; k < 600; ++k) {
        for (int i = 0; i < block; ++i) hotIn[static_cast<std::size_t>(i)] = 0.9f * static_cast<float>(std::sin(kTwoPi * 80.0 * (k * block + i) / 44100.0));
        ch->Process(hotIn.data(), hotIn.data(), ol.data(), orr.data(), block);
        for (int i = 0; i < block; ++i) peakOut = Max(peakOut, Max(Abs(ol[static_cast<std::size_t>(i)]), Abs(orr[static_cast<std::size_t>(i)])));
    }
    Expect(peakOut < 1.0f && peakOut > 0.85f && ch->MeterOutLeft() > 0.8f, "SafetyLimiter", "soft limiter prevents digital clipping under +12 dB Cathedral overload", peakOut);
    Expect(sizeof(Churech) < 2621440u, "Memory", "Churech instance fits under 2.5 MiB", static_cast<double>(sizeof(Churech)) / 1048576.0);
}

void MonoCombFree() {
    // At Width = 0, mono L+R sum has zero comb notches across frequency (0..40 ms early window).
    Churech::Controls c = Wet();
    c.width = 0.0;
    const Take monoW = Play(c, Click, 0.25);
    double maxStereoDiff = 0.0;
    for (std::size_t i = 0; i < monoW.left.size(); ++i) maxStereoDiff = Max(maxStereoDiff, static_cast<double>(Abs(monoW.left[i] - monoW.right[i])));
    Expect(maxStereoDiff < 1.0e-5, "MonoSafe", "Width = 0 yields identical L and R with zero comb coloring", maxStereoDiff);
}

void LiveLoftUpdatesGeometry() {
    Churech::Controls low = Wet();
    low.type = Churech::kCathedral;
    low.loft = 0.0;
    Churech::Controls high = low;
    high.loft = 1.0;
    const Take stayLow = Play(low, Burst, 1.2);
    const Take sweptHigh = Play(low, Burst, 1.2, 48000, 64, &high, 0.25);
    double sqDiff = 0.0;
    const std::size_t from = sweptHigh.At(0.35), to = sweptHigh.At(1.0);
    for (std::size_t i = from; i < to; ++i) {
        const double dl = sweptHigh.left[i] - stayLow.left[i], dr = sweptHigh.right[i] - stayLow.right[i];
        sqDiff += dl * dl + dr * dr;
    }
    const double rmsDiff = std::sqrt(sqDiff / static_cast<double>(Max<std::size_t>(to - from, 1)));
    Expect(rmsDiff > 1.0e-3, "LiveLoft", "sweeping Loft during live audio updates 3D early reflections", rmsDiff);
}

void DistinctBuildingFdnSizes() {
    std::array<double, Churech::kTypes> onsetMs {};
    for (int t = 0; t < Churech::kTypes; ++t) {
        Churech::Controls c = Wet();
        c.type = t;
        c.earlyLevel = 0.0;
        const Take ir = Play(c, Click, 0.12);
        std::size_t hit = ir.left.size();
        for (std::size_t i = 0; i < ir.left.size(); ++i) {
            if (Abs(ir.left[i]) + Abs(ir.right[i]) > 1.0e-4f) {
                hit = i;
                break;
            }
        }
        onsetMs[static_cast<std::size_t>(t)] = static_cast<double>(hit) * 1000.0 / ir.rate;
    }
    const bool strict = onsetMs[0] < onsetMs[1] && onsetMs[1] < onsetMs[2] && onsetMs[2] < onsetMs[3];
    Expect(strict && onsetMs[0] < 26.0 && onsetMs[3] > 55.0 && onsetMs[3] < 80.0, "FdnSizes",
           "Chapel..Cathedral have strictly distinct FDN onsets, ms (Cathedral)", onsetMs[3]);

    // Pre-delay = 200 ms on a 1-sample impulse must not be eaten by idle sleep before returning.
    Churech::Controls pd = Wet();
    pd.preDelayMs = 200.0;
    const Take pdTake = Play(pd, Click, 0.4);
    Expect(pdTake.Rms(0.22, 0.35) > 1.0e-3 && pdTake.Peak(0.0, 0.19) == 0.0, "PreDelay200",
           "200 ms Pre-delay preserves impulse past idle threshold", pdTake.Rms(0.22, 0.35));
}

void NoCathedralReflectionPileup() {
    Churech::Controls c = Wet();
    c.type = Churech::kCathedral;
    const Take ir = Play(c, Click, 0.25);
    const double preRms = ir.Rms(0.160, 0.168);
    const double at170Rms = ir.Rms(0.170, 0.176);
    const double jumpDb = Db(at170Rms / (preRms + 1.0e-30));
    Expect(jumpDb < 3.0, "CathedralEarly", "no reflection pileup spike at 170-176 ms, dB jump", jumpDb);
}

void DryPathPureAtZeroMix() {
    Churech::Controls c;
    c.mix = 0.0;
    c.outputDb = 0.0;
    // Since 0.15.1, Distance, Seat, and Loft never alter the dry path at Mix = 0%.
    c.distance = 1.0;
    c.seat = 1.0;
    c.loft = 0.0;
    auto hotSine = [](std::size_t i, int rate) {
        return 0.96f * static_cast<float>(std::sin(kTwoPi * 440.0 * static_cast<double>(i) / rate));
    };
    const Take t = Play(c, hotSine, 0.2);
    double maxErr = 0.0;
    for (std::size_t i = 0; i < t.left.size(); ++i)
        maxErr = Max(maxErr, static_cast<double>(Max(Abs(t.left[i] - hotSine(i, 48000)), Abs(t.right[i] - hotSine(i, 48000)))));
    Expect(maxErr == 0.0, "DryPurity", "-0.35 dBFS signal at Mix 0% is bit-exact clean even at Distance 100% / Seat R100", maxErr);
}

void SeatMonoSafeAndFastGlide() {
    // Measure direct-path Seat before Cathedral's 65.9 ms FDN onset (earlyLevel = 0.0, Mix = 20%).
    Churech::Controls centered, offAxis;
    centered.type = offAxis.type = Churech::kCathedral;
    centered.mix = offAxis.mix = 0.20;
    centered.earlyLevel = offAxis.earlyLevel = 0.0;
    offAxis.seat = 1.0;
    auto notchTone = [](std::size_t i, int rate) {
        return 0.4f * static_cast<float>(std::sin(kTwoPi * 1412.0 * static_cast<double>(i) / rate));
    };
    const Take tc = Play(centered, notchTone, 0.064, 48000, 64);
    const Take ts = Play(offAxis, notchTone, 0.064, 48000, 64);
    auto monoRms = [](const Take& t, double a, double b) {
        double p = 0.0;
        for (std::size_t i = t.At(a); i < t.At(b); ++i) {
            const double m = 0.5 * (t.left[i] + t.right[i]);
            p += m * m;
        }
        return std::sqrt(p / static_cast<double>(Max<std::size_t>(t.At(b) - t.At(a), 1)));
    };
    const double monoDropDb = Db(monoRms(ts, 0.005, 0.060) / monoRms(tc, 0.005, 0.060));
    Expect(monoDropDb > -3.0, "SeatMonoSafe", "Seat = 1.0 preserves mono sum at 1412 Hz without comb notch, dB", monoDropDb);

    // Check fast 20 ms glide even at 192 kHz (0.045..0.064 s stays before 65.9 ms Cathedral FDN onset).
    const Take swept192 = Play(centered, notchTone, 0.065, 192000, 64, &offAxis, 0.020);
    const Take settled192 = Play(offAxis, notchTone, 0.065, 192000, 64);
    double lateDiff192 = 0.0;
    for (std::size_t i = swept192.At(0.045); i < swept192.At(0.064); ++i)
        lateDiff192 = Max(lateDiff192, static_cast<double>(Abs(swept192.right[i] - settled192.right[i])));
    Expect(lateDiff192 < 1.0e-4, "SeatGlide192", "192 kHz Seat transition finishes within 25 ms", lateDiff192);
}

void LivingFreezeAndIdleSleep() {
    // Idle room with Freeze enabled outputs exact silence and stays at rest.
    Churech::Controls idleF = Wet();
    idleF.freeze = true;
    const Take silentFreeze = Play(idleF, [](std::size_t, int) { return 0.0f; }, 0.5);
    Expect(silentFreeze.Peak(0.0, 0.5) == 0.0, "FreezeIdle", "idle room with Freeze enabled emits exact silence", silentFreeze.Peak(0.0, 0.5));

    // Active frozen tail maintains living phase evolution rather than locking identically period-to-period.
    auto ch = std::make_unique<Churech>();
    ch->Prepare(48000);
    Churech::Controls c = Wet();
    c.type = Churech::kChapel;
    c.modulation = 0.5;
    ch->Set(c);
    const int block = 64;
    std::vector<float> zero(block, 0.0f), burst(block), ol(block), orr(block);
    std::uint32_t rng = 33;
    for (int pos = 0; pos < 12000; pos += block) {
        for (int i = 0; i < block; ++i) burst[static_cast<std::size_t>(i)] = 0.25f * NextNoise(rng);
        ch->Process(burst.data(), burst.data(), ol.data(), orr.data(), block);
    }
    c.freeze = true;
    ch->Set(c);
    for (int k = 0; k < 400; ++k) ch->Process(zero.data(), zero.data(), ol.data(), orr.data(), block);
    std::vector<float> snapA(24000), snapB(24000);
    for (std::size_t pos = 0; pos < 24000; pos += block) {
        ch->Process(zero.data(), zero.data(), &snapA[pos], orr.data(), block);
    }
    for (int k = 0; k < 1500; ++k) ch->Process(zero.data(), zero.data(), ol.data(), orr.data(), block);
    for (std::size_t pos = 0; pos < 24000; pos += block) {
        ch->Process(zero.data(), zero.data(), &snapB[pos], orr.data(), block);
    }
    double dot = 0.0, eA = 0.0, eB = 0.0;
    for (std::size_t i = 0; i < 24000; ++i) {
        dot += static_cast<double>(snapA[i]) * snapB[i];
        eA += static_cast<double>(snapA[i]) * snapA[i];
        eB += static_cast<double>(snapB[i]) * snapB[i];
    }
    const double wfCorr = Abs(dot / Sqrt(eA * eB + 1.0e-30));
    const double driftDb = Db(Sqrt(eB / (eA + 1.0e-30)));
    Expect(Abs(driftDb) < 0.2 && wfCorr < 0.5, "FreezeAlive", "lossless modulated Freeze evolves naturally without losing level", wfCorr);
}

void CvDirtyFlagMatchesFreshSet() {
    auto chSingle = std::make_unique<Churech>();
    auto chRepeated = std::make_unique<Churech>();
    chSingle->Prepare(48000);
    chRepeated->Prepare(48000);
    Churech::Controls base = Wet();
    base.mix = 0.45;
    chSingle->Set(base);
    chRepeated->Set(base);
    const int block = 64, batches = 300;
    std::vector<float> in(block), lA(block), rA(block), lB(block), rB(block);
    double maxDiff = 0.0;
    for (int b = 0; b < batches; ++b) {
        for (int i = 0; i < block; ++i) in[static_cast<std::size_t>(i)] = Tone(static_cast<std::size_t>(b * block + i), 48000);
        chRepeated->Set(base);
        chSingle->Process(in.data(), in.data(), lA.data(), rA.data(), block);
        chRepeated->Process(in.data(), in.data(), lB.data(), rB.data(), block);
        for (int i = 0; i < block; ++i)
            maxDiff = Max(maxDiff, static_cast<double>(Max(Abs(lA[static_cast<std::size_t>(i)] - lB[static_cast<std::size_t>(i)]),
                                                           Abs(rA[static_cast<std::size_t>(i)] - rB[static_cast<std::size_t>(i)]))));
    }
    Expect(maxDiff == 0.0, "CvDirtyFlagParity", "repeated Set() with fixed room params is bit-exact with single Set()", maxDiff);

    // Modulating Mix at batch rate leaves FDN untouched and glides cleanly without clicks.
    chRepeated->Prepare(48000);
    float prevL = 0.0f, maxJump = 0.0f;
    Churech::Controls lfo = base;
    for (int b = 0; b < 400; ++b) {
        const double phase = static_cast<double>(b) * 64.0 / 48000.0 * 2.0;
        const double tri = 1.0 - 2.0 * Abs((phase - std::floor(phase)) - 0.5);
        lfo.mix = 0.05 + 0.80 * tri;
        chRepeated->Set(lfo);
        for (int i = 0; i < block; ++i) in[static_cast<std::size_t>(i)] = Tone(static_cast<std::size_t>(b * block + i), 48000);
        chRepeated->Process(in.data(), in.data(), lB.data(), rB.data(), block);
        for (int i = 0; i < block; ++i) {
            if (b > 0 || i > 0) maxJump = Max(maxJump, Abs(lB[static_cast<std::size_t>(i)] - prevL));
            prevL = lB[static_cast<std::size_t>(i)];
        }
    }
    Expect(maxJump < 0.05f, "CvMixLfoSmooth", "batch-rate Mix LFO glides without FDN clicks", maxJump);
}

void StereoLinkedLimiterPreservesBalance() {
    auto ch = std::make_unique<Churech>();
    ch->Prepare(48000);
    Churech::Controls c;
    c.mix = 0.0;
    c.outputDb = 0.0;
    ch->Set(c);
    const int block = 64, batches = 200;
    std::vector<float> inL(block), inR(block), outL(block), outR(block);
    float peak = 0.0f;
    double pL = 0.0, pR = 0.0;
    for (int b = 0; b < batches; ++b) {
        for (int i = 0; i < block; ++i) {
            const double s = std::sin(kTwoPi * 330.0 * static_cast<double>(b * block + i) / 48000.0);
            inL[static_cast<std::size_t>(i)] = static_cast<float>(1.6 * s);
            inR[static_cast<std::size_t>(i)] = static_cast<float>(0.4 * s);
        }
        ch->Process(inL.data(), inR.data(), outL.data(), outR.data(), block);
        for (int i = 0; i < block; ++i) {
            peak = Max(peak, Max(Abs(outL[static_cast<std::size_t>(i)]), Abs(outR[static_cast<std::size_t>(i)])));
            if (b >= 40) {
                pL += static_cast<double>(outL[static_cast<std::size_t>(i)]) * outL[static_cast<std::size_t>(i)];
                pR += static_cast<double>(outR[static_cast<std::size_t>(i)]) * outR[static_cast<std::size_t>(i)];
            }
        }
    }
    const double ratio = std::sqrt(pL / (pR + 1.0e-30));
    Expect(peak < 1.0f && Abs(ratio - 4.0) < 0.04, "StereoLinkedLimiter",
           "peak limiter caps asymmetric +4 dBFS signal while preserving 4:1 L/R balance", ratio);
}

void BrightTrebleRelaxesAirCap() {
    Churech::Controls dark = Wet(), bright = Wet();
    dark.type = bright.type = Churech::kCathedral;
    dark.decayTreble = 0.2;
    bright.decayTreble = 0.6;
    const Take td = Play(dark, Burst, 2.2), tb = Play(bright, Burst, 2.2);
    auto hfTailRms = [](const Take& t) {
        double p = 0.0;
        const std::size_t a = t.At(1.0), b = t.At(2.0);
        for (std::size_t i = Max<std::size_t>(a, 2); i < b; ++i) {
            const double d2L = static_cast<double>(t.left[i]) - 2.0 * t.left[i - 1] + t.left[i - 2];
            const double d2R = static_cast<double>(t.right[i]) - 2.0 * t.right[i - 1] + t.right[i - 2];
            p += d2L * d2L + d2R * d2R;
        }
        return std::sqrt(p / static_cast<double>(Max<std::size_t>(b - a, 1)));
    };
    const double liftDb = Db(hfTailRms(tb) / hfTailRms(td));
    Expect(liftDb > 6.0, "BrightTrebleAir", "Cathedral 0.6x Treble retains > +6 dB late HF energy vs 0.2x, dB", liftDb);
}

void WideModulationDepthAtFull() {
    for (int type : { static_cast<int>(Churech::kChapel), static_cast<int>(Churech::kCathedral) }) {
        const int sr = (type == Churech::kChapel) ? 44100 : 48000;
        Churech::Controls normal = Wet(), wideMod = Wet();
        normal.type = wideMod.type = type;
        normal.modulation = 0.25;
        wideMod.modulation = 1.0;
        const Take tn = Play(normal, Burst, 2.0, sr), tw = Play(wideMod, Burst, 2.0, sr);
        double peak = 0.0, diffSq = 0.0, refSq = 0.0;
        const std::size_t a = tw.At(1.0), b = tw.At(1.8);
        for (std::size_t i = 0; i < tw.left.size(); ++i) {
            const float l = tw.left[i], r = tw.right[i];
            if (!(l == l && r == r)) peak = 999.0;
            peak = Max(peak, static_cast<double>(Max(Abs(l), Abs(r))));
        }
        for (std::size_t i = a; i < b; ++i) {
            const double dl = tw.left[i] - tn.left[i], dr = tw.right[i] - tn.right[i];
            diffSq += dl * dl + dr * dr;
            refSq += tn.left[i] * tn.left[i] + tn.right[i] * tn.right[i];
        }
        const double relDiff = std::sqrt(diffSq / (refSq + 1.0e-30));
        char what[80];
        std::snprintf(what, sizeof what, "type %d at %d Hz Mod 100%% is bounded and distinct vs 25%%", type, sr);
        Expect(peak < 1.0 && relDiff > 0.1, "WideModulation", what, relDiff);
    }
}

} // namespace

int main() {
    SilenceStaysSilent();
    DryIsTheSound();
    LargerRingsLonger();
    DistanceLowersTheSound();
    DuckDrawsTheChurchDown();
    CutsTakeWhatTheyCut();
    ChangesDoNotClick();
    BlocksAndRates();
    TrueStereoPreservesImage();
    EarlyReflectionsAreWideAndDense();
    LoftAndSeat();
    FreezeSidechainSafetyAndMemory();
    MonoCombFree();
    LiveLoftUpdatesGeometry();
    DistinctBuildingFdnSizes();
    NoCathedralReflectionPileup();
    DryPathPureAtZeroMix();
    SeatMonoSafeAndFastGlide();
    LivingFreezeAndIdleSleep();
    CvDirtyFlagMatchesFreshSet();
    StereoLinkedLimiterPreservesBalance();
    BrightTrebleRelaxesAirCap();
    WideModulationDepthAtFull();
    std::printf("%s: %d failure(s)\n", failures == 0 ? "CHURECH_PASS" : "CHURECH_FAIL", failures);
    return failures == 0 ? 0 : 1;
}
