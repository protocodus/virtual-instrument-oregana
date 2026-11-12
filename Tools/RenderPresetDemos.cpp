// Renders the README demos (README G3) from the committed phrases in
// Docs/audio/midi, each through the factory registration its file names,
// through the shipping engine. The contract and the fixed rate, block size
// and seed live in the shared PresetDemoRenderer.h.

#include "DSP/OreganaPresets.h"
#include "OreganaVoice.h"

#include <memory>
#include <stdexcept>

int main(int argc, char** argv) {
    protocodus::demos::Instrument instrument;
    instrument.name = "Oregana";
    for (const auto& preset : oregana::kPresets) instrument.presets.push_back(preset.name);
    instrument.open = [](std::size_t preset, double sampleRate, int, std::uint32_t) {
        // The engine runs at the rate its pipes are tuned at, and each pipe's
        // share of the imperfections is a hash of its place, so it takes
        // neither a rate nor a seed.
        if (sampleRate != oregana::kPipeRate) throw std::runtime_error("Oregana renders at 48 kHz only");
        return std::make_unique<OreganaVoice>(oregana::kPresets[preset].ToControls());
    };
    // About four times the slowest demo's median, 0.16x realtime (Tutti, 468
    // pipes at most), measured 2026-11-12 in the development container
    // (GCC 13, Release).
    instrument.cpuBudget = 0.75;
    // The default stress phrase suits an organ as it is: sixteen keys of the
    // Hauptwerk at once, which the Tutti's couplers carry to every manual
    // stop. The sustain pedal it holds is no part of an organ and is ignored.
    return protocodus::demos::Main(argc, argv, instrument);
}
