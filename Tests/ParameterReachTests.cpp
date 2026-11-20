// U11 parameter reach over oregana::Instrument and the controls the Rack
// device gives it, through the shared portfolio harness (Tests/ParameterMatrix.h
// is a byte-identical copy of the master; change it in reason-rack-extensions).
// There is no plug-in yet, so the device stores each control normalised and
// maps it as the Rack adapter's Push() does (Examples/Oregana/Oregana.cpp):
// every control of OreganaControls.h by its id, the mod wheel (the crescendo
// roller) and expression (the swell pedal), which Reason
// automates like them, and, with the editor-only controls since no
// automation reaches it, Reason's master tune on the organ's Pitch, which the
// host's settings change. The Rack device has no Crescendo property (its mod
// wheel is the roller); a plug-in would carry the shared control, so it is
// here too. MIDI pitch bend is the pitch wheel, the air's warmth.
//
// The base patch is the first factory registration, Principal 8: the
// Hauptwerk's one open 8' rank, and the Pedal's 16' and 8' under the lowest
// key. A new Oregana in Reason loads the Plenum (the Rack's info.lua), but
// under its sixteen stops a mutation, the Positiv's 1' or the Pedal's 2'
// moves the reach phrase's sound by under half a decibel, so a stop that did
// not reach the pipes could pass unheard there; over the one 8' rank every
// stop, the church and the output are heard. A control the base patch leaves
// inaudible (a Positiv or Recit level) is tested on the first factory
// registration where a player hears it, or with the controls its rule sets.
//
//   OreganaParameterReachTests --reach [--jobs N] [--case PARAMETER]

#include "DSP/OreganaInstrument.h"
#include "DSP/OreganaPresets.h"
#include "ParameterMatrix.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace oregana;
namespace matrix = protocodus::matrix;

// The engine's controls in their own order, then the Rack's performance
// controls; the editor-only master tune last.
enum Extra { kModWheel = kControlCount, kExpression, kAutomatable, kMasterTune = kAutomatable, kValueCount };

// The stops of the base patch, Principal 8 (OreganaPresets.h).
constexpr int kBaseStops[] = { kHwOctave8, kPedSubbass16, kPedOctave8 };

// What the controls that remake a pipe leave the run's probe 1 (ReachFor).
constexpr const char* kRinging = "the held chord's G3, whose pipes still ring when probe 1 strikes it again and keep their old making";

double Unit(double x) { return x == x ? std::clamp(x, 0.0, 1.0) : 0.0; }

matrix::ReachRule Rule(matrix::Reach reach, std::string reason, std::vector<std::pair<std::size_t, float>> with = {},
                       std::string history = {}) {
    matrix::ReachRule rule;
    rule.reach = reach;
    rule.reason = std::move(reason);
    rule.with = std::move(with);
    rule.history = std::move(history);
    return rule;
}

class OreganaDevice final : public matrix::Device {
public:
    OreganaDevice() : instrument(std::make_unique<Instrument>()) {
        for (int i = 0; i < kControlCount; ++i)
            values[static_cast<std::size_t>(i)] = static_cast<float>(ToNormalized(i, kControlInfo[static_cast<std::size_t>(i)].initial));
        values[kModWheel] = 0.0f;
        values[kExpression] = 1.0f;
        values[kMasterTune] = 0.5f;
        LoadPreset(0);   // Principal 8
    }

