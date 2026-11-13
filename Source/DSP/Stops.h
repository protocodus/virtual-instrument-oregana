#pragma once

// The organ: its divisions, stops and ranks, and how each pipe is scaled and
// voiced.
//
// The core is a German baroque organ of the kind Bach played: the plan of Arp
// Schnitger's organs (a principal chorus with its own mixtures on each
// manual, and an independent pedal from 32' to 2' with a reed at every pitch;
// Hamburg, St. Jacobi, 1693) with the colours of Gottfried Silbermann and
// Zacharias Hildebrandt (Viola da Gamba, Cornet, Unda maris, Vox humana;
// Freiberg, 1714, and Naumburg, 1746). The third manual is what the other
// schools add (owner's choice, 2026-11-12): an enclosed Recit after Aristide
// Cavaille-Coll (harmonic flutes, Gambe and Voix celeste, a reed chorus from
// 16' to 4'; Paris, Saint-Sulpice, 1862) with the Italian Voce umana, a
// principal tuned to beat against its neighbour (Brescia, San Carlo).
// Docs/research/organ-registers-2026-11-12.md has those organs' stop lists and
// their sources. It is no one organ's specification, and the scales and
// voicing below are typical of each kind of stop, not measurements of any
// instrument.
//
// A stop draws one rank of pipes, or several (a mixture, the Cornet). A rank
// is a family of pipe (how it is built and voiced), a pitch against the 8'
// that may break back from octave to octave, a scale and a level. Every pipe
// is tuned and regulated by measurement (PipeRegulation.h).

#include "FluePipe.h"
#include "ReedPipe.h"

#include <cmath>

