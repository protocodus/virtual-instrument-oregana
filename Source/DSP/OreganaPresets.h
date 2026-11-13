#pragma once

// The factory bank every front end and the demo renderer read. First the
// registrations, from one stop alone to full organ, as an organist would draw
// them; then twenty whole organs (kFirstOrgan), each a smaller or larger
// instrument made of this one's stops, in its own building and in its own
// state: five chapel organs, ten church organs and five cathedral organs.
// Each preset plays from the Hauptwerk's keys (MIDI channel 1), with the
// couplers drawn that bring the other manuals' stops there, and its Pedal
// stops from the Pedal's (channel 4).

#include "OreganaControls.h"

#include <array>
#include <initializer_list>

namespace oregana {

struct Preset {
    const char* name;
    // Physical values in Control order.
    std::array<double, kControlCount> values;

    constexpr Controls ToControls() const {
        Controls c;
        c.value = values;
        return c;
    }
};

struct Setting {
    int control;
    double value;
};

// The building a preset stands in follows the size of what is drawn (owner,
// 2026-11-13: the smaller the organ, the less reverberation): a few stops are
// a positive in a chapel, fifteen the organ as it loads, twenty-two and more a
// cathedral's. Typical values, between which a preset's stop count is placed.
struct Building {
    int stops;
    double room, size, decay, bass, width;
};
inline constexpr Building kBuildings[] {
    { 3, 0.35, 0.5, 1.3, 1.0, 0.5 },
    { 6, 0.45, 0.65, 2.0, 1.1, 0.6 },
    { 10, 0.52, 0.9, 3.2, 1.2, 0.75 },
    { 15, 0.6, 1.0, 4.5, 1.3, 0.8 },
    { 22, 0.68, 1.8, 8.0, 1.4, 1.0 },
};

constexpr Building BuildingFor(int stops) {
    const Building* above = kBuildings;
    while (above->stops < stops && above != kBuildings + 4) ++above;
    if (above == kBuildings || above->stops <= stops) return *above;
    const Building& below = above[-1];
    const double t = static_cast<double>(stops - below.stops) / (above->stops - below.stops);
    const auto between = [t](double a, double b) { return a + t * (b - a); };
    return { stops, between(below.room, above->room), between(below.size, above->size), between(below.decay, above->decay),
             between(below.bass, above->bass), between(below.width, above->width) };
}

// The default controls with `drawn` (stops, couplers, tremulants) drawn, the
// building their stops call for, and `set` set.
constexpr Preset Registration(const char* name, std::initializer_list<int> drawn, std::initializer_list<Setting> set = {}) {
    Preset preset { name, Controls().value };
    int stops = 0;
    for (const int control : drawn) {
        preset.values[static_cast<std::size_t>(control)] = 1.0;
        if (control < kStopCount) ++stops;
    }
    const Building building = BuildingFor(stops);
    preset.values[kRoom] = building.room;
    preset.values[kRoomSize] = building.size;
    preset.values[kDecay] = building.decay;
    preset.values[kDecayBass] = building.bass;
    preset.values[kWidth] = building.width;
    for (const Setting& s : set) preset.values[static_cast<std::size_t>(s.control)] = s.value;
    return preset;
}

// The soft registrations carry a Volume above 0 dB, so that one stop alone
// can be heard beside full organ without reaching for the level: at Volume 0
// every preset has the organ's own dynamics, 18 dB from the Gedackt to the
// Tutti on one chord. The trims are the whole decibels RenderOregana --levels
// reads, which bring each preset to -25 dBFS RMS on that chord in five keys
// or leave it where it is above that.
//
// An organ draws only the stops it would have, and what it sets beside them
// is typical of such an instrument and its building, not a measurement of
// one: a chapel's short dry room and a positive's treadled bellows, a
// cathedral's long nave. The other stops stay on the panel, and the
// crescendo roller still brings on this organ's whole stop list.
inline constexpr int kFirstOrgan = 17;
inline constexpr std::array<Preset, 37> kPresets { {
    Registration("Principal 8", { kHwOctave8, kPedSubbass16, kPedOctave8 }, { { kVolume, 7.0 } }),
    Registration("Gedackt 8", { kPosGedackt8, kCouplerPosHw, kPedSubbass16 }, { { kVolume, 11.0 } }),
    Registration("Flutes 8 and 4", { kHwRohrfloete8, kHwSpitzfloete4, kPedSubbass16, kPedGedackt8 }, { { kVolume, 9.0 } }),
    Registration("Flute harmonique", { kRecFluteHarmonique8, kCouplerRecHw, kPedSubbass16, kPedGedackt8 }, { { kVolume, 9.0 } }),
    Registration("Voix celeste", { kRecGambe8, kRecVoixCeleste8, kCouplerRecHw, kPedSubbass16 }, { { kVolume, 10.0 } }),
    Registration("Voce umana", { kRecPrincipal8, kRecVoceUmana8, kCouplerRecHw, kPedSubbass16, kPedOctave8 }, { { kVolume, 7.0 } }),
    Registration("Cornet", { kHwCornet, kHwRohrfloete8, kPedSubbass16, kPedOctave8 }, { { kVolume, 7.0 } }),
    Registration("Jeu de tierce", { kPosGedackt8, kPosRohrfloete4, kPosNasat223, kPosOctave2, kPosTerz135, kCouplerPosHw,
                                    kPedSubbass16, kPedGedackt8 }, { { kVolume, 8.0 } }),
    Registration("Krummhorn", { kPosKrummhorn8, kPosGedackt8, kTremulantPos, kCouplerPosHw, kPedSubbass16, kPedGedackt8 },
                 { { kVolume, 8.0 } }),
    Registration("Vox humana", { kPosVoxHumana8, kPosGedackt8, kTremulantPos, kCouplerPosHw, kPedSubbass16 }, { { kVolume, 10.0 } }),
    Registration("Hautbois", { kRecHautbois8, kRecFluteHarmonique8, kCouplerRecHw, kPedSubbass16, kPedGedackt8 }, { { kVolume, 7.0 } }),
    Registration("Trompete", { kHwTrompete8, kHwOctave8, kHwOctave4, kPedPrincipal16, kPedOctave8, kPedTrompete8 }, { { kVolume, 2.0 } }),
    Registration("Principal Chorus", { kHwOctave8, kHwOctave4, kHwSuperoctave2, kPedPrincipal16, kPedOctave8, kPedOctave4 },
                 { { kVolume, 5.0 } }),
    Registration("Plenum", { kHwPrincipal16, kHwOctave8, kHwOctave4, kHwQuinte223, kHwSuperoctave2, kHwMixtur,
                             kPosGedackt8, kPosPrincipal4, kPosOctave2, kPosCimbel, kCouplerPosHw,
                             kPedPrincipal16, kPedOctave8, kPedOctave4, kPedMixtur, kPedPosaune16 }),
    Registration("Grand jeu", { kHwCornet, kHwOctave4, kHwTrompete8, kHwTrompete16, kPosKrummhorn8, kPosPrincipal4,
                                kRecBasson16, kRecTrompette8, kRecClairon4, kCouplerPosHw, kCouplerRecHw,
                                kPedPrincipal16, kPedOctave8, kPedPosaune16, kPedTrompete8 }),
    Registration("Full Swell", { kRecPrincipal8, kRecFluteHarmonique8, kRecGambe8, kRecFluteOctaviante4, kRecOctavin2,
                                 kRecPleinJeu, kRecBasson16, kRecTrompette8, kRecHautbois8, kRecClairon4, kCouplerRecHw,
                                 kPedSubbass16, kPedPrincipal16, kPedOctave8, kCouplerRecPed }, { { kVolume, 4.0 }, { kSwell, 0.5 } }),
    Registration("Tutti", { kHwPrincipal16, kHwOctave8, kHwGamba8, kHwRohrfloete8, kHwOctave4, kHwSpitzfloete4, kHwQuinte223,
                            kHwSuperoctave2, kHwCornet, kHwMixtur, kHwScharff, kHwTrompete16, kHwTrompete8,
                            kPosGedackt8, kPosPrincipal4, kPosRohrfloete4, kPosOctave2, kPosQuinte113, kPosCimbel, kPosDulcian16,
                            kRecPrincipal8, kRecFluteHarmonique8, kRecGambe8, kRecFluteOctaviante4, kRecOctavin2, kRecPleinJeu,
                            kRecBasson16, kRecTrompette8, kRecHautbois8, kRecClairon4,
                            kPedUntersatz32, kPedPrincipal16, kPedSubbass16, kPedOctave8, kPedGedackt8, kPedOctave4, kPedMixtur,
                            kPedContraposaune32, kPedPosaune16, kPedTrompete8,
                            kCouplerPosHw, kCouplerRecHw, kCouplerHwPed, kCouplerPosPed, kCouplerRecPed }),

    // Five chapel organs: a handful of stops in a small room.
    Registration("Chapel Positive", { kPosGedackt8, kPosRohrfloete4, kPosOctave2, kCouplerPosHw },
                 { { kVolume, 12.0 }, { kPedalMode, kPedalOff },
                   { kWindSupply, kWindBellows }, { kTemperament, 4 }, { kPitch, 415.0 }, { kAction, 0.5 } }),
    Registration("Chapel Italian", { kRecPrincipal8, kHwOctave4, kHwSuperoctave2, kPosQuinte113, kCouplerRecHw, kCouplerPosHw,
                                     kPedSubbass16 },
                 { { kVolume, 6.0 }, { kWindSupply, kWindBellows }, { kTemperament, 4 }, { kVoicing, 0.4 } }),
    Registration("Chapel Village", { kHwOctave8, kHwRohrfloete8, kHwOctave4, kHwSuperoctave2, kPedSubbass16 },
                 { { kVolume, 6.0 }, { kWindSupply, kWindBellows },
                   { kTemperament, 1 }, { kTuning, 0.5 }, { kVoicing, 0.5 }, { kAction, 0.5 },
                   { kWind, 0.7 } }),
    Registration("Chapel Romantic", { kRecPrincipal8, kRecFluteHarmonique8, kRecGambe8, kRecFluteOctaviante4, kRecHautbois8,
                                      kCouplerRecHw, kPedSubbass16 },
                 { { kVolume, 8.0 }, { kSwell, 0.7 }, { kWind, 0.3 } }),
    Registration("Chapel Derelict", { kPosGedackt8, kPosPrincipal4, kPosOctave2, kPosQuinte113, kCouplerPosHw, kPedSubbass16 },
                 { { kVolume, 10.0 }, { kWindSupply, kWindBellows },
                   { kPressure, 0.85 }, { kWind, 0.9 }, { kBlower, 0.5 }, { kTemperament, 1 },
                   { kTuning, 0.9 }, { kVoicing, 0.8 }, { kTemperature, -6.0 }, { kAction, 0.8 }, { kDeadPipes, 0.35 },
                   { kRunnings, 0.4 } }),

    // Ten church organs: two manuals and pedal in a parish church.
    Registration("Church Baroque", { kHwOctave8, kHwOctave4, kHwQuinte223, kHwSuperoctave2, kHwMixtur,
                                     kPosGedackt8, kPosPrincipal4, kPosOctave2, kCouplerPosHw,
                                     kPedPrincipal16, kPedOctave8, kPedOctave4, kPedPosaune16 },
                 { { kVolume, 1.0 }, { kTemperament, 1 } }),
    Registration("Church Saxon", { kHwOctave8, kHwRohrfloete8, kHwOctave4, kHwQuinte223, kHwSuperoctave2, kHwCornet, kHwMixtur,
                                   kHwTrompete8, kPedSubbass16, kPedPrincipal16, kPedOctave8, kPedPosaune16 },
                 { { kTemperament, 3 }, { kPitch, 466.0 } }),
    Registration("Church Flutes", { kHwRohrfloete8, kHwSpitzfloete4, kPosGedackt8, kPosRohrfloete4, kCouplerPosHw,
                                    kPedSubbass16, kPedGedackt8 },
                 { { kVolume, 8.0 } }),
    Registration("Church French", { kHwCornet, kHwOctave4, kHwTrompete8, kPosKrummhorn8, kPosPrincipal4, kCouplerPosHw,
                                    kPedOctave8, kPedTrompete8 },
                 { { kVolume, 2.0 }, { kTemperament, 2 }, { kPitch, 415.0 } }),
    Registration("Church Italian", { kRecPrincipal8, kHwOctave8, kHwOctave4, kHwQuinte223, kHwSuperoctave2, kPosQuinte113,
                                     kPosSiffloete1, kCouplerRecHw, kCouplerPosHw, kPedSubbass16, kPedOctave8 },
                 { { kVolume, 3.0 }, { kTemperament, 4 } }),
    Registration("Church Romantic", { kHwPrincipal16, kHwOctave8, kHwGamba8, kHwRohrfloete8, kHwOctave4,
                                      kRecPrincipal8, kRecFluteHarmonique8, kRecGambe8, kRecFluteOctaviante4, kRecHautbois8,
                                      kCouplerRecHw, kPedSubbass16, kPedPrincipal16, kPedOctave8, kPedGedackt8 },
                 { { kVolume, 2.0 }, { kSwell, 0.8 }, { kWind, 0.3 } }),
    Registration("Church Hymn", { kHwOctave8, kHwRohrfloete8, kHwOctave4, kHwSuperoctave2, kPosGedackt8, kPosPrincipal4,
                                  kCouplerPosHw, kPedSubbass16, kPedOctave8 },
                 { { kVolume, 4.0 } }),
    Registration("Church Reeds", { kHwOctave8, kHwOctave4, kHwTrompete8, kPosDulcian16, kPosKrummhorn8, kPosPrincipal4,
                                   kPosOctave2, kCouplerPosHw, kPedPosaune16, kPedTrompete8, kPedCornet2 },
                 { { kTemperament, 1 } }),
    Registration("Church Evensong", { kRecPrincipal8, kRecFluteHarmonique8, kRecGambe8, kRecVoixCeleste8, kRecHautbois8,
                                      kCouplerRecHw, kPedSubbass16, kPedGedackt8 },
                 { { kVolume, 9.0 }, { kSwell, 0.45 }, { kWind, 0.3 } }),
    Registration("Church Winter", { kHwOctave8, kHwOctave4, kHwSuperoctave2, kHwMixtur, kHwTrompete8,
                                    kPedPrincipal16, kPedOctave8, kPedPosaune16 },
                 { { kVolume, 1.0 }, { kWindSupply, kWindBellows },
                   { kWind, 0.8 }, { kBlower, 0.5 }, { kTemperament, 1 }, { kTuning, 0.6 },
                   { kTemperature, -8.0 }, { kAction, 0.6 } }),

    // Five cathedral organs: three manuals and a 32' Pedal down a long nave.
    Registration("Cathedral Plenum", { kHwPrincipal16, kHwOctave8, kHwOctave4, kHwQuinte223, kHwSuperoctave2, kHwMixtur, kHwScharff,
                                       kPosGedackt8, kPosPrincipal4, kPosOctave2, kPosQuinte113, kPosCimbel,
                                       kRecPrincipal8, kRecFluteOctaviante4, kRecOctavin2, kRecPleinJeu,
                                       kCouplerPosHw, kCouplerRecHw,
                                       kPedUntersatz32, kPedPrincipal16, kPedOctave8, kPedOctave4, kPedMixtur, kPedPosaune16 }),
    Registration("Cathedral Symphony", { kHwPrincipal16, kHwOctave8, kHwGamba8, kHwRohrfloete8, kHwOctave4,
                                         kPosGedackt8, kPosQuintadena8, kPosPrincipal4,
                                         kRecPrincipal8, kRecFluteHarmonique8, kRecGambe8, kRecFluteOctaviante4, kRecOctavin2,
                                         kRecBasson16, kRecTrompette8, kRecHautbois8, kRecClairon4,
                                         kCouplerPosHw, kCouplerRecHw, kCouplerRecPed,
                                         kPedUntersatz32, kPedPrincipal16, kPedSubbass16, kPedOctave8, kPedGedackt8 },
                 { { kSwell, 0.6 }, { kWind, 0.3 } }),
    Registration("Cathedral Reeds", { kHwOctave8, kHwOctave4, kHwCornet, kHwMixtur, kHwTrompete16, kHwTrompete8,
                                      kPosPrincipal4, kPosCimbel, kPosDulcian16, kPosKrummhorn8,
                                      kRecPleinJeu, kRecBasson16, kRecTrompette8, kRecClairon4,
                                      kCouplerPosHw, kCouplerRecHw,
                                      kPedPrincipal16, kPedOctave8, kPedMixtur, kPedContraposaune32, kPedPosaune16, kPedTrompete8,
                                      kPedCornet2 }),
    Registration("Cathedral Fonds", { kHwPrincipal16, kHwOctave8, kHwGamba8, kHwRohrfloete8,
                                      kPosGedackt8, kPosQuintadena8,
                                      kRecPrincipal8, kRecFluteHarmonique8, kRecGambe8, kRecVoixCeleste8,
                                      kCouplerPosHw, kCouplerRecHw,
                                      kPedUntersatz32, kPedPrincipal16, kPedSubbass16, kPedOctave8, kPedGedackt8 },
                 { { kVolume, 1.0 }, { kWind, 0.3 } }),
    Registration("Cathedral Nave", { kHwPrincipal16, kHwOctave8, kHwOctave4, kHwQuinte223, kHwSuperoctave2, kHwMixtur, kHwTrompete8,
                                     kPosGedackt8, kPosPrincipal4, kPosOctave2, kPosCimbel,
                                     kRecPrincipal8, kRecFluteOctaviante4, kRecPleinJeu, kRecTrompette8,
                                     kCouplerPosHw, kCouplerRecHw,
                                     kPedUntersatz32, kPedPrincipal16, kPedOctave8, kPedOctave4, kPedMixtur, kPedPosaune16 },
                 { { kRoom, 0.95 } }),
} };

} // namespace oregana
