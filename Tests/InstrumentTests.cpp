// Instrument contracts: at 48 kHz the engine's own samples; at any other
// rate the same pitch and level, the stated latency and the same samples
// however the host cuts its blocks; the keyboard on the manual it is given;
// the Pedal from the lowest key or below the split; a key held by two
// sources; the mod wheel and the Crescendo control as the roller and
// expression as the swell pedal; and every key of the piano on every factory registration's keyboard.
#include "DSP/OreganaInstrument.h"
#include "DSP/OreganaPresets.h"
#include "../Tools/PipeMeasure.h"

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

// The organ on the bench (no church, no noises, a steady wind, every pipe as
// the tuner left it) behind the instrument.
struct Rig {
    std::unique_ptr<Instrument> organ = std::make_unique<Instrument>();
    Controls controls;
    double wheel = 0.0, expression = 1.0;
    std::vector<float> left, right;

    explicit Rig(double rate = 48000.0) {
        controls[kRoom] = 0.0;
        controls[kBlower] = controls[kAction] = 0.0;
        controls[kWind] = 0.0;
        controls[kTuning] = controls[kVoicing] = 0.0;
        controls[kPedalMode] = kPedalOff;
        organ->Prepare(rate);
        Set();
    }
    void Set() { organ->SetControls(controls, wheel, expression); }
    void Set(int control, double value) {
        controls[control] = value;
        Set();
    }
    // Plays on, and gives back the new stretch's level (both channels' power).
    double Play(double seconds, int block = 256) {
        const auto from = left.size(), frames = static_cast<std::size_t>(seconds * organ->SampleRate());
        left.resize(from + frames);
        right.resize(from + frames);
        for (std::size_t done = 0; done < frames;) {
            const auto now = Min(frames - done, static_cast<std::size_t>(block));
            organ->Render(&left[from + done], &right[from + done], static_cast<int>(now));
            done += now;
        }
        double sum = 0.0;
        for (std::size_t i = from; i < left.size(); ++i) sum += static_cast<double>(left[i]) * left[i] + static_cast<double>(right[i]) * right[i];
        return std::sqrt(sum / static_cast<double>(frames));
    }
    double Hz(double guess) const { return measure::Hz(left, left.size() / 2, left.size(), guess, organ->SampleRate()); }
};

constexpr double kSilent = 1.0e-5;

void EngineRateIsTheEngine() {
    Rig rig;
    auto engine = std::make_unique<Engine>();
    rig.Set(kHwOctave8, 1.0);
    engine->SetControls(rig.controls);
    rig.organ->KeyDown(60, 80);
    engine->KeyDown(kHauptwerk, 60, 80);
    rig.Play(0.5);
    std::vector<float> left(rig.left.size()), right(rig.left.size());
    engine->Render(left.data(), right.data(), static_cast<int>(left.size()));
    std::size_t differing = 0;
    for (std::size_t i = 0; i < left.size(); ++i) differing += left[i] != rig.left[i] || right[i] != rig.right[i];
    Expect(differing == 0, "EngineRateIsTheEngine", "samples that differ from the engine's at 48 kHz", static_cast<double>(differing));
    Expect(rig.organ->LatencySamples() == engine->LatencySamples(), "EngineRateIsTheEngine", "latency, samples", rig.organ->LatencySamples());
}

