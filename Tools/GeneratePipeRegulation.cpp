// Tunes and regulates every pipe of the organ, as the tuner and the voicer
// do when it is built, and writes what they found: Source/DSP/PipeRegulation.h.
//
//   GeneratePipeRegulation --write Source/DSP/PipeRegulation.h
//   GeneratePipeRegulation --check     the committed table still tunes every pipe (Oregana.PipeRegulation)
//   GeneratePipeRegulation --report    each rank's corrections, levels and speech, and every pipe at fault
//
// The protocol. A pipe sounds alone from silence on steady nominal wind for
// 300 periods and at least a second. Its pitch is the phase a component near
// the note gains across the second half (PipeMeasure.h), its level the RMS of
// the last quarter at unity gain. The tuner's correction (the resonator's
// frequency against the note for a flue, the tongue's and the resonator's for
// a reed) is moved by what the pitch is off until the pipe sounds within
// 0.2 cents, in at most ten tries, and the nearest try is kept. A pipe is at
// fault when none came within 0.5 cents, when its correction leaves 0.6 to
// 1.4 (it speaks another note), when its last quarter differs from the one
// before by 0.5 dB (it is not steady) or when it does not speak. The check
// allows 1 cent and 0.3 dB against the table: another compiler rounds a
// pipe's noise otherwise.
#include "DSP/PipeRegulation.h"
#include "DSP/Stops.h"
#include "PipeMeasure.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace oregana;

namespace {

struct Measured {
    double cents = 0.0, rms = 0.0, unsteady = 0.0, speech = 0.0;
};

Measured Measure(const PipePlan& plan, double tuning) {
    const auto frames = static_cast<std::size_t>(Max(1.0, 300.0 / plan.hz) * kPipeRate);
    const std::vector<float> x = measure::Sound(plan, tuning, frames);
    const std::size_t quarter = frames / 4;
    Measured m;
    m.rms = measure::Rms(x, frames - quarter, frames);
    const double before = measure::Rms(x, frames - 2 * quarter, frames - quarter);
    m.unsteady = 20.0 * std::log10((m.rms + 1e-30) / (before + 1e-30));
    const std::size_t coarseFrom = frames - Min(frames / 2, static_cast<std::size_t>(12.0 * kPipeRate / plan.hz));
    const double coarse = measure::CoarseHz(x, coarseFrom, frames, plan.hz, kPipeRate);
    m.cents = 1200.0 * std::log2(measure::Hz(x, frames / 2, frames, coarse, kPipeRate) / plan.hz);
    m.speech = measure::SpeechSeconds(x, plan.hz, m.rms, 0.5, kPipeRate);
    return m;
}

struct Regulated {
    double tuning = 1.0;
    Measured found;
    const char* fault = nullptr;
};

Regulated Regulate(const PipePlan& plan) {
    Regulated best;
    best.found.cents = 1.0e9;
    double tuning = 1.0, lastTuning = 0.0, lastCents = 0.0;
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (tuning < 0.6 || tuning > 1.4) { best.fault = "speaks another note"; return best; }
        const Measured found = Measure(plan, tuning);
        if (found.rms < 1.0e-4) { best.tuning = tuning; best.found = found; best.fault = "does not speak"; return best; }
        if (Abs(found.cents) < Abs(best.found.cents)) { best.tuning = tuning; best.found = found; }
        if (Abs(found.cents) < 0.2) break;
        // The pitch follows the correction one for one in most pipes; where
        // the last two tries say otherwise, by what they say.
        double slope = 1.0;
        if (attempt > 0) {
            const double moved = 1200.0 * std::log2(tuning / lastTuning);
            if (Abs(moved) > 0.05) slope = Clamp((found.cents - lastCents) / moved, 0.25, 4.0);
        }
        lastTuning = tuning;
        lastCents = found.cents;
        tuning *= std::pow(2.0, -found.cents / (1200.0 * slope));
    }
    if (Abs(best.found.cents) >= 0.5) best.fault = "does not hold its pitch";
    else if (Abs(best.found.unsteady) > 0.5) best.fault = "is not steady";
    return best;
}

