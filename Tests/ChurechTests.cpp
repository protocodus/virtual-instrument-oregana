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
    Churech::Controls near, far;
    near.mix = far.mix = 0.0;
    far.distance = 1.0;
    const double drop = Db(Play(far, Tone, 0.5).Rms(0.1, 0.5) / Play(near, Tone, 0.5).Rms(0.1, 0.5));
    // 4.0 m against 19.5 m on the floor: 13.8 dB.
    Expect(drop < -13.0 && drop > -14.6, "Distance", "the furthest place is the sound 13.8 dB down", drop);
    // And the church's answer comes later from further away.
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
    std::printf("%s: %d failure(s)\n", failures == 0 ? "CHURECH_PASS" : "CHURECH_FAIL", failures);
    return failures == 0 ? 0 : 1;
}
