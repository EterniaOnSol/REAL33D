// CLIENTCORE-OUTFIT-772-001.
//
// Both directions of the 7.72 outfit: the two commands the client builds
// (CL_CMD_GET_OUTFIT, CL_CMD_SET_OUTFIT) and the two the server sends
// (SV_CMD_OUTFIT, SV_CMD_CREATURE_OUTFIT), plus the accept/reject rules
// CSetOutfit applies to the answer.
//
// Every golden vector below is hand-computed from the Fusion32 source first
// and only then compared against a generator. The shared emitter in
// fixtures/fullscreen_772_vectors.h is a literal port of
// sending.cc::SendOutfit, so it is asserted against the hand-computed bytes
// before it is trusted to build anything else: a generator that agrees with a
// decoder written from the same misreading proves nothing.

#include "fusion32/protocol772/map_scan.h"
#include "fusion32/protocol772/movement.h"
#include "fusion32/protocol772/outfit.h"
#include "fusion32/protocol772/player_state.h"

#include "fixtures/fullscreen_772_vectors.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace fusion32::protocol772;
namespace vectors = fusion32::protocol772::test_vectors;

class Failure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

#define CHECK(condition) do { if (!(condition)) throw Failure(#condition); } while (false)

using TileMap = std::map<MapPosition, std::vector<MapThing>>;

vectors::FullScreenEncoder::TileProvider Provider(const TileMap& tiles) {
    return [&tiles](const MapPosition& position) -> const std::vector<MapThing>* {
        const auto found = tiles.find(position);
        return found == tiles.end() ? nullptr : &found->second;
    };
}

const ObjectTypeTable& Types() {
    static const ObjectTypeTable table = vectors::GoldenObjectTypeTable();
    return table;
}

const MapPosition kAnchor{32097, 32219, 7};

ServerUpdateDecodeResult Decode(const std::vector<std::uint8_t>& bytes) {
    return DecodeServerUpdate(bytes, 0, kAnchor, Types());
}

std::string Hex(const std::vector<std::uint8_t>& bytes) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (const std::uint8_t byte : bytes) {
        out.push_back(digits[byte >> 4]);
        out.push_back(digits[byte & 0x0F]);
    }
    return out;
}

OutfitDescriptor Colored(std::uint16_t outfit_id, const OutfitColors& colors) {
    OutfitDescriptor outfit;
    outfit.outfit_id = outfit_id;
    outfit.colors = colors;
    return outfit;
}

OutfitDescriptor Disguised(std::uint16_t object_type) {
    OutfitDescriptor outfit;
    outfit.outfit_id = 0;
    outfit.disguised_as_object = true;
    outfit.object_type = object_type;
    return outfit;
}

std::vector<std::uint8_t> EmitDialog(const OutfitDescriptor& current,
                                     std::uint16_t first, std::uint16_t last) {
    const TileMap empty;
    vectors::ServerEmitter emitter(Provider(empty));
    emitter.OutfitDialog(current, first, last);
    return emitter.bytes();
}

std::vector<std::uint8_t> EmitCreatureOutfit(std::uint32_t creature_id,
                                             const OutfitDescriptor& outfit) {
    const TileMap empty;
    vectors::ServerEmitter emitter(Provider(empty));
    emitter.CreatureOutfit(creature_id, outfit);
    return emitter.bytes();
}

// ------------------------------------------------------------ golden: client

