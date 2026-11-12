// Engine contracts: silence until a key is down and after the church has
// rung out, every pipe at the pitch it is tuned to at any Pitch, Temperament
// and Temperature, the regulated level, stops, couplers and the crescendo
// roller, the wind's sag and bounce, the tremulants, the swell box, the
// church's level, each imperfection and breakage, block-partition invariance
// and determinism, and every control at its extremes.
#include "DSP/OreganaEngine.h"
#include "DSP/OreganaPresets.h"
#include "../Tools/PipeMeasure.h"

#include <cmath>
#include <cstdio>
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
    bench.organ->KeyDown(kHauptwerk, 24, 80);
    Expect(bench.Play(1.0).Rms(0.5, 1.0) > 1.0e-3, "SilentUntilPlayed", "a key down sounds", 0.0);
    bench.organ->KeyUp(kHauptwerk, 24);
    // The church takes its decay to fall 60 dB, and the blower fades after it.
    bench.Play(40.0);
    const Take rest = bench.Play(60.0);
    Expect(rest.Rms(59.0, 60.0) == 0.0, "SilentUntilPlayed", "the organ comes to rest: exact silence", rest.Rms(59.0, 60.0));
    Expect(bench.organ->Diag().pipes == 0, "SilentUntilPlayed", "no pipe left ringing", bench.organ->Diag().pipes);
}