namespace oregana {

// The keys are a piano's, so that every key of a controller keyboard sounds
// at its own pitch: an organ's own compass is C to c4 on the manuals and C to
// g1 on the Pedal (MIDI 36 to 96 and 36 to 67), and the keys beyond it are no
// organ's. A rank runs on to them as it stands, with the limits PlanPipe gives.
constexpr int kFirstNote = 21;
constexpr int kManualKeys = 88;   // A0 to C8, MIDI 21 to 108
constexpr int kPedalKeys = 47;    // A0 to g1, MIDI 21 to 67
constexpr int kOrganC = 36;       // the organ's own lowest key, which the ranks' octaves count from

enum Division { kHauptwerk, kPositiv, kRecit, kPedal, kDivisions };
constexpr int kDivisionKeys[kDivisions] = { kManualKeys, kManualKeys, kManualKeys, kPedalKeys };

enum class Family {
    Principal,    // open, medium scale: the organ's own tone
    Flute,        // open, wide and gently blown
    Taper,        // open, narrowing to a small top: Spitzfloete
    Stopped,      // stopped, wide: Gedackt, Subbass
    Chimney,      // stopped, with a chimney in the stopper: Rohrfloete
    Quintadena,   // stopped, narrow and hard blown: the twelfth as loud as the note
    String,       // open, very narrow: Viola da Gamba
    Harmonic,     // open, wide, of double length from c1 up and blown to its octave: Flute harmonique
    Trumpet,      // reed, full-length cone
    Trompette,    // reed, full-length cone on a flatter tongue: the French trumpet
    Hautbois,     // reed, narrow cone: Basson and Hautbois
    Posaune,      // reed, full-length cone of wide scale on a curved tongue
    Krummhorn,    // reed, half-length cylinder
    Dulcian,      // reed, half-length cylinder, soft
    VoxHumana,    // reed, short cylinder
};

struct RankSpec {
    Family family;
    float harmonics[6];   // pitch against the 8' in each octave of the keys: C, c0, c1, c2, c3, and c4
    float scale;          // halftones wider than the family's own scale
    float level;          // dB against the Hauptwerk's Octave 8'
    int firstKey;         // the lowest key with a pipe, in keys above C; 0 for a rank of the whole compass
    float cents;          // tuned off its pitch at c1 and below, for a rank that beats
};

struct StopSpec {
    const char* id;       // never renamed: sessions and patches recall stops by it
    const char* name;
    Division division;
    int firstRank, ranks;
};

#define OREGANA_PLAIN(h) { h, h, h, h, h, h }

// Mixtures break back as the keys rise, so their pipes stay between about
// 1 and 8 kHz the whole compass through. Each row is a rank; each column an
// octave of the keys.
constexpr RankSpec kRanks[] = {
    // Hauptwerk
    { Family::Principal, OREGANA_PLAIN(0.5f), 0.0f, -1.0f, 0, 0.0f },      //  0 Principal 16'
    { Family::Principal, OREGANA_PLAIN(1.0f), 0.0f, 0.0f, 0, 0.0f },       //  1 Octave 8'
    { Family::String, OREGANA_PLAIN(1.0f), 0.0f, -7.0f, 0, 0.0f },         //  2 Viola da Gamba 8'
    { Family::Chimney, OREGANA_PLAIN(1.0f), 0.0f, -6.0f, 0, 0.0f },        //  3 Rohrfloete 8'
    { Family::Principal, OREGANA_PLAIN(2.0f), -1.0f, -2.0f, 0, 0.0f },     //  4 Octave 4'
    { Family::Taper, OREGANA_PLAIN(2.0f), 0.0f, -8.0f, 0, 0.0f },          //  5 Spitzfloete 4'
    { Family::Principal, OREGANA_PLAIN(3.0f), -1.0f, -6.0f, 0, 0.0f },     //  6 Quinte 2 2/3'
    { Family::Principal, OREGANA_PLAIN(4.0f), -1.0f, -4.0f, 0, 0.0f },     //  7 Superoctave 2'
    { Family::Chimney, OREGANA_PLAIN(1.0f), 2.0f, -6.0f, 24, 0.0f },       //  8 Cornet V from c1: 8'
    { Family::Flute, OREGANA_PLAIN(2.0f), 0.0f, -7.0f, 24, 0.0f },         //  9   4'
    { Family::Flute, OREGANA_PLAIN(3.0f), 0.0f, -7.0f, 24, 0.0f },         // 10   2 2/3'
    { Family::Flute, OREGANA_PLAIN(4.0f), 0.0f, -8.0f, 24, 0.0f },         // 11   2'
    { Family::Flute, OREGANA_PLAIN(5.0f), 0.0f, -7.0f, 24, 0.0f },         // 12   1 3/5'
    { Family::Principal, { 6, 4, 3, 2, 1, 1 }, -2.0f, -7.0f, 0, 0.0f },    // 13 Mixtur V: 1 1/3' at C
    { Family::Principal, { 8, 6, 4, 3, 2, 2 }, -2.0f, -7.0f, 0, 0.0f },    // 14   1'
    { Family::Principal, { 12, 8, 6, 4, 2, 2 }, -2.0f, -7.5f, 0, 0.0f },   // 15   2/3'
    { Family::Principal, { 16, 12, 8, 6, 3, 3 }, -2.0f, -8.0f, 0, 0.0f },  // 16   1/2'
    { Family::Principal, { 24, 16, 12, 8, 4, 4 }, -2.0f, -8.5f, 0, 0.0f }, // 17   1/3'
    { Family::Principal, { 16, 12, 8, 4, 2, 2 }, -3.0f, -9.0f, 0, 0.0f },  // 18 Scharff III: 1/2' at C
    { Family::Principal, { 24, 16, 12, 6, 3, 3 }, -3.0f, -9.5f, 0, 0.0f }, // 19   1/3'
    { Family::Principal, { 32, 24, 16, 8, 4, 4 }, -3.0f, -10.0f, 0, 0.0f },// 20   1/4'
    { Family::Trumpet, OREGANA_PLAIN(0.5f), 0.0f, -1.0f, 0, 0.0f },        // 21 Trompete 16'
    { Family::Trumpet, OREGANA_PLAIN(1.0f), 0.0f, 1.0f, 0, 0.0f },         // 22 Trompete 8'
    // Positiv
    { Family::Stopped, OREGANA_PLAIN(1.0f), 0.0f, -8.0f, 0, 0.0f },        // 23 Gedackt 8'
    { Family::Quintadena, OREGANA_PLAIN(1.0f), 0.0f, -10.0f, 0, 0.0f },    // 24 Quintadena 8'
    // Sharp by 7 cents: against the Gedackt it beats once a second at c1.
    { Family::Flute, OREGANA_PLAIN(1.0f), -5.0f, -11.0f, 12, 7.0f },       // 25 Unda maris 8' from c0
    { Family::Principal, OREGANA_PLAIN(2.0f), -2.0f, -5.0f, 0, 0.0f },     // 26 Principal 4'
    { Family::Chimney, OREGANA_PLAIN(2.0f), 0.0f, -10.0f, 0, 0.0f },       // 27 Rohrfloete 4'
    { Family::Taper, OREGANA_PLAIN(3.0f), 0.0f, -10.0f, 0, 0.0f },         // 28 Nasat 2 2/3'
    { Family::Principal, OREGANA_PLAIN(4.0f), -2.0f, -7.0f, 0, 0.0f },     // 29 Octave 2'
    { Family::Flute, OREGANA_PLAIN(5.0f), -3.0f, -11.0f, 0, 0.0f },        // 30 Terz 1 3/5'
    { Family::Principal, OREGANA_PLAIN(6.0f), -1.0f, -11.0f, 0, 0.0f },    // 31 Quinte 1 1/3'
    { Family::Flute, OREGANA_PLAIN(8.0f), -2.0f, -12.0f, 0, 0.0f },        // 32 Siffloete 1'
    { Family::Principal, { 8, 6, 4, 3, 2, 2 }, -3.0f, -9.5f, 0, 0.0f },    // 33 Cimbel III: 1' at C
    { Family::Principal, { 12, 8, 6, 4, 3, 3 }, -3.0f, -10.0f, 0, 0.0f },  // 34   2/3'
    { Family::Principal, { 16, 12, 8, 6, 4, 4 }, -3.0f, -10.5f, 0, 0.0f }, // 35   1/2'
    { Family::Dulcian, OREGANA_PLAIN(0.5f), 0.0f, -6.0f, 0, 0.0f },        // 36 Dulcian 16'
    { Family::Krummhorn, OREGANA_PLAIN(1.0f), 0.0f, -3.0f, 0, 0.0f },      // 37 Krummhorn 8'
    { Family::VoxHumana, OREGANA_PLAIN(1.0f), 0.0f, -8.0f, 0, 0.0f },      // 38 Vox humana 8'
    // Recit
    { Family::Principal, OREGANA_PLAIN(1.0f), -1.0f, -3.0f, 0, 0.0f },     // 39 Principal 8'
    // Sharp by 6 cents: against the Principal it beats once a second at c1.
    { Family::Principal, OREGANA_PLAIN(1.0f), -2.0f, -5.0f, 12, 6.0f },    // 40 Voce umana 8' from c0
    { Family::Harmonic, OREGANA_PLAIN(1.0f), 0.0f, -3.0f, 0, 0.0f },       // 41 Flute harmonique 8'
    { Family::String, OREGANA_PLAIN(1.0f), 1.0f, -7.0f, 0, 0.0f },         // 42 Gambe 8'
    { Family::String, OREGANA_PLAIN(1.0f), 1.0f, -8.0f, 12, 9.0f },        // 43 Voix celeste 8' from c0
    { Family::Harmonic, OREGANA_PLAIN(2.0f), -1.0f, -6.0f, 0, 0.0f },      // 44 Flute octaviante 4'
    { Family::Harmonic, OREGANA_PLAIN(4.0f), -2.0f, -8.0f, 0, 0.0f },      // 45 Octavin 2'
    { Family::Principal, { 4, 4, 3, 2, 2, 2 }, -2.0f, -8.0f, 0, 0.0f },    // 46 Plein jeu IV: 2' at C
    { Family::Principal, { 6, 6, 4, 3, 3, 3 }, -2.0f, -8.0f, 0, 0.0f },    // 47   1 1/3'
    { Family::Principal, { 8, 8, 6, 4, 4, 4 }, -2.0f, -8.5f, 0, 0.0f },    // 48   1'
    { Family::Principal, { 12, 12, 8, 6, 6, 6 }, -2.0f, -9.0f, 0, 0.0f },  // 49   2/3'
    { Family::Hautbois, OREGANA_PLAIN(0.5f), 2.0f, -4.0f, 0, 0.0f },       // 50 Basson 16'
    { Family::Trompette, OREGANA_PLAIN(1.0f), 0.0f, 1.0f, 0, 0.0f },       // 51 Trompette 8'
    { Family::Hautbois, OREGANA_PLAIN(1.0f), 0.0f, -5.0f, 0, 0.0f },       // 52 Basson-Hautbois 8'
    // A reed's tongue cannot be made for the top of a 4': the French Clairon
    // repeats an octave lower from c3.
    { Family::Trompette, { 2, 2, 2, 2, 1, 1 }, -1.0f, -1.0f, 0, 0.0f },    // 53 Clairon 4'
    // Pedal
    { Family::Stopped, OREGANA_PLAIN(0.25f), 2.0f, 3.0f, 0, 0.0f },        // 54 Untersatz 32'
    { Family::Principal, OREGANA_PLAIN(0.5f), 0.0f, 3.0f, 0, 0.0f },       // 55 Principal 16'
    { Family::Stopped, OREGANA_PLAIN(0.5f), 2.0f, 1.0f, 0, 0.0f },         // 56 Subbass 16'
    { Family::Principal, OREGANA_PLAIN(1.0f), 0.0f, 1.0f, 0, 0.0f },       // 57 Octave 8'
    { Family::Stopped, OREGANA_PLAIN(1.0f), 0.0f, -5.0f, 0, 0.0f },        // 58 Gedackt 8'
    { Family::Principal, OREGANA_PLAIN(2.0f), 0.0f, -1.0f, 0, 0.0f },      // 59 Octave 4'
    { Family::Flute, OREGANA_PLAIN(4.0f), 2.0f, -6.0f, 0, 0.0f },          // 60 Nachthorn 2'
    { Family::Principal, OREGANA_PLAIN(3.0f), -1.0f, -5.0f, 0, 0.0f },     // 61 Mixtur IV: 2 2/3'
    { Family::Principal, OREGANA_PLAIN(4.0f), -1.0f, -5.0f, 0, 0.0f },     // 62   2'
    { Family::Principal, OREGANA_PLAIN(6.0f), -1.0f, -6.0f, 0, 0.0f },     // 63   1 1/3'
    { Family::Principal, OREGANA_PLAIN(8.0f), -1.0f, -6.0f, 0, 0.0f },     // 64   1'
    { Family::Posaune, OREGANA_PLAIN(0.25f), 0.0f, 4.0f, 0, 0.0f },        // 65 Contraposaune 32'
    { Family::Posaune, OREGANA_PLAIN(0.5f), 0.0f, 5.0f, 0, 0.0f },         // 66 Posaune 16'
    { Family::Trumpet, OREGANA_PLAIN(1.0f), 0.0f, 3.0f, 0, 0.0f },         // 67 Trompete 8'
    { Family::Trumpet, OREGANA_PLAIN(4.0f), -4.0f, 0.0f, 0, 0.0f },        // 68 Cornet 2'
};

#undef OREGANA_PLAIN

constexpr int kRankCount = static_cast<int>(sizeof(kRanks) / sizeof(kRanks[0]));

constexpr StopSpec kStops[] = {
    { "hw_principal_16", "Principal 16'", kHauptwerk, 0, 1 },
    { "hw_octave_8", "Octave 8'", kHauptwerk, 1, 1 },
    { "hw_gamba_8", "Viola da Gamba 8'", kHauptwerk, 2, 1 },
    { "hw_rohrfloete_8", "Rohrfloete 8'", kHauptwerk, 3, 1 },
    { "hw_octave_4", "Octave 4'", kHauptwerk, 4, 1 },
    { "hw_spitzfloete_4", "Spitzfloete 4'", kHauptwerk, 5, 1 },
    { "hw_quinte_223", "Quinte 2 2/3'", kHauptwerk, 6, 1 },
    { "hw_superoctave_2", "Superoctave 2'", kHauptwerk, 7, 1 },
    { "hw_cornet", "Cornet V", kHauptwerk, 8, 5 },
    { "hw_mixtur", "Mixtur V", kHauptwerk, 13, 5 },
    { "hw_scharff", "Scharff III", kHauptwerk, 18, 3 },
    { "hw_trompete_16", "Trompete 16'", kHauptwerk, 21, 1 },
    { "hw_trompete_8", "Trompete 8'", kHauptwerk, 22, 1 },
    { "pos_gedackt_8", "Gedackt 8'", kPositiv, 23, 1 },
    { "pos_quintadena_8", "Quintadena 8'", kPositiv, 24, 1 },
    { "pos_unda_maris_8", "Unda maris 8'", kPositiv, 25, 1 },
    { "pos_principal_4", "Principal 4'", kPositiv, 26, 1 },
    { "pos_rohrfloete_4", "Rohrfloete 4'", kPositiv, 27, 1 },
    { "pos_nasat_223", "Nasat 2 2/3'", kPositiv, 28, 1 },
    { "pos_octave_2", "Octave 2'", kPositiv, 29, 1 },
    { "pos_terz_135", "Terz 1 3/5'", kPositiv, 30, 1 },
    { "pos_quinte_113", "Quinte 1 1/3'", kPositiv, 31, 1 },
    { "pos_siffloete_1", "Siffloete 1'", kPositiv, 32, 1 },
    { "pos_cimbel", "Cimbel III", kPositiv, 33, 3 },
    { "pos_dulcian_16", "Dulcian 16'", kPositiv, 36, 1 },
    { "pos_krummhorn_8", "Krummhorn 8'", kPositiv, 37, 1 },
    { "pos_vox_humana_8", "Vox humana 8'", kPositiv, 38, 1 },
    { "rec_principal_8", "Principal 8'", kRecit, 39, 1 },
    { "rec_voce_umana_8", "Voce umana 8'", kRecit, 40, 1 },
    { "rec_flute_harmonique_8", "Flute harmonique 8'", kRecit, 41, 1 },
    { "rec_gambe_8", "Gambe 8'", kRecit, 42, 1 },
    { "rec_voix_celeste_8", "Voix celeste 8'", kRecit, 43, 1 },
    { "rec_flute_octaviante_4", "Flute octaviante 4'", kRecit, 44, 1 },
    { "rec_octavin_2", "Octavin 2'", kRecit, 45, 1 },
    { "rec_plein_jeu", "Plein jeu IV", kRecit, 46, 4 },
    { "rec_basson_16", "Basson 16'", kRecit, 50, 1 },
    { "rec_trompette_8", "Trompette 8'", kRecit, 51, 1 },
    { "rec_hautbois_8", "Basson-Hautbois 8'", kRecit, 52, 1 },
    { "rec_clairon_4", "Clairon 4'", kRecit, 53, 1 },
    { "ped_untersatz_32", "Untersatz 32'", kPedal, 54, 1 },
    { "ped_principal_16", "Principal 16'", kPedal, 55, 1 },
    { "ped_subbass_16", "Subbass 16'", kPedal, 56, 1 },
    { "ped_octave_8", "Octave 8'", kPedal, 57, 1 },
    { "ped_gedackt_8", "Gedackt 8'", kPedal, 58, 1 },
    { "ped_octave_4", "Octave 4'", kPedal, 59, 1 },
    { "ped_nachthorn_2", "Nachthorn 2'", kPedal, 60, 1 },
    { "ped_mixtur", "Mixtur IV", kPedal, 61, 4 },
    { "ped_contraposaune_32", "Contraposaune 32'", kPedal, 65, 1 },
    { "ped_posaune_16", "Posaune 16'", kPedal, 66, 1 },
    { "ped_trompete_8", "Trompete 8'", kPedal, 67, 1 },
    { "ped_cornet_2", "Cornet 2'", kPedal, 68, 1 },
};

constexpr int kStopCount = static_cast<int>(sizeof(kStops) / sizeof(kStops[0]));

// The stops by name, in kStops' order, for registrations.
enum Stop : int {
    kHwPrincipal16, kHwOctave8, kHwGamba8, kHwRohrfloete8, kHwOctave4, kHwSpitzfloete4, kHwQuinte223,
    kHwSuperoctave2, kHwCornet, kHwMixtur, kHwScharff, kHwTrompete16, kHwTrompete8,
    kPosGedackt8, kPosQuintadena8, kPosUndaMaris8, kPosPrincipal4, kPosRohrfloete4, kPosNasat223, kPosOctave2,
    kPosTerz135, kPosQuinte113, kPosSiffloete1, kPosCimbel, kPosDulcian16, kPosKrummhorn8, kPosVoxHumana8,
    kRecPrincipal8, kRecVoceUmana8, kRecFluteHarmonique8, kRecGambe8, kRecVoixCeleste8, kRecFluteOctaviante4,
    kRecOctavin2, kRecPleinJeu, kRecBasson16, kRecTrompette8, kRecHautbois8, kRecClairon4,
    kPedUntersatz32, kPedPrincipal16, kPedSubbass16, kPedOctave8, kPedGedackt8, kPedOctave4, kPedNachthorn2,
    kPedMixtur, kPedContraposaune32, kPedPosaune16, kPedTrompete8, kPedCornet2,
    kStopEnd
};
static_assert(kStopEnd == kStopCount, "the Stop names follow kStops");

// Which division a rank stands in, and where its pipes begin among all the
// organ's pipes: a rank has a place for every key of its division, whether or
// not a pipe stands there.
constexpr Division RankDivision(int rank) {
    for (const StopSpec& stop : kStops)
        if (rank >= stop.firstRank && rank < stop.firstRank + stop.ranks) return stop.division;
    return kHauptwerk;
}
constexpr int RankFirstPipe(int rank) {
    int first = 0;
    for (int r = 0; r < rank; ++r) first += kDivisionKeys[RankDivision(r)];
    return first;
}
constexpr int kPipeCount = RankFirstPipe(kRankCount);

// The smallest pipe an organ builder makes sounds about 8.4 kHz (the top of
// a 2' rank); a rank that would pass it repeats an octave lower. The flue
// model's own limit is the same: above it a period is under six samples and
// the pipe no longer speaks reliably (ProbePipe, 8.9 to 11.2 kHz). A reed's
// tongue is not made above the top of an 8' (c4, 2093 Hz): its rank repeats
// from there. And no pipe is longer than the 32' C's (16.4 Hz): below it a
// rank has none.
constexpr double kSmallestPipeHz = 8400.0, kSmallestReedHz = 2100.0, kLargestPipeHz = 16.0;

namespace detail {

// A family's voicing at 8' C (65.4 Hz) and six octaves above, between which a
// pipe's own is taken by its pitch.
struct FlueFamily {
    bool stopped;
    double chimney, chimneyLength, harmonic;
    // scale, q, drive, offset, noise, chiff, shedding, corner, speech
    double low[9], high[9];
};

struct ReedFamily {
    bool cone;
    // length, apex, scale, q, flow, closing, curve, tongue, tongueQ, boot, noise, speech
    double low[12], high[12];
};

inline const FlueFamily& Flue(Family family) {
    static const FlueFamily principal { false, 0.0, 0.0, 0.0,
        { 0.0, 35.0, 6.0, 1.3, 0.05, 0.6, 1.2, 1.0, 1.0 }, { 0.0, 30.0, 4.0, 1.0, 0.06, 0.5, 0.8, 1.0, 1.0 } };
    static const FlueFamily flute { false, 0.0, 0.0, 0.0,
        { 6.0, 25.0, 2.2, 0.35, 0.09, 0.4, 0.6, 1.0, 1.0 }, { 5.0, 25.0, 1.9, 0.3, 0.09, 0.4, 0.4, 1.0, 1.0 } };
    static const FlueFamily taper { false, 0.0, 0.0, 0.0,
        { 2.0, 30.0, 3.0, 0.6, 0.07, 0.5, 0.8, 2.0, 1.0 }, { 2.0, 28.0, 2.5, 0.5, 0.07, 0.5, 0.6, 2.0, 1.0 } };
    static const FlueFamily stopped { true, 0.0, 0.0, 0.0,
        { 4.0, 30.0, 2.6, 0.45, 0.10, 0.9, 0.6, 1.0, 1.0 }, { 3.0, 28.0, 2.2, 0.35, 0.10, 0.8, 0.5, 1.0, 1.0 } };
    static const FlueFamily chimney { true, 0.3, 0.4, 0.0,
        { 4.0, 30.0, 3.0, 0.45, 0.09, 0.8, 0.6, 1.0, 1.0 }, { 3.0, 28.0, 2.4, 0.4, 0.09, 0.7, 0.5, 1.0, 1.0 } };
    static const FlueFamily quintadena { true, 0.0, 0.0, 0.0,
        { -6.0, 40.0, 6.0, 0.3, 0.06, 1.2, 1.0, 1.0, 1.0 }, { -5.0, 35.0, 4.5, 0.3, 0.06, 1.0, 0.8, 1.0, 1.0 } };
    static const FlueFamily string { false, 0.0, 0.0, 0.0,
        { -9.0, 45.0, 8.0, 1.6, 0.05, 0.4, 1.5, 1.0, 1.6 }, { -8.0, 40.0, 5.0, 1.2, 0.05, 0.4, 1.0, 1.0, 1.4 } };
    // Blown harder than a flute of ordinary length, which would jump to its
    // octave: this one is there already.
    static const FlueFamily harmonic { false, 0.0, 0.0, 0.3,
        { 4.0, 30.0, 4.0, 0.7, 0.07, 0.5, 0.8, 1.0, 1.0 }, { 4.0, 28.0, 3.5, 0.6, 0.07, 0.5, 0.6, 1.0, 1.0 } };
    switch (family) {
        case Family::Harmonic: return harmonic;
        case Family::Flute: return flute;
        case Family::Taper: return taper;
        case Family::Stopped: return stopped;
        case Family::Chimney: return chimney;
        case Family::Quintadena: return quintadena;
        case Family::String: return string;
        default: return principal;
    }
}

inline const ReedFamily& Reed(Family family) {
    // A reed's flow falls towards the treble, as the voicer thins the
    // tongues there: the top of a trumpet joins the chorus and does not lead it.
    // A trumpet's flow in the bass is an estimate from typical sizes: a slit
    // of some 48 mm2 under the resting tongue into a shallot 12 mm wide on
    // 70 mm of wind gives 8 (c sqrt(2 rho / p) times the areas' ratio). The
    // open tongue then passes more than the shallot takes, so the flow is cut
    // only in the short time the tongue is nearly shut, and that short gap is
    // the trumpet's brightness: at 2 the eighth harmonic of 8' C was 24 dB
    // under the first, at 6 it is 4 dB under, where a recorded Trumpet 8' has
    // it level (Docs/research/benchmark-burea-2026-11-13.md). Above 6 a bass
    // pipe takes longer to speak (80 ms to half at 6, 132 ms at 8), so 6 it is.
    static const ReedFamily trumpet { true,
        { 1.06, 0.15, 0.0, 25.0, 6.0, 0.5, 0.01, 1.03, 40.0, 0.2, 0.01, 1.0 },
        { 1.06, 0.15, 0.0, 25.0, 1.6, 0.5, 0.01, 1.03, 40.0, 0.2, 0.01, 1.0 } };
    static const ReedFamily posaune { true,
        { 1.06, 0.15, 3.0, 20.0, 5.0, 0.5, 0.03, 1.03, 40.0, 0.25, 0.012, 1.3 },
        { 1.06, 0.15, 3.0, 20.0, 1.6, 0.5, 0.03, 1.03, 40.0, 0.25, 0.012, 1.3 } };
    // The French trumpet's tongue is flatter and shuts at once: brassier.
    static const ReedFamily trompette { true,
        { 1.06, 0.15, -1.0, 28.0, 6.5, 0.5, 0.004, 1.03, 40.0, 0.2, 0.01, 1.0 },
        { 1.06, 0.15, -1.0, 28.0, 2.0, 0.5, 0.004, 1.03, 40.0, 0.2, 0.01, 1.0 } };
    static const ReedFamily hautbois { true,
        { 1.06, 0.15, -8.0, 22.0, 1.0, 0.6, 0.02, 1.03, 40.0, 0.2, 0.012, 1.0 },
        { 1.06, 0.15, -8.0, 22.0, 0.7, 0.6, 0.02, 1.03, 40.0, 0.2, 0.012, 1.0 } };
    static const ReedFamily krummhorn { false,
        { 1.0, 0.15, -4.0, 25.0, 1.0, 0.8, 0.03, 2.0, 10.0, 0.2, 0.01, 1.0 },
        { 1.0, 0.15, -4.0, 25.0, 1.0, 0.8, 0.03, 2.0, 10.0, 0.2, 0.01, 1.0 } };
    static const ReedFamily dulcian { false,
        { 1.0, 0.15, -2.0, 18.0, 0.8, 0.8, 0.08, 2.0, 10.0, 0.2, 0.01, 1.2 },
        { 1.0, 0.15, -2.0, 18.0, 0.8, 0.8, 0.08, 2.0, 10.0, 0.2, 0.01, 1.2 } };
    // A resonator a third as long as the note's: its resonance stands three
    // times above the note as a formant, and the tongue alone sets the pitch,
    // well above its own (so the tongue is made slow). With no resonator to
    // hold it, its pitch follows the wind: 2.3 cents flat for each 1 % the
    // wind falls at this flow and closing, the least of the voicings tried
    // (4.7 at flow 0.8; a stiffer or lighter tongue did not help).
    static const ReedFamily voxHumana { false,
        { 3.0, 0.15, 0.0, 15.0, 2.0, 0.5, 0.05, 0.76, 15.0, 0.2, 0.015, 1.0 },
        { 3.0, 0.15, 0.0, 15.0, 2.0, 0.5, 0.05, 0.76, 15.0, 0.2, 0.015, 1.0 } };
    switch (family) {
        case Family::Trompette: return trompette;
        case Family::Hautbois: return hautbois;
        case Family::Posaune: return posaune;
        case Family::Krummhorn: return krummhorn;
        case Family::Dulcian: return dulcian;
        case Family::VoxHumana: return voxHumana;
        default: return trumpet;
    }
}

} // namespace detail

inline bool IsReed(Family family) {
    return family == Family::Trumpet || family == Family::Trompette || family == Family::Hautbois
        || family == Family::Posaune || family == Family::Krummhorn || family == Family::Dulcian
        || family == Family::VoxHumana;
}

// One pipe as the builder plans it, before tuning and regulation.
struct PipePlan {
    bool exists = false, reed = false;
    double hz = 0.0;       // at a1 = 440 Hz in equal temperament
    double pitch = 1.0;    // its pitch against its key's 8' note
    double level = 0.0;    // the level it is regulated to: 1 is the Hauptwerk's Octave 8' at c1
    FlueVoicing flue;
    ReedVoicing tongue;
};

inline PipePlan PlanPipe(int rank, int key) {
    const RankSpec& spec = kRanks[rank];
    PipePlan plan;
    const int fromC = kFirstNote + key - kOrganC;
    if (key < 0 || key >= kDivisionKeys[RankDivision(rank)] || (spec.firstKey > 0 && fromC < spec.firstKey)) return plan;
    const double note = 440.0 * std::pow(2.0, (kFirstNote + key - 69) / 12.0);
    // A beating rank is tuned by its beats, not by cents: above c1 the tuner
    // lets them quicken with the square root of the pitch only, or the top
    // would flutter.
    const double beating = spec.cents * std::sqrt(Min(1.0, 261.63 / note));
    plan.reed = IsReed(spec.family);
    // The keys below C take the lowest octave's pitches and those above c4 the highest's.
    plan.pitch = spec.harmonics[Clamp(fromC / 12, 0, 5)] * std::pow(2.0, beating / 1200.0);
    while (note * plan.pitch > (plan.reed ? kSmallestReedHz : kSmallestPipeHz)) plan.pitch *= 0.5;
    if (note * plan.pitch < kLargestPipeHz) return plan;
    plan.exists = true;
    plan.hz = note * plan.pitch;

    // Octaves above 8' C, where the families' voicings are given.
    const double octaves = std::log2(plan.hz / 65.406);
    const double t = Clamp(octaves / 6.0, -0.34, 1.17);
    const auto at = [t](double low, double high) { return low + t * (high - low); };
    if (plan.reed) {
        const detail::ReedFamily& f = detail::Reed(spec.family);
        ReedVoicing& v = plan.tongue;
        v.cone = f.cone;
        v.length = at(f.low[0], f.high[0]);
        v.apex = at(f.low[1], f.high[1]);
        v.scale = at(f.low[2], f.high[2]) + spec.scale;
        v.q = at(f.low[3], f.high[3]);
        v.flow = at(f.low[4], f.high[4]);
        v.closing = at(f.low[5], f.high[5]);
        v.curve = at(f.low[6], f.high[6]);
        v.tongue = at(f.low[7], f.high[7]);
        v.tongueQ = at(f.low[8], f.high[8]);
        v.boot = at(f.low[9], f.high[9]);
        v.noise = at(f.low[10], f.high[10]);
        v.speech = at(f.low[11], f.high[11]);
    } else {
        const detail::FlueFamily& f = detail::Flue(spec.family);
        FlueVoicing& v = plan.flue;
        v.stopped = f.stopped;
        // The top of a chimney flute is plain stopped pipes: a chimney a few
        // millimetres long cannot be made.
        v.chimney = plan.hz <= 2500.0 ? f.chimney : 0.0;
        v.chimneyLength = f.chimneyLength;
        // The bass of a harmonic rank is of ordinary length, and so is the
        // top, where the half pipe is under fifteen samples there and back
        // and does not speak reliably (3951 Hz failed).
        v.harmonic = plan.hz >= 250.0 && plan.hz <= 3200.0 ? f.harmonic : 0.0;
        v.scale = at(f.low[0], f.high[0]) + spec.scale;
        v.q = at(f.low[1], f.high[1]);
        v.drive = at(f.low[2], f.high[2]);
        v.offset = at(f.low[3], f.high[3]);
        v.noise = at(f.low[4], f.high[4]);
        v.chiff = at(f.low[5], f.high[5]);
        v.shedding = at(f.low[6], f.high[6]);
        v.corner = at(f.low[7], f.high[7]);
        v.speech = at(f.low[8], f.high[8]);
    }
    // Regulated a little down towards the treble and up towards the bass, as
    // a voicer balances a rank by ear: the ear is keenest from 2 to 4 kHz and
    // dull below 100 Hz.
    const double tilt = -1.5 * Max(0.0, std::log2(plan.hz / 523.25)) + 1.5 * Max(0.0, std::log2(130.81 / plan.hz));
    plan.level = std::pow(10.0, (spec.level + tilt) / 20.0);
    return plan;
}

} // namespace oregana
