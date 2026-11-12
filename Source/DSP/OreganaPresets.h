#pragma once

// The factory bank every front end and the demo renderer read: registrations,
// from one stop alone to full organ, as an organist would draw them. Each
// plays from the Hauptwerk's keys (MIDI channel 1), with the couplers drawn
// that bring the other manuals' stops there, and its Pedal stops from the
// Pedal's (channel 4).

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

// The default controls with `drawn` (stops, couplers, tremulants) drawn and
// `set` set.
constexpr Preset Registration(const char* name, std::initializer_list<int> drawn, std::initializer_list<Setting> set = {}) {
    Preset preset { name, Controls().value };
    for (const int control : drawn) preset.values[static_cast<std::size_t>(control)] = 1.0;
    for (const Setting& s : set) preset.values[static_cast<std::size_t>(s.control)] = s.value;
    return preset;
}

// The soft registrations carry a Volume above 0 dB, so that one stop alone
// can be heard beside full organ without reaching for the level: at Volume 0
// every preset has the organ's own dynamics, 18 dB from the Gedackt to the
// Tutti on one chord. The trims are the whole decibels RenderOregana --levels
// reads, which bring each preset to -25 dBFS RMS on that chord in five keys
// or leave it where it is above that.
inline constexpr std::array<Preset, 17> kPresets { {
    Registration("Principal 8", { kHwOctave8, kPedSubbass16, kPedOctave8 }, { { kVolume, 6.0 } }),
    Registration("Gedackt 8", { kPosGedackt8, kCouplerPosHw, kPedSubbass16 }, { { kVolume, 12.0 } }),
    Registration("Flutes 8 and 4", { kHwRohrfloete8, kHwSpitzfloete4, kPedSubbass16, kPedGedackt8 }, { { kVolume, 10.0 } }),
    Registration("Flute harmonique", { kRecFluteHarmonique8, kCouplerRecHw, kPedSubbass16, kPedGedackt8 }, { { kVolume, 9.0 } }),
    Registration("Voix celeste", { kRecGambe8, kRecVoixCeleste8, kCouplerRecHw, kPedSubbass16 }, { { kVolume, 11.0 } }),
    Registration("Voce umana", { kRecPrincipal8, kRecVoceUmana8, kCouplerRecHw, kPedSubbass16, kPedOctave8 }, { { kVolume, 8.0 } }),
    Registration("Cornet", { kHwCornet, kHwRohrfloete8, kPedSubbass16, kPedOctave8 }, { { kVolume, 7.0 } }),
    Registration("Jeu de tierce", { kPosGedackt8, kPosRohrfloete4, kPosNasat223, kPosOctave2, kPosTerz135, kCouplerPosHw,
                                    kPedSubbass16, kPedGedackt8 }, { { kVolume, 9.0 } }),
    Registration("Krummhorn", { kPosKrummhorn8, kPosGedackt8, kTremulantPos, kCouplerPosHw, kPedSubbass16, kPedGedackt8 },
                 { { kVolume, 8.0 } }),
    Registration("Vox humana", { kPosVoxHumana8, kPosGedackt8, kTremulantPos, kCouplerPosHw, kPedSubbass16 }, { { kVolume, 11.0 } }),
    Registration("Hautbois", { kRecHautbois8, kRecFluteHarmonique8, kCouplerRecHw, kPedSubbass16, kPedGedackt8 }, { { kVolume, 8.0 } }),
    Registration("Trompete", { kHwTrompete8, kHwOctave8, kHwOctave4, kPedPrincipal16, kPedOctave8, kPedTrompete8 }, { { kVolume, 2.0 } }),
    Registration("Principal Chorus", { kHwOctave8, kHwOctave4, kHwSuperoctave2, kPedPrincipal16, kPedOctave8, kPedOctave4 },
                 { { kVolume, 4.0 } }),
    Registration("Plenum", { kHwPrincipal16, kHwOctave8, kHwOctave4, kHwQuinte223, kHwSuperoctave2, kHwMixtur,
                             kPosGedackt8, kPosPrincipal4, kPosOctave2, kPosCimbel, kCouplerPosHw,
                             kPedPrincipal16, kPedOctave8, kPedOctave4, kPedMixtur, kPedPosaune16 }),
    Registration("Grand jeu", { kHwCornet, kHwOctave4, kHwTrompete8, kHwTrompete16, kPosKrummhorn8, kPosPrincipal4,
                                kRecBasson16, kRecTrompette8, kRecClairon4, kCouplerPosHw, kCouplerRecHw,
                                kPedPrincipal16, kPedOctave8, kPedPosaune16, kPedTrompete8 }),
    Registration("Full Swell", { kRecPrincipal8, kRecFluteHarmonique8, kRecGambe8, kRecFluteOctaviante4, kRecOctavin2,
                                 kRecPleinJeu, kRecBasson16, kRecTrompette8, kRecHautbois8, kRecClairon4, kCouplerRecHw,
                                 kPedSubbass16, kPedPrincipal16, kPedOctave8, kCouplerRecPed }, { { kVolume, 5.0 }, { kSwell, 0.5 } }),
    Registration("Tutti", { kHwPrincipal16, kHwOctave8, kHwGamba8, kHwRohrfloete8, kHwOctave4, kHwSpitzfloete4, kHwQuinte223,
                            kHwSuperoctave2, kHwCornet, kHwMixtur, kHwScharff, kHwTrompete16, kHwTrompete8,
                            kPosGedackt8, kPosPrincipal4, kPosRohrfloete4, kPosOctave2, kPosQuinte113, kPosCimbel, kPosDulcian16,
                            kRecPrincipal8, kRecFluteHarmonique8, kRecGambe8, kRecFluteOctaviante4, kRecOctavin2, kRecPleinJeu,
                            kRecBasson16, kRecTrompette8, kRecHautbois8, kRecClairon4,
                            kPedUntersatz32, kPedPrincipal16, kPedSubbass16, kPedOctave8, kPedGedackt8, kPedOctave4, kPedMixtur,
                            kPedContraposaune32, kPedPosaune16, kPedTrompete8,
                            kCouplerPosHw, kCouplerRecHw, kCouplerHwPed, kCouplerPosPed, kCouplerRecPed }),
} };

} // namespace oregana