void GoldenClientCommands() {
    // CGetOutfit reads nothing from the buffer, so the request is the bare
    // opcode 210 = 0xD2.
    {
        const auto command = BuildGetOutfitCommand();
        CHECK(Hex(command) == "d2");
        CHECK(command.size() == kGetOutfitCommandSize);
        CHECK(command.size() == 1);
        CHECK(command[0] == kClientCommandGetOutfit);
    }

    // CSetOutfit: opcode 211 = 0xD3, readWord, then readBytes of four.
    // The default male outfit from CheckOutfit: id 128 = 0x0080 little-endian,
    // then 78 69 58 76 = 0x4E 0x45 0x3A 0x4C.
    //   d3 | 8000 | 4e 45 3a 4c
    {
        const auto command = BuildSetOutfitCommand(128, {{78, 69, 58, 76}});
        CHECK(Hex(command) == "d380004e453a4c");
        CHECK(command.size() == kSetOutfitCommandSize);
        CHECK(command.size() == 7);
        CHECK(command[0] == kClientCommandSetOutfit);
    }

    // The same bytes from a descriptor, which is how the dialog answers:
    // echo back OutfitDialogUpdate::current with whatever colours were edited.
    {
        const auto from_descriptor =
            BuildSetOutfitCommand(Colored(128, kDefaultOutfitColors));
        CHECK(Hex(from_descriptor) == "d380004e453a4c");
        CHECK(from_descriptor == BuildSetOutfitCommand(128, kDefaultOutfitColors));
    }

    // The id is a little-endian word and the four colours are raw bytes in
    // wire order, so an asymmetric id and four distinct colours pin both.
    //   0x1234 = 4660 -> 34 12
    {
        const auto command = BuildSetOutfitCommand(0x1234, {{1, 2, 3, 4}});
        CHECK(Hex(command) == "d3341201020304");
        CHECK(command[1] == 0x34);
        CHECK(command[2] == 0x12);
        CHECK(command[3] == 1);
        CHECK(command[6] == 4);
    }

    // Both extremes a word can hold, and the highest colour CSetOutfit accepts
    // (132 = 0x84) in all four slots.
    {
        CHECK(Hex(BuildSetOutfitCommand(0, {{0, 0, 0, 0}})) == "d3000000000000");
        CHECK(Hex(BuildSetOutfitCommand(0xFFFF, {{132, 132, 132, 132}}))
              == "d3ffff84848484");
    }

    // The last outfit a premium female character may select, 142 = 0x008E,
    // with the two colour extremes interleaved.
    {
        const auto command = BuildSetOutfitCommand(142, {{0, 132, 132, 0}});
        CHECK(Hex(command) == "d38e0000848400");
    }

    std::cout << "  client command golden bytes: PASS\n";
}

// ------------------------------------------------------------ golden: server

void GoldenServerCommands() {
    // SV_CMD_OUTFIT, colour branch. SendOutfit(TConnection*) writes the opcode,
    // then SendOutfit(Connection, Player->OrgOutfit), then FirstOutfit and
    // LastOutfit as words. Male non-premium is 128..131, and 131 = 0x0083.
    //   c8 | 8000 | 4e453a4c | 8000 | 8300
    const auto dialog = EmitDialog(Colored(128, kDefaultOutfitColors), 128, 131);
    CHECK(Hex(dialog) == "c880004e453a4c80008300");
    CHECK(dialog.size() == 11);
    // The same vector the player_state suite already pins, reached from here
    // too so the two suites cannot drift apart.
    CHECK(dialog == vectors::HexBytes(vectors::kGoldenOutfitDialogHex));

    {
        const auto decoded = Decode(dialog);
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::OutfitDialog);
        CHECK(decoded.update.bytes_consumed == 11);
        const OutfitDialogUpdate& got = decoded.update.outfit_dialog;
        CHECK(got.current.outfit_id == 128);
        CHECK(!got.current.disguised_as_object);
        CHECK(got.current.colors == kDefaultOutfitColors);
        CHECK(got.first_outfit == 128);
        CHECK(got.last_outfit == 131);
    }

    // SV_CMD_OUTFIT, disguise branch. SendOutfit branches on `OutfitID == 0`
    // and writes an object type word *instead of* the four colours, so the
    // command is two bytes shorter. Reached in practice when the chooser opens
    // while the character's stored outfit is the invisible one, TOutfit{}:
    // cr.hh::TOutfit::Invisible is id 0 with object type 0.
    //   c8 | 0000 | d204 | 8000 | 8600      (0x04D2 = 1234, 0x0086 = 134)
    const auto disguised_dialog = EmitDialog(Disguised(1234), 128, 134);
    CHECK(Hex(disguised_dialog) == "c80000d20480008600");
    CHECK(disguised_dialog.size() == 9);

    {
        const auto decoded = Decode(disguised_dialog);
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::OutfitDialog);
        CHECK(decoded.update.bytes_consumed == 9);
        const OutfitDialogUpdate& got = decoded.update.outfit_dialog;
        CHECK(got.current.outfit_id == 0);
        CHECK(got.current.disguised_as_object);
        CHECK(got.current.object_type == 1234);
        // 128..134 is the premium male band: 131 + 3.
        CHECK(got.first_outfit == 128);
        CHECK(got.last_outfit == 134);
    }

    // TOutfit::Invisible() exactly: id 0, object type 0.
    {
        const auto invisible = EmitDialog(Disguised(0), 136, 139);
        CHECK(Hex(invisible) == "c80000000088008b00");
        const auto decoded = Decode(invisible);
        CHECK(decoded.ok());
        CHECK(decoded.update.outfit_dialog.current.disguised_as_object);
        CHECK(decoded.update.outfit_dialog.current.object_type == 0);
    }

    // SV_CMD_CREATURE_OUTFIT, colour branch: opcode 142 = 0x8E, the creature
    // quad, then the same outfit encoding.
    //   8e | e9030000 | 8100 | 01020304      (1001, id 129)
    const auto creature = EmitCreatureOutfit(1001, Colored(129, {{1, 2, 3, 4}}));
    CHECK(Hex(creature) == "8ee9030000810001020304");
    CHECK(creature.size() == 11);

    {
        const auto decoded = Decode(creature);
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::CreatureAttribute);
        CHECK(decoded.update.creature_attribute.attribute == CreatureAttribute::Outfit);
        CHECK(decoded.update.creature_attribute.creature_id == 1001);
        CHECK(decoded.update.bytes_consumed == 11);
        const OutfitDescriptor& got = decoded.update.creature_attribute.outfit;
        CHECK(got.outfit_id == 129);
        CHECK(!got.disguised_as_object);
        CHECK((got.colors == OutfitColors{{1, 2, 3, 4}}));
    }

    // SV_CMD_CREATURE_OUTFIT, disguise branch. This is the shape every
    // illusion arrives in: magic.cc builds a TOutfitImpact whose TOutfit has
    // OutfitID 0 and an ObjectType, and operate.cc line 81 turns the
    // resulting CREATURE_OUTFIT_CHANGED into this command.
    //   8e | e9030000 | 0000 | 260b           (0x0B26 = 2854)
    const auto creature_disguised = EmitCreatureOutfit(1001, Disguised(2854));
    CHECK(Hex(creature_disguised) == "8ee90300000000260b");
    CHECK(creature_disguised.size() == 9);

    {
        const auto decoded = Decode(creature_disguised);
        CHECK(decoded.ok());
        CHECK(decoded.update.kind == ServerUpdateKind::CreatureAttribute);
        CHECK(decoded.update.creature_attribute.attribute == CreatureAttribute::Outfit);
        CHECK(decoded.update.bytes_consumed == 9);
        const OutfitDescriptor& got = decoded.update.creature_attribute.outfit;
        CHECK(got.outfit_id == 0);
        CHECK(got.disguised_as_object);
        CHECK(got.object_type == 2854);
    }

    std::cout << "  server command golden bytes: PASS\n";
}

