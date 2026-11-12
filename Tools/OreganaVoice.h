// The engine as the shared demo renderer plays it (PresetDemoRenderer.h),
// for every tool that renders MIDI through Oregana.
#pragma once

#include "DSP/OreganaEngine.h"
#include "PresetDemoRenderer.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class OreganaVoice final : public protocodus::demos::Voice {
public:
    // `mostPipes`, if given, is kept at the most pipes that spoke at once.
    explicit OreganaVoice(const oregana::Controls& initial, int* mostPipes = nullptr)
        : engine(std::make_unique<oregana::Engine>()), controls(initial), most(mostPipes) {
        engine->SetControls(controls);
        engine->Reset();
    }

    // Notes and controllers go to the engine as a plug-in hands them over.
    // Anything else in a phrase is an error, not a silently dropped event.
    void Midi(const std::uint8_t* bytes, int size) override {
        const int kind = bytes[0] & 0xF0;
        if (kind != 0x80 && kind != 0x90 && kind != 0xB0)
            throw std::runtime_error("unsupported MIDI message " + std::to_string(bytes[0]));
        engine->Midi(bytes, size);
    }

    // Registration changes while playing. Render() splits its block at each
    // one's sample, in order, as a host hands over automation.
    struct Change { double seconds; int control; double value; };
    void SetChanges(std::vector<Change> list) { changes = std::move(list); }

    void Render(float* left, float* right, int frames) override {
        while (frames > 0) {
            for (; next < changes.size() && changes[next].seconds * protocodus::demos::kSampleRate <= static_cast<double>(frame); ++next) {
                controls[changes[next].control] = changes[next].value;
                engine->SetControls(controls);
            }
            int run = frames;
            if (next < changes.size())
                run = static_cast<int>(oregana::Min<double>(run, std::ceil(changes[next].seconds * protocodus::demos::kSampleRate - static_cast<double>(frame))));
            engine->Render(left, right, run);
            if (most && engine->Diag().pipes > *most) *most = engine->Diag().pipes;
            left += run; right += run; frames -= run; frame += run;
        }
    }

private:
    std::unique_ptr<oregana::Engine> engine;
    oregana::Controls controls;
    std::vector<Change> changes;
    std::size_t next = 0;
    std::int64_t frame = 0;
    int* most;
};