void OtherRates() {
    Rig reference;
    reference.Set(kHwOctave8, 1.0);
    reference.organ->KeyDown(69, 80);
    reference.Play(1.0);
    const double level = reference.Play(1.0), hz = reference.organ->Organ().PipeHz(1, 69);
    for (const double rate : { 44100.0, 88200.0, 96000.0, 192000.0 }) {
        Rig rig(rate);
        rig.Set(kHwOctave8, 1.0);
        rig.organ->KeyDown(69, 80);
        rig.Play(1.0);
        const double here = rig.Play(1.0);
        char what[64];
        std::snprintf(what, sizeof what, "a1 at %.0f Hz against its tuning, cents", rate);
        const double cents = 1200.0 * std::log2(rig.Hz(hz) / hz);
        Expect(Abs(cents) < 0.5, "OtherRates", what, cents);
        std::snprintf(what, sizeof what, "its level at %.0f Hz against 48 kHz, dB", rate);
        Expect(Abs(Db(here / level)) < 0.1, "OtherRates", what, Db(here / level));

        const int latency = rig.organ->LatencySamples();
        const int expected = static_cast<int>((reference.organ->LatencySamples() + 32) * rate / 48000.0 + 0.5);
        std::snprintf(what, sizeof what, "latency at %.0f Hz, samples", rate);
        Expect(latency == expected, "OtherRates", what, latency);
    }

    // The same render cut into other blocks is the same samples.
    Rig a(44100.0), b(44100.0);
    for (Rig* rig : { &a, &b }) {
        rig->Set(kHwOctave8, 1.0);
        rig->organ->KeyDown(60, 80);
    }
    a.Play(0.5, 64);
    b.Play(0.5, 37);
    std::size_t differing = 0;
    for (std::size_t i = 0; i < a.left.size(); ++i) differing += a.left[i] != b.left[i] || a.right[i] != b.right[i];
    Expect(differing == 0, "OtherRates", "samples that differ between blocks of 64 and 37 at 44.1 kHz", static_cast<double>(differing));

    // Resetting plays as new: as an organ just built with the stop drawn.
    Rig fresh(44100.0);
    fresh.Set(kHwOctave8, 1.0);
    for (Rig* rig : { &a, &fresh }) {
        rig->organ->Reset();
        rig->left.clear();
        rig->right.clear();
        rig->organ->KeyDown(60, 80);
        rig->Play(0.5, 37);
    }
    differing = 0;
    for (std::size_t i = 0; i < a.left.size(); ++i) differing += a.left[i] != fresh.left[i] || a.right[i] != fresh.right[i];
    Expect(differing == 0, "OtherRates", "samples that differ after a reset", static_cast<double>(differing));
}

void KeyboardPlaysItsManual() {
    Rig rig;
    rig.Set(kPosGedackt8, 1.0);
    rig.organ->KeyDown(60, 80);
    Expect(rig.Play(0.5) < kSilent, "KeyboardPlaysItsManual", "a Positiv stop from the Hauptwerk's keys", rig.Play(0.1));
    // The held key moves with the keyboard.
    rig.Set(kKeyboard, kPositiv);
    const double moved = rig.Play(0.5);
    Expect(moved > 100.0 * kSilent, "KeyboardPlaysItsManual", "the held key once the keyboard plays the Positiv", moved);
    rig.Set(kKeyboard, kRecit);
    rig.Play(1.0);
    const double gone = rig.Play(0.2);
    Expect(gone < kSilent, "KeyboardPlaysItsManual", "and once it plays the Recit", gone);
}