// --------------------------------------------- the selectable band, not a list

void SelectionBandMirrorsTheServer() {
    // sending.cc lines 1671-1675 and receiving.cc lines 1425-1429 compute the
    // same four lines, so these are both halves at once.
    const auto male = SelectableOutfits(kPlayerSexMale, false);
    CHECK(male.first == 128 && male.last == 131);
    CHECK(male.count() == 4);

    const auto male_premium = SelectableOutfits(kPlayerSexMale, true);
    CHECK(male_premium.first == 128 && male_premium.last == 134);
    CHECK(male_premium.count() == 7);

    const auto female = SelectableOutfits(kPlayerSexFemale, false);
    CHECK(female.first == 136 && female.last == 139);
    CHECK(female.count() == 4);

    const auto female_premium = SelectableOutfits(kPlayerSexFemale, true);
    CHECK(female_premium.first == 136 && female_premium.last == 142);
    CHECK(female_premium.count() == 7);

    // The server's test is `Sex == 1`, with no test for 2 on the other branch.
    // Every value that is not 1 therefore takes the 136 series, and this is
    // the shape rather than an accident of two known values.
    for (const std::uint8_t sex : {std::uint8_t{0}, std::uint8_t{2},
                                   std::uint8_t{3}, std::uint8_t{255}}) {
        const auto range = SelectableOutfits(sex, false);
        CHECK(range.first == 136 && range.last == 139);
    }
    CHECK(SelectableOutfits(1, false).first == 128);

    // Premium adds exactly three, both sexes, and nothing else moves.
    CHECK(male_premium.last - male.last == kPremiumExtraOutfits);
    CHECK(female_premium.last - female.last == kPremiumExtraOutfits);
    CHECK(male_premium.first == male.first);
    CHECK(female_premium.first == female.first);

    // contains() is CSetOutfit's test `id < First || id > Last`, so both
    // endpoints are inside and both neighbours are outside.
    CHECK(!male.contains(127));
    CHECK(male.contains(128));
    CHECK(male.contains(131));
    CHECK(!male.contains(132));
    CHECK(male_premium.contains(134));
    CHECK(!male_premium.contains(135));
    CHECK(!female.contains(135));
    CHECK(female.contains(136));
    CHECK(female.contains(139));
    CHECK(!female.contains(140));
    CHECK(female_premium.contains(142));
    CHECK(!female_premium.contains(143));

    // The two bands never meet: a male id is never selectable by a female
    // character and the other way round.
    CHECK(!male_premium.contains(136));
    CHECK(!female.contains(128));

    // The gamemaster outfit CheckOutfit forces (id 75, all colours zero) is
    // outside every band, which is consistent with CGetOutfit refusing the
    // chooser to a character holding GAMEMASTER_OUTFIT rather than offering a
    // band that could not contain what the character is wearing.
    CHECK(!male.contains(kGamemasterOutfitId));
    CHECK(!male_premium.contains(kGamemasterOutfitId));
    CHECK(!female_premium.contains(kGamemasterOutfitId));
    CHECK((kGamemasterOutfitColors == OutfitColors{{0, 0, 0, 0}}));

    // The defaults CheckOutfit writes are the first id of each band.
    CHECK(male.first == kMaleFirstOutfitId);
    CHECK(female.first == kFemaleFirstOutfitId);
    CHECK((kDefaultOutfitColors == OutfitColors{{78, 69, 58, 76}}));
    for (const std::uint8_t color : kDefaultOutfitColors) {
        CHECK(color <= kMaxOutfitColor);
    }

    std::cout << "  selectable band mirrors the server: PASS\n";
}