void PipesSoundAtTheirPitch() {
    struct Pipe { int stop, rank, division, key; };
    const Pipe pipes[] = {
        { kHwOctave8, 1, kHauptwerk, 0 }, { kHwOctave8, 1, kHauptwerk, 24 }, { kHwOctave8, 1, kHauptwerk, 53 },
        { kHwRohrfloete8, 3, kHauptwerk, 31 }, { kHwTrompete8, 22, kHauptwerk, 24 }, { kPosGedackt8, 23, kPositiv, 17 },
        { kPosKrummhorn8, 37, kPositiv, 28 }, { kRecFluteHarmonique8, 41, kRecit, 36 }, { kRecGambe8, 42, kRecit, 20 },
        { kPedPrincipal16, 55, kPedal, 7 }, { kPedPosaune16, 66, kPedal, 12 },
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
            bench.organ->KeyDown(pipe.division, pipe.key, 80);
            const double tuned = bench.organ->PipeHz(pipe.rank, pipe.key);
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
    Expect(Abs(CentsBetween(bench.organ->PipeHz(1, 33), 415.0)) < 1.0e-6, "PipesSoundAtTheirPitch", "a1 is the Pitch in any temperament",
           bench.organ->PipeHz(1, 33));
    Expect(Abs(CentsBetween(bench.organ->PipeHz(1, 24), 415.0 * std::pow(2.0, -9.0 / 12.0)) - 11.73) < 1.0e-6, "PipesSoundAtTheirPitch",
           "Werckmeister's c is 11.73 cents above equal", CentsBetween(bench.organ->PipeHz(1, 24), 415.0 * std::pow(2.0, -9.0 / 12.0)));
    // A mutation is tuned pure to its key.
    Expect(Abs(bench.organ->PipeHz(6, 24) / bench.organ->PipeHz(1, 24) - 3.0) < 1.0e-9, "PipesSoundAtTheirPitch", "the Quinte 2 2/3' is a pure twelfth",
           bench.organ->PipeHz(6, 24) / bench.organ->PipeHz(1, 24));
}

void LevelsAreRegulated() {
    Bench bench;
    bench.Draw({ kHwOctave8 });
    bench.organ->KeyDown(kHauptwerk, 24, 80);
    const double octave = Db(bench.Play(2.0).Rms(1.0, 2.0));
    // kMaster in OreganaEngine.cpp: 0.012, -38.4 dBFS.
    Expect(Abs(octave + 38.4) < 0.5, "LevelsAreRegulated", "the Octave 8' at c1 alone, dBFS", octave);

    Bench soft;
    soft.Draw({ kPosGedackt8 });
    soft.organ->KeyDown(kPositiv, 24, 80);
    const double gedackt = Db(soft.Play(2.0).Rms(1.0, 2.0));
    Expect(Abs(gedackt - octave + 8.0) < 0.7, "LevelsAreRegulated", "the Gedackt 8' is 8 dB under it", gedackt - octave);

    // Velocity is the key's speed, not a level.
    double levels[2];
    int at = 0;
    for (int velocity : { 20, 127 }) {
        Bench touch;
        touch.Draw({ kHwOctave8 });
        touch.organ->KeyDown(kHauptwerk, 24, velocity);
        levels[at++] = Db(touch.Play(2.0).Rms(1.0, 2.0));
    }
    Expect(Abs(levels[0] - levels[1]) < 0.2, "LevelsAreRegulated", "a soft and a hard stroke sound alike once speaking, dB apart", levels[0] - levels[1]);
}

void StopsCouplersAndCrescendo() {
    Bench bench;
    bench.organ->KeyDown(kHauptwerk, 24, 80);
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
    bench.organ->KeyUp(kHauptwerk, 24);

    // Couplers chain: the Pedal pulls the Hauptwerk, which pulls the Recit.
    bench.Draw({ kRecGambe8, kCouplerRecHw, kCouplerHwPed });
    bench.organ->KeyDown(kPedal, 12, 80);
    bench.Play(0.2);
    Expect(bench.organ->Diag().pipes == 1, "StopsCouplersAndCrescendo", "a Pedal key reaches the Recit through two couplers", bench.organ->Diag().pipes);
    bench.organ->AllKeysUp();
    bench.Play(3.0);

    // The roller only ever adds.
    Bench roller;
    roller.organ->KeyDown(kHauptwerk, 24, 80);
    roller.organ->KeyDown(kPedal, 0, 80);
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
    Controls controls = kPresets.back().ToControls();   // Tutti
    controls[kRoom] = 0.0;
    Bench bench(controls);
    for (int key : { 0, 12, 19, 24, 28, 31, 36 }) bench.organ->KeyDown(kHauptwerk, key, 80);
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
    bench.organ->KeyDown(kPositiv, 24, 80);
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
        bench.organ->KeyDown(kRecit, 24, 80);
        levels[at++] = Db(bench.Play(2.0).Rms(1.0, 2.0));
    }
    Expect(levels[0] - levels[1] > 8.0 && levels[0] - levels[1] < 24.0, "SwellBoxCloses", "the shut box takes 8 to 24 dB of a trumpet", levels[0] - levels[1]);

    // The pedal moves it under a held note, and only the Recit.
    Bench bench;
    bench.Draw({ kRecTrompette8, kHwOctave8 });
    bench.organ->KeyDown(kHauptwerk, 24, 80);
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
        for (int key : { 0, 12, 19, 24, 28, 31, 36 }) bench.organ->KeyDown(kHauptwerk, key, 80);
        bench.organ->KeyDown(kPedal, 0, 80);
        levels[at++] = Db(bench.Play(8.0).Rms(6.0, 8.0));
    }
    Expect(Abs(levels[1] - levels[0]) < 1.5, "NaveLevel", "the church alone is as loud as a plenum chord alone, dB apart", levels[1] - levels[0]);

    // The decay is the Decay: a staccato chord's tail falls 30 dB in half of it.
    Controls controls = kPresets[13].ToControls();
    controls[kBlower] = controls[kAction] = 0.0;
    controls[kRoom] = 1.0;
    Bench bench(controls);
    for (int key : { 12, 19, 24 }) bench.organ->KeyDown(kHauptwerk, key, 80);
    bench.Play(1.0);
    bench.organ->AllKeysUp();
    const Take tail = bench.Play(4.0);
    const double fall = Db(tail.Rms(0.5, 0.75)) - Db(tail.Rms(2.5, 2.75));
    // 2 s of a 4.5 s decay is 26.7 dB at low frequencies and more above.
    Expect(fall > 24.0 && fall < 40.0, "NaveLevel", "the tail falls as the Decay says, dB in 2 s", fall);
}

