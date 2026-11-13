// Engine contracts: silence until a key is down and after the church has
// rung out, every pipe at the pitch it is tuned to at any Pitch, Temperament
// and Temperature, the regulated level, stops, couplers and the crescendo
// roller, the wind's sag and bounce, the tremulants, the swell box, the
// church's level, each imperfection and breakage, every key of the piano,
// the sliders and half-drawn stops, each rank's own moment, pitch and
// speech, the couplers' lost motion, the wind supply and pressure, the
// smooth crescendo, the church's size and decay in each band,
// block-partition invariance and determinism, and every control at its
// extremes.
#include "DSP/OreganaEngine.h"
#include "DSP/OreganaPresets.h"
#include "DSP/PipeRegulation.h"
#include "../Tools/PipeMeasure.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <vector>

namespace {

using namespace oregana;

int failures = 0;

void Expect(bool ok, const char* test, const char* what, double value) {
    std::printf("%s %s: %s (%.4g)\n", ok ? "PASS" : "FAIL", test, what, value);
    if (!ok) ++failures;
}

constexpr int kRate = 48000;

struct Take {
    std::vector<float> left, right;
    std::size_t Frames(double seconds) const { return Min(left.size(), static_cast<std::size_t>(seconds * kRate)); }
    // Both channels' power together.
    double Rms(double from, double to) const {
        double sum = 0.0;
        for (std::size_t i = Frames(from); i < Frames(to); ++i) sum += static_cast<double>(left[i]) * left[i] + static_cast<double>(right[i]) * right[i];
        return std::sqrt(sum / static_cast<double>(Max<std::size_t>(Frames(to) - Frames(from), 1)));
    }
    double Peak() const {
        float peak = 0.0f;
        for (std::size_t i = 0; i < left.size(); ++i) peak = Max(peak, Max(Abs(left[i]), Abs(right[i])));
        return peak;
    }
    // The sounding frequency of the take's second half, near `guess`.
    double Hz(double guess) const {
        std::vector<float> mid(left.size());
        for (std::size_t i = 0; i < mid.size(); ++i) mid[i] = left[i] + right[i];
        const double coarse = measure::CoarseHz(mid, mid.size() - static_cast<std::size_t>(12.0 * kRate / guess), mid.size(), guess, kRate);
        return measure::Hz(mid, mid.size() / 2, mid.size(), coarse, kRate);
    }
};

double Db(double x) { return 20.0 * std::log10(x + 1.0e-30); }
double CentsBetween(double hz, double reference) { return 1200.0 * std::log2(hz / reference); }

// The organ on the bench: no church, no noises, a steady wind and every pipe
// as the tuner and the voicer left it, so that one thing can be measured.
struct Bench {
    std::unique_ptr<Engine> organ = std::make_unique<Engine>();
    Controls controls;