void DialogBandIsTheAuthority() {
    // The band on screen is the one SV_CMD_OUTFIT carried, because the game
    // protocol never tells the client its own sex. DialogSelectionRange is
    // that band, and here it is taken off a decoded command rather than built
    // by hand.
    const auto decoded = Decode(EmitDialog(Colored(136, {{0, 0, 0, 0}}), 136, 142));
    CHECK(decoded.ok());
    const auto range = DialogSelectionRange(decoded.update.outfit_dialog);
    CHECK(range.first == 136 && range.last == 142);
    CHECK(range.count() == 7);
    // It agrees with the band the server would have computed for a premium
    // female character, which is the case this vector represents.
    const auto expected = SelectableOutfits(kPlayerSexFemale, true);
    CHECK(range.first == expected.first);
    CHECK(range.last == expected.last);

    // A band of one is well formed: first == last names exactly one outfit.
    const auto single = Decode(EmitDialog(Colored(128, kDefaultOutfitColors), 128, 128));
    CHECK(single.ok());
    const auto one = DialogSelectionRange(single.update.outfit_dialog);
    CHECK(one.count() == 1);
    CHECK(one.contains(128));
    CHECK(!one.contains(129));

    // An inverted band names nothing. SendOutfit cannot emit one -- it always
    // writes a first below or equal to its last -- so this is a desynchronised
    // stream rather than a server opinion.
    //
    // The decoder is deliberately permissive here and reports the two words as
    // they arrived: unlike a container number or an inventory slot, an
    // inverted band leaves the rest of the command perfectly locatable, so
    // there is nothing to resynchronise and nothing to guess. count() is 0,
    // contains() is false everywhere, and a caller that offers the band an
    // empty dialog is correct.
    const auto inverted = Decode(EmitDialog(Colored(128, kDefaultOutfitColors), 140, 130));
    CHECK(inverted.ok());
    const auto empty_band = DialogSelectionRange(inverted.update.outfit_dialog);
    CHECK(empty_band.first == 140);
    CHECK(empty_band.last == 130);
    CHECK(empty_band.count() == 0);
    CHECK(!empty_band.contains(128));
    CHECK(!empty_band.contains(135));
    CHECK(!empty_band.contains(140));

    // Both words are full words, so a band at the top of the range is carried
    // without truncation.
    const auto wide = Decode(EmitDialog(Colored(1, {{0, 0, 0, 0}}), 0, 0xFFFF));
    CHECK(wide.ok());
    const auto wide_band = DialogSelectionRange(wide.update.outfit_dialog);
    CHECK(wide_band.first == 0);
    CHECK(wide_band.last == 0xFFFF);
    CHECK(wide_band.count() == 65536U);

    std::cout << "  the dialog's own band is the authority: PASS\n";
}

// ----------------------------------------------------- CSetOutfit's two tests

