#pragma once

// Oregana: a physically modelled church organ.
//
// Every pipe of every stop is there all the time, 5,130 of them (Stops.h): a
// flue pipe is a jet driving an air column, a reed pipe a tongue beating on a
// shallot under a resonator (FluePipe.h, ReedPipe.h). A key opens its pallet
// and the pipes of the drawn stops on its channel take wind and speak as
// their voicing lets them; nothing here is an oscillator, an envelope or a
// sample. The wind comes from a blower or a calcant's bellows, or from
// neither (OreganaEngine.cpp), through one reservoir for each division, which
// sags and bounces as the pipes draw on it (Wind.h), the Recit stands in a
// swell box, and the church answers (Nave.h).
//
// MIDI (Midi)
//   channel 1  Hauptwerk (I), notes 21 to 108: an organ's C to c4 is 36 to 96
//   channel 2  Positiv (II)
//   channel 3  Recit (III), enclosed
//   channel 4  Pedal, notes 21 to 67: an organ's C to g1 is 36 to 67
//   CC1        the crescendo roller: adds stops and couplers in kCrescendoOrder's
//              order, from one soft stop to full organ, to what is drawn. The
//              roller takes 0.8 s over its whole travel and each stop comes
//              on its slider, so a wheel thrown over swells and does not step
//   CC11       the swell pedal: 0 shut .. 127 open
//   CC123      all keys up
//   CC120      all keys up, and every hanging pallet freed (Ciphers)
// Other channels play the Hauptwerk. Velocity is how fast the key opens the
// pallet, which a tracker action passes on to the pipes' speech: it changes
// the attack, never the level. A pipe organ has no sustaining pedal.
//
// Nothing in an organ answers at once or in step. A stop comes on as its
// slider crosses the chest, each in its own time (0.1 to 0.2 s), and stays
// part open if its knob is left part drawn: its pipes then get less wind, and
// speak flat, weak and slow. A pipe is heard from where it stands, each
// division at its own depth in the case. A coupler has lost motion, so the
// manual it pulls speaks a few milliseconds late, later for a slow key.
//
// Imperfections. An organ is some thousands of hand-made pipes in an unheated
// building, and no two of them are alike or stay as the tuner left them.
// Each control below turns one such thing from none to plenty, and each pipe
// keeps its own share of it whatever is played (a hash of its place in the
// organ, not a random number):
//   Tuning       every pipe off its pitch, flues by up to 10 cents and reeds
//                by up to 20
//   Temperature  the air's: flue pipes follow the speed of sound, 2.95 cents
//                a degree, and reed tongues do not, so the reeds stand flat
//                of the flues in a warm church and sharp in a cold one
//   Voicing      every pipe off its rank's level (up to 3 dB), speech, chiff
//                and wind noise
//   Wind         how far the reservoirs' sag, dip and bounce reach the pipes,
//                with a slow unrest of the pressure itself
//   Wind Noise   the blower's hum, the chests' hiss and the bellows' thud
//   Action       the keys' and stop knobs' knocks (Mechanics.h), and the
//                couplers' lost motion: none at 0, up to 36 ms at 1
//   Dead Pipes   pipes that do not speak at all
//   Ciphers      pallets that hang part open when their key is let go, so
//                the note whines on, flat and weak, on every stop drawn,
//                until the stops go in or all sound is cut (CC120)
//   Runnings     wind leaking from an open channel into the one a whole
//                tone away on the chest, whose pipes whisper along
// Tuning, Temperature, Voicing, Pitch and Temperament are the pipe maker's
// and the tuner's work: a pipe takes them the next time it speaks.
//
// The engine runs at 48 kHz, the rate its pipes are tuned at (DspMath.h); a
// front end at another rate converts. JUCE-free, C++17, no allocation, locks
// or exceptions after construction. It is a large object: keep it on the heap.

#include "Mechanics.h"
#include "Nave.h"
#include "OreganaControls.h"
#include "OutputStage.h"
#include "Stops.h"
#include "Wind.h"

#include <array>
#include <cstdint>
#include <vector>

