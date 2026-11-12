// U4 input fuzzing of the JUCE-free engine through the shared portfolio
// harness (Tests/FuzzHarness.h is a byte-identical copy of the master; change
// it in reason-rack-extensions). Every input starts from a fresh engine, so a
// saved crash replays alone; commands and rendered frames are bounded
// whatever the input length.
#include "DSP/OreganaEngine.h"
#include "DSP/OreganaPresets.h"
#include "FuzzHarness.h"

#include <cmath>
#include <memory>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    using namespace protocodus::fuzz;
    Input input { data, size };

    auto engine = std::make_unique<oregana::Engine>();
    oregana::Controls controls = oregana::kPresets[input.Pick(oregana::kPresets.size())].ToControls();
    engine->SetControls(controls);

    constexpr std::size_t maxFrames = 512;
    Sentinels<maxFrames> block;
    for (int command = 0; command < 64 && !input.Empty(); ++command) {
        switch (input.Pick(9)) {
        case 0: {   // any channel message, including truncated and running-status-like ones
            const std::uint8_t bytes[3] = { input.Byte(), input.Byte(), input.Byte() };
            engine->Midi(bytes, static_cast<int>(input.Pick(4)));
            break;
        }
        case 1:
        case 2: {   // a key of any division, down or up, twice as often as the rest
            const std::uint8_t bytes[3] = { static_cast<std::uint8_t>((input.Byte() & 1 ? 0x90 : 0x80) | (input.Byte() & 0x0F)),
                                            static_cast<std::uint8_t>(input.Byte() & 0x7F),
                                            static_cast<std::uint8_t>(input.Byte() & 0x7F) };
            engine->Midi(bytes, 3);
            break;
        }
        case 3: {   // keys called directly, in and out of every range
            const int division = static_cast<int>(input.Byte()) - 100, key = static_cast<int>(input.Byte()) - 100;
            if (input.Byte() & 1) engine->KeyDown(division, key, static_cast<int>(input.Byte()) - 64);
            else engine->KeyUp(division, key);
            break;
        }
        case 4:
            // One raw write, which may be any float, to any control.
            controls[static_cast<int>(input.Pick(oregana::kControlCount))] = input.Scalar() * 100.0f;
            engine->SetControls(controls);
            break;
        case 5:
            controls = oregana::kPresets[input.Pick(oregana::kPresets.size())].ToControls();
            engine->SetControls(controls);
            break;
        case 6:
            if (input.Byte() & 1) engine->SetSwell(input.Scalar());
            else engine->SetCrescendo(input.Scalar());
            break;
        case 7: engine->Reset(); break;
        default:
            if (input.Byte() & 1) engine->AllSoundOff();
            else engine->AllKeysUp();
            break;
        }
        const auto frames = 1 + input.Word() % maxFrames;
        block.Arm();
        engine->Render(block.Left(), block.Right(), static_cast<int>(frames));
        block.Check(frames);
        // The output stage's peak limiter holds every sample at -1.5 dBFS
        // (Tests/EngineTests.cpp, ExtremesStayFinite); full scale here.
        for (std::size_t i = 0; i < frames; ++i)
            Require(std::fabs(block.Left()[i]) < 1.0f && std::fabs(block.Right()[i]) < 1.0f);
        const auto& d = engine->Diag();
        Require(d.pipes >= 0 && d.pipes <= oregana::kPipeCount);
        for (const double pressure : d.pressure) Require(std::isfinite(pressure) && pressure >= 0.0 && pressure <= 4.0);
    }
    return 0;
}