void ValidationMirrorsCSetOutfit() {
    const auto male = SelectableOutfits(kPlayerSexMale, false);

    CHECK(ValidateOutfitSelection(male, 128, kDefaultOutfitColors)
          == OutfitSelectionError::None);
    CHECK(ValidateOutfitSelection(male, 131, {{0, 0, 0, 0}})
          == OutfitSelectionError::None);

    // The band test, receiving.cc lines 1431-1438.
    CHECK(ValidateOutfitSelection(male, 127, kDefaultOutfitColors)
          == OutfitSelectionError::OutfitIdOutOfRange);
    CHECK(ValidateOutfitSelection(male, 132, kDefaultOutfitColors)
          == OutfitSelectionError::OutfitIdOutOfRange);
    // Id 0 gets no special treatment in CSetOutfit: there is no disguise
    // branch to take, so it simply falls below FirstOutfit like any other
    // number under 128.
    CHECK(ValidateOutfitSelection(male, 0, kDefaultOutfitColors)
          == OutfitSelectionError::OutfitIdOutOfRange);
    CHECK(ValidateOutfitSelection(male, 0xFFFF, kDefaultOutfitColors)
          == OutfitSelectionError::OutfitIdOutOfRange);
    // A female id offered to a male band, and the other way round.
    CHECK(ValidateOutfitSelection(male, 136, kDefaultOutfitColors)
          == OutfitSelectionError::OutfitIdOutOfRange);
    CHECK(ValidateOutfitSelection(SelectableOutfits(kPlayerSexFemale, true), 128,
                                  kDefaultOutfitColors)
          == OutfitSelectionError::OutfitIdOutOfRange);
    // The premium ids are refused on a non-premium band and accepted on the
    // premium one, which is the only thing the `+= 3` changes.
    CHECK(ValidateOutfitSelection(male, 134, kDefaultOutfitColors)
          == OutfitSelectionError::OutfitIdOutOfRange);
    CHECK(ValidateOutfitSelection(SelectableOutfits(kPlayerSexMale, true), 134,
                                  kDefaultOutfitColors)
          == OutfitSelectionError::None);

    // The colour test, receiving.cc lines 1440-1446: `Colors[n] > 132`. So 132
    // is accepted, 133 is not, and the test is applied to each of the four
    // slots independently.
    static_assert(kMaxOutfitColor == 132, "receiving.cc rejects Colors[n] > 132");
    CHECK(ValidateOutfitSelection(male, 128, {{132, 132, 132, 132}})
          == OutfitSelectionError::None);
    for (std::size_t slot = 0; slot < kOutfitColorCount; ++slot) {
        OutfitColors colors{{0, 0, 0, 0}};
        colors[slot] = 133;
        CHECK(ValidateOutfitSelection(male, 128, colors)
              == OutfitSelectionError::ColorOutOfRange);
        colors[slot] = 132;
        CHECK(ValidateOutfitSelection(male, 128, colors) == OutfitSelectionError::None);
        colors[slot] = 255;
        CHECK(ValidateOutfitSelection(male, 128, colors)
              == OutfitSelectionError::ColorOutOfRange);
    }

    // The server tests the band before the colours, so a selection that fails
    // both is reported as the band failure -- the first complaint CSetOutfit
    // would have made.
    CHECK(ValidateOutfitSelection(male, 200, {{255, 255, 255, 255}})
          == OutfitSelectionError::OutfitIdOutOfRange);

    // An inverted band refuses everything, consistently with contains().
    CHECK(ValidateOutfitSelection(OutfitSelectionRange{140, 130}, 135,
                                  kDefaultOutfitColors)
          == OutfitSelectionError::OutfitIdOutOfRange);

    // The descriptor overload: a disguise is refused ahead of both server
    // tests, because CL_CMD_SET_OUTFIT has no field for an object type at all.
    CHECK(ValidateOutfitSelection(male, Disguised(2854))
          == OutfitSelectionError::DisguiseNotSelectable);
    // Even an otherwise valid-looking band membership cannot rescue it: a
    // disguised descriptor has outfit_id 0, which is out of band anyway, but
    // the reported reason is the one that is about the command.
    {
        OutfitDescriptor odd = Disguised(2854);
        odd.outfit_id = 128;  // not a shape SendOutfit emits; belt and braces
        CHECK(ValidateOutfitSelection(male, odd)
              == OutfitSelectionError::DisguiseNotSelectable);
    }
    CHECK(ValidateOutfitSelection(male, Colored(128, kDefaultOutfitColors))
          == OutfitSelectionError::None);
    CHECK(ValidateOutfitSelection(male, Colored(128, {{133, 0, 0, 0}}))
          == OutfitSelectionError::ColorOutOfRange);

    CHECK(std::string(OutfitSelectionErrorName(OutfitSelectionError::None)) == "None");
    CHECK(std::string(OutfitSelectionErrorName(
              OutfitSelectionError::DisguiseNotSelectable)) == "DisguiseNotSelectable");
    CHECK(std::string(OutfitSelectionErrorName(
              OutfitSelectionError::OutfitIdOutOfRange)) == "OutfitIdOutOfRange");
    CHECK(std::string(OutfitSelectionErrorName(
              OutfitSelectionError::ColorOutOfRange)) == "ColorOutOfRange");

    std::cout << "  validation mirrors CSetOutfit: PASS\n";
}