void Imperfections() {
    // Tuning: every pipe off its pitch, flues within 10 cents and reeds within 20.
    {
        Bench bench;
        const double tuned = bench.organ->PipeHz(1, 24), reed = bench.organ->PipeHz(22, 24);
        bench.Set(kTuning, 1.0);
        double flueWorst = 0.0, reedWorst = 0.0, sum = 0.0;
        for (int key = 0; key < kManualKeys; ++key) {
            const double off = CentsBetween(bench.organ->PipeHz(1, key), tuned * std::pow(2.0, (key - 24) / 12.0));
            flueWorst = Max(flueWorst, Abs(off));
            sum += off * off;
            reedWorst = Max(reedWorst, Abs(CentsBetween(bench.organ->PipeHz(22, key), reed * std::pow(2.0, (key - 24) / 12.0))));
        }
        Expect(flueWorst > 5.0 && flueWorst <= 10.0, "Imperfections", "Tuning at 1 puts flues up to 10 cents off", flueWorst);
        Expect(reedWorst > 10.0 && reedWorst <= 20.0, "Imperfections", "and reeds up to 20", reedWorst);
        Expect(std::sqrt(sum / kManualKeys) > 2.0, "Imperfections", "most of them audibly, RMS cents", std::sqrt(sum / kManualKeys));
        // And the pipe sounds where the engine says.
        bench.Draw({ kHwOctave8 });
        bench.organ->KeyDown(kHauptwerk, 30, 80);
        const double said = bench.organ->PipeHz(1, 30);
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
            bench.organ->KeyDown(kHauptwerk, 24, 80);
            flue[at] = bench.Play(2.0).Hz(261.6);
            Bench trumpet;
            trumpet.Set(kTemperature, warm);
            trumpet.Draw({ kHwTrompete8 });
            trumpet.organ->KeyDown(kHauptwerk, 24, 80);
            reed[at++] = trumpet.Play(2.0).Hz(261.6);
        }
        Expect(Abs(CentsBetween(flue[1], flue[0]) - 29.5) < 1.5, "Imperfections", "10 degrees warmer, a flue is 29.5 cents sharp", CentsBetween(flue[1], flue[0]));
        Expect(Abs(CentsBetween(reed[1], reed[0])) < 0.5, "Imperfections", "and a trumpet has not moved, cents", CentsBetween(reed[1], reed[0]));
    }
    // Voicing: the pipes of a rank no longer match.
    {
        double low = 1.0e9, high = -1.0e9;
        for (int key : { 20, 24, 27, 31, 35, 40 }) {
            Bench bench;
            bench.Set(kVoicing, 1.0);
            bench.Draw({ kHwOctave8 });
            bench.organ->KeyDown(kHauptwerk, key, 80);
            const double level = Db(bench.Play(1.5).Rms(1.0, 1.5));
            low = Min(low, level);
            high = Max(high, level);
        }
        Expect(high - low > 1.0 && high - low < 7.5, "Imperfections", "Voicing at 1 spreads six neighbours' levels, dB", high - low);
    }
    // Dead pipes: about one in sixteen is silent, and comes back.
    {
        Controls controls = kPresets.back().ToControls();
        Bench bench(controls);
        for (int key = 12; key < 36; ++key) bench.organ->KeyDown(kHauptwerk, key, 80);
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
        for (int key = 0; key < kManualKeys; ++key) bench.organ->KeyDown(kHauptwerk, key, 80);
        bench.Play(0.2);
        bench.organ->AllKeysUp();
        bench.Play(4.0);
        const int hanging = bench.organ->Diag().pipes;
        Expect(hanging >= 1 && hanging <= 8, "Imperfections", "Ciphers at 1 leaves a few of 61 keys sounding", hanging);
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
        for (int key = 0; key < kManualKeys; ++key) {
            bench.organ->KeyDown(kHauptwerk, key, 80);
            bench.Play(0.05);
            most = Max(most, bench.organ->Diag().pipes);
            bench.organ->KeyUp(kHauptwerk, key);
            bench.Play(2.0);
        }
        Expect(most >= 2 && most <= 3, "Imperfections", "Runnings at 1 makes some keys' neighbours whisper along, pipes on one key", most);
        Expect(bench.organ->Diag().pipes == 0, "Imperfections", "and they stop with the key", bench.organ->Diag().pipes);
    }
    // The organ's own noises: a key's knock with no stop drawn, and the wind's.
    {
        Bench bench;
        bench.Set(kAction, 1.0);
        bench.organ->KeyDown(kHauptwerk, 24, 100);
        const double knock = bench.Play(0.2).Peak();
        Expect(knock > 1.0e-3 && knock < 0.05, "Imperfections", "Action: a key knocks with no stop drawn, peak", knock);
        Bench hum;
        hum.Set(kBlower, 1.0);
        hum.Draw({ kPosGedackt8 });
        hum.organ->KeyDown(kPositiv, 24, 80);
        hum.Play(3.0);
        hum.organ->KeyUp(kPositiv, 24);
        hum.Play(1.0);
        const double noise = Db(hum.Play(1.0).Rms(0.0, 1.0));
        Expect(noise > -60.0 && noise < -45.0, "Imperfections", "Blower: the wind is heard between the notes, dBFS", noise);
    }
}

