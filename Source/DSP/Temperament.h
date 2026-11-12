#pragma once

// How the tuner sets the twelve notes of the octave: each temperament is the
// cents each pitch class stands from equal temperament, with a at 0, so the
// Pitch control means a1 in every one of them. A mutation or mixture pipe is
// tuned pure to its own key's note, as a tuner tunes it, so the temperament
// moves a whole key, never one rank of it.
//
// Werckmeister III (Andreas Werckmeister, Musicalische Temperatur, 1691) and
// Kirnberger III (Johann Philipp Kirnberger, 1779) are the two usually set on
// organs for Bach; Vallotti spreads the comma over six fifths; quarter-comma
// meantone is the tuning of the organs before him, with eight pure thirds
// and a wolf between g sharp and e flat.

#include <array>

namespace oregana {

struct Temperament {
    const char* name;
    std::array<double, 12> cents;   // c, c sharp, d ... b
};

inline constexpr std::array<Temperament, 5> kTemperaments { {
    { "Equal", { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { "Werckmeister III", { 11.73, 1.955, 3.91, 5.865, 1.955, 9.775, 0.0, 7.82, 3.91, 0.0, 7.82, 3.91 } },
    { "Kirnberger III", { 10.265, 0.49, 3.422, 4.4, -3.421, 8.31, 0.489, 6.843, 2.445, 0.0, 6.355, -1.466 } },
    { "Vallotti", { 5.865, 0.0, 1.955, 3.91, -1.955, 7.82, -1.955, 3.91, 1.955, 0.0, 5.865, -3.91 } },
    { "Meantone", { 10.265, -13.686, 3.422, 20.53, -3.421, 13.687, -10.264, 6.843, -17.108, 0.0, 17.108, -6.843 } },
} };

} // namespace oregana