void BuildersAreNotGates() {
    // Same reason BuildUseObjectCommand does not test MULTIUSE and
    // BuildTradeObjectCommand does not pre-apply isMapContainer: a builder
    // encodes what it is handed. The refusal is a separate call, so a caller
    // chooses when to apply it instead of having it applied invisibly.
    const auto male = SelectableOutfits(kPlayerSexMale, false);

    const auto out_of_band = BuildSetOutfitCommand(200, kDefaultOutfitColors);
    CHECK(Hex(out_of_band) == "d3c8004e453a4c");
    CHECK(out_of_band.size() == kSetOutfitCommandSize);
    CHECK(ValidateOutfitSelection(male, 200, kDefaultOutfitColors)
          == OutfitSelectionError::OutfitIdOutOfRange);

    // 133 = 0x85, one past kMaxOutfitColor, encoded verbatim.
    const auto bad_color = BuildSetOutfitCommand(128, {{133, 0, 0, 0}});
    CHECK(Hex(bad_color) == "d3800085000000");
    CHECK(ValidateOutfitSelection(male, 128, {{133, 0, 0, 0}})
          == OutfitSelectionError::ColorOutOfRange);

    // A disguised descriptor still produces seven well-formed bytes, carrying
    // its colours member and never its object type, because the command has
    // nowhere to put one. Validation is what refuses it.
    const auto disguised = BuildSetOutfitCommand(Disguised(2854));
    CHECK(disguised.size() == kSetOutfitCommandSize);
    CHECK(Hex(disguised) == "d3000000000000");
    CHECK(ValidateOutfitSelection(male, Disguised(2854))
          == OutfitSelectionError::DisguiseNotSelectable);

    std::cout << "  builders encode, validation refuses: PASS\n";
}

// ----------------------------------------------------------------- negatives

void TruncationIsAlwaysAnError() {
    struct Case {
        const char* name;
        std::vector<std::uint8_t> bytes;
    };
    const std::vector<Case> cases = {
        {"dialog, colours", EmitDialog(Colored(128, kDefaultOutfitColors), 128, 131)},
        {"dialog, disguise", EmitDialog(Disguised(1234), 128, 134)},
        {"creature outfit, colours", EmitCreatureOutfit(1001, Colored(129, {{1, 2, 3, 4}}))},
        {"creature outfit, disguise", EmitCreatureOutfit(1001, Disguised(2854))},
    };

    std::size_t prefixes = 0;
    for (const Case& entry : cases) {
        // The whole command decodes.
        const auto whole = Decode(entry.bytes);
        CHECK(whole.ok());
        CHECK(whole.update.bytes_consumed == entry.bytes.size());

        // Every proper prefix, including the bare opcode, is an explicit
        // Truncated failure and never a half-filled outfit.
        for (std::size_t length = 1; length < entry.bytes.size(); ++length) {
            ++prefixes;
            const std::vector<std::uint8_t> prefix(entry.bytes.begin(),
                                                   entry.bytes.begin()
                                                       + static_cast<std::ptrdiff_t>(length));
            const auto decoded = Decode(prefix);
            CHECK(!decoded.ok());
            CHECK(decoded.error == MapDecodeError::Truncated);
            CHECK(decoded.update.bytes_consumed == 0);
        }
    }

    // An empty buffer is not an outfit command either.
    const auto nothing = Decode({});
    CHECK(!nothing.ok());

    // Both branches of both commands, so the count is 10 + 8 + 10 + 8.
    CHECK(prefixes == 36);

    std::cout << "  truncation, " << prefixes
              << " prefixes across both branches of both commands: PASS\n";
}

