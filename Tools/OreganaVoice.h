// The engine as the shared demo renderer plays it (PresetDemoRenderer.h),
// for every tool that renders MIDI through Oregana.
#pragma once

#include "DSP/OreganaEngine.h"
#include "PresetDemoRenderer.h"

#include <memory>
#include <stdexcept>
#include <string>

class OreganaVoice final : public protocodus::demos::Voice {
public:
    // `mostPipes`, if given, is kept at the most pipes that spoke at once.
    explicit OreganaVoice(const oregana::Controls& controls, int* mostPipes = nullptr)
        : engine(std::make_unique<oregana::Engine>()), most(mostPipes) {
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

    void Render(float* left, float* right, int frames) override {
        engine->Render(left, right, frames);
        if (most && engine->Diag().pipes > *most) *most = engine->Diag().pipes;
    }

private:
    std::unique_ptr<oregana::Engine> engine;
    int* most;
};