    Bench() {
        controls[kRoom] = 0.0;
        controls[kBlower] = controls[kAction] = 0.0;
        controls[kWind] = 0.0;
        controls[kTuning] = controls[kVoicing] = 0.0;
        Set();
    }
    explicit Bench(const Controls& from) : controls(from) { Set(); }
    void Set() {
        organ->SetControls(controls);
        organ->Reset();
    }
    void Set(int control, double value) {
        controls[control] = value;
        organ->SetControls(controls);
    }
    void Draw(std::initializer_list<int> drawn) {
        for (const int control : drawn) controls[control] = 1.0;
        organ->SetControls(controls);
    }
    Take Play(double seconds, int block = 256) {
        Take take;
        const auto frames = static_cast<std::size_t>(seconds * kRate);
        take.left.assign(frames, 0.0f);
        take.right.assign(frames, 0.0f);
        for (std::size_t done = 0; done < frames;) {
            const auto now = Min(frames - done, static_cast<std::size_t>(block));
            organ->Render(&take.left[done], &take.right[done], static_cast<int>(now));
            done += now;
        }
        return take;
    }
};

void SilentUntilPlayed() {
    Controls controls;
    controls[kHwOctave8] = 1.0;
    Bench bench(controls);
    Expect(bench.Play(1.0).Peak() == 0.0, "SilentUntilPlayed", "an organ nobody plays is silent, blower and all", 0.0);
    bench.organ->KeyDown(kHauptwerk, 60, 80);
    Expect(bench.Play(1.0).Rms(0.5, 1.0) > 1.0e-3, "SilentUntilPlayed", "a key down sounds", 0.0);
    bench.organ->KeyUp(kHauptwerk, 60);
    // The church takes its decay to fall 60 dB, and the blower fades after it.
    bench.Play(40.0);
    const Take rest = bench.Play(60.0);
    Expect(rest.Rms(59.0, 60.0) == 0.0, "SilentUntilPlayed", "the organ comes to rest: exact silence", rest.Rms(59.0, 60.0));
    Expect(bench.organ->Diag().pipes == 0, "SilentUntilPlayed", "no pipe left ringing", bench.organ->Diag().pipes);
}

void PipesSoundAtTheirPitch() {
    // The last four stand beyond an organ's own compass: A0, C8, a trumpet
    // that repeats above c4 and the Pedal's A0.
    struct Pipe { int stop, rank, division, note; };
    const Pipe pipes[] = {
        { kHwOctave8, 1, kHauptwerk, 36 }, { kHwOctave8, 1, kHauptwerk, 60 }, { kHwOctave8, 1, kHauptwerk, 89 },
        { kHwRohrfloete8, 3, kHauptwerk, 67 }, { kHwTrompete8, 22, kHauptwerk, 60 }, { kPosGedackt8, 23, kPositiv, 53 },
        { kPosKrummhorn8, 37, kPositiv, 64 }, { kRecFluteHarmonique8, 41, kRecit, 72 }, { kRecGambe8, 42, kRecit, 56 },
        { kPedPrincipal16, 55, kPedal, 43 }, { kPedPosaune16, 66, kPedal, 48 },
        { kHwOctave8, 1, kHauptwerk, 21 }, { kHwOctave8, 1, kHauptwerk, 108 }, { kHwTrompete8, 22, kHauptwerk, 103 },
        { kPedOctave8, 57, kPedal, 21 },
    };
    struct Setting { const char* what; double pitch, temperament, temperature; };
    const Setting settings[] = {
        { "at 440 Hz, equal", 440.0, 0.0, 0.0 }, { "at 415 Hz", 415.0, 0.0, 0.0 }, { "at 466 Hz", 466.0, 0.0, 0.0 },
        { "in Werckmeister III", 440.0, 1.0, 0.0 }, { "in meantone at 415 Hz", 415.0, 4.0, 0.0 },
    };
    for (const Setting& setting : settings) {
        double worst = 0.0;
        for (const Pipe& pipe : pipes) {
            Bench bench;
            bench.Set(kPitch, setting.pitch);
            bench.Set(kTemperament, setting.temperament);
            bench.Draw({ pipe.stop });
            bench.organ->KeyDown(pipe.division, pipe.note, 80);
            const double tuned = bench.organ->PipeHz(pipe.rank, pipe.note);
            const double off = CentsBetween(bench.Play(Max(2.0, 400.0 / tuned)).Hz(tuned), tuned);
            if (Abs(off) > Abs(worst)) worst = off;
        }
        // The tuner works to half a cent at 440 Hz; a pipe made a semitone
        // away keeps its regulation within a cent and a half.
        Expect(Abs(worst) < 1.5, "PipesSoundAtTheirPitch", setting.what, worst);
    }
    // Temperament and Pitch are what they say: a1 and c1 of the Octave 8'.
    Bench bench;
    bench.Set(kTemperament, 1.0);
    bench.Set(kPitch, 415.0);
    Expect(Abs(CentsBetween(bench.organ->PipeHz(1, 69), 415.0)) < 1.0e-6, "PipesSoundAtTheirPitch", "a1 is the Pitch in any temperament",
           bench.organ->PipeHz(1, 69));
    Expect(Abs(CentsBetween(bench.organ->PipeHz(1, 60), 415.0 * std::pow(2.0, -9.0 / 12.0)) - 11.73) < 1.0e-6, "PipesSoundAtTheirPitch",
           "Werckmeister's c is 11.73 cents above equal", CentsBetween(bench.organ->PipeHz(1, 60), 415.0 * std::pow(2.0, -9.0 / 12.0)));
    // A mutation is tuned pure to its key.
    Expect(Abs(bench.organ->PipeHz(6, 60) / bench.organ->PipeHz(1, 60) - 3.0) < 1.0e-9, "PipesSoundAtTheirPitch", "the Quinte 2 2/3' is a pure twelfth",
           bench.organ->PipeHz(6, 60) / bench.organ->PipeHz(1, 60));
}

void LevelsAreRegulated() {
    Bench bench;
    bench.Draw({ kHwOctave8 });
    bench.organ->KeyDown(kHauptwerk, 60, 80);
    const double octave = Db(bench.Play(2.0).Rms(1.0, 2.0));
    // kMaster in OreganaEngine.cpp: 0.012, -38.4 dBFS.
    Expect(Abs(octave + 38.4) < 0.5, "LevelsAreRegulated", "the Octave 8' at c1 alone, dBFS", octave);

    Bench soft;
    soft.Draw({ kPosGedackt8 });
    soft.organ->KeyDown(kPositiv, 60, 80);
    const double gedackt = Db(soft.Play(2.0).Rms(1.0, 2.0));
    Expect(Abs(gedackt - octave + 8.0) < 0.7, "LevelsAreRegulated", "the Gedackt 8' is 8 dB under it", gedackt - octave);

    // Velocity is the key's speed, not a level.
    double levels[2];
    int at = 0;
    for (int velocity : { 20, 127 }) {
        Bench touch;
        touch.Draw({ kHwOctave8 });
        touch.organ->KeyDown(kHauptwerk, 60, velocity);
        levels[at++] = Db(touch.Play(2.0).Rms(1.0, 2.0));
    }
    Expect(Abs(levels[0] - levels[1]) < 0.2, "LevelsAreRegulated", "a soft and a hard stroke sound alike once speaking, dB apart", levels[0] - levels[1]);
}

void StopsCouplersAndCrescendo() {
    Bench bench;
    bench.organ->KeyDown(kHauptwerk, 60, 80);
    Expect(bench.Play(0.5).Peak() == 0.0, "StopsCouplersAndCrescendo", "a key with no stop drawn is silent", 0.0);
    bench.Draw({ kPosGedackt8 });
    Expect(bench.Play(0.5).Peak() == 0.0, "StopsCouplersAndCrescendo", "another manual's stop does not sound", 0.0);
    bench.Draw({ kCouplerPosHw });
    Expect(bench.Play(0.5).Rms(0.3, 0.5) > 1.0e-3, "StopsCouplersAndCrescendo", "until its coupler is drawn, on a held key", 0.0);
    Expect(bench.organ->Diag().pipes == 1, "StopsCouplersAndCrescendo", "one pipe speaks", bench.organ->Diag().pipes);
    bench.Draw({ kHwMixtur });
    bench.Play(0.1);
    Expect(bench.organ->Diag().pipes == 6, "StopsCouplersAndCrescendo", "a mixture of five ranks adds five", bench.organ->Diag().pipes);
    bench.Set(kHwMixtur, 0.0);
    bench.Set(kPosGedackt8, 0.0);
    bench.Play(3.0);
    Expect(bench.organ->Diag().pipes == 0, "StopsCouplersAndCrescendo", "stops pushed in fall silent under a held key", bench.organ->Diag().pipes);
    bench.organ->KeyUp(kHauptwerk, 60);

    // Couplers chain: the Pedal pulls the Hauptwerk, which pulls the Recit.
    bench.Draw({ kRecGambe8, kCouplerRecHw, kCouplerHwPed });
    bench.organ->KeyDown(kPedal, 48, 80);
    bench.Play(0.2);
    Expect(bench.organ->Diag().pipes == 1, "StopsCouplersAndCrescendo", "a Pedal key reaches the Recit through two couplers", bench.organ->Diag().pipes);
    bench.organ->AllKeysUp();
    bench.Play(3.0);

    // The roller only ever adds.
    Bench roller;
    roller.organ->KeyDown(kHauptwerk, 60, 80);
    roller.organ->KeyDown(kPedal, 36, 80);
    int before = 0;
    bool grows = true;
    for (int step = 0; step <= 127; step += 9) {
        const std::uint8_t wheel[3] = { 0xB0, 1, static_cast<std::uint8_t>(step) };
        roller.organ->Midi(wheel, 3);
        roller.Play(0.05);
        grows = grows && roller.organ->Diag().pipes >= before;
        before = roller.organ->Diag().pipes;
    }
    Expect(grows && before > 80, "StopsCouplersAndCrescendo", "the crescendo roller builds to full organ, pipes on two keys", before);
}

void WindSagsAndBounces() {
    Controls controls = kPresets[16].ToControls();   // Tutti
    controls[kRoom] = 0.0;
    Bench bench(controls);
    for (int note : { 36, 48, 55, 60, 64, 67, 72 }) bench.organ->KeyDown(kHauptwerk, note, 80);
    double lowest = 1.0;
    for (int i = 0; i < 150; ++i) {
        bench.Play(0.002);
        lowest = Min(lowest, bench.organ->Diag().pressure[kHauptwerk]);
    }
    bench.Play(2.0);
    const double held = bench.organ->Diag().pressure[kHauptwerk];
    Expect(lowest < held - 0.01 && lowest > 0.9, "WindSagsAndBounces", "a full chord's attack dips the wind", lowest);
    Expect(held < 0.995 && held > 0.96, "WindSagsAndBounces", "and it sags while the chord is held", held);

    bench.Set(kWind, 0.0);
    bench.Play(0.1);
    Expect(bench.organ->Diag().pressure[kHauptwerk] == 1.0, "WindSagsAndBounces", "Wind at 0 is a steady supply", bench.organ->Diag().pressure[kHauptwerk]);
}

// The level's swing over a held note, from 50 ms windows.
double Swing(const Take& take, double from, double to) {
    double low = 1.0e9, high = -1.0e9;
    for (double t = from; t + 0.05 <= to; t += 0.025) {
        const double level = Db(take.Rms(t, t + 0.05));
        low = Min(low, level);
        high = Max(high, level);
    }
    return high - low;
}

void TremulantBeats() {
    Bench bench;
    bench.Draw({ kPosGedackt8 });
    bench.organ->KeyDown(kPositiv, 60, 80);
    const double still = Swing(bench.Play(3.0), 1.0, 3.0);
    bench.Draw({ kTremulantPos });
    const double beating = Swing(bench.Play(3.0), 1.0, 3.0);
    Expect(still < 0.15, "TremulantBeats", "a held note is steady, dB", still);
    Expect(beating > 0.5 && beating < 3.0, "TremulantBeats", "and the tremulant swings its level, dB", beating);
}

void SwellBoxCloses() {
    double levels[2];
    int at = 0;
    for (double swell : { 1.0, 0.0 }) {
        Bench bench;
        bench.Draw({ kRecTrompette8 });
        bench.Set(kSwell, swell);
        bench.organ->Reset();
        bench.organ->KeyDown(kRecit, 60, 80);
        levels[at++] = Db(bench.Play(2.0).Rms(1.0, 2.0));
    }
    Expect(levels[0] - levels[1] > 8.0 && levels[0] - levels[1] < 24.0, "SwellBoxCloses", "the shut box takes 8 to 24 dB of a trumpet", levels[0] - levels[1]);

    // The pedal moves it under a held note, and only the Recit.
    Bench bench;
    bench.Draw({ kRecTrompette8, kHwOctave8 });
    bench.organ->KeyDown(kHauptwerk, 60, 80);
    const double open = Db(bench.Play(2.0).Rms(1.0, 2.0));
    const std::uint8_t pedal[3] = { 0xB0, 11, 0 };
    bench.organ->Midi(pedal, 3);
    Expect(Abs(Db(bench.Play(2.0).Rms(1.0, 2.0)) - open) < 0.2, "SwellBoxCloses", "the Hauptwerk does not hear the swell pedal", open);
}

void NaveLevel() {
    double levels[2];
    int at = 0;
    for (double room : { 0.0, 1.0 }) {
        Controls controls = kPresets[13].ToControls();   // Plenum
        controls[kVolume] = controls[kBlower] = controls[kAction] = 0.0;
        controls[kRoom] = room;
        Bench bench(controls);
        for (int note : { 36, 48, 55, 60, 64, 67, 72 }) bench.organ->KeyDown(kHauptwerk, note, 80);
        bench.organ->KeyDown(kPedal, 36, 80);
        levels[at++] = Db(bench.Play(8.0).Rms(6.0, 8.0));
    }
    Expect(Abs(levels[1] - levels[0]) < 1.5, "NaveLevel", "the church alone is as loud as a plenum chord alone, dB apart", levels[1] - levels[0]);

    // The decay is the Decay: a staccato chord's tail falls 30 dB in half of it.
    Controls controls = kPresets[13].ToControls();
    controls[kBlower] = controls[kAction] = 0.0;
    controls[kRoom] = 1.0;
    controls[kDecayBass] = 1.0;   // the bass no longer than the Decay itself, which this measures
    Bench bench(controls);
    for (int note : { 48, 55, 60 }) bench.organ->KeyDown(kHauptwerk, note, 80);
    bench.Play(1.0);
    bench.organ->AllKeysUp();
    const Take tail = bench.Play(4.0);
    const double fall = Db(tail.Rms(0.5, 0.75)) - Db(tail.Rms(2.5, 2.75));
    // 2 s of a 4.5 s decay is 26.7 dB up to the middle of the range and more above.
    Expect(fall > 24.0 && fall < 40.0, "NaveLevel", "the tail falls as the Decay says, dB in 2 s", fall);
}

void Imperfections() {
    // Tuning: every pipe off its pitch, flues within 10 cents and reeds within 20.
    {
        Bench bench;
        const double tuned = bench.organ->PipeHz(1, 60), reed = bench.organ->PipeHz(22, 60);
        bench.Set(kTuning, 1.0);
        double flueWorst = 0.0, reedWorst = 0.0, sum = 0.0;
        for (int note = 36; note <= 96; ++note) {
            const double off = CentsBetween(bench.organ->PipeHz(1, note), tuned * std::pow(2.0, (note - 60) / 12.0));
            flueWorst = Max(flueWorst, Abs(off));
            sum += off * off;
            reedWorst = Max(reedWorst, Abs(CentsBetween(bench.organ->PipeHz(22, note), reed * std::pow(2.0, (note - 60) / 12.0))));
        }
        Expect(flueWorst > 5.0 && flueWorst <= 10.0, "Imperfections", "Tuning at 1 puts flues up to 10 cents off", flueWorst);
        Expect(reedWorst > 10.0 && reedWorst <= 20.0, "Imperfections", "and reeds up to 20", reedWorst);
        Expect(std::sqrt(sum / 61.0) > 2.0, "Imperfections", "most of them audibly, RMS cents", std::sqrt(sum / 61.0));
        // And the pipe sounds where the engine says.
        bench.Draw({ kHwOctave8 });
        bench.organ->KeyDown(kHauptwerk, 66, 80);
        const double said = bench.organ->PipeHz(1, 66);
        Expect(Abs(CentsBetween(bench.Play(2.0).Hz(said), said)) < 1.5, "Imperfections", "a drifted pipe sounds at its drifted pitch", said);
    }
    // Temperature: flues follow the air, 2.95 cents a degree; reeds hold.
    {
        double flue[2], reed[2];
        int at = 0;
        for (double warm : { 0.0, 10.0 }) {
            Bench bench;
            bench.Set(kTemperature, warm);
            bench.Draw({ kHwOctave8 });
            bench.organ->KeyDown(kHauptwerk, 60, 80);
            flue[at] = bench.Play(2.0).Hz(261.6);
            Bench trumpet;
            trumpet.Set(kTemperature, warm);
            trumpet.Draw({ kHwTrompete8 });
            trumpet.organ->KeyDown(kHauptwerk, 60, 80);
            reed[at++] = trumpet.Play(2.0).Hz(261.6);
        }
        Expect(Abs(CentsBetween(flue[1], flue[0]) - 29.5) < 1.5, "Imperfections", "10 degrees warmer, a flue is 29.5 cents sharp", CentsBetween(flue[1], flue[0]));
        Expect(Abs(CentsBetween(reed[1], reed[0])) < 0.5, "Imperfections", "and a trumpet has not moved, cents", CentsBetween(reed[1], reed[0]));
    }
    // Voicing: the pipes of a rank no longer match.
    {
        double low = 1.0e9, high = -1.0e9;
        for (int note : { 56, 60, 63, 67, 71, 76 }) {
            Bench bench;
            bench.Set(kVoicing, 1.0);
            bench.Draw({ kHwOctave8 });
            bench.organ->KeyDown(kHauptwerk, note, 80);
            const double level = Db(bench.Play(1.5).Rms(1.0, 1.5));
            low = Min(low, level);
            high = Max(high, level);
        }
        Expect(high - low > 1.0 && high - low < 7.5, "Imperfections", "Voicing at 1 spreads six neighbours' levels, dB", high - low);
    }
    // Dead pipes: about one in sixteen is silent, and comes back.
    {
        Controls controls = kPresets[16].ToControls();
        Bench bench(controls);
        for (int note = 48; note < 72; ++note) bench.organ->KeyDown(kHauptwerk, note, 80);
        bench.Play(0.1);
        const int all = bench.organ->Diag().pipes;
        bench.Set(kDeadPipes, 1.0);
        bench.Play(4.0);
        const int left = bench.organ->Diag().pipes;
        const double share = 1.0 - static_cast<double>(left) / all;
        Expect(share > 0.03 && share < 0.1, "Imperfections", "Dead Pipes at 1 silences about one in sixteen, share", share);
        bench.Set(kDeadPipes, 0.0);
        bench.Play(0.1);
        Expect(bench.organ->Diag().pipes == all, "Imperfections", "and at 0 they all speak again", bench.organ->Diag().pipes);
    }
    // Ciphers: some pallets hang when their key is let go, until all sound is cut.
    {
        Bench bench;
        bench.Set(kCiphers, 1.0);
        bench.Draw({ kHwOctave8 });
        for (int note = kFirstNote; note < kFirstNote + kManualKeys; ++note) bench.organ->KeyDown(kHauptwerk, note, 80);
        bench.Play(0.2);
        bench.organ->AllKeysUp();
        bench.Play(4.0);
        const int hanging = bench.organ->Diag().pipes;
        Expect(hanging >= 1 && hanging <= 10, "Imperfections", "Ciphers at 1 leaves a few of 88 keys sounding", hanging);
        const std::uint8_t cut[3] = { 0xB0, 120, 0 };
        bench.organ->Midi(cut, 3);
        bench.Play(4.0);
        Expect(bench.organ->Diag().pipes == 0, "Imperfections", "until all sound is cut", bench.organ->Diag().pipes);
    }
    // Runnings: wind leaks into the channel a whole tone away.
    {
        Bench bench;
        bench.Set(kRunnings, 1.0);
        bench.Draw({ kHwOctave8 });
        int most = 0;
        // From c up: the pipes below it ring on for longer than the wait between two keys.
        for (int note = 48; note < kFirstNote + kManualKeys; ++note) {
            bench.organ->KeyDown(kHauptwerk, note, 80);
            bench.Play(0.05);
            most = Max(most, bench.organ->Diag().pipes);
            bench.organ->KeyUp(kHauptwerk, note);
            bench.Play(2.0);
        }
        Expect(most >= 2 && most <= 3, "Imperfections", "Runnings at 1 makes some keys' neighbours whisper along, pipes on one key", most);
        Expect(bench.organ->Diag().pipes == 0, "Imperfections", "and they stop with the key", bench.organ->Diag().pipes);
    }
    // The organ's own noises: a key's knock with no stop drawn, and the wind's.
    {
        Bench bench;
        bench.Set(kAction, 1.0);
        bench.organ->KeyDown(kHauptwerk, 60, 100);
        const double knock = bench.Play(0.2).Peak();
        Expect(knock > 1.0e-3 && knock < 0.05, "Imperfections", "Action: a key knocks with no stop drawn, peak", knock);
        Bench hum;
        hum.Set(kBlower, 1.0);
        hum.Draw({ kPosGedackt8 });
        hum.organ->KeyDown(kPositiv, 60, 80);
        hum.Play(3.0);
        hum.organ->KeyUp(kPositiv, 60);
        hum.Play(1.0);
        const double noise = Db(hum.Play(1.0).Rms(0.0, 1.0));
        Expect(noise > -60.0 && noise < -45.0, "Imperfections", "Blower: the wind is heard between the notes, dBFS", noise);
    }
}

// The action's and the wind's other noises, with no stop drawn.
void MechanismIsHeard() {
    const auto bench = [] {
        auto b = std::make_unique<Bench>();
        b->Set(kAction, 1.0);
        return b;
    };
    const auto crossings = [](const Take& take) {
        int count = 0;
        for (std::size_t i = 1; i < take.left.size(); ++i) count += (take.left[i] < 0.0f) != (take.left[i - 1] < 0.0f);
        return count;
    };
    // A coupled key knocks again at the chest it pulls, after the lost motion.
    auto alone = bench(), coupled = bench();
    coupled->Draw({ kCouplerPosHw, kCouplerRecHw });
    alone->organ->KeyDown(kHauptwerk, 60, 100);
    coupled->organ->KeyDown(kHauptwerk, 60, 100);
    const double one = alone->Play(0.1).Rms(0.008, 0.1), three = coupled->Play(0.1).Rms(0.008, 0.1);
    Expect(three > 1.1 * one, "MechanismIsHeard", "a key coupled to two manuals knocks at their chests too, against the key alone", three / one);
    // A pedal key lands lower than a manual key.
    auto manual = bench(), pedal = bench();
    manual->organ->KeyDown(kHauptwerk, 48, 100);
    pedal->organ->KeyDown(kPedal, 48, 100);
    const int high = crossings(manual->Play(0.05)), low = crossings(pedal->Play(0.05));
    Expect(low < high && low > 4, "MechanismIsHeard", "a pedal key lands lower than a manual key, zero crossings in 50 ms", low);
    // The shutters knock when the pedal is thrown shut, and hardly when it is eased shut.
    auto thrown = bench(), eased = bench();
    thrown->organ->SetSwell(0.0);
    const double thud = thrown->Play(0.3).Peak();
    double soft = 0.0;
    for (int step = 126; step >= 0; --step) {
        eased->organ->SetSwell(step / 127.0);
        soft = Max(soft, eased->Play(0.01).Peak());
    }
    soft = Max(soft, eased->Play(0.3).Peak());
    Expect(thud > 1.0e-3 && soft < 0.2 * thud && soft > 0.0, "MechanismIsHeard", "the shutters knock as hard as the pedal is thrown, eased against thrown", soft / thud);
    // A slider rubs across the chest while it travels, and no longer.
    auto drawn = bench();
    drawn->Play(0.1);
    drawn->Set(kAction, 1.0);
    drawn->controls[kHwOctave8] = drawn->controls[kHwOctave4] = drawn->controls[kHwMixtur] = 1.0;
    drawn->Set(kVolume, drawn->controls[kVolume]);
    const Take rub = drawn->Play(1.0);
    Expect(Db(rub.Rms(0.06, 0.1)) > -70.0 && rub.Rms(0.6, 1.0) == 0.0, "MechanismIsHeard", "sliders rub while they travel, dBFS", Db(rub.Rms(0.06, 0.1)));
    // The tremulant's valve thumps once a beat while the organ is played.
    Bench wind;
    wind.Set(kBlower, 0.0);
    wind.Draw({ kPosGedackt8, kTremulantPos });
    wind.organ->KeyDown(kPositiv, 60, 80);
    const Take smooth = wind.Play(3.0);
    Bench beaten;
    beaten.Set(kBlower, 1.0);
    beaten.Draw({ kPosGedackt8, kTremulantPos });
    beaten.organ->KeyDown(kPositiv, 60, 80);
    const Take thumped = beaten.Play(3.0);
    double differs = 0.0;
    for (std::size_t i = 0; i < smooth.left.size(); ++i) differs = Max(differs, static_cast<double>(Abs(thumped.left[i] - smooth.left[i])));
    Expect(differs > 1.0e-3, "MechanismIsHeard", "the tremulant's valve is heard with the wind's noise, peak", differs);
    // The blower is heard coming up to speed, and falls silent when nobody plays.
    Bench blower;
    blower.Set(kBlower, 1.0);
    blower.Set(kWindSupply, kWindOff);
    blower.Play(30.0);
    Expect(blower.Play(1.0).Peak() == 0.0, "MechanismIsHeard", "a blower run down is silent", 0.0);
    blower.Set(kWindSupply, kWindBlower);
    const double start = Db(blower.Play(2.0).Rms(0.5, 2.0));
    blower.Play(40.0);
    Expect(start > -75.0 && blower.Play(1.0).Peak() == 0.0, "MechanismIsHeard", "the blower is heard starting and then leaves an unplayed organ silent, dBFS", start);
}

// Four pipes stepped together (RunFour) are the bits of four stepped apart:
// open flues, flues with a chimney or a harmonic pipe's upper half, and reeds,
// through speech, a sagging wind and the ring after the pallet shuts.
void FourPipesAreFourPipes() {
#if defined(OREGANA_FOUR)
    int rankFirst[kRankCount + 1] = {};
    for (int r = 0; r < kRankCount; ++r) rankFirst[r + 1] = rankFirst[r] + kDivisionKeys[RankDivision(r)];
    int compared[3] = {};
    bool same = true;
    for (int rank = 0; rank < kRankCount; ++rank) {
        const int keys[4] = { 15, 27, 44, 63 };
        std::vector<std::vector<float>> room(8, std::vector<float>(3 * 8192)), sound(8, std::vector<float>(32));
        FluePipe flue[8];
        ReedPipe reed[8];
        bool exists = true, isReed = false;
        for (int l = 0; l < 8; ++l) {
            const int key = Min(keys[l % 4], kDivisionKeys[RankDivision(rank)] - 1 - (l % 4));
            const PipePlan plan = PlanPipe(rank, key);
            exists = exists && plan.exists;
            if (!plan.exists) break;
            isReed = plan.reed;
            const double tuning = kPipeRegulation[rankFirst[rank] + key].tuning;
            if (plan.reed) {
                reed[l].loop = room[static_cast<std::size_t>(l)].data();
                reed[l].Design(plan.tongue, plan.hz, tuning);
                reed[l].Clear();
                reed[l].random = static_cast<std::uint32_t>(77 + l % 4);
            } else {
                flue[l].loop = room[static_cast<std::size_t>(l)].data();
                flue[l].jet = flue[l].loop + 8192;
                flue[l].upper = flue[l].loop + 2 * 8192;
                flue[l].Design(plan.flue, plan.hz, tuning);
                flue[l].Clear();
                flue[l].random = static_cast<std::uint32_t>(77 + l % 4);
            }
        }
        if (!exists) continue;
        const int kind = isReed ? 2 : flue[0].upperLength > 0 ? 1 : 0;
        // A harmonic flute is harmonic from its middle up only: the engine
        // never steps the two kinds together, and this rank's four are both.
        bool mixed = false;
        for (int l = 1; l < 4; ++l) mixed = mixed || (!isReed && (flue[l].upperLength > 0) != (flue[0].upperLength > 0));
        if (mixed) continue;
        ++compared[kind];
        FluePipe* flues[4] = { &flue[4], &flue[5], &flue[6], &flue[7] };
        ReedPipe* reeds[4] = { &reed[4], &reed[5], &reed[6], &reed[7] };
        float* out[4] = { sound[4].data(), sound[5].data(), sound[6].data(), sound[7].data() };
        for (int tick = 0; tick < 1500 && same; ++tick) {
            const float wind = tick < 600 ? 1.0f : tick < 900 ? 0.55f : tick < 1000 ? 1.3f : 0.0f;
            // The air's bend swings both ways from tick 300 and is back at rest by 1200.
            const float stretch = tick < 300 || tick >= 1200 ? 0.0f : 0.017f * static_cast<float>(std::sin((tick - 300) * 0.02));
            const int frames = 1 + (tick * 7) % 32;
            for (int l = 0; l < 4; ++l) {
                if (isReed) {
                    for (ReedPipe* pipe : { &reed[l], &reed[l + 4] }) {
                        pipe->wind = wind;
                        pipe->Tick();
                    }
                    reed[l].Run(sound[static_cast<std::size_t>(l)].data(), frames);
                } else {
                    for (FluePipe* pipe : { &flue[l], &flue[l + 4] }) {
                        pipe->wind = wind;
                        pipe->Tick(stretch);
                    }
                    flue[l].Run(sound[static_cast<std::size_t>(l)].data(), frames);
                }
            }
            if (isReed) ReedPipe::RunFour(reeds, out, frames);
            else if (kind == 1) FluePipe::RunFour<true, false>(flues, out, frames);
            else FluePipe::RunFour<false, false>(flues, out, frames);
            for (std::size_t l = 0; l < 4; ++l)
                same = same && std::memcmp(sound[l].data(), sound[l + 4].data(), static_cast<std::size_t>(frames) * sizeof(float)) == 0;
        }
        for (int l = 0; l < 4 && same; ++l)
            same = isReed ? reed[l].tongue == reed[l + 4].tongue && reed[l].foot == reed[l + 4].foot && reed[l].random == reed[l + 4].random
                          : flue[l].jetTransit == flue[l + 4].jetTransit && flue[l].foot == flue[l + 4].foot && flue[l].random == flue[l + 4].random
                                && flue[l].jetAt == flue[l + 4].jetAt && flue[l].bendNow == 0.0f && flue[l + 4].bendNow == 0.0f;
        if (!same) {
            std::printf("  rank %d differs (kind %d)\n", rank, kind);
            break;
        }
    }
    Expect(same && compared[0] > 10 && compared[1] > 0 && compared[2] > 5, "FourPipesAreFourPipes",
           "every rank's pipes give the same samples four at a time as one at a time, ranks compared", compared[0] + compared[1] + compared[2]);
#endif
}

// When a take first sounds, ms.
double FirstSound(const Take& take) {
    for (std::size_t i = 0; i < take.left.size(); ++i)
        if (take.left[i] != 0.0f || take.right[i] != 0.0f) return 1000.0 * static_cast<double>(i) / kRate;
    return -1.0;
}

// When a take's level first comes within 6 dB of where it settles, ms.
double HalfWay(const Take& take) {
    const double seconds = static_cast<double>(take.left.size()) / kRate, settled = take.Rms(0.75 * seconds, seconds);
    // To a tenth of a millisecond: at whole milliseconds two of StopsSlide's
    // nine stops can fall on the same one (the Rohrfloete 8' at 187.8 ms and
    // the Gedackt 8' at 187.9 did once the pipes' turbulence changed).
    for (double t = 0.0; t + 0.01 <= seconds; t += 0.0001)
        if (take.Rms(t, t + 0.01) > 0.5 * settled) return 1000.0 * t;
    return -1.0;
}

// How long after a key goes down its first pipe takes wind, ms, to one tick.
double PalletOpens(Bench& bench, int division, int note, int velocity) {
    float left[FluePipe::kTick], right[FluePipe::kTick];
    bench.organ->KeyDown(division, note, velocity);
    for (int ticks = 0; ticks < 4000; ++ticks) {
        if (bench.organ->Diag().pipes > 0) return 1000.0 * ticks * FluePipe::kTick / kRate;
        bench.organ->Render(left, right, FluePipe::kTick);
    }
    return -1.0;
}

void EveryKeySounds() {
    // The keyboards are a piano's 88 keys and the Pedal runs from the same A
    // to g1: each sounds and no other does.
    struct Board { const char* what; int division, stop, rank, last; };
    const Board boards[] = {
        { "every manual key from A0 to C8 sounds and no other, wrong keys", kHauptwerk, kHwOctave8, 1, 108 },
        { "every Pedal key from A0 to g1 sounds and no other, wrong keys", kPedal, kPedOctave8, 57, 67 },
    };
    for (const Board& board : boards) {
        Bench bench;
        bench.Draw({ board.stop });
        int wrong = 0;
        double quietest = 0.0;
        for (int note = 0; note < 128; ++note) {
            bench.organ->Reset();
            bench.organ->KeyDown(board.division, note, 80);
            const bool key = note >= kFirstNote && note <= board.last;
            const double seconds = key ? Max(0.3, 40.0 / bench.organ->PipeHz(board.rank, note)) : 0.1;
            const double level = Db(bench.Play(seconds).Rms(0.5 * seconds, seconds));
            if (key) quietest = Min(quietest, level);
            if (key != (level > -60.0)) ++wrong;
        }
        Expect(wrong == 0, "EveryKeySounds", board.what, wrong);
        Expect(quietest > -50.0, "EveryKeySounds", "the quietest of them, dBFS", quietest);
    }
    // No pipe is longer than a 32' one: a 16' stop has none below the piano's C1.
    Bench bench;
    Expect(bench.organ->PipeHz(0, 23) == 0.0 && bench.organ->PipeHz(0, 24) > 16.0, "EveryKeySounds", "the Principal 16' starts at C1, Hz",
           bench.organ->PipeHz(0, 24));
}

void StopsSlide() {
    // A stop drawn under a held key comes on as its slider crosses the
    // channels, not at once, and each knob takes its own time.
    const int stops[] = { kHwOctave8, kHwRohrfloete8, kHwGamba8, kHwOctave4, kHwSpitzfloete4, kHwSuperoctave2, kHwTrompete8, kPosGedackt8, kPosPrincipal4 };
    std::vector<double> times;
    for (const int stop : stops) {
        Bench bench;
        const int division = stop >= kPosGedackt8 ? kPositiv : kHauptwerk;
        bench.organ->KeyDown(division, 60, 80);
        bench.Play(0.1);
        bench.Draw({ stop });
        times.push_back(HalfWay(bench.Play(1.5)));
    }
    double least = times[0], most = times[0];
    int alike = 0;
    for (std::size_t i = 0; i < times.size(); ++i) {
        least = Min(least, times[i]);
        most = Max(most, times[i]);
        for (std::size_t j = 0; j < i; ++j)
            if (times[i] == times[j]) ++alike;
    }
    Expect(least > 30.0 && most < 400.0, "StopsSlide", "a stop drawn under a held key takes its slider's travel to come on, ms at least", least);
    Expect(most - least > 30.0 && alike == 0, "StopsSlide", "and no two of nine stops come on alike, ms between the first and the last", most - least);

    // Half drawn, a stop starves its pipes: flat, soft and steady there.
    Bench full, half;
    full.Draw({ kHwOctave8 });
    half.Set(kHwOctave8, 0.5);
    full.organ->KeyDown(kHauptwerk, 60, 80);
    half.organ->KeyDown(kHauptwerk, 60, 80);
    const Take whole = full.Play(3.0), starved = half.Play(3.0);
    const double tuned = full.organ->PipeHz(1, 60);
    const double softer = Db(whole.Rms(2.0, 3.0)) - Db(starved.Rms(2.0, 3.0)), flat = CentsBetween(starved.Hz(tuned), whole.Hz(tuned));
    Expect(softer > 1.0 && softer < 12.0, "StopsSlide", "a half-drawn stop is softer, dB", softer);
    Expect(flat < -3.0 && flat > -60.0, "StopsSlide", "and flat, cents", flat);
    Expect(Abs(Db(starved.Rms(1.0, 2.0)) - Db(starved.Rms(2.0, 3.0))) < 0.2, "StopsSlide", "and stays so, dB between two seconds",
           Db(starved.Rms(1.0, 2.0)) - Db(starved.Rms(2.0, 3.0)));
}

void RegistersSpeakApart() {
    // The organ as it ships (a season after its tuning, voiced by hand), dry:
    // each rank of one key is heard from its own place, a little off its
    // pitch, at its own level and with its own speech.
    Controls controls;
    controls[kRoom] = controls[kBlower] = controls[kAction] = 0.0;
    struct Rank { int stop; double multiple; };
    const Rank ranks[] = {
        { kHwOctave8, 1.0 }, { kHwRohrfloete8, 1.0 }, { kHwGamba8, 1.0 }, { kHwOctave4, 2.0 },
        { kHwSuperoctave2, 4.0 }, { kHwTrompete8, 1.0 }, { kPosGedackt8, 1.0 }, { kRecGambe8, 1.0 },
    };
    const double c1 = 440.0 * std::pow(2.0, -9.0 / 12.0);
    std::vector<double> heard, cents, speech;
    for (const Rank& rank : ranks) {
        Controls drawn = controls;
        drawn[rank.stop] = 1.0;
        Bench bench(drawn);
        bench.organ->KeyDown(kStops[rank.stop].division, 60, 80);
        const Take take = bench.Play(2.0);
        heard.push_back(FirstSound(take));
        cents.push_back(CentsBetween(bench.organ->PipeHz(kStops[rank.stop].firstRank, 60), c1 * rank.multiple));
        speech.push_back(HalfWay(take) - heard.back());
        std::printf("     %-18s heard after %5.2f ms, %+5.2f cents, speaks in %5.1f ms\n", kStops[rank.stop].name, heard.back(), cents.back(), speech.back());
    }
    const auto apart = [](const std::vector<double>& values) {
        double least = 1.0e9;
        for (std::size_t i = 0; i < values.size(); ++i)
            for (std::size_t j = 0; j < i; ++j) least = Min(least, Abs(values[i] - values[j]));
        return least;
    };
    const auto spread = [](const std::vector<double>& values) {
        double least = values[0], most = values[0];
        for (const double value : values) {
            least = Min(least, value);
            most = Max(most, value);
        }
        return most - least;
    };
    Expect(apart(heard) > 0.0 && spread(heard) > 3.0, "RegistersSpeakApart", "eight ranks of one key are heard at eight moments, ms from first to last", spread(heard));
    Expect(apart(cents) > 0.0 && spread(cents) > 1.0 && spread(cents) < 10.0, "RegistersSpeakApart", "each a little off its pitch, cents from flattest to sharpest", spread(cents));
    Expect(apart(speech) > 0.0 && spread(speech) > 5.0, "RegistersSpeakApart", "and each takes its own time to speak, ms from quickest to slowest", spread(speech));

    // The divisions stand apart: the Recit's box is behind the Positiv.
    Bench bench;
    bench.Draw({ kPosGedackt8, kRecGambe8 });
    bench.organ->Reset();
    bench.organ->KeyDown(kPositiv, 60, 80);
    const double front = FirstSound(bench.Play(0.2));
    bench.organ->Reset();
    bench.organ->KeyDown(kRecit, 60, 80);
    const double behind = FirstSound(bench.Play(0.2));
    Expect(behind - front > 4.0 && behind - front < 14.0, "RegistersSpeakApart", "the Recit is heard later than the Positiv in front of it, ms", behind - front);
}

void CouplersPullLate() {
    // A new action couples exactly; a worn one's lost motion makes the
    // coupled manual late, each key by its own amount and more for a slow key.
    const auto late = [](double action, int note, int velocity) {
        Bench direct, coupled;
        for (Bench* bench : { &direct, &coupled }) {
            bench->Set(kAction, action);
            bench->Draw({ kPosGedackt8, kCouplerPosHw });
            bench->organ->Reset();   // the slider home before the key goes down
        }
        return PalletOpens(coupled, kHauptwerk, note, velocity) - PalletOpens(direct, kPositiv, note, velocity);
    };
    Expect(late(0.0, 60, 80) == 0.0, "CouplersPullLate", "with Action at 0 a coupled manual speaks with its own key, ms late", late(0.0, 60, 80));
    const double worn = late(1.0, 60, 80);
    Expect(worn > 5.0 && worn < 25.0, "CouplersPullLate", "a worn action couples late, ms", worn);
    Expect(late(1.0, 62, 80) != worn && late(1.0, 64, 80) != worn, "CouplersPullLate", "each key by its own lost motion, ms on the next", late(1.0, 62, 80));
    Expect(late(1.0, 60, 15) > late(1.0, 60, 127) + 3.0, "CouplersPullLate", "and later for a slow key than a fast one, ms", late(1.0, 60, 15) - late(1.0, 60, 127));
    Expect(Abs(late(0.25, 60, 80) - 0.25 * worn) < 1.0, "CouplersPullLate", "at the default Action a quarter of that, ms", late(0.25, 60, 80));
}

void WindSupplies() {
    // The blower switched off: the reservoir runs down and the pipe dies,
    // flat on its way; switched on, the wind and the pipe come back.
    Bench bench;
    bench.Draw({ kHwOctave8 });
    bench.organ->KeyDown(kHauptwerk, 60, 80);
    const Take sounding = bench.Play(2.0);
    const double steady = Db(sounding.Rms(1.0, 2.0)), tuned = bench.organ->PipeHz(1, 60);
    bench.Set(kWindSupply, kWindOff);
    double sagged = 0.0, weakened = 0.0;
    for (int i = 0; i < 200 && bench.organ->Diag().supply > 0.7; ++i) {
        const Take sinking = bench.Play(0.1);
        sagged = CentsBetween(sinking.Hz(tuned), tuned);
        weakened = Db(sinking.Rms(0.0, 0.1)) - steady;
    }
    Expect(bench.organ->Diag().supply <= 0.7 && sagged < -3.0 && weakened < -1.0 && weakened > -10.0, "WindSupplies",
           "with the blower off the wind runs down and a held pipe goes flat and weak, cents", sagged);
    const Take dying = bench.Play(8.0);
    Expect(bench.organ->Diag().supply == 0.0 && dying.Rms(7.0, 8.0) == 0.0 && bench.organ->Diag().pipes == 0, "WindSupplies",
           "until there is none and the organ is silent under a held key, supply", bench.organ->Diag().supply);
    bench.Set(kWindSupply, kWindBlower);
    const double back = Db(bench.Play(5.0).Rms(4.0, 5.0));
    Expect(Abs(back - steady) < 0.3 && bench.organ->Diag().supply == 1.0, "WindSupplies", "the blower brings the held pipe back, dB from before", back - steady);

    // Hand-pumped: full organ empties a bellows every few seconds, and each
    // changeover is a dip in the wind.
    Controls controls = kPresets[16].ToControls();   // Tutti
    controls[kRoom] = 0.0;
    controls[kWindSupply] = kWindBellows;
    Bench pumped(controls);
    for (int note : { 36, 48, 55, 60, 64, 67, 72 }) pumped.organ->KeyDown(kHauptwerk, note, 80);
    pumped.organ->KeyDown(kPedal, 36, 80);
    pumped.Play(2.0);
    int turns = 0;
    double supply = pumped.organ->Diag().supply, deepest = 0.0;
    for (int i = 0; i < 3000; ++i) {
        const double pressure = pumped.organ->Diag().pressure[kHauptwerk];
        pumped.Play(0.01);
        // The supply droops as a bellows falls and is whole again with the next.
        if (pumped.organ->Diag().supply > supply + 0.01) {
            ++turns;
            deepest = Max(deepest, pressure - pumped.organ->Diag().pressure[kHauptwerk]);
        }
        supply = pumped.organ->Diag().supply;
    }
    Expect(turns >= 2 && turns <= 8, "WindSupplies", "hand-pumped, a full chord has the calcant change bellows, times in 30 s", turns);
    Expect(deepest > 0.01 && deepest < 0.08, "WindSupplies", "and each changeover dips the wind, of the pressure", deepest);
    pumped.organ->AllKeysUp();
    pumped.Play(3.0);
    const double before = pumped.organ->Diag().pressure[kHauptwerk];
    pumped.Play(10.0);
    Expect(Abs(pumped.organ->Diag().pressure[kHauptwerk] - before) < 0.01, "WindSupplies", "and nobody pumps while nobody plays, pressure moved", pumped.organ->Diag().pressure[kHauptwerk] - before);
}

void PressureBendsPitch() {
    double level[3], cents[3];
    int at = 0;
    for (double pressure : { 0.6, 1.0, 1.5 }) {
        Bench bench;
        bench.Set(kPressure, pressure);
        bench.Draw({ kHwOctave8 });
        bench.organ->KeyDown(kHauptwerk, 60, 80);
        const Take take = bench.Play(3.0);
        level[at] = Db(take.Rms(2.0, 3.0));
        cents[at++] = CentsBetween(take.Hz(bench.organ->PipeHz(1, 60)), bench.organ->PipeHz(1, 60));
        Expect(bench.organ->Diag().supply == pressure, "PressureBendsPitch", "the supply holds the Wind Pressure", bench.organ->Diag().supply);
    }
    Expect(cents[0] < -5.0 && cents[0] > -80.0, "PressureBendsPitch", "on the least wind a flue is flat, cents", cents[0]);
    Expect(level[0] < level[1] - 2.0 && level[0] > level[1] - 10.0, "PressureBendsPitch", "and soft, but it speaks, dB", level[0] - level[1]);
    Expect(cents[2] > 3.0 && cents[2] < 60.0 && level[2] > level[1] + 1.0, "PressureBendsPitch", "on half as much again it is sharp and loud, cents", cents[2]);
}

// How far a held note's level wanders, dB RMS over 200 ms windows: slower than
// the turbulence in its tone.
double Wander(const Take& take, double from, double to) {
    double sum = 0.0, squares = 0.0;
    int count = 0;
    for (double t = from; t + 0.2 <= to; t += 0.2, ++count) {
        const double level = Db(take.Rms(t, t + 0.2));
        sum += level;
        squares += level * level;
    }
    return std::sqrt(Max(squares / count - (sum / count) * (sum / count), 0.0));
}

void FluesWaver() {
    double wander[2];
    int at = 0;
    for (double wind : { 0.0, 0.5 }) {
        Bench bench;
        bench.Set(kWind, wind);
        bench.Draw({ kHwOctave8 });
        bench.organ->KeyDown(kHauptwerk, 60, 80);
        wander[at++] = Wander(bench.Play(9.0), 1.0, 9.0);
    }
    Expect(wander[1] > 0.03 && wander[1] < 0.3, "FluesWaver", "a held flue's level wanders as a recorded one's, dB RMS", wander[1]);
    Expect(wander[0] < 0.5 * wander[1], "FluesWaver", "and with Wind at 0 it is the turbulence's alone", wander[0]);
}

// How far a held note's pitch wanders from one half second to the next, cents RMS.
double Drift(const Take& take, double hz, double from, double to) {
    std::vector<float> mid(take.left.size());
    for (std::size_t i = 0; i < mid.size(); ++i) mid[i] = take.left[i] + take.right[i];
    double sum = 0.0, squares = 0.0;
    int count = 0;
    for (double t = from; t + 0.5 <= to; t += 0.5, ++count) {
        const double cents = CentsBetween(measure::Hz(mid, take.Frames(t), take.Frames(t + 0.5), hz, kRate), hz);
        sum += cents;
        squares += cents * cents;
    }
    return std::sqrt(Max(squares / count - (sum / count) * (sum / count), 0.0));
}

void ReedsWaver() {
    double drift[2];
    int at = 0;
    for (double wind : { 0.0, 0.5 }) {
        Bench bench;
        bench.Set(kWind, wind);
        bench.Draw({ kHwTrompete8 });
        bench.organ->KeyDown(kHauptwerk, 60, 80);
        const Take take = bench.Play(9.0);
        drift[at++] = Drift(take, take.Hz(261.6), 1.0, 9.0);
    }
    Expect(drift[1] > 0.08 && drift[1] < 1.0, "ReedsWaver", "a held reed's pitch walks as a recorded one's, cents RMS between half seconds", drift[1]);
    Expect(drift[0] < 0.25 * drift[1], "ReedsWaver", "and with Wind at 0 it stands", drift[0]);
}

// A key struck again before its pipe's foot has emptied (a repeated note, or
// a sequencer's note-off and note-on a tick apart) refills a foot that is
// part full. The puff of that filling begins from nothing: until 0.5.0 it
// began with a step, a click some twenty times the pipe's steepest sample,
// and ended with one where the wind went in mid-puff.
void RepeatedKeysDoNotClick() {
    const auto steepest = [](const Take& take) {
        double most = 0.0;
        for (std::size_t n = 1; n < take.left.size(); ++n) most = Max(most, Abs(static_cast<double>(take.left[n]) - take.left[n - 1]));
        return most;
    };
    double worst = 0.0, brief = 0.0;
    for (const int stop : { kPosGedackt8, kHwOctave8 }) {
        Bench bench;
        bench.Draw({ stop });
        const int division = stop == kPosGedackt8 ? kPositiv : kHauptwerk;
        bench.organ->KeyDown(division, 60, 100);
        bench.Play(1.0);
        const double held = steepest(bench.Play(0.5));
        for (const double gap : { 0.002, 0.005, 0.01, 0.02 }) {
            bench.organ->KeyUp(division, 60);
            const double falling = steepest(bench.Play(gap, 48));
            bench.organ->KeyDown(division, 60, 100);
            worst = Max(worst, Max(falling, steepest(bench.Play(0.3, 48))) / held);
        }
        // A note shorter than its foot takes to fill: the wind goes in mid-puff.
        bench.organ->KeyUp(division, 60);
        bench.Play(1.0);
        for (const double length : { 0.001, 0.003, 0.006 }) {
            bench.organ->KeyDown(division, 60, 100);
            const double rising = steepest(bench.Play(length, 48));
            bench.organ->KeyUp(division, 60);
            brief = Max(brief, Max(rising, steepest(bench.Play(0.5, 48))) / held);
        }
    }
    Expect(worst < 2.0, "RepeatedKeysDoNotClick", "a key struck again within 20 ms steps no further than its held sound, times", worst);
    Expect(brief < 2.0, "RepeatedKeysDoNotClick", "nor does a note of 1 to 6 ms, times", brief);
}

void QuietRegistrationDoesNotLiftTheLoud() {
    // Tutti to Voix celeste under a held chord: the quiet registration's
    // Volume is 10 dB higher, and it must not arrive before Tutti's stops
    // have gone in.
    Controls controls = kPresets[16].ToControls();   // Tutti
    controls[kBlower] = controls[kAction] = 0.0;
    Bench bench(controls);
    for (int note : { 43, 55, 60, 64, 67 }) bench.organ->KeyDown(kHauptwerk, note, 100);
    bench.Play(1.5);
    const double loud = bench.Play(0.5).Peak();
    controls = kPresets[4].ToControls();   // Voix celeste
    controls[kBlower] = controls[kAction] = 0.0;
    bench.organ->SetControls(controls);
    const double change = bench.Play(0.5).Peak();
    Expect(change < 1.3 * loud, "QuietRegistrationDoesNotLiftTheLoud", "the change peaks no higher than Tutti did, times", change / loud);
}

void CrescendoIsSmooth() {
    // The roller thrown from nothing to full organ: the organ swells over
    // most of a second, never by a step.
    Controls controls;
    controls[kRoom] = controls[kBlower] = controls[kAction] = 0.0;
    controls[kHwRohrfloete8] = 1.0;
    Bench bench(controls);
    for (int note : { 48, 55, 60, 64, 67 }) bench.organ->KeyDown(kHauptwerk, note, 80);
    bench.organ->KeyDown(kPedal, 36, 80);
    const double soft = Db(bench.Play(2.0).Rms(1.0, 2.0));
    const std::uint8_t full[3] = { 0xB0, 1, 127 };
    bench.organ->Midi(full, 3);
    const Take swell = bench.Play(3.0);
    double step = 0.0, before = Db(swell.Rms(0.0, 0.05));
    for (double t = 0.025; t + 0.05 <= 3.0; t += 0.025) {
        const double now = Db(swell.Rms(t, t + 0.05));
        step = Max(step, Abs(now - before));
        before = now;
    }
    const double loud = Db(swell.Rms(2.0, 3.0));
    Expect(bench.organ->Diag().stage == kCrescendoStages && loud - soft > 12.0, "CrescendoIsSmooth", "the roller thrown over builds the full organ, dB", loud - soft);
    Expect(Db(swell.Rms(0.1, 0.2)) < soft + 0.5 * (loud - soft), "CrescendoIsSmooth", "not at once: a tenth of a second in it is under half way, dB above the start",
           Db(swell.Rms(0.1, 0.2)) - soft);
    Expect(step < 3.0, "CrescendoIsSmooth", "and never by a step, the largest in 25 ms, dB", step);

    // A wheel trembling on a stage's edge does not rattle the stop.
    Bench edge;
    const double at = 12.0 / kCrescendoStages;
    int changes = 0, stage = -1;
    for (int i = 0; i < 200; ++i) {
        edge.organ->SetCrescendo(at + (i % 2 ? 0.2 : -0.2) / kCrescendoStages);
        edge.Play(0.05);
        if (i > 20 && edge.organ->Diag().stage != stage) ++changes;
        stage = edge.organ->Diag().stage;
    }
    Expect(changes == 0, "CrescendoIsSmooth", "a wheel trembling on a stage's edge moves no stop, changes", changes);
}

// The church by itself, on test tones.
void ChurchIsThePlayers() {
    const auto nave = std::make_unique<Nave>();
    // Church Size stretches the paths: the floor's reflection, the first,
    // comes 0.97 m after the direct sound at Size 1.
    double first[2];
    int at = 0;
    for (double size : { 1.0, 2.0 }) {
        nave->Set(4.5, 1.0, 0.2, size, 1.0);
        nave->Clear();
        first[at] = -1.0;
        for (int i = 0; i < 2000 && first[at] < 0.0; ++i) {
            float left, right;
            nave->Process(i == 0 ? 1.0f : 0.0f, i == 0 ? 1.0f : 0.0f, left, right);
            if (left != 0.0f) first[at] = i;
        }
        ++at;
    }
    Expect(first[0] == 136.0 && first[1] == 272.0, "ChurchIsThePlayers", "the first reflection comes twice as late in a church twice the size, samples", first[1]);

    // A source stood 0.5 m nearer and 2 m lower has its own paths: the
    // floor's reflection is then 0.78 m behind its direct sound, not 0.97 m.
    {
        const auto apart = std::make_unique<Nave>();
        apart->Stand(1, 0.5, -2.0);
        apart->Set(4.5, 1.0, 0.2, 1.0, 1.0);
        apart->Clear();
        int came = -1;
        for (int i = 0; i < 2000 && came < 0; ++i) {
            const float lower[Nave::kSources - 1] = { i == 0 ? 1.0f : 0.0f, 0.0f, 0.0f };
            float left, right;
            apart->Process(lower[0], lower[0], left, right, lower);
            if (left != 0.0f) came = i;
        }
        Expect(came == 109, "ChurchIsThePlayers", "a division nearer and lower is reflected by its own paths, samples", came);
        // One asked behind the wall stands half a metre before it, and the
        // wall's reflection is the first: a metre further, 0.91 m as the
        // listener sees it.
        apart->Stand(2, -2.5, 2.5);
        apart->Clear();
        came = -1;
        for (int i = 0; i < 2000 && came < 0; ++i) {
            const float back[Nave::kSources - 1] = { 0.0f, i == 0 ? 1.0f : 0.0f, 0.0f };
            float left, right;
            apart->Process(back[1], back[1], left, right, back);
            if (left != 0.0f) came = i;
        }
        Expect(came == 127, "ChurchIsThePlayers", "a division at the back wall is first returned by that wall, samples", came);
    }

    // The time a band's tail takes to fall 60 dB: a second of sixteen tones
    // within a sixth of `hz`, then the fall over half the time expected.
    const auto decay = [&](double hz, double rt60, double bass, double treble, double size, double expected) {
        nave->Set(rt60, bass, treble, size, 1.0);
        nave->Clear();
        const int from = static_cast<int>(1.5 * kRate), apart = static_cast<int>(0.5 * expected * kRate);
        double early = 0.0, late = 0.0;
        for (int i = 0; i < from + apart + kRate; ++i) {
            float in = 0.0f;
            if (i < kRate)
                for (int tone = 0; tone < 16; ++tone)
                    in += 0.1f * static_cast<float>(std::sin(kTwoPi * hz * (0.85 + 0.02 * tone) * i / kRate + 2.4 * tone));
            float left, right;
            nave->Process(in, in, left, right);
            const double power = static_cast<double>(left) * left + static_cast<double>(right) * right;
            if (i >= from && i < from + kRate) early += power;
            if (i >= from + apart) late += power;
        }
        return 60.0 * (0.5 * expected) / (10.0 * std::log10(early / late));
    };
    for (double seconds : { 2.0, 4.5, 9.0 }) {
        const double heard = decay(1000.0, seconds, 1.0, 0.2, 1.0, seconds);
        Expect(Abs(heard / seconds - 1.0) < 0.1, "ChurchIsThePlayers", "a tail at 1 kHz takes the Decay to fall 60 dB, s", heard);
    }
    const double small = decay(1000.0, 4.5, 1.0, 0.2, 0.5, 4.5), large = decay(1000.0, 4.5, 1.0, 0.2, 2.0, 4.5);
    Expect(Abs(small / 4.5 - 1.0) < 0.1 && Abs(large / 4.5 - 1.0) < 0.1, "ChurchIsThePlayers", "in a small church and a large one, s in the large", large);
    const double bass = decay(80.0, 4.5, 1.0, 0.2, 1.0, 4.5), longBass = decay(80.0, 4.5, 2.0, 0.2, 1.0, 9.0), shortBass = decay(80.0, 4.5, 0.5, 0.2, 1.0, 2.25);
    Expect(longBass / bass > 1.6 && longBass / bass < 2.05, "ChurchIsThePlayers", "Decay Bass at 2 nearly doubles a tail at 80 Hz, times", longBass / bass);
    Expect(shortBass / bass > 0.48 && shortBass / bass < 0.65, "ChurchIsThePlayers", "and at 0.5 nearly halves it, times", shortBass / bass);
    // As shipped the air leaves 0.6 of the Decay at 4 kHz (Nave.h, kTrebleFit).
    const double air = decay(4000.0, 4.5, 1.0, 0.2, 1.0, 2.7);
    Expect(Abs(air / 2.7 - 1.0) < 0.1, "ChurchIsThePlayers", "the air takes a tail at 4 kHz sooner, s as shipped", air);
    const double dull = decay(4000.0, 4.5, 1.0, 0.1, 1.0, 1.8), bright = decay(4000.0, 4.5, 1.0, 0.6, 1.0, 4.0);
    Expect(dull < 0.8 * air && bright > 1.3 * air && bright < 4.5, "ChurchIsThePlayers", "and Decay Treble says how much sooner, s at its least", dull);

    // Size and Width turned under a held tone: no click, and it stays a tone.
    nave->Set(4.5, 1.0, 0.2, 1.0, 1.0);
    nave->Clear();
    double steady = 0.0, turning = 0.0;
    float before = 0.0f;
    for (int i = 0; i < 6 * kRate; ++i) {
        const float in = static_cast<float>(std::sin(kTwoPi * 220.0 * i / kRate));
        if (i >= 3 * kRate && i % 480 == 0) {
            const double turn = static_cast<double>(i - 3 * kRate) / (3.0 * kRate);
            nave->Set(4.5, 1.0, 0.2, 0.5 + 1.5 * turn, 1.0 - turn);
        }
        float left, right;
        nave->Process(in, in, left, right);
        if (i >= 2 * kRate) (i < 3 * kRate ? steady : turning) = Max(i < 3 * kRate ? steady : turning, static_cast<double>(Abs(left - before)));
        before = left;
    }
    Expect(turning < 2.0 * steady, "ChurchIsThePlayers", "Church Size and Width turned under a held tone make no click, largest step against the tone's own", turning / steady);

    // The seat. On the axis the organ's own sound is left as it is; 5.5 m to
    // the right the organ is 16 degrees to the left, and the right ear has
    // it 8 samples later and 0.725 as strong (5.5 m of the 20.0 m to it).
    nave->Set(4.5, 1.0, 0.2, 1.0, 1.0);
    nave->Clear();
    float left = 0.3f, right = -0.7f;
    nave->Direct(left, right);
    Expect(left == 0.3f && right == -0.7f, "ChurchIsThePlayers", "on the nave's axis the organ's own sound is untouched", left);
    // The reflections before the reverberant field (its shortest line is
    // 2017 samples): a seat to the left hears what one to the right does,
    // the ears changed over.
    std::array<std::array<std::vector<float>, 2>, 2> heard;
    int farEarCame = -1;
    float farEar = 0.0f;
    for (std::size_t side = 0; side < 2; ++side) {
        nave->Set(4.5, 1.0, 0.2, 1.0, 1.0, side == 0 ? 5.5 : -5.5);
        nave->Clear();
        for (int i = 0; i < 2000; ++i) {
            float in[2] = { i == 0 ? 1.0f : 0.0f, i == 0 ? 1.0f : 0.0f };
            float roomLeft, roomRight;
            nave->Process(in[0], in[1], roomLeft, roomRight);
            nave->Direct(in[0], in[1]);
            heard[side][0].push_back(roomLeft);
            heard[side][1].push_back(roomRight);
            if (side == 0 && in[1] != 0.0f && farEarCame < 0) {
                farEarCame = i;
                farEar = in[1];
            }
            if (side == 0 && i == 0) Expect(in[0] == 1.0f && in[1] == 0.0f, "ChurchIsThePlayers", "seated to the right, the left ear has the organ first", in[0]);
        }
    }
    Expect(farEarCame == 8 && Abs(farEar - 0.725f) < 0.001f, "ChurchIsThePlayers", "and the right ear 8 samples later and weaker, level", farEar);
    Expect(heard[0][0] == heard[1][1] && heard[0][1] == heard[1][0], "ChurchIsThePlayers", "a seat to the left is one to the right with the ears changed over", 0.0);
    double energy[2] = {};
    for (std::size_t ear = 0; ear < 2; ++ear)
        for (const float x : heard[0][ear]) energy[ear] += static_cast<double>(x) * x;
    // The organ, the floor's and the vault's reflections come from the left
    // and the near wall's from the right, 1.0 m behind the organ's own sound
    // where it was 4.6 m on the axis: together the right ear has a little more.
    Expect(energy[1] > 1.05 * energy[0] && energy[1] < 1.2 * energy[0], "ChurchIsThePlayers", "the ear by the wall has a little more of the reflections, times", energy[1] / energy[0]);

    // The seat moved across the nave under a held tone: no click in the
    // organ's own sound or the church's.
    nave->Set(4.5, 1.0, 0.2, 1.0, 1.0, -5.5);
    nave->Clear();
    steady = turning = 0.0;
    float last[4] = {};
    for (int i = 0; i < 6 * kRate; ++i) {
        const float in = static_cast<float>(std::sin(kTwoPi * 220.0 * i / kRate));
        if (i >= 3 * kRate && i % 480 == 0) nave->Set(4.5, 1.0, 0.2, 1.0, 1.0, -5.5 + 11.0 * (i - 3 * kRate) / (3.0 * kRate));
        float now[4] = { in, in, 0.0f, 0.0f };
        nave->Process(in, in, now[2], now[3]);
        nave->Direct(now[0], now[1]);
        for (std::size_t c = 0; c < 4; ++c) {
            if (i >= 2 * kRate) (i < 3 * kRate ? steady : turning) = Max(i < 3 * kRate ? steady : turning, static_cast<double>(Abs(now[c] - last[c])));
            last[c] = now[c];
        }
    }
    Expect(turning < 2.0 * steady, "ChurchIsThePlayers", "the seat moved across the nave under a held tone makes no click, largest step against the tone's own", turning / steady);
}

// A short piece on three manuals and the pedal, with the stops, the roller and the wind supply moving.
std::vector<float> Piece(const std::vector<int>& blocks) {
    Controls controls = kPresets[13].ToControls();
    controls[kTremulantPos] = 1.0;
    controls[kCiphers] = controls[kRunnings] = 0.5;
    Bench bench(controls);
    struct Event { int frame; std::uint8_t bytes[3]; };
    const Event events[] = {
        { 0, { 0x90, 48, 90 } }, { 0, { 0x93, 36, 80 } }, { 1000, { 0x91, 60, 70 } }, { 2500, { 0x92, 67, 100 } },
        { 6001, { 0x80, 48, 0 } }, { 6001, { 0x90, 50, 127 } }, { 9000, { 0xB0, 1, 90 } }, { 12345, { 0xB0, 11, 40 } },
        { 15000, { 0x83, 36, 0 } }, { 15000, { 0x93, 43, 60 } }, { 20000, { 0xB0, 1, 0 } }, { 26000, { 0xB0, 123, 0 } },
    };
    // A reed drawn, a stop left half drawn, the blower switched off and the
    // bellows taken up, at a lower pressure.
    struct Change { int frame, control; double value; };
    const Change changes[] = {
        { 18000, kHwTrompete8, 1.0 }, { 18000, kHwOctave4, 0.5 }, { 21000, kWindSupply, kWindOff }, { 30001, kWindSupply, kWindBellows },
        { 30001, kPressure, 0.9 },
    };
    const int frames = 48000;
    std::vector<float> out(2 * static_cast<std::size_t>(frames));
    std::size_t next = 0, change = 0, turn = 0;
    for (int done = 0; done < frames;) {
        while (next < std::size(events) && events[next].frame <= done) bench.organ->Midi(events[next++].bytes, 3);
        while (change < std::size(changes) && changes[change].frame <= done) {
            bench.Set(changes[change].control, changes[change].value);
            ++change;
        }
        // Every event and change lands on its own frame whatever the blocks.
        int now = Min(frames - done, blocks[turn++ % blocks.size()]);
        if (next < std::size(events)) now = Min(now, events[next].frame - done);
        if (change < std::size(changes)) now = Min(now, changes[change].frame - done);
        bench.organ->Render(&out[static_cast<std::size_t>(done)], &out[static_cast<std::size_t>(frames + done)], now);
        done += now;
    }
    return out;
}

void BlockPartitionInvariance() {
    const auto whole = Piece({ 256 });
    Expect(Piece({ 256 }) == whole, "BlockPartitionInvariance", "the same piece twice gives the same samples", 0.0);
    Expect(Piece({ 1 }) == whole, "BlockPartitionInvariance", "one frame at a time", 0.0);
    Expect(Piece({ 7, 64, 33, 512, 1, 31 }) == whole, "BlockPartitionInvariance", "ragged blocks", 0.0);
    double energy = 0.0;
    for (const float sample : whole) energy += static_cast<double>(sample) * sample;
    Expect(energy > 1.0, "BlockPartitionInvariance", "and it is a piece, not silence", energy);
}

void ResetPlaysAsNew() {
    Controls controls = kPresets[8].ToControls();   // Krummhorn with the tremulant
    Bench bench(controls);
    const auto play = [&] {
        bench.organ->KeyDown(kHauptwerk, 60, 80);
        bench.organ->KeyDown(kPedal, 36, 80);
        Take take = bench.Play(1.0);
        bench.organ->AllKeysUp();
        return take;
    };
    const Take first = play();
    bench.Play(0.3);
    bench.organ->Reset();
    const Take again = play();
    Expect(first.left == again.left && first.right == again.right, "ResetPlaysAsNew", "after Reset the organ plays as a new one does", 0.0);

    // And while the pipes wait to be remade for a new tuning: one that
    // speaks first is the same pipe as one remade in its turn.
    bench.Set(kTuning, 0.8);
    bench.organ->Reset();
    const Take waiting = play();
    bench.Play(0.3);
    bench.organ->Reset();
    const Take remade = play();
    Expect(waiting.left == remade.left && waiting.right == remade.right, "ResetPlaysAsNew", "and with a new tuning still reaching the pipes", 0.0);
}

void ExtremesStayFinite() {
    // Every control at its least is an organ with no stop drawn and no wind;
    // the third case draws every stop on the least of everything else, the
    // blower running at half the pressure.
    const char* const cases[] = { "ExtremesStayFinite (every control at its least)", "ExtremesStayFinite (every control at its most)",
                                  "ExtremesStayFinite (every stop on the least wind)" };
    for (int which = 0; which < 3; ++which) {
        Controls controls;
        for (int i = 0; i < kControlCount; ++i) controls[i] = which == 1 ? kControlInfo[static_cast<std::size_t>(i)].max : kControlInfo[static_cast<std::size_t>(i)].min;
        if (which == 2) {
            for (int i = 0; i < kStopCount; ++i) controls[i] = 1.0;
            controls[kWindSupply] = kWindBlower;
        }
        Bench bench(controls);
        for (int key = 0; key < kManualKeys; key += 5) bench.organ->KeyDown(kHauptwerk, kFirstNote + key, 127);
        for (int key = 0; key < kPedalKeys; key += 7) bench.organ->KeyDown(kPedal, kFirstNote + key, 127);
        const Take take = bench.Play(1.5);
        bool finite = true;
        for (std::size_t i = 0; i < take.left.size(); ++i) finite = finite && std::isfinite(take.left[i]) && std::isfinite(take.right[i]);
        Expect(finite && take.Peak() <= 0.8415 && (which == 0 || take.Peak() > 0.01), cases[which], "finite and under the ceiling, peak", take.Peak());
        if (which == 1) Expect(bench.organ->Diag().pipes > 700, "ExtremesStayFinite", "with every stop on eighteen manual keys and seven pedal keys, pipes", bench.organ->Diag().pipes);
    }
    // Controls outside their ranges, and no number at all.
    Controls wild;
    for (int i = 0; i < kControlCount; ++i) wild[i] = (i % 3 == 0) ? 1.0e9 : (i % 3 == 1 ? -1.0e9 : std::nan(""));
    Bench bench(wild);
    bench.organ->KeyDown(kHauptwerk, 60, 80);
    const Take take = bench.Play(0.5);
    bool finite = true;
    for (const float sample : take.left) finite = finite && std::isfinite(sample);
    Expect(finite, "ExtremesStayFinite", "nonsense controls are brought into range", take.Peak());
}

// Every control, on an organ with all four divisions sounding: the one
// control at its other end against the organ as it stood.
struct Thrown {
    Controls base;
    Thrown() {
        for (const int drawn : std::initializer_list<int> { kHwOctave8, kHwTrompete8, kPosPrincipal4, kPosKrummhorn8, kRecPrincipal8, kRecTrompette8, kPedSubbass16, kPedPosaune16,
                           kCouplerPosHw, kCouplerRecHw, kTremulantPos, kTremulantRec })
            base[drawn] = 1.0;
    }
    // The end of its range further from where the control stands.
    double Other(int i) const {
        const ControlInfo& info = kControlInfo[static_cast<std::size_t>(i)];
        if (info.steps > 0) return base[i] == info.min ? info.min + 1.0 : info.min;
        return base[i] - info.min > info.max - base[i] ? info.min : info.max;
    }
    // Keys held on three divisions (the Recit is heard through its couplers)
    // for `seconds`; `control` stands at `value` from `from` to `until`, or
    // throughout when both are 0.
    Take Play(int control, double value, double from, double until, double seconds) const {
        Bench bench(base);
        if (from <= 0.0) bench.Set(control, value);
        for (int note : { 48, 55, 64 }) bench.organ->KeyDown(kHauptwerk, note, 90);
        bench.organ->KeyDown(kPositiv, 60, 90);
        bench.organ->KeyDown(kPedal, 36, 90);
        Take whole;
        for (const double end : { from, until, seconds }) {
            if (end <= 0.0) continue;
            const Take part = bench.Play(end - static_cast<double>(whole.left.size()) / kRate, 64);
            whole.left.insert(whole.left.end(), part.left.begin(), part.left.end());
            whole.right.insert(whole.right.end(), part.right.begin(), part.right.end());
            bench.Set(control, end == from ? value : base[control]);
        }
        return whole;
    }
};

void EveryControlTells() {
    // No control is a dead knob: each one at its other end changes what the
    // organ gives by more than a thousandth of it.
    const Thrown thrown;
    const Take base = thrown.Play(0, thrown.base[0], 0.0, 0.0, 2.5);
    int silent = 0;
    const char* which = "";
    for (int i = 0; i < kControlCount; ++i) {
        // The keyboard's manual and the Pedal's keys are the Instrument's
        // (Oregana.Instrument holds them); a cipher tells only on the one key
        // in 25 that hangs, after it is let go (Imperfections).
        if (i == kKeyboard || i == kPedalMode || i == kCiphers) continue;
        const Take take = thrown.Play(i, thrown.Other(i), 0.0, 0.0, 2.5);
        double sum = 0.0;
        for (std::size_t n = 0; n < take.left.size(); ++n) {
            const double l = take.left[n] - base.left[n], r = take.right[n] - base.right[n];
            sum += l * l + r * r;
        }
        if (Db(std::sqrt(sum / static_cast<double>(take.left.size())) / base.Rms(0.0, 2.5)) < -60.0) {
            ++silent;
            which = kControlInfo[static_cast<std::size_t>(i)].id;
        }
    }
    std::printf("%s", silent ? which : "");
    Expect(silent == 0, "EveryControlTells", "every control changes the sound, controls that do not", silent);
}

void ThrownControlsDoNotClick() {
    // Automation and a patch change throw a control from one value to
    // another at once. Held notes take that without a click: no 2 ms of the
    // output's second difference stands more than four times over the
    // 2 ms a little before and after it. (Wind Pressure and Width did,
    // five and ten times over, until 0.5.0 made them glide.)
    const Thrown thrown;
    int clicks = 0;
    double worst = 0.0;
    const char* which = "";
    for (int i = 0; i < kControlCount; ++i) {
        const Take take = thrown.Play(i, thrown.Other(i), 1.0, 1.6, 2.2);
        constexpr std::size_t kWindow = 96;
        std::vector<double> peaks;
        for (std::size_t at = take.Frames(0.9); at + kWindow <= take.left.size(); at += kWindow) {
            double peak = 0.0;
            for (std::size_t n = at; n < at + kWindow; ++n)
                peak = Max(peak, Max(Abs(static_cast<double>(take.left[n]) - 2.0 * take.left[n - 1] + take.left[n - 2]),
                                     Abs(static_cast<double>(take.right[n]) - 2.0 * take.right[n - 1] + take.right[n - 2])));
            peaks.push_back(peak);
        }
        for (std::size_t w = 3; w + 3 < peaks.size(); ++w) {
            const double around = Max(Max(Max(peaks[w - 3], peaks[w - 2]), Max(peaks[w + 2], peaks[w + 3])), 1.0e-7);
            if (peaks[w] > 2.0e-4 && peaks[w] / around > worst) {
                worst = peaks[w] / around;
                if (worst > 4.0) which = kControlInfo[static_cast<std::size_t>(i)].id;
            }
            if (peaks[w] > 2.0e-4 && peaks[w] / around > 4.0) ++clicks;
        }
    }
    std::printf("%s", clicks ? which : "");
    Expect(clicks == 0, "ThrownControlsDoNotClick", "no control thrown under held notes clicks, the sharpest edge over its surroundings", worst);
}

} // namespace

