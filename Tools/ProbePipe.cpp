// One pipe alone at a steady wind: its sounding pitch against the note, its
// level, how long it takes to speak and its harmonics, with any voicing
// changed on the command line. The voicer's bench.
//
//   ProbePipe flue|reed <hz> [key=value ...] [--wav out.wav] [--seconds s] [--wind p]
//
// Keys are the fields of FlueVoicing or ReedVoicing, plus `tuning`, the
// tuner's correction.
#include "DSP/FluePipe.h"
#include "DSP/ReedPipe.h"
#include "PipeMeasure.h"
#include "PresetDemoRenderer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace oregana;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: ProbePipe flue|reed <hz> [key=value ...] [--wav out.wav] [--seconds s] [--wind p]\n");
        return 2;
    }
    const bool reed = std::strcmp(argv[1], "reed") == 0;
    const double hz = std::atof(argv[2]);
    FlueVoicing flue;
    ReedVoicing tongue;
    double tuning = 1.0, seconds = 0.0, wind = 1.0, off = 0.0;
    std::string wav;
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--wav" && i + 1 < argc) { wav = argv[++i]; continue; }
        if (arg == "--seconds" && i + 1 < argc) { seconds = std::atof(argv[++i]); continue; }
        if (arg == "--wind" && i + 1 < argc) { wind = std::atof(argv[++i]); continue; }
        if (arg == "--off" && i + 1 < argc) { off = std::atof(argv[++i]); continue; }
        const auto equals = arg.find('=');
        const std::string key = arg.substr(0, equals);
        const double value = equals == std::string::npos ? 1.0 : std::atof(arg.c_str() + equals + 1);
        if (key == "tuning") tuning = value;
        else if (key == "stopped") flue.stopped = value != 0.0;
        else if (key == "scale") flue.scale = tongue.scale = value;
        else if (key == "q") flue.q = tongue.q = value;
        else if (key == "jetTransit") flue.jetTransit = value;
        else if (key == "drive") flue.drive = value;
        else if (key == "offset") flue.offset = value;
        else if (key == "noise") flue.noise = tongue.noise = value;
        else if (key == "chiff") flue.chiff = value;
        else if (key == "shedding") flue.shedding = value;
        else if (key == "tongue") tongue.tongue = value;
        else if (key == "boot") tongue.boot = value;
        else if (key == "corner") flue.corner = value;
        else if (key == "chimney") flue.chimney = value;
        else if (key == "chimneyLength") flue.chimneyLength = value;
        else if (key == "harmonic") flue.harmonic = value;
        else if (key == "speech") flue.speech = tongue.speech = value;
        else if (key == "cone") tongue.cone = value != 0.0;
        else if (key == "length") tongue.length = value;
        else if (key == "apex") tongue.apex = value;
        else if (key == "flow") tongue.flow = value;
        else if (key == "closing") tongue.closing = value;
        else if (key == "curve") tongue.curve = value;
        else if (key == "tongueQ") tongue.tongueQ = value;
        else { std::fprintf(stderr, "unknown key %s\n", key.c_str()); return 2; }
    }
    if (seconds <= 0.0) seconds = Max(1.0, 400.0 / hz);
    const auto frames = static_cast<std::size_t>(seconds * kPipeRate);
    const auto offAt = off > 0.0 ? static_cast<std::size_t>(off * kPipeRate) : frames;
    std::vector<float> left(frames, 0.0f), right(frames, 0.0f), loop, jet, upper;

    FluePipe f;
    ReedPipe r;
    if (reed) {
        r.Design(tongue, hz, tuning);
        loop.assign(static_cast<std::size_t>(r.loopLength), 0.0f);
        r.loop = loop.data();
        r.Clear();
        r.gainLeft = 1.0f;
        r.wind = static_cast<float>(wind);
        std::printf("reed %.2f Hz: loop %d, allpass %.4f, lowpass %.4f, gain %.4f, mass %.5f\n", hz, r.loopLength,
                    r.allpass, r.lowpass, r.loopGain, r.mass);
    } else {
        f.Design(flue, hz, tuning);
        loop.assign(static_cast<std::size_t>(f.loopRoom), 0.0f);
        jet.assign(static_cast<std::size_t>(f.jetLength), 0.0f);
        upper.assign(static_cast<std::size_t>(f.upperRoom), 0.0f);
        f.loop = loop.data();
        f.jet = jet.data();
        f.upper = upper.data();
        f.Clear();
        f.gainLeft = 1.0f;
        f.wind = static_cast<float>(wind);
        std::printf("flue %.2f Hz: loop %d, allpass %.4f, lowpass %.4f, gain %.4f, jet %.2f (lp %.4f, gain %.3f), source %.4f\n",
                    hz, f.loopLength, f.allpass, f.lowpass, f.loopGain, f.jetNominal, f.jetLowpass, f.jetGain, f.source);
    }
    float tongueLow = 9.0f, tongueHigh = -9.0f, shallotLow = 9.0f, shallotHigh = -9.0f;
    for (std::size_t done = 0; done < frames; done += FluePipe::kTick) {
        const int n = static_cast<int>(Min<std::size_t>(FluePipe::kTick, frames - done));
        if (done >= offAt) f.wind = r.wind = 0.0f;
        if (!reed) { f.Tick(); f.Process(&left[done], &right[done], n); continue; }
        r.Tick();
        for (int i = 0; i < n; ++i) {
            r.Process(&left[done + static_cast<std::size_t>(i)], &right[done + static_cast<std::size_t>(i)], 1);
            if (done > frames / 2 && done < offAt) {
                tongueLow = Min(tongueLow, r.tongue); tongueHigh = Max(tongueHigh, r.tongue);
                shallotLow = Min(shallotLow, r.shallot); shallotHigh = Max(shallotHigh, r.shallot);
            }
        }
    }
    if (reed) std::printf("tongue %.3f..%.3f, shallot %.3f..%.3f\n", tongueLow, tongueHigh, shallotLow, shallotHigh);
    const std::size_t steadyEnd = Min(frames, offAt), steadyFrom = steadyEnd - steadyEnd / 4;
    const std::size_t coarseFrom = steadyEnd - Min(steadyEnd / 2, static_cast<std::size_t>(12.0 * kPipeRate / hz));
    const double coarse = measure::CoarseHz(left, coarseFrom, steadyEnd, hz, kPipeRate);
    const double sounding = measure::Hz(left, steadyEnd / 2, steadyEnd, coarse, kPipeRate);
    const double rms = measure::Rms(left, steadyFrom, steadyEnd);
    std::printf("sounds %.3f Hz (%+.2f cents), rms %.4f (%.1f dB), speaks in %.0f ms (half), %.0f ms (90%%)\n", sounding,
                1200.0 * std::log2(sounding / hz), rms, 20.0 * std::log10(rms + 1e-30),
                1000.0 * measure::SpeechSeconds(left, hz, rms, 0.5, kPipeRate),
                1000.0 * measure::SpeechSeconds(left, hz, rms, 0.9, kPipeRate));
    const auto span = static_cast<std::size_t>(std::floor(static_cast<double>(steadyEnd - steadyFrom) * sounding / kPipeRate)
                                               * kPipeRate / sounding);
    const double first = std::abs(measure::Component(left, steadyEnd - span, span, sounding, kPipeRate));
    std::printf("harmonics (dB re 1st, 1st at %.1f dB):", 20.0 * std::log10(first + 1e-30));
    for (int h = 1; h <= 24 && h * sounding < 0.48 * kPipeRate; ++h)
        std::printf(" %.0f", 20.0 * std::log10(std::abs(measure::Component(left, steadyEnd - span, span, h * sounding, kPipeRate)) / first + 1e-30));
    std::printf("\n");
    if (!wav.empty()) {
        protocodus::demos::detail::Demo demo;
        const float scale = static_cast<float>(0.25 / (rms + 1e-12));
        demo.left = left;
        for (float& s : demo.left) s = Clamp(s * scale, -1.0f, 1.0f);
        demo.right = demo.left;
        protocodus::demos::detail::WriteWav(wav, demo);
    }
    return 0;
}
