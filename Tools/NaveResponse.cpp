// The church's answer to one click at the organ's place, as the seat hears
// it, for Tools/room_measures.py to set beside a measured room impulse
// response. Any of the church's controls can be changed; the rest are as the
// organ loads.
//
//   NaveResponse <out.wav> [--seconds s] [id=value ...]
//
// The click is 1 on both ears at the first sample, from the Hauptwerk's
// place. What comes out is sqrt(1 - Room) of the organ's own sound at the
// seat (Nave::Direct) and sqrt(Room) of the church's (Nave::Process), as
// Engine::RenderChunk mixes them, before Volume and the output stage: a click
// is not what the output stage is for.
#include "DSP/Nave.h"
#include "DSP/OreganaControls.h"
#include "PresetDemoRenderer.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>

using namespace oregana;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: NaveResponse <out.wav> [--seconds s] [id=value ...]\n");
        return 2;
    }
    try {
        double seconds = 4.0;
        Controls controls {};
        for (int c = 0; c < kControlCount; ++c) controls[static_cast<std::size_t>(c)] = kControlInfo[static_cast<std::size_t>(c)].initial;
        for (int i = 2; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--seconds" && i + 1 < argc) {
                seconds = std::atof(argv[++i]);
                continue;
            }
            const auto equals = arg.find('=');
            int found = -1;
            for (int c = 0; c < kControlCount; ++c)
                if (arg.substr(0, equals) == kControlInfo[static_cast<std::size_t>(c)].id) found = c;
            if (equals == std::string::npos || found < 0) throw std::runtime_error("no control " + arg);
            controls[static_cast<std::size_t>(found)] = std::atof(arg.c_str() + equals + 1);
        }

        // 4 MB of delay lines: on the heap, as the engine keeps it.
        auto nave = std::make_unique<Nave>();
        nave->Set(controls[kDecay], controls[kDecayBass], controls[kDecayTreble], controls[kRoomSize], controls[kWidth],
                  controls[kSeat] * Nave::kSeatMost);
        nave->Clear();
        const auto dry = static_cast<float>(std::sqrt(1.0 - controls[kRoom]));
        const auto wet = static_cast<float>(std::sqrt(controls[kRoom]));

        protocodus::demos::detail::Demo demo;
        const auto frames = static_cast<std::size_t>(seconds * kPipeRate);
        demo.left.assign(frames, 0.0f);
        demo.right.assign(frames, 0.0f);
        for (std::size_t i = 0; i < frames; ++i) {
            float left = i == 0 ? 1.0f : 0.0f, right = left, roomLeft, roomRight;
            nave->Process(left, right, roomLeft, roomRight);
            nave->Direct(left, right);
            demo.left[i] = dry * left + wet * roomLeft;
            demo.right[i] = dry * right + wet * roomRight;
        }
        protocodus::demos::detail::WriteWav(argv[1], demo);
        std::printf("%s: %.1f s, Decay %.2f s, Decay Bass %.2f, Decay Treble %.2f, Church Size %.2f, Room %.2f\n", argv[1], seconds,
                    controls[kDecay], controls[kDecayBass], controls[kDecayTreble], controls[kRoomSize], controls[kRoom]);
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