void PedalFromTheKeyboard() {
    Rig rig;
    rig.Set(kPedSubbass16, 1.0);
    const double hz = rig.organ->Organ().PipeHz(kStops[kPedSubbass16].firstRank, 43);

    rig.organ->KeyDown(60, 80);
    rig.organ->KeyDown(43, 80);
    Expect(rig.Play(0.5) < kSilent, "PedalFromTheKeyboard", "the Pedal with Pedal Keys off", rig.Play(0.1));

    rig.Set(kPedalMode, kPedalBass);
    rig.Play(1.0);
    rig.Play(1.0);
    const double cents = 1200.0 * std::log2(rig.Hz(hz) / hz);
    Expect(Abs(cents) < 2.0, "PedalFromTheKeyboard", "Bass: the Pedal's pipe of the lowest key against its tuning, cents", cents);
    // The lowest key rises: the next one takes the Pedal, if the Pedal has it.
    rig.organ->KeyUp(43);
    rig.Play(1.5);
    const double next = rig.Play(0.5);
    Expect(next > 100.0 * kSilent, "PedalFromTheKeyboard", "Bass: c1 once it is the lowest", next);
    rig.organ->KeyUp(60);
    rig.organ->KeyDown(72, 80);
    rig.Play(1.5);
    const double above = rig.Play(0.5);
    Expect(above < kSilent, "PedalFromTheKeyboard", "Bass: a key above the Pedal's compass", above);
    rig.organ->AllKeysUp();

    rig.Set(kPedalMode, kPedalSplit);
    rig.Set(kHwOctave8, 1.0);
    rig.Set(kPedSubbass16, 0.0);
    rig.Play(1.0);
    rig.organ->KeyDown(kSplitNote - 1, 80);
    const double below = rig.Play(0.5);
    Expect(below < kSilent, "PedalFromTheKeyboard", "Split: a key below the split on a manual stop", below);
    rig.organ->KeyDown(kSplitNote, 80);
    const double at = rig.Play(0.5);
    Expect(at > 100.0 * kSilent, "PedalFromTheKeyboard", "Split: the split's own key on it", at);
    rig.organ->AllKeysUp();
    rig.Set(kHwOctave8, 0.0);
    rig.Set(kPedSubbass16, 1.0);
    rig.Play(1.0);
    rig.organ->KeyDown(kSplitNote - 1, 80);
    const double pedal = rig.Play(0.5);
    Expect(pedal > 100.0 * kSilent, "PedalFromTheKeyboard", "Split: a key below the split on a Pedal stop", pedal);
}

void TwoSourcesHoldOneKey() {
    Rig rig;
    rig.Set(kHwOctave8, 1.0);
    rig.organ->KeyDown(60, 80, 0);
    rig.organ->KeyDown(60, 80, 1);
    rig.Play(0.5);
    rig.organ->KeyUp(60, 0);
    rig.Play(0.5);
    const double held = rig.Play(0.5);
    Expect(held > 100.0 * kSilent, "TwoSourcesHoldOneKey", "the key with the gate still holding it", held);
    rig.organ->KeyUp(60, 1);
    rig.Play(0.5);
    const double let = rig.Play(0.2);
    Expect(let < kSilent, "TwoSourcesHoldOneKey", "and once both have let go", let);
}

void WheelAndExpression() {
    Rig rig;
    rig.organ->KeyDown(60, 80);
    Expect(rig.Play(0.5) < kSilent, "WheelAndExpression", "no stop drawn and the wheel down", rig.Play(0.1));
    rig.wheel = 1.0;
    rig.Set();
    rig.Play(2.0);
    Expect(rig.organ->Organ().Diag().stage == kCrescendoStages && rig.Play(0.5) > kSilent, "WheelAndExpression",
           "the wheel up is the roller thrown over: its stage", rig.organ->Organ().Diag().stage);
    rig.wheel = 0.0;
    rig.Set();
    rig.Play(2.0);
    Expect(rig.organ->Organ().Diag().stage == 0, "WheelAndExpression", "and the wheel at rest takes it all off again",
           rig.organ->Organ().Diag().stage);
    rig.Set(kCrescendo, 1.0);
    rig.Play(2.0);
    Expect(rig.organ->Organ().Diag().stage == kCrescendoStages, "WheelAndExpression", "the Crescendo control up does the same",
           rig.organ->Organ().Diag().stage);
    rig.wheel = 0.5;
    rig.Set(kCrescendo, 0.0);
    rig.Play(2.0);
    const int half = rig.organ->Organ().Diag().stage;
    Expect(half > 0 && half < kCrescendoStages, "WheelAndExpression", "and the further of the two is the roller: the wheel half up", half);
    rig.wheel = 0.0;
    rig.Set();
    rig.Play(2.0);

    rig.Set(kRecPrincipal8, 1.0);
    rig.Set(kKeyboard, kRecit);
    rig.Play(1.0);
    const double open = rig.Play(0.5);
    rig.expression = 0.0;
    rig.Set();
    rig.Play(1.0);
    const double shut = rig.Play(0.5);
    Expect(Db(shut / open) < -6.0, "WheelAndExpression", "the Recit with expression down against up, dB", Db(shut / open));
    rig.expression = 1.0;
    rig.Set();
    rig.Play(1.0);
    const double again = rig.Play(0.5);
    Expect(std::abs(Db(again / open)) < 0.5, "WheelAndExpression", "expression up again opens the box, dB", Db(again / open));
}