    std::vector<matrix::Parameter> Parameters() const override {
        std::vector<matrix::Parameter> parameters;
        for (int i = 0; i < kControlCount; ++i) {
            const ControlInfo& c = kControlInfo[static_cast<std::size_t>(i)];
            parameters.push_back({ c.id, c.steps, static_cast<float>(ToNormalized(i, c.initial)) });
        }
        parameters.push_back({ "mod_wheel", 0, 0.0f });
        parameters.push_back({ "expression", 0, 1.0f });
        return parameters;
    }
    std::vector<matrix::Parameter> EditorOnlyParameters() const override { return { { "master_tune", 0, 0.5f } }; }
    int PresetCount() const override { return static_cast<int>(kPresets.size()); }
    void Prepare(double rate, int) override {
        instrument->Prepare(rate);
        Apply();
    }
    // A patch sets the panel's controls; the performance controls and the
    // host's master tune stay where they are. The organ then stands as the
    // patch leaves it, its stops across and not sliding there (Oregana.cpp,
    // Snapshot).
    void LoadPreset(int index) override {
        const Preset& preset = kPresets[static_cast<std::size_t>(index)];
        for (int i = 0; i < kControlCount; ++i)
            values[static_cast<std::size_t>(i)] = static_cast<float>(ToNormalized(i, preset.values[static_cast<std::size_t>(i)]));
        Apply();
        instrument->Reset();
    }
    float GetParameter(std::size_t index) const override { return values[index]; }
    void SetParameter(std::size_t index, float value) override {
        if (values[index] != value) {
            values[index] = value;
            pending = true;
        }
    }
    // Instrument::Render takes no events, so the block is split at each one,
    // as the Rack device splits its batch at each change.
    void Process(float* left, float* right, int frames, const matrix::Event* events, int count) override {
        // The host applies a batch of simultaneous control changes before
        // sounding the first note at this frame.
        if (pending) Apply();
        int done = 0;
        for (int e = 0; e < count; ++e) {
            const int at = std::clamp(events[e].offset, done, frames);
            instrument->Render(left + done, right + done, at - done);
            done = at;
            Handle(events[e]);
        }
        instrument->Render(left + done, right + done, frames - done);
    }
    std::vector<std::uint8_t> SaveState() override {
        std::vector<std::uint8_t> state(sizeof values);
        std::memcpy(state.data(), values.data(), sizeof values);
        return state;
    }
    void RestoreState(const std::vector<std::uint8_t>& state) override {
        if (state.size() != sizeof values) return;
        std::memcpy(values.data(), state.data(), sizeof values);
        Apply();
    }

    matrix::Limits LimitsFor(const matrix::Case& c) const override {
        matrix::Limits limits;
        const auto& v = c.values;
        const bool stopDrawn = std::any_of(v.begin(), v.begin() + kStopCount, [](float x) { return x > 0; });
        const bool audibleDivision = std::any_of(v.begin() + kMuteHauptwerk, v.begin() + kMutePedal + 1,
                                                [](float x) { return x < .5f; });
        // The ordinary held-note assertion applies when a registration can
        // reach the main mix. A silent organ at all-min is a valid result.
        limits.soundsWhileHeld = (stopDrawn || v[kModWheel] > 0 || v[kCrescendo] > 0) && audibleDivision;
        // With the blower running or pallets stuck open the organ can sound
        // after every key is released. A church can also have a long decay.
        if ((v[kWindSupply] > 0 && v[kBlower] > 0) || v[kCiphers] > 0 || v[kRoom] > 0) {
            limits.releaseSeconds = -1;
        }
        return limits;
    }