// Neighbouring pipes hear each other: two of one key standing a row apart
// and nearly in tune draw into one pitch and stop beating; further out of
// tune they beat as before.
void PipesHearEachOther() {
    const auto ripple = [](int key, double& apart) {
        Bench bench;
        bench.Set(kTuning, 1.0);
        bench.Draw({ kHwOctave8, kHwGamba8 });
        apart = CentsBetween(bench.organ->PipeHz(2, key), bench.organ->PipeHz(1, key));
        bench.organ->KeyDown(kHauptwerk, key, 80);
        const Take take = bench.Play(8.0);
        double low = 1.0e9, high = -1.0e9;
        for (double t = 2.0; t < 7.95; t += 0.05) {
            const double level = Db(take.Rms(t, t + 0.05));
            low = Min(low, level);
            high = Max(high, level);
        }
        return high - low;
    };
    // Tuning at 1 leaves the Octave and the Gamba up to 14 cents apart, a
    // different distance on each key.
    int near = 0, far = 0;
    double mostLocked = 0.0, leastBeating = 1.0e9;
    for (int key = 48; key <= 72; ++key) {
        double apart = 0.0;
        const double moved = ripple(key, apart);
        if (Abs(apart) < 2.0) {
            ++near;
            mostLocked = Max(mostLocked, moved);
        } else if (Abs(apart) > 4.0) {
            ++far;
            leastBeating = Min(leastBeating, moved);
        }
    }
    Expect(near >= 3 && mostLocked < 1.0, "PipesHearEachOther", "neighbours within 2 cents sing as one, the level's largest swing in dB", mostLocked);
    Expect(far >= 10 && leastBeating > 3.0, "PipesHearEachOther", "and over 4 cents apart they beat as before, the smallest swing in dB", leastBeating);
}

