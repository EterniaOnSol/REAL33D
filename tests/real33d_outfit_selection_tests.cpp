// The outfit chooser's rules, without Slate, without a bridge and without a
// server. What a live run then has to prove is that Fusion32 accepts what
// these rules allow and that the player can see the result.
//
// The band cases use the real 7.72 numbers, because they are the ones a live
// session will present: 128-131 for a male character, 136-139 otherwise, each
// three wider with premium. Source for those is
// `reference/game/src/crplayer.cc::CheckOutfit` and the `LastOutfit += 3`
// under `CheckRight(PREMIUM_ACCOUNT)` in both `sending.cc` and `receiving.cc`.

#include "Real33DOutfitSelection.h"

#include <cassert>
#include <iostream>
#include <string>

namespace
{
using Real33D::kMaxOutfitColor;
using Real33D::kOutfitSlotCount;
using Real33D::OutfitDraft;
using Real33D::OutfitSlot;
using Real33D::OutfitSlotAt;
using Real33D::OutfitSlotIndex;
using Real33D::OutfitSlotName;

/** The default colours CheckOutfit gives a fresh character. */
constexpr std::uint8_t kDefaults[kOutfitSlotCount] = { 78, 69, 58, 76 };
constexpr std::uint8_t kZero[kOutfitSlotCount] = { 0, 0, 0, 0 };

std::string Names(const OutfitDraft&)
{
	std::string All;
	for (int Index = 0; Index < kOutfitSlotCount; ++Index)
	{
		if (Index != 0) All += ",";
		All += OutfitSlotName(OutfitSlotAt(Index));
	}
	return All;
}

// ------------------------------------------------------------- slot identity

void SlotsAreWireIndices()
{
	// The enum's value IS the wire index, so nothing needs a lookup table.
	assert(OutfitSlotIndex(OutfitSlot::Head) == 0);
	assert(OutfitSlotIndex(OutfitSlot::Body) == 1);
	assert(OutfitSlotIndex(OutfitSlot::Legs) == 2);
	assert(OutfitSlotIndex(OutfitSlot::Feet) == 3);

	// And the round trip holds for all four.
	for (int Index = 0; Index < kOutfitSlotCount; ++Index)
	{
		assert(OutfitSlotIndex(OutfitSlotAt(Index)) == Index);
	}

	// Out of range folds rather than reading past the enum.
	assert(OutfitSlotAt(-1) == OutfitSlot::Head);
	assert(OutfitSlotAt(4) == OutfitSlot::Head);
	assert(OutfitSlotAt(9999) == OutfitSlot::Head);
}

void SlotNamesAreTheClassicClientsOwn()
{
	// These four words, in this order, are what the classic 7.72 client's own
	// jump table at 0x0047D648 selects for rows 0..3. The byte-level proof is
	// `tests/verify_outfit_color_mapping.py`; this only pins that the header
	// did not drift away from it.
	assert(std::string(OutfitSlotName(OutfitSlot::Head)) == "Head");
	assert(std::string(OutfitSlotName(OutfitSlot::Body)) == "Body");
	assert(std::string(OutfitSlotName(OutfitSlot::Legs)) == "Legs");
	assert(std::string(OutfitSlotName(OutfitSlot::Feet)) == "Feet");

	OutfitDraft Draft;
	assert(Names(Draft) == "Head,Body,Legs,Feet");
}

// ------------------------------------------------------------- opening state

void AnUnopenedDraftOffersNothing()
{
	OutfitDraft Draft;
	assert(!Draft.IsOpen());
	assert(!Draft.IsUsable());
	assert(!Draft.CanSubmit());
	assert(Draft.Count() == 0);
	assert(Draft.OutfitId() == 0);
	// Nothing may be chosen before the server has said what is choosable.
	assert(!Draft.SelectOutfit(128));
	assert(!Draft.StepOutfit(1));
	assert(!Draft.SetColor(OutfitSlot::Head, 5));
	assert(!Draft.StepColor(OutfitSlot::Head, 1));
	assert(!Draft.Contains(128));
	for (int Index = 0; Index < kOutfitSlotCount; ++Index)
	{
		assert(Draft.Color(OutfitSlotAt(Index)) == 0);
	}
}

void OpeningAdoptsTheServersBand()
{
	OutfitDraft Draft;
	// A male character without premium: SendOutfit's 128..131.
	Draft.Open(128, 131, 129, kDefaults);
	assert(Draft.IsOpen());
	assert(Draft.IsUsable());
	assert(Draft.FirstOutfit() == 128);
	assert(Draft.LastOutfit() == 131);
	assert(Draft.Count() == 4);
	assert(Draft.OutfitId() == 129);
	assert(Draft.CanSubmit());
	// The colours came from the server untouched.
	for (int Index = 0; Index < kOutfitSlotCount; ++Index)
	{
		assert(Draft.Color(OutfitSlotAt(Index)) == kDefaults[Index]);
	}
	// Head is where the palette starts, as the 2D window opens.
	assert(Draft.Slot() == OutfitSlot::Head);
}

void OpeningClampsAnOutfitOutsideTheBand()
{
	// Legitimate: a character under an illusion, or one whose stored outfit
	// CheckOutfit has not corrected yet. The chooser may only offer the band.
	OutfitDraft Low;
	Low.Open(136, 139, 75, kZero);  // GAMEMASTER_OUTFIT is 75
	assert(Low.OutfitId() == 136);
	assert(Low.CanSubmit());

	OutfitDraft High;
	High.Open(136, 139, 400, kDefaults);
	assert(High.OutfitId() == 139);
	assert(High.CanSubmit());
}

void OpeningDropsAColourTheServerWouldRefuse()
{
	// Not clamped to 132: a value the server would refuse did not come from a
	// healthy SV_CMD_OUTFIT, and guessing 132 was meant would be inventing a
	// colour. Zero is the one value that is certainly in range.
	const std::uint8_t Bad[kOutfitSlotCount] = { 133, 69, 255, 76 };
	OutfitDraft Draft;
	Draft.Open(128, 131, 128, Bad);
	assert(Draft.Color(OutfitSlot::Head) == 0);
	assert(Draft.Color(OutfitSlot::Body) == 69);
	assert(Draft.Color(OutfitSlot::Legs) == 0);
	assert(Draft.Color(OutfitSlot::Feet) == 76);
	assert(Draft.CanSubmit());
}

void ClosingReturnsToUnopened()
{
	OutfitDraft Draft;
	Draft.Open(128, 131, 130, kDefaults);
	Draft.SelectSlot(OutfitSlot::Feet);
	Draft.Close();
	assert(!Draft.IsOpen());
	assert(!Draft.IsUsable());
	assert(!Draft.CanSubmit());
	assert(Draft.Count() == 0);
	assert(Draft.OutfitId() == 0);
	assert(Draft.Slot() == OutfitSlot::Head);
	for (int Index = 0; Index < kOutfitSlotCount; ++Index)
	{
		assert(Draft.Color(OutfitSlotAt(Index)) == 0);
	}
}

// ---------------------------------------------------------------- every band

void TheFourRealBands()
{
	struct Case { std::uint16_t First, Last; int Count; };
	// Male/female, without and with premium. `LastOutfit += 3`.
	const Case Cases[] = {
		{ 128, 131, 4 },
		{ 128, 134, 7 },
		{ 136, 139, 4 },
		{ 136, 142, 7 },
	};
	for (const Case& Each : Cases)
	{
		OutfitDraft Draft;
		Draft.Open(Each.First, Each.Last, Each.First, kDefaults);
		assert(Draft.Count() == Each.Count);
		// Every id in the band is selectable, and the two just outside are not.
		for (std::uint16_t Id = Each.First; Id <= Each.Last; ++Id)
		{
			assert(Draft.Contains(Id));
			assert(Draft.SelectOutfit(Id));
			assert(Draft.OutfitId() == Id);
		}
		assert(!Draft.Contains(static_cast<std::uint16_t>(Each.First - 1)));
		assert(!Draft.SelectOutfit(static_cast<std::uint16_t>(Each.First - 1)));
		assert(!Draft.Contains(static_cast<std::uint16_t>(Each.Last + 1)));
		assert(!Draft.SelectOutfit(static_cast<std::uint16_t>(Each.Last + 1)));
		// A refusal changed nothing.
		assert(Draft.OutfitId() == Each.Last);
	}
}

void ARefusedChoiceIsNotQuietlyClamped()
{
	OutfitDraft Draft;
	Draft.Open(128, 131, 130, kDefaults);
	assert(!Draft.SelectOutfit(127));
	assert(Draft.OutfitId() == 130);
	assert(!Draft.SelectOutfit(132));
	assert(Draft.OutfitId() == 130);
	// Clamp is the other verb, and it is the one stepping uses.
	assert(Draft.Clamp(127) == 128);
	assert(Draft.Clamp(132) == 131);
	assert(Draft.Clamp(129) == 129);
}

void AnInvertedBandIsKeptAndRefused()
{
	// Cannot happen against a healthy server -- SendOutfit computes First and
	// Last from one expression -- so it is reported rather than repaired.
	OutfitDraft Draft;
	Draft.Open(139, 136, 137, kDefaults);
	assert(Draft.IsOpen());
	assert(!Draft.IsUsable());
	assert(!Draft.CanSubmit());
	assert(Draft.Count() == 0);
	assert(Draft.FirstOutfit() == 139);
	assert(Draft.LastOutfit() == 136);
	assert(!Draft.Contains(137));
	assert(!Draft.SelectOutfit(137));
	assert(!Draft.StepOutfit(1));
}

void ASingleOutfitBandIsUsable()
{
	OutfitDraft Draft;
	Draft.Open(128, 128, 128, kDefaults);
	assert(Draft.IsUsable());
	assert(Draft.Count() == 1);
	assert(Draft.CanSubmit());
	// Stepping succeeds and stays put rather than failing, so the control is
	// not dead on a band of one.
	assert(Draft.StepOutfit(1));
	assert(Draft.OutfitId() == 128);
	assert(Draft.StepOutfit(-1));
	assert(Draft.OutfitId() == 128);
}

// ------------------------------------------------------------------ stepping

void SteppingWrapsInsideTheBand()
{
	OutfitDraft Draft;
	Draft.Open(128, 131, 128, kDefaults);
	const std::uint16_t Forward[] = { 129, 130, 131, 128, 129 };
	for (std::uint16_t Expected : Forward)
	{
		assert(Draft.StepOutfit(1));
		assert(Draft.OutfitId() == Expected);
	}

	Draft.Open(128, 131, 128, kDefaults);
	const std::uint16_t Backward[] = { 131, 130, 129, 128, 131 };
	for (std::uint16_t Expected : Backward)
	{
		assert(Draft.StepOutfit(-1));
		assert(Draft.OutfitId() == Expected);
	}
}

void SteppingNeverLeavesTheBand()
{
	OutfitDraft Draft;
	Draft.Open(136, 142, 139, kDefaults);
	// Every stride, including ones far larger than the band and negative.
	for (int Delta = -40; Delta <= 40; ++Delta)
	{
		Draft.SelectOutfit(139);
		assert(Draft.StepOutfit(Delta));
		assert(Draft.Contains(Draft.OutfitId()));
		assert(Draft.CanSubmit());
	}
}

void SteppingAcrossTheWholeBandReturnsHome()
{
	OutfitDraft Draft;
	Draft.Open(128, 134, 131, kDefaults);
	const int Size = Draft.Count();
	for (int Step = 0; Step < Size; ++Step) { assert(Draft.StepOutfit(1)); }
	assert(Draft.OutfitId() == 131);
	for (int Step = 0; Step < Size; ++Step) { assert(Draft.StepOutfit(-1)); }
	assert(Draft.OutfitId() == 131);
}

// ------------------------------------------------------------------- colours

void EachSlotIsIndependent()
{
	OutfitDraft Draft;
	Draft.Open(128, 131, 128, kZero);
	// One distinct value per slot, so a crossed index would be visible.
	assert(Draft.SetColor(OutfitSlot::Head, 1));
	assert(Draft.SetColor(OutfitSlot::Body, 2));
	assert(Draft.SetColor(OutfitSlot::Legs, 3));
	assert(Draft.SetColor(OutfitSlot::Feet, 4));
	assert(Draft.Color(OutfitSlot::Head) == 1);
	assert(Draft.Color(OutfitSlot::Body) == 2);
	assert(Draft.Color(OutfitSlot::Legs) == 3);
	assert(Draft.Color(OutfitSlot::Feet) == 4);

	// And the array handed to the command builder is in the same wire order.
	const auto& Wire = Draft.AllColors();
	assert(Wire[0] == 1 && Wire[1] == 2 && Wire[2] == 3 && Wire[3] == 4);
}

void TheServersColourBoundIsRespected()
{
	OutfitDraft Draft;
	Draft.Open(128, 131, 128, kZero);
	for (int Index = 0; Index < kOutfitSlotCount; ++Index)
	{
		const OutfitSlot Slot = OutfitSlotAt(Index);
		// 132 is accepted, because the server's test is `> 132`.
		assert(Draft.SetColor(Slot, kMaxOutfitColor));
		assert(Draft.Color(Slot) == kMaxOutfitColor);
		// 133 and anything above is refused, and changes nothing.
		assert(!Draft.SetColor(Slot, kMaxOutfitColor + 1));
		assert(!Draft.SetColor(Slot, 200));
		assert(!Draft.SetColor(Slot, 255));
		assert(Draft.Color(Slot) == kMaxOutfitColor);
		// Zero is valid.
		assert(Draft.SetColor(Slot, 0));
		assert(Draft.Color(Slot) == 0);
	}
	assert(Draft.CanSubmit());
}

void EveryColourInRangeIsAccepted()
{
	OutfitDraft Draft;
	Draft.Open(128, 131, 128, kZero);
	for (int Value = 0; Value <= kMaxOutfitColor; ++Value)
	{
		assert(Draft.SetColor(OutfitSlot::Body, static_cast<std::uint8_t>(Value)));
		assert(Draft.Color(OutfitSlot::Body) == Value);
		assert(Draft.CanSubmit());
	}
}

void ColourSteppingWrapsOverTheAcceptedRange()
{
	OutfitDraft Draft;
	Draft.Open(128, 131, 128, kZero);
	// Up from the top wraps to the bottom; down from the bottom wraps to 132.
	assert(Draft.SetColor(OutfitSlot::Legs, kMaxOutfitColor));
	assert(Draft.StepColor(OutfitSlot::Legs, 1));
	assert(Draft.Color(OutfitSlot::Legs) == 0);
	assert(Draft.StepColor(OutfitSlot::Legs, -1));
	assert(Draft.Color(OutfitSlot::Legs) == kMaxOutfitColor);

	// No stride can land outside the range the server accepts.
	for (int Delta = -500; Delta <= 500; ++Delta)
	{
		assert(Draft.StepColor(OutfitSlot::Feet, Delta));
		assert(Draft.Color(OutfitSlot::Feet) <= kMaxOutfitColor);
		assert(Draft.CanSubmit());
	}
}

void ColourSteppingTouchesOnlyItsOwnSlot()
{
	OutfitDraft Draft;
	Draft.Open(128, 131, 128, kDefaults);
	for (int Index = 0; Index < kOutfitSlotCount; ++Index)
	{
		OutfitDraft One = Draft;
		assert(One.StepColor(OutfitSlotAt(Index), 7));
		for (int Other = 0; Other < kOutfitSlotCount; ++Other)
		{
			if (Other == Index) continue;
			assert(One.Color(OutfitSlotAt(Other)) == kDefaults[Other]);
		}
	}
}

void TheActiveSlotIsPresentationOnly()
{
	OutfitDraft Draft;
	Draft.Open(128, 131, 128, kDefaults);
	for (int Index = 0; Index < kOutfitSlotCount; ++Index)
	{
		Draft.SelectSlot(OutfitSlotAt(Index));
		assert(Draft.Slot() == OutfitSlotAt(Index));
		// Choosing which row the palette edits sends nothing and changes no
		// colour and no outfit.
		assert(Draft.OutfitId() == 128);
		for (int Each = 0; Each < kOutfitSlotCount; ++Each)
		{
			assert(Draft.Color(OutfitSlotAt(Each)) == kDefaults[Each]);
		}
	}
}

// -------------------------------------------------------------- submit gate

void SubmitNeedsAnOpenUsableBand()
{
	OutfitDraft Unopened;
	assert(!Unopened.CanSubmit());

	OutfitDraft Inverted;
	Inverted.Open(140, 130, 135, kDefaults);
	assert(!Inverted.CanSubmit());

	OutfitDraft Good;
	Good.Open(128, 131, 128, kDefaults);
	assert(Good.CanSubmit());
	// A close takes the gate away again.
	Good.Close();
	assert(!Good.CanSubmit());
}

}  // namespace

int main()
{
	SlotsAreWireIndices();
	SlotNamesAreTheClassicClientsOwn();

	AnUnopenedDraftOffersNothing();
	OpeningAdoptsTheServersBand();
	OpeningClampsAnOutfitOutsideTheBand();
	OpeningDropsAColourTheServerWouldRefuse();
	ClosingReturnsToUnopened();

	TheFourRealBands();
	ARefusedChoiceIsNotQuietlyClamped();
	AnInvertedBandIsKeptAndRefused();
	ASingleOutfitBandIsUsable();

	SteppingWrapsInsideTheBand();
	SteppingNeverLeavesTheBand();
	SteppingAcrossTheWholeBandReturnsHome();

	EachSlotIsIndependent();
	TheServersColourBoundIsRespected();
	EveryColourInRangeIsAccepted();
	ColourSteppingWrapsOverTheAcceptedRange();
	ColourSteppingTouchesOnlyItsOwnSlot();
	TheActiveSlotIsPresentationOnly();

	SubmitNeedsAnOpenUsableBand();

	std::cout << "real33d outfit selection tests: 20 cases passed\n";
	return 0;
}