void MalformedBodies() {
    const auto dialog = EmitDialog(Colored(128, kDefaultOutfitColors), 128, 131);

    // Trailing bytes are not swallowed. The decoder reports exactly what it
    // consumed, so a caller comparing bytes_consumed against the frame length
    // sees the residue; this layer does not reject it, because a framed
    // connection carries several commands back to back and the next byte is
    // the next command.
    {
        std::vector<std::uint8_t> with_residue = dialog;
        with_residue.push_back(0xAB);
        with_residue.push_back(0xCD);
        const auto decoded = Decode(with_residue);
        CHECK(decoded.ok());
        CHECK(decoded.update.bytes_consumed == dialog.size());
        CHECK(with_residue.size() - decoded.update.bytes_consumed == 2);
    }

    // The outfit id is what decides the body length, so flipping it to zero
    // inside otherwise identical bytes changes which branch is read and makes
    // the two trailing words land two bytes earlier. This is the one field a
    // reader cannot skip past without understanding.
    {
        std::vector<std::uint8_t> flipped = dialog;
        flipped[1] = 0;
        flipped[2] = 0;
        const auto decoded = Decode(flipped);
        CHECK(decoded.ok());
        CHECK(decoded.update.bytes_consumed == 9);
        const OutfitDialogUpdate& got = decoded.update.outfit_dialog;
        CHECK(got.current.disguised_as_object);
        // 0x454e, the first two of what were the colours, is now the object
        // type, and the band has moved onto the remaining bytes.
        CHECK(got.current.object_type == 0x454E);
        CHECK(got.first_outfit == 0x4C3A);
        CHECK(got.last_outfit == 0x0080);
    }

    // The two directions reuse these opcode numbers, and this is the proof
    // that nothing may dispatch on a byte without knowing which way it was
    // travelling. 210 and 211 as *server* bytes are the buddy commands.
    {
        // The collision itself is a compile-time fact about connections.hh,
        // so it is asserted as one: if either enum is ever renumbered this
        // stops building rather than quietly passing.
        static_assert(kClientCommandGetOutfit == kServerCommandBuddyData,
                      "CL_CMD_GET_OUTFIT and SV_CMD_BUDDY_DATA are both 210");
        static_assert(kClientCommandSetOutfit == kServerCommandBuddyOnline,
                      "CL_CMD_SET_OUTFIT and SV_CMD_BUDDY_ONLINE are both 211");

        // The set command's own bytes, fed to the server-side decoder as if
        // the server had sent them, do not fail: SV_CMD_BUDDY_ONLINE is a bare
        // creature quad, so 211 followed by four bytes is a well-formed buddy
        // record with an id assembled from the outfit id and the first two
        // colours. It succeeds, and it means something else entirely.
        //
        // That is the point of this check. A wrong-direction dispatch here is
        // silent, not loud, so the only defence is never routing a byte
        // without knowing which way it travelled.
        const auto command = BuildSetOutfitCommand(128, kDefaultOutfitColors);
        const auto misread = Decode(command);
        CHECK(misread.ok());
        CHECK(misread.update.kind == ServerUpdateKind::Buddy);
        CHECK(misread.update.kind != ServerUpdateKind::OutfitDialog);
        // 80 00 4e 45 read as a little-endian quad.
        CHECK(misread.update.buddy.character_id == 0x454E0080U);
        CHECK(misread.update.bytes_consumed == 5);
        CHECK(command.size() - misread.update.bytes_consumed == 2);
    }

    // And the server opcode for the dialog, 200, is not a client command the
    // builders here produce.
    static_assert(kServerCommandOutfitDialog != kClientCommandGetOutfit, "200 != 210");
    static_assert(kServerCommandOutfitDialog != kClientCommandSetOutfit, "200 != 211");
    CHECK(IsPlayerStateCommand(kServerCommandOutfitDialog));
    CHECK(IsPlayerStateCommand(kServerCommandCreatureOutfit));

    std::cout << "  malformed bodies: PASS\n";
}

// --------------------------------------------------------------- application

WorldState LoggedInWorld(std::uint32_t player_id, TileMap* tiles) {
    const MapPosition player{1000, 1000, 7};
    for (std::int32_t x = player.x - 20; x <= player.x + 20; ++x) {
        for (std::int32_t y = player.y - 20; y <= player.y + 20; ++y) {
            (*tiles)[{x, y, 7}] = {vectors::PlainItem(100)};
        }
    }
    std::vector<MapThing>& stack = (*tiles)[player];
    stack.push_back(vectors::IntroducedCreature(player_id, 0, "Walker"));

    vectors::ServerEmitter screen(Provider(*tiles));
    screen.FullScreen(1000, 1000, 7);
    const auto decoded = DecodeFullScreen(screen.bytes(), Types());
    if (!decoded.ok()) throw Failure("fixture full screen did not decode");
    WorldState state;
    state.local_creature_id = player_id;
    ApplyFullScreen(&state, decoded.message);
    return state;
}