// A pipe let go rings on long after its foot has emptied. The foot must reach
// nothing, not fall for ever through numbers a processor steps slowly
// (denormals): on a host that does not flush them, that doubled the cost of
// every ringing pipe.
void FeetEmpty() {
    for (const bool isReed : { false, true }) {
        int rank = 0;
        while (PlanPipe(rank, 12).reed != isReed || !PlanPipe(rank, 12).exists) ++rank;
        const PipePlan plan = PlanPipe(rank, 12);
        std::vector<float> room(3 * 8192), sound(FluePipe::kTick);
        FluePipe flue;
        ReedPipe reed;
        if (isReed) {
            reed.loop = room.data();
            reed.Design(plan.tongue, plan.hz, 1.0);
            reed.Clear();
        } else {
            flue.loop = room.data();
            flue.jet = flue.loop + 8192;
            flue.upper = flue.loop + 2 * 8192;
            flue.Design(plan.flue, plan.hz, 1.0);
            flue.Clear();
        }
        float least = 1.0f;
        bool ringing = false;
        for (int tick = 0; tick < 3750; ++tick) {
            const float wind = tick < 750 ? 1.0f : 0.0f;
            if (isReed) {
                reed.wind = wind;
                reed.Tick();
                reed.Run(sound.data(), FluePipe::kTick);
                if (tick >= 750 && reed.foot > 0.0f) least = Min(least, reed.foot);
                if (tick >= 750 && reed.foot == 0.0f && !reed.Silent()) ringing = true;
            } else {
                flue.wind = wind;
                flue.Tick();
                flue.Run(sound.data(), FluePipe::kTick);
                if (tick >= 750 && flue.foot > 0.0f) least = Min(least, flue.foot);
                if (tick >= 750 && flue.foot == 0.0f && !flue.Silent()) ringing = true;
            }
        }
        const float foot = isReed ? reed.foot : flue.foot;
        Expect(foot == 0.0f, "FeetEmpty", isReed ? "a reed's foot two seconds after its wind stopped" : "a flue's foot two seconds after its wind stopped", foot);
        Expect(least > 1.0e-30f, "FeetEmpty", "the least it held on the way", least);
        Expect(ringing, "FeetEmpty", "and the pipe rang on with it empty", ringing);
    }
}