// A short piece on three manuals and the pedal, with a stop and the roller moving.
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
    const int frames = 48000;
    std::vector<float> out(2 * static_cast<std::size_t>(frames));
    std::size_t next = 0, turn = 0;
    for (int done = 0; done < frames;) {
        while (next < std::size(events) && events[next].frame <= done) bench.organ->Midi(events[next++].bytes, 3);
        int now = Min(frames - done, blocks[turn++ % blocks.size()]);
        if (next < std::size(events)) now = Min(now, events[next].frame - done);
        if (done == 18000) {
            bench.Set(kHwTrompete8, 1.0);
        }
        // The control change lands on its own frame whatever the blocks.
        if (done < 18000) now = Min(now, 18000 - done);
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
        bench.organ->KeyDown(kHauptwerk, 24, 80);
        bench.organ->KeyDown(kPedal, 0, 80);
        Take take = bench.Play(1.0);
        bench.organ->AllKeysUp();
        return take;
    };
    const Take first = play();
    bench.Play(0.3);
    bench.organ->Reset();
    const Take again = play();
    Expect(first.left == again.left && first.right == again.right, "ResetPlaysAsNew", "after Reset the organ plays as a new one does", 0.0);
}

void ExtremesStayFinite() {
    for (double end : { 0.0, 1.0 }) {
        Controls controls;
        for (int i = 0; i < kControlCount; ++i) controls[i] = end > 0.5 ? kControlInfo[static_cast<std::size_t>(i)].max : kControlInfo[static_cast<std::size_t>(i)].min;
        Bench bench(controls);
        for (int key = 0; key < kManualKeys; key += 5) bench.organ->KeyDown(kHauptwerk, key, 127);
        for (int key = 0; key < kPedalKeys; key += 7) bench.organ->KeyDown(kPedal, key, 127);
        const Take take = bench.Play(1.5);
        bool finite = true;
        for (std::size_t i = 0; i < take.left.size(); ++i) finite = finite && std::isfinite(take.left[i]) && std::isfinite(take.right[i]);
        Expect(finite && take.Peak() <= 0.8415, end > 0.5 ? "ExtremesStayFinite (every control at its most)" : "ExtremesStayFinite (every control at its least)",
               "finite and under the ceiling, peak", take.Peak());
        if (end > 0.5) Expect(bench.organ->Diag().pipes > 500, "ExtremesStayFinite", "with every stop on thirteen manual keys and five pedal keys, pipes", bench.organ->Diag().pipes);
    }
    // Controls outside their ranges, and no number at all.
    Controls wild;
    for (int i = 0; i < kControlCount; ++i) wild[i] = (i % 3 == 0) ? 1.0e9 : (i % 3 == 1 ? -1.0e9 : std::nan(""));
    Bench bench(wild);
    bench.organ->KeyDown(kHauptwerk, 24, 80);
    const Take take = bench.Play(0.5);
    bool finite = true;
    for (const float sample : take.left) finite = finite && std::isfinite(sample);
    Expect(finite, "ExtremesStayFinite", "nonsense controls are brought into range", take.Peak());
}

} // namespace

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
    BlockPartitionInvariance();
    ResetPlaysAsNew();
    ExtremesStayFinite();
    std::printf("%d failure%s\n", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