    // Every other control reaches the pipes, the wind, the keys or the
    // church at once: the stops and couplers through Register and the
    // sliders, the rest gliding (OreganaEngine.h, Nave.h).
    matrix::ReachRule ReachFor(std::size_t index, float) const override {
        using matrix::Reach;
        using With = std::vector<std::pair<std::size_t, float>>;
        constexpr float drawn = 1.0f, positiv = 0.5f, recit = 1.0f;
        // Mechanics.h, Knocks::Strike: every knock takes its small change of
        // pitch and level from one sequence, so a knock a control strikes
        // itself (a knob landing, the shutters meeting their frame, a
        // tremulant's valve) changes every key's knock after it, and a
        // moving slider's rub moves the wind's noise on by the samples it
        // ran. R, T and the run then differ by that alone: a Recit stop
        // drawn with no coupler to bring it, which no pipe follows, put
        // probe 1 0.94 dB from R over the one 8' rank (2026-11-15). Such a
        // control is tested with the Action at 0, so that no key or knob
        // knocks and none of their knocks moves with it; the knocks are the
        // Action's, whose reach is tested on its own, and Oregana.Engine
        // (MechanismIsHeard) hears the shutters, the sliders and the
        // tremulant's valve. And it is tested with the
        // base patch's stops put in, so that only the pipes it brings or
        // acts on sound: over the Octave 8' a soft 8' (the Quintadena, the
        // Unda maris) moved probe 1 by 0.8 dB, too little to tell from what
        // T's long play left (1.1 dB from T against R's 1.3), and a Pedal
        // coupler brings one key's 8' to the Pedal's two (0.3 dB on the held
        // chord).
        const auto alone = [index](With with) {
            with.push_back({ kAction, 0.0f });
            for (const int stop : kBaseStops) {
                const auto at = static_cast<std::size_t>(stop);
                if (index != at && std::none_of(with.begin(), with.end(), [at](const auto& w) { return w.first == at; }))
                    with.push_back({ at, 0.0f });
            }
            return with;
        };
        if (static_cast<int>(index) < kStopCount) {
            // Every factory registration brings a Positiv or Recit stop to
            // the keys through its coupler to the Hauptwerk (OreganaPresets.h).
            // One of the base patch's own stops is put in with the others
            // still sounding, so that T is not silence.
            const int division = kStops[index].division;
            if (std::find(std::begin(kBaseStops), std::end(kBaseStops), static_cast<int>(index)) != std::end(kBaseStops))
                return Rule(Reach::Held, "", { { kAction, 0.0f } });
            if (division == kPositiv) return Rule(Reach::Held, "", alone({ { kCouplerPosHw, drawn } }));
            if (division == kRecit) return Rule(Reach::Held, "", alone({ { kCouplerRecHw, drawn } }));
            return Rule(Reach::Held, "", alone({}));
        }
        switch (static_cast<int>(index)) {
        // Engine::Apply: these remake a pipe, so a speaking pipe keeps what it
        // was made with and takes the change the next time it speaks; silent
        // pipes are remade two a tick (OreganaEngine.h, Imperfections). A
        // pipe still ringing when its key is struck again keeps its making
        // too (Engine::Take): half a second after the held chord is let go
        // four of its five pipes on Principal 8 still ring, and probe 1
        // strikes its G3 again.
        case kTuning: return Rule(Reach::NewNotes, "each pipe's drift, which it is made with the next time it speaks", {}, kRinging);
        case kTemperature:
            return Rule(Reach::NewNotes, "the air's temperature each pipe is tuned for the next time it speaks (the pitch wheel warms speaking flues)",
                        {}, kRinging);
        case kPitch:
        case kMasterTune: return Rule(Reach::NewNotes, "the organ's pitch, which each pipe is made for the next time it speaks", {}, kRinging);
        case kTemperament: return Rule(Reach::NewNotes, "the temperament each pipe is tuned to the next time it speaks", {}, kRinging);
        // Voicing sets each pipe apart from its rank by a hash of its place,
        // Dead Pipes silences one pipe in sixteen, Ciphers hangs one key's
        // pallet in twenty-five when it is let go and Runnings leaks one
        // channel in twelve into its neighbour's (OreganaEngine.cpp, Unit and
        // Spread). Over the one 8' rank the held chord is three pipes, and
        // whether any of them is one so chosen is chance; they are tested on
        // full organ, the roller all the way over, where every division's
        // pipes and channels open under the held chord and the probes.
        case kVoicing:
            return Rule(Reach::NewNotes, "each pipe's voicing, which it is made with the next time it speaks", { { kCrescendo, drawn } },
                        kRinging);
        // A leak makes the pipes of the channel beside an open one whisper
        // along, their breath most of what they give, and each pipe's breath
        // runs on in its own sequence while it sounds (Engine::Reset): in T
        // they have whispered with the long play's keys, in the run they
        // begin at the change. Over probe 1 the run's spectrum, averaged,
        // stands 0.32 dB from T's against R's 0.53, and frame by frame 1.68
        // dB against 2.01 (2026-11-15).
        case kRunnings:
            return Rule(Reach::Held, "", { { kCrescendo, drawn } },
                        "the pipes the leaks make whisper, whose breath runs on in T from the long play's keys");
        case kDeadPipes:
        case kCiphers: return Rule(Reach::Held, "", { { kCrescendo, drawn } });
        // With the blower off the organ plays on the wind in store for
        // seconds (OreganaEngine.cpp, the wind's supply), and on the bellows
        // the pressure sinks only as a bellows does: what changes at once is
        // that the blower's hum runs down and stops, heard with the wind's
        // noise all the way up.
        case kWindSupply: return Rule(Reach::Held, "", { { kBlower, drawn } });
        // The church's decay at the top of the range, over the one 8' rank's
        // few high partials, is heard where the church is all there is.
        case kDecayTreble: return Rule(Reach::Held, "", { { kRoom, drawn } });
        // A coupler is heard only where the manual it brings has a stop
        // drawn and its keys are not already played; a Pedal coupler, from
        // keys that play a manual with nothing drawn.
        case kCouplerPosHw: return Rule(Reach::Held, "", alone({ { kPosGedackt8, drawn } }));
        case kCouplerRecHw: return Rule(Reach::Held, "", alone({ { kRecPrincipal8, drawn } }));
        case kCouplerRecPos: return Rule(Reach::Held, "", alone({ { kRecPrincipal8, drawn }, { kKeyboard, positiv } }));
        case kCouplerHwPed: return Rule(Reach::Held, "", alone({ { kHwOctave8, drawn }, { kKeyboard, recit } }));
        case kCouplerPosPed: return Rule(Reach::Held, "", alone({ { kPosGedackt8, drawn }, { kKeyboard, recit } }));
        case kCouplerRecPed: return Rule(Reach::Held, "", alone({ { kRecPrincipal8, drawn }, { kKeyboard, positiv } }));
        // The swell box and its pedal act on the Recit, whose shutters knock
        // at either end of their travel; a tremulant beats on its manual's
        // wind, its valve thumping once a beat.
        case kSwell:
        case kExpression: return Rule(Reach::Held, "", alone({ { kRecPrincipal8, drawn }, { kCouplerRecHw, drawn } }));
        // A tremulant's beat is a slow swing of the wind: its own spectrum
        // averaged over a probe is the steady pipe's, so it is told by when
        // the wind falls, and that is where the beat has got to. Switched
        // on, the valve begins its beat at the change (Tremulant::Step), so
        // the run's beat is in another phase than T's, which has beaten since
        // the start; Tremulant::Design sets the rate of a beat that goes on
        // from where the valve is in it, so after a change of rate the run's
        // beat is in the phase the old rate left.
        case kTremulantRec:
            return Rule(Reach::Held, "", alone({ { kRecPrincipal8, drawn }, { kCouplerRecHw, drawn } }),
                        "the tremulant's beat, which in T has run since the start and in the run begins at the change");
        case kTremulantPos:
            return Rule(Reach::Held, "", alone({ { kPosGedackt8, drawn }, { kCouplerPosHw, drawn } }),
                        "the tremulant's beat, which in T has run since the start and in the run begins at the change");
        case kTremulantRate: return Rule(Reach::Held, "", {}, "the tremulant's beat, which goes on at the new rate from where the old rate left it");
        // Its depth is heard where the switch is, over the one flute rank. At
        // 0 the beat stops and the trunk rings down to its reservoir: a
        // second after the change the Positiv's wind under a held chord
        // stands within 6e-5 of T's. On the first registration with the
        // tremulant, the Krummhorn's, the run kept where the beat had left
        // each pipe in its cycle, the Krummhorn and the Gedackt on each key
        // adding in other phases than in T: probe 1 stood 2.04 dB from T
        // against R's 2.54, a quarter of its 2.10 dB move short by 0.03,
        // once the pipes' regulation was written again (2026-11-18).
        case kTremulantDepth:
            return Rule(Reach::Held, "", alone({ { kPosGedackt8, drawn }, { kCouplerPosHw, drawn }, { kTremulantPos, drawn } }));
        default: return {};
        }
    }

private:
    void Handle(const matrix::Event& event) {
        const int status = event.bytes[0] & 0xF0, data1 = event.bytes[1] & 0x7F, data2 = event.bytes[2] & 0x7F;
        if (status == 0x90 && data2 > 0) instrument->KeyDown(data1, data2);
        else if (status == 0x80 || status == 0x90) instrument->KeyUp(data1);
        else if (status == 0xE0) instrument->SetBend(((data2 << 7 | data1) - 8192) / 8192.0);
    }
    // Oregana.cpp, COregana::Push.
    void Apply() {
        pending = false;
        Controls played;
        for (int i = 0; i < kControlCount; ++i) played[i] = FromNormalized(i, values[static_cast<std::size_t>(i)]);
        // Reason's master tune, -100 to +100 cents, on the organ's pitch.
        played[kPitch] *= std::pow(2.0, (200.0 * values[kMasterTune] - 100.0) / 1200.0);
        instrument->SetControls(played, Unit(values[kModWheel]), Unit(values[kExpression]));
    }

    std::unique_ptr<Instrument> instrument;
    std::array<float, kValueCount> values {};
    bool pending = false;
};

} // namespace

int main(int argc, char** argv) {
    return protocodus::matrix::Main(argc, argv, [] { return std::make_unique<OreganaDevice>(); });
}