// The pitch wheel warms and cools the air: flues follow while they sound,
// by 2.95 cents a degree, reeds do not, and the glide is smooth.
void AirBendsTheFlues() {
    struct Pipe { int stop, rank, division, note; bool reed; };
    // Open, chimneyed, stopped and harmonic flues from A0 to C8, and two reeds.
    const Pipe pipes[] = {
        { kHwOctave8, 1, kHauptwerk, 21, false }, { kHwOctave8, 1, kHauptwerk, 36, false }, { kHwOctave8, 1, kHauptwerk, 60, false },
        { kHwOctave8, 1, kHauptwerk, 89, false }, { kHwOctave8, 1, kHauptwerk, 108, false }, { kHwRohrfloete8, 3, kHauptwerk, 67, false },
        { kPosGedackt8, 23, kPositiv, 53, false }, { kRecFluteHarmonique8, 41, kRecit, 72, false }, { kPedPrincipal16, 55, kPedal, 43, false },
        { kHwTrompete8, 22, kHauptwerk, 60, true }, { kPedPosaune16, 66, kPedal, 48, true },
    };
    double worstPitch = 0.0, worstLevel = 0.0, worstReed = 0.0, worstSet = 0.0;
    for (const Pipe& pipe : pipes) {
        double rest = 0.0;
        for (const double bend : { 0.0, 1.0, -1.0 }) {
            Bench bench;
            bench.Draw({ pipe.stop });
            bench.organ->KeyDown(pipe.division, pipe.note, 80);
            const double made = bench.organ->PipeHz(pipe.rank, pipe.note);
            bench.Play(1.0);
            bench.organ->SetBend(bend);
            const Take take = bench.Play(Max(2.0, 400.0 / made));
            const double tuned = bench.organ->PipeHz(pipe.rank, pipe.note);
            worstSet = Max(worstSet, Abs(CentsBetween(tuned, made) - (pipe.reed ? 0.0 : 29.5 * bend)));
            const double off = CentsBetween(take.Hz(tuned), tuned);
            if (pipe.reed) worstReed = Abs(off) > Abs(worstReed) ? off : worstReed;
            else if (Abs(off) > Abs(worstPitch)) worstPitch = off;
            const double level = Db(take.Rms(1.0, 2.0));
            if (bend == 0.0) rest = level;
            else if (Abs(level - rest) > Abs(worstLevel)) worstLevel = level - rest;
        }
    }
    Expect(worstSet < 1.0e-6, "AirBendsTheFlues", "ten degrees are 29.5 cents to a flue and nothing to a reed, cents off", worstSet);
    // A pipe made a semitone away keeps its regulation within a cent and a
    // half (PipesSoundAtTheirPitch); one bent a third of that way is held to
    // the same.
    Expect(Abs(worstPitch) < 1.5, "AirBendsTheFlues", "a held flue sounds where the wheel's air puts it, cents off at worst", worstPitch);
    Expect(Abs(worstReed) < 1.5, "AirBendsTheFlues", "a held reed stays where it was, cents off at worst", worstReed);
    Expect(Abs(worstLevel) < 1.0, "AirBendsTheFlues", "and is as loud bent as at rest, dB at worst", worstLevel);

    // The glide: a chord of flues from the bass to the top while the wheel
    // swings end to end twice a second, and while it is thrown from end to
    // end at once. No 2 ms of the output's second difference stands more
    // than four times over the 2 ms a little before and after it
    // (ThrownControlsDoNotClick).
    double sharpest = 0.0;
    for (const bool thrown : { false, true }) {
        Bench bench;
        bench.Draw({ kHwPrincipal16, kHwOctave8, kHwRohrfloete8, kHwSuperoctave2, kPedPrincipal16, kRecFluteHarmonique8, kCouplerRecHw });
        for (int note : { 36, 48, 55, 64, 84, 96 }) bench.organ->KeyDown(kHauptwerk, note, 90);
        bench.organ->KeyDown(kPedal, 28, 90);
        Take take = bench.Play(1.0, 64);
        for (int block = 0; block < 1500; ++block) {
            const double turn = block * 64.0 / kRate * 2.0;   // swings since the start
            const double swing = 4.0 * Abs(turn - std::floor(turn + 0.5)) - 1.0;
            bench.organ->SetBend(thrown ? (static_cast<int>(turn * 2.0) % 2 ? 1.0 : -1.0) : swing);
            const Take part = bench.Play(64.0 / kRate, 64);
            take.left.insert(take.left.end(), part.left.begin(), part.left.end());
            take.right.insert(take.right.end(), part.right.begin(), part.right.end());
        }
        constexpr std::size_t kWindow = 96;
        std::vector<double> peaks;
        for (std::size_t at = take.Frames(0.9); at + kWindow <= take.left.size(); at += kWindow) {
            double peak = 0.0;
            for (std::size_t n = at; n < at + kWindow; ++n)
                peak = Max(peak, Max(Abs(static_cast<double>(take.left[n]) - 2.0 * take.left[n - 1] + take.left[n - 2]),
                                     Abs(static_cast<double>(take.right[n]) - 2.0 * take.right[n - 1] + take.right[n - 2])));
            peaks.push_back(peak);
        }
        for (std::size_t w = 3; w + 3 < peaks.size(); ++w) {
            const double around = Max(Max(Max(peaks[w - 3], peaks[w - 2]), Max(peaks[w + 2], peaks[w + 3])), 1.0e-7);
            if (peaks[w] > 2.0e-4) sharpest = Max(sharpest, peaks[w] / around);
        }
    }
    Expect(sharpest < 4.0, "AirBendsTheFlues", "the wheel swung and thrown under held flues does not click, the sharpest edge over its surroundings", sharpest);
}

int main() {
    SilentUntilPlayed();
    PipesSoundAtTheirPitch();
    LevelsAreRegulated();
    StopsCouplersAndCrescendo();
    WindSagsAndBounces();
    TremulantBeats();
    SwellBoxCloses();
    NaveLevel();
    Imperfections();
    MechanismIsHeard();
    FourPipesAreFourPipes();
    FeetEmpty();
    AirBendsTheFlues();
    PipesHearEachOther();
    EveryKeySounds();
    StopsSlide();
    RegistersSpeakApart();
    CouplersPullLate();
    WindSupplies();
    PressureBendsPitch();
    FluesWaver();
    ReedsWaver();
    RepeatedKeysDoNotClick();
    QuietRegistrationDoesNotLiftTheLoud();
    CrescendoIsSmooth();
    ChurchIsThePlayers();
    BlockPartitionInvariance();
    ResetPlaysAsNew();
    ExtremesStayFinite();
    EveryControlTells();
    ThrownControlsDoNotClick();
    std::printf("%d failure%s\n", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