void OutfitReachesTheCreatureRecord() {
    const std::uint32_t player_id = 1001;
    TileMap tiles;
    WorldState state = LoggedInWorld(player_id, &tiles);

    // What FULLSCREEN established: the fixture's creature wears the default
    // male outfit, colours and all.
    const auto known = state.known_creatures.find(player_id);
    CHECK(known != state.known_creatures.end());
    CHECK(known->second.outfit.outfit_id == 128);
    CHECK(!known->second.outfit.disguised_as_object);
    CHECK(known->second.outfit.colors == kDefaultOutfitColors);

    // SV_CMD_CREATURE_OUTFIT replaces it, colours included. This is the whole
    // render path for a selection another player made: no reply to the setter,
    // one 142 to everyone who can see them.
    {
        const auto decoded = Decode(EmitCreatureOutfit(player_id,
                                                       Colored(131, {{10, 20, 30, 40}})));
        CHECK(decoded.ok());
        ApplyServerUpdate(&state, decoded.update, Types());
        const auto& record = state.known_creatures.at(player_id);
        CHECK(record.outfit.outfit_id == 131);
        CHECK(!record.outfit.disguised_as_object);
        CHECK((record.outfit.colors == OutfitColors{{10, 20, 30, 40}}));
    }

    // And an illusion replaces it with an object disguise, which carries no
    // colours at all.
    {
        const auto decoded = Decode(EmitCreatureOutfit(player_id, Disguised(2854)));
        CHECK(decoded.ok());
        ApplyServerUpdate(&state, decoded.update, Types());
        const auto& record = state.known_creatures.at(player_id);
        CHECK(record.outfit.outfit_id == 0);
        CHECK(record.outfit.disguised_as_object);
        CHECK(record.outfit.object_type == 2854);
    }

    std::cout << "  a creature outfit reaches the creature record: PASS\n";
}

void TheDialogStoresNothing() {
    const std::uint32_t player_id = 1001;
    TileMap tiles;
    WorldState state = LoggedInWorld(player_id, &tiles);

    const auto decoded = Decode(EmitDialog(Colored(131, {{5, 6, 7, 8}}), 128, 134));
    CHECK(decoded.ok());

    const WorldState before = state;
    ApplyServerUpdate(&state, decoded.update, Types());

    // SV_CMD_OUTFIT is a question, not a fact about the world. It carries the
    // player's *OrgOutfit* -- SendOutfit sends Player->OrgOutfit, not
    // Player->Outfit -- which is what they would look like without an active
    // illusion, and is therefore not what is on screen. Writing it into the
    // creature record would make an invisible player visible at the moment
    // they opened the chooser.
    CHECK(state.known_creatures.size() == before.known_creatures.size());
    const auto& record = state.known_creatures.at(player_id);
    CHECK(record.outfit.outfit_id == before.known_creatures.at(player_id).outfit.outfit_id);
    CHECK(record.outfit.colors == before.known_creatures.at(player_id).outfit.colors);
    CHECK(record.outfit.outfit_id == 128);
    CHECK(state.viewport_anchor == before.viewport_anchor);
    CHECK(state.local_creature_id == before.local_creature_id);
    CHECK(state.tile_count() == before.tile_count());
    CHECK(state.thing_count() == before.thing_count());

    CHECK(std::string(ServerUpdateKindName(ServerUpdateKind::OutfitDialog))
          == "OutfitDialog");

    std::cout << "  the dialog stores nothing: PASS\n";
}

}  // namespace

int main() {
    try {
        GoldenClientCommands();
        GoldenServerCommands();
        SelectionBandMirrorsTheServer();
        DialogBandIsTheAuthority();
        ValidationMirrorsCSetOutfit();
        BuildersAreNotGates();
        TruncationIsAlwaysAnError();
        MalformedBodies();
        OutfitReachesTheCreatureRecord();
        TheDialogStoresNothing();
    } catch (const std::exception& error) {
        std::cerr << "outfit_tests FAILED: " << error.what() << '\n';
        return 1;
    }
    std::cout << "outfit_tests: PASS\n";
    return 0;
}