namespace oregana {

// The crescendo roller's order: the controls it adds, one by one, to what is
// drawn by hand. Mutations, the undulating stops and the solo reeds stay the
// player's own.
inline constexpr int kCrescendoOrder[] = {
    kPosGedackt8, kHwRohrfloete8, kPedSubbass16, kRecFluteHarmonique8, kCouplerRecHw, kCouplerPosHw, kCouplerHwPed,
    kRecGambe8, kHwGamba8, kPedGedackt8, kPosRohrfloete4, kHwOctave8, kPedOctave8, kRecPrincipal8,
    kRecFluteOctaviante4, kHwSpitzfloete4, kPosPrincipal4, kHwOctave4, kPedPrincipal16, kPedOctave4, kHwPrincipal16,
    kRecOctavin2, kPosOctave2, kHwQuinte223, kHwSuperoctave2, kPosQuinte113, kRecPleinJeu, kHwMixtur, kPedMixtur,
    kPosCimbel, kRecHautbois8, kRecTrompette8, kHwTrompete8, kPedPosaune16, kPedTrompete8, kCouplerRecPed,
    kRecBasson16, kRecClairon4, kHwScharff, kHwTrompete16, kPosDulcian16, kPedUntersatz32, kCouplerPosPed,
    kHwCornet, kPedContraposaune32,
};
constexpr int kCrescendoStages = static_cast<int>(sizeof(kCrescendoOrder) / sizeof(kCrescendoOrder[0]));

struct Diagnostics {
    int pipes = 0;                               // speaking or ringing
    std::array<double, kDivisions> pressure {};  // each reservoir's, against nominal
    std::array<double, kDivisions> demand {};    // the wind each division takes: 1 at full organ
    double supply = 1.0;                         // the pressure the wind supply holds, against nominal
    int stage = 0;                               // how many of kCrescendoOrder the roller has added
};

class Engine {
public:
    Engine();
    void Reset();                         // silence, every key up, the wind at rest
    void SetControls(const Controls& controls);
    const Controls& GetControls() const { return controls; }

    void Midi(const std::uint8_t* bytes, int size);
    void KeyDown(int division, int note, int velocity);   // MIDI note numbers: 60 is c1
    void KeyUp(int division, int note);
    void AllKeysUp();
    void AllSoundOff();                   // and every hanging pallet freed
    void SetSwell(double position);       // as the Swell control and CC11 do
    void SetCrescendo(double position);   // as the Crescendo control and CC1 do: where the roller is turned to
    // The pitch wheel, -1 .. 1, as MIDI pitch bend does: it warms and cools
    // the air by up to kBendDegrees on top of Temperature. Flues follow while
    // they sound, gliding; reeds, whose tongues set their pitch, do not.
    void SetBend(double bend);
    static constexpr double kBendDegrees = 10.0;
    void Render(float* left, float* right, int frames);

    // The output stage's look-ahead, samples: a front end reports it to its
    // host as latency.
    int LatencySamples() const { return output.DelaySamples(); }
    const Diagnostics& Diag() const { return diag; }
    // A pipe's sounding frequency as it is tuned now, Hz, or 0 where no pipe
    // stands: for tools and tests.
    double PipeHz(int rank, int note) const;

private:
    struct Slot {
        std::int16_t index = -1;      // among the flues or the reeds; -1: no pipe stands here
        std::uint8_t rank = 0, key = 0, division = 0;
        bool reed = false, active = false;
        bool resting = false;         // its key is held and there is no wind for it: silent and cleared until some comes
        std::uint32_t epoch = 0;      // the tuning it was last made for
        std::uint16_t delay = 0;      // samples from its pallet's opening to its being heard (Engine::Engine)
        std::uint64_t lastDue = 0;    // when the wind last sent to it arrives
        float offered = 0.0f;         // the share of its division's wind its pallet now gives
        float share = 0.0f;           // and the share that has reached it
        float gain = 0.0f;            // its regulated level
        float air = 0.0f;             // the wind it takes
        // A flue pipe among its neighbours (OreganaEngine.cpp, kHearMetres).
        std::uint8_t row = 0;         // its rank's row on the chest, from the division's first
        std::uint8_t outDelay = 0;    // samples its sound is heard later for its row
        bool apart = false;           // a beating rank stands away from the others
        bool coupled = false;         // it hears a neighbour, and is heard
        float radius = 0.0f;          // metres
        std::int16_t hears[2] = { -1, -1 };   // the flues it hears, towards the front and the back: -1 none
        float hearGain[2] = {};       // each one's sign and 1 / distance
        std::uint8_t hearDelay[2] = {};   // and its sound's way here, samples
        float* buffer = nullptr;      // its delay lines, and the room each has
        int loopRoom = 0, jetRoom = 0, upperRoom = 0;
    };

    void Apply(const Controls& before, bool all);
    void PlaceKeys();
    void Register();
    void SetSlider(int stop, float position);
    void UpdateKey(int key, bool struck = false, int own = -1);
    void UpdateChannel(int division, int key, int late = 0);
    void Offer(int slot, float share, int late);
    bool Take(int slot, float share);
    void Design(int slot);
    void Place(Slot& slot);
    void Tick();
    void RenderChunk(float* left, float* right, int frames);
    double Cents(const Slot& slot) const;