const char* RankName(int rank) {
    for (const StopSpec& stop : kStops)
        if (rank >= stop.firstRank && rank < stop.firstRank + stop.ranks) return stop.name;
    return "";
}

} // namespace

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "--check") {
        int faults = 0;
        for (int rank = 0; rank < kRankCount; ++rank)
            for (int key = 0; key < kDivisionKeys[RankDivision(rank)]; ++key) {
                const PipePlan plan = PlanPipe(rank, key);
                if (!plan.exists) continue;
                const PipeRegulated& stored = kPipeRegulation[RankFirstPipe(rank) + key];
                const Measured m = Measure(plan, stored.tuning);
                const double level = 20.0 * std::log10((m.rms + 1e-30) / stored.rms);
                if (Abs(m.cents) < 1.0 && Abs(level) < 0.3) continue;
                std::printf("rank %d (%s) key %d at %.2f Hz: %+.2f cents, %+.2f dB against the table\n", rank,
                            RankName(rank), key, plan.hz, m.cents, level);
                ++faults;
            }
        std::printf("%d pipes at fault\n", faults);
        return faults == 0 ? 0 : 1;
    }
    if (mode != "--report" && !(mode == "--write" && argc > 2)) {
        std::fprintf(stderr, "usage: GeneratePipeRegulation --write <PipeRegulation.h> | --check | --report\n");
        return 2;
    }

    std::vector<Regulated> all(static_cast<std::size_t>(kPipeCount));
    int faults = 0;
    for (int rank = 0; rank < kRankCount; ++rank) {
        double lowCents = 1e9, highCents = -1e9, lowRms = 1e9, highRms = 0.0, slowest = 0.0;
        for (int key = 0; key < kDivisionKeys[RankDivision(rank)]; ++key) {
            const PipePlan plan = PlanPipe(rank, key);
            if (!plan.exists) continue;
            const Regulated r = Regulate(plan);
            all[static_cast<std::size_t>(RankFirstPipe(rank) + key)] = r;
            if (r.fault) {
                std::printf("  rank %d (%s) key %d at %.2f Hz %s: tuning %.4f, %+.2f cents, rms %.5f, %+.2f dB over its last half\n",
                            rank, RankName(rank), key, plan.hz, r.fault, r.tuning, r.found.cents, r.found.rms, r.found.unsteady);
                ++faults;
                continue;
            }
            const double cents = 1200.0 * std::log2(r.tuning);
            lowCents = Min(lowCents, cents); highCents = Max(highCents, cents);
            lowRms = Min(lowRms, r.found.rms); highRms = Max(highRms, r.found.rms);
            slowest = Max(slowest, r.found.speech * plan.hz);
        }
        std::printf("rank %2d %-18s tuned %+7.1f to %+7.1f cents, %6.1f to %6.1f dB, speaks within %.0f periods\n", rank,
                    RankName(rank), lowCents, highCents, 20.0 * std::log10(lowRms), 20.0 * std::log10(highRms), slowest);
        std::fflush(stdout);
    }
    std::printf("%d pipes at fault\n", faults);
    if (mode != "--write") return faults == 0 ? 0 : 1;
    if (faults != 0) return 1;

    std::ofstream out(argv[2]);
    out << "#pragma once\n\n"
           "// Generated by Tools/GeneratePipeRegulation.cpp, which has the protocol: every\n"
           "// pipe's tuning correction and its level at unity gain, in the order of\n"
           "// Stops.h (RankFirstPipe(rank) + key). A place without a pipe holds 1, 1.\n"
           "// Regenerate it with any change to FluePipe.h, ReedPipe.h or the voicing in\n"
           "// Stops.h; Oregana.PipeRegulation fails on a stale table.\n\n"
           "namespace oregana {\n\n"
           "struct PipeRegulated {\n"
           "    float tuning;   // the resonator's (a reed's tongue's too) frequency against the note\n"
           "    float rms;      // the steady pipe's level on nominal wind at unity gain\n"
           "};\n\n"
           "inline constexpr PipeRegulated kPipeRegulation[] = {\n";
    char line[64];
    for (int rank = 0; rank < kRankCount; ++rank) {
        out << "    // " << rank << " " << RankName(rank) << "\n";
        const int keys = kDivisionKeys[RankDivision(rank)];
        for (int key = 0; key < keys; ++key) {
            const Regulated& r = all[static_cast<std::size_t>(RankFirstPipe(rank) + key)];
            const bool exists = PlanPipe(rank, key).exists;
            std::snprintf(line, sizeof line, "{ %.6ff, %.7ff },", exists ? r.tuning : 1.0, exists ? r.found.rms : 1.0);
            out << (key % 4 == 0 ? "    " : " ") << line << (key % 4 == 3 || key == keys - 1 ? "\n" : "");
        }
    }
    out << "};\n\n} // namespace oregana\n";
    return out.good() ? 0 : 1;
}
