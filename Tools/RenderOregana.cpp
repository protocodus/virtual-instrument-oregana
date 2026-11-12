// A MIDI file through one registration, for listening and for measurement.
//
//   RenderOregana <preset> <phrase.mid> <out.wav> [--tail s] [id=value ...]
//       <preset> is a factory preset's name or number from 1; id=value sets a
//       control (OreganaControls.h) over the preset's, in physical units.
//       Prints the render's level, the most pipes speaking at once and the
//       time it took.
//   RenderOregana --levels
//       each factory preset on a reference chord (C, c0, g0, c1, e1, g1, c2
//       on the Hauptwerk's keys and C on the Pedal's, held 4 s) at Volume 0,
//       on C, D, E flat, F and G: the mean power of its last three seconds,
//       and the whole decibels of Volume that bring it to -25 dBFS RMS, which
//       a soft preset carries (OreganaPresets.h). A preset already above that
//       plays at 0. One chord alone reads a few stops decibels off, as their
//       steady tones meet their own reflections in the church.
//   RenderOregana --list
#include "DSP/OreganaPresets.h"
#include "OreganaVoice.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace oregana;
namespace demos = protocodus::demos;

namespace {

double Rms(const demos::detail::Demo& demo, std::size_t from, std::size_t end) {
    double sum = 0.0;
    for (std::size_t i = from; i < end; ++i) sum += 0.5 * (static_cast<double>(demo.left[i]) * demo.left[i] + static_cast<double>(demo.right[i]) * demo.right[i]);
    return std::sqrt(sum / static_cast<double>(Max<std::size_t>(end - from, 1)));
}
double Db(double x) { return 20.0 * std::log10(x + 1.0e-12); }

int FindPreset(const std::string& name) {
    for (std::size_t i = 0; i < kPresets.size(); ++i)
        if (name == kPresets[i].name) return static_cast<int>(i);
    char* end = nullptr;
    const long number = std::strtol(name.c_str(), &end, 10);
    return *end == '\0' && number >= 1 && number <= static_cast<long>(kPresets.size()) ? static_cast<int>(number - 1) : -1;
}

demos::detail::Demo Play(const Controls& controls, const std::vector<demos::MidiEvent>& events, double seconds, int* mostPipes) {
    demos::Instrument instrument;
    instrument.name = "Oregana";
    instrument.presets = { "render" };
    instrument.open = [&](std::size_t, double, int, std::uint32_t) { return std::make_unique<OreganaVoice>(controls, mostPipes); };
    demos::detail::Demo demo = demos::detail::Play(instrument, 0, events, seconds, nullptr);
    demos::detail::Measure(demo);
    return demo;
}

int Levels() {
    std::printf("%-18s %8s %8s %6s %6s\n", "preset", "RMS dB", "peak dB", "pipes", "Volume");
    for (const Preset& preset : kPresets) {
        Controls controls = preset.ToControls();
        controls[kVolume] = 0.0;
        int pipes = 0;
        double power = 0.0, peak = 0.0;
        const int steps[] = { 0, 2, 3, 5, 7 };
        for (const int up : steps) {
            std::vector<demos::MidiEvent> chord;
            const auto add = [&](double seconds, int status, int note, int velocity) {
                chord.push_back({ seconds, { static_cast<std::uint8_t>(status), static_cast<std::uint8_t>(note + up), static_cast<std::uint8_t>(velocity) }, 3 });
            };
            for (int note : { 36, 48, 55, 60, 64, 67, 72 }) add(0.0, 0x90, note, 80);
            add(0.0, 0x93, 36, 80);
            for (int note : { 36, 48, 55, 60, 64, 67, 72 }) add(4.0, 0x80, note, 0);
            add(4.0, 0x83, 36, 0);
            const auto demo = Play(controls, chord, 4.0, &pipes);
            const double rms = Rms(demo, 1 * 48000, 4 * 48000);
            power += rms * rms / std::size(steps);
            peak = Max(peak, static_cast<double>(demo.peak));
        }
        const double rms = Db(std::sqrt(power));
        std::printf("%-18s %8.1f %8.1f %6d %6.0f\n", preset.name, rms, demos::detail::Dbfs(peak), pipes, 0.0 + Clamp(std::round(-25.0 - rms), 0.0, 24.0));
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::strcmp(argv[1], "--levels") == 0) return Levels();
        if (argc == 2 && std::strcmp(argv[1], "--list") == 0) {
            for (std::size_t i = 0; i < kPresets.size(); ++i) std::printf("%2zu %s\n", i + 1, kPresets[i].name);
            return 0;
        }
        if (argc < 4) {
            std::fprintf(stderr, "usage: RenderOregana <preset> <phrase.mid> <out.wav> [--tail s] [id=value ...]\n"
                                 "       RenderOregana --levels | --list\n");
            return 2;
        }
        const int preset = FindPreset(argv[1]);
        if (preset < 0) {
            std::fprintf(stderr, "no preset %s\n", argv[1]);
            return 2;
        }
        Controls controls = kPresets[static_cast<std::size_t>(preset)].ToControls();
        double tail = 6.0;
        for (int i = 4; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--tail" && i + 1 < argc) { tail = std::atof(argv[++i]); continue; }
            const auto equals = arg.find('=');
            int control = -1;
            for (int c = 0; c < kControlCount; ++c)
                if (equals != std::string::npos && arg.substr(0, equals) == kControlInfo[static_cast<std::size_t>(c)].id) control = c;
            if (control < 0) {
                std::fprintf(stderr, "no control in %s\n", arg.c_str());
                return 2;
            }
            controls[control] = std::atof(arg.c_str() + equals + 1);
        }
        const auto events = demos::ReadMidi(argv[2]);
        if (events.empty()) throw std::runtime_error("no MIDI channel events");
        const double seconds = events.back().seconds + tail;
        int pipes = 0;
        const auto start = std::chrono::steady_clock::now();
        auto demo = Play(controls, events, seconds, &pipes);
        const std::chrono::duration<double> spent = std::chrono::steady_clock::now() - start;
        demo.stem = argv[3];
        demos::detail::WriteWav(argv[3], demo);
        std::printf("%s: %.1f s, RMS %.1f dBFS, peak %.1f dBFS, %d pipes at most, rendered in %.1f s (%.2fx realtime)\n",
                    kPresets[static_cast<std::size_t>(preset)].name, seconds, Db(Rms(demo, 0, demo.left.size())),
                    demos::detail::Dbfs(demo.peak), pipes, spent.count(), spent.count() / seconds);
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "RenderOregana: %s\n", e.what());
        return 1;
    }
}