    Controls controls;
    std::vector<Slot> slots;
    std::vector<FluePipe> flues;
    std::vector<ReedPipe> reeds;
    std::vector<float> arena;
    // The speaking and ringing pipes' slots, flues and reeds in one list:
    // its order is the order their sound is added up in, which must not
    // depend on how the host cuts its blocks.
    std::vector<int> active;
    struct Pending { int slot; std::uint64_t due; float share; };
    std::vector<Pending> pending;                // wind on its way to a pipe, soonest first
    std::uint64_t now = 0;                       // samples rendered
    std::uint32_t epoch = 1;
    int redesignAt = 0, stale = 0;

    // The keys and what they open.
    std::array<std::array<bool, kManualKeys>, kDivisions> played {}, open {};
    std::array<std::array<float, kManualKeys>, kDivisions> hang {}, share {}, robbed {}, fullAir {};
    std::array<std::array<float, kManualKeys>, kDivisions> placeLeft {}, placeRight {};
    std::array<float, kManualKeys> touch {};
    // Each stop's slider, 0 shut .. 1 open, where its knob and the roller
    // want it and how far it travels a tick; and what that leaves its ranks.
    std::array<float, kStopCount> slider {}, sliderTarget {}, sliderStep {};
    std::array<float, kRankCount> opening {};      // the share of the channel's pressure the slider passes
    std::array<bool, kRankCount> rankOpen {};
    std::array<bool, kCouplerCount> coupled {};
    double swell = 1.0, crescendo = 0.0, roller = 0.0;
    double pressureSet = 1.0, widthSet = 0.0;      // Wind Pressure and Width as they have arrived: both glide
    // The wheel's air, C: where it is turned to and where it has glided, and
    // how much longer that makes a flue's every length, against 1.
    double bendSet = 0.0, bendAir = 0.0;
    float stretch = 0.0f;
    int stage = 0;

    // The wind.
    double spin = 1.0, stock = 1.0;                // the blower's speed and the wind in store, each 0..1
    std::array<Reservoir, kDivisions> reservoirs {};
    std::array<Tremulant, 2> tremulants {};        // Positiv, Recit
    std::array<double, kDivisions> fullWind {}, demand {}, unrest {}, unrestSlow {};
    std::array<float, kDivisions> pressure {};
    std::uint32_t unrestRandom = 1u;

    // From the pipes to the listener.
    static constexpr int kChunk = FluePipe::kTick;
    std::array<std::array<std::array<float, kChunk>, 2>, kDivisions> bus {};
    // The rows one and two steps behind the front, on their way to the bus.
    static constexpr int kRowSamples = 17;   // a row's 12 cm at the pipes' rate
    std::array<std::array<std::array<std::array<float, kChunk + 2 * kRowSamples>, 2>, 2>, kDivisions> rowBus {};
    // Each speaking pipe's sound for a chunk, made before any is added up,
    // and the pipes of each kind that are stepped four at a time.
    std::vector<float> scratch;
    // Flues by kind: with a pipe beyond a junction (+1), hearing neighbours (+2).
    std::array<std::vector<FluePipe*>, 4> flueList;
    std::array<std::vector<float*>, 4> flueSound;
    std::array<std::vector<std::size_t>, 2> flueAt;    // the hearing flues' places among the speaking pipes
    // The last kRing samples of what each flue tells its neighbours, by the
    // sample's number, with the first block again after the last; and each
    // hearing flue's block of what it hears.
    static constexpr int kRing = 64, kHearBlock = 16, kRingSize = kRing + kHearBlock;
    std::vector<float> toldRing, hearScratch;
    std::vector<ReedPipe*> reedList;
    std::vector<float*> reedSound;
    // Silent pipes that make up a four when two or three of a kind are left
    // over (flues without and with a pipe beyond a junction, and reeds):
    // they take no wind, and their sound goes nowhere.
    static constexpr int kSpares = 2;
    std::array<std::array<FluePipe, kSpares>, 2> spareFlue;
    std::array<ReedPipe, kSpares> spareReed;
    std::vector<float> spareArena;
    std::array<std::array<float, kChunk>, kSpares> spareSound {};
    std::array<std::array<float, kHearBlock>, kSpares> spareTold {};
    std::array<float, kHearBlock> spareHeard {};
    int untilTick = 0;
    float boxLow[2] {}, boxLowGain = 1.0f, boxHighGain = 1.0f, boxLowTarget = 1.0f, boxHighTarget = 1.0f;
    float dry = 0.0f, wet = 0.0f, dryTarget = 0.0f, wetTarget = 0.0f;
    float presence = 0.0f, hum = 0.0f, hiss = 0.0f, rub = 0.0f;
    Knocks knocks;
    WindNoise windNoise;
    Nave nave;
    protocodus::OutputStage<2> output;
    Diagnostics diag;
};

} // namespace oregana
