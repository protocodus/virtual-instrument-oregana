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
    Controls controls = kPresets.back().ToControls();   // Tutti
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
        Controls controls = kPresets.back().ToControls();
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

// When a take first sounds, ms.
double FirstSound(const Take& take) {
    for (std::size_t i = 0; i < take.left.size(); ++i)
        if (take.left[i] != 0.0f || take.right[i] != 0.0f) return 1000.0 * static_cast<double>(i) / kRate;
    return -1.0;
}

// When a take's level first comes within 6 dB of where it settles, ms.
double HalfWay(const Take& take) {
    const double seconds = static_cast<double>(take.left.size()) / kRate, settled = take.Rms(0.75 * seconds, seconds);
    for (double t = 0.0; t + 0.01 <= seconds; t += 0.001)
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
    Controls controls = kPresets.back().ToControls();   // Tutti
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
    EveryKeySounds();
    StopsSlide();
    RegistersSpeakApart();
    CouplersPullLate();
    WindSupplies();
    PressureBendsPitch();
    CrescendoIsSmooth();
    ChurchIsThePlayers();
    BlockPartitionInvariance();
    ResetPlaysAsNew();
    ExtremesStayFinite();
    std::printf("%d failure%s\n", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