void EveryKeySounds() {
    // As a front end's default patch plays: the first registration, the
    // church and all.
    auto organ = std::make_unique<Instrument>();
    organ->SetControls(kPresets[0].ToControls());
    std::vector<float> left(24000), right(24000);
    int silent = 0, strays = 0;
    for (int note = 0; note < 128; ++note) {
        organ->Reset();
        organ->KeyDown(note, 80);
        organ->Render(left.data(), right.data(), static_cast<int>(left.size()));
        double peak = 0.0;
        for (std::size_t i = 0; i < left.size(); ++i) peak = Max(peak, static_cast<double>(Max(Abs(left[i]), Abs(right[i]))));
        const bool piano = note >= 21 && note <= 108;
        if (piano && peak < 0.003) ++silent;
        if (!piano && peak > kSilent) ++strays;
    }
    Expect(silent == 0, "EveryKeySounds", "keys from A0 to C8 under -50 dBFS in half a second", silent);
    Expect(strays == 0, "EveryKeySounds", "keys outside them that sound", strays);
}

void DivisionOutputsAtEveryRate() {
    // The divisions' own outputs through the resampler: with the church
    // off they add up to the level the main output has, and taking them
    // leaves the main output's samples alone.
    for (const double rate : { 44100.0, 48000.0, 96000.0 }) {
        std::vector<float> mains[2];
        double own = 0.0, main = 0.0;
        for (int taken = 0; taken < 2; ++taken) {
            auto organ = std::make_unique<Instrument>();
            organ->Prepare(rate);
            Controls controls = kPresets[0].ToControls();
            controls[kRoom] = controls[kBlower] = controls[kAction] = controls[kSeat] = 0.0;
            organ->SetControls(controls);
            for (int note : { 48, 60, 64, 67 }) organ->KeyDown(note, 90);
            const int frames = static_cast<int>(rate);
            std::vector<float> left(static_cast<std::size_t>(frames)), right(left);
            std::vector<std::vector<float>> pairs(Engine::kDivisionOutputs, left);
            float* to[Engine::kDivisionOutputs];
            for (int done = 0; done < frames; done += 100) {
                for (int n = 0; n < Engine::kDivisionOutputs; ++n) to[n] = &pairs[static_cast<std::size_t>(n)][static_cast<std::size_t>(done)];
                organ->Render(&left[static_cast<std::size_t>(done)], &right[static_cast<std::size_t>(done)], Min(100, frames - done), taken ? to : nullptr);
            }
            mains[taken] = left;
            if (!taken) continue;
            for (int i = frames / 2; i < frames; ++i) {
                double sum = 0.0;
                for (int d = 0; d < kDivisions; ++d) sum += pairs[static_cast<std::size_t>(2 * d)][static_cast<std::size_t>(i)];
                own += sum * sum;
                main += static_cast<double>(left[static_cast<std::size_t>(i)]) * left[static_cast<std::size_t>(i)];
            }
        }
        Expect(mains[0] == mains[1], "DivisionOutputsAtEveryRate", "the main output is the same samples with them taken, at Hz", rate);
        Expect(own > 0.0 && std::abs(Db(std::sqrt(own / main))) < 1.5, "DivisionOutputsAtEveryRate", "together they are as loud as the main output with no church, dB",
               Db(std::sqrt(own / main)));
    }
}

} // namespace

int main() {
    EngineRateIsTheEngine();
    OtherRates();
    KeyboardPlaysItsManual();
    PedalFromTheKeyboard();
    TwoSourcesHoldOneKey();
    WheelAndExpression();
    EveryKeySounds();
    DivisionOutputsAtEveryRate();
    std::printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
