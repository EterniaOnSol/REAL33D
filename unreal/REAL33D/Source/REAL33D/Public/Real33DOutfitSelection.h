#pragma once

#include <cstdint>

// The outfit chooser's rules, with no Slate, no bridge and no protocol.
//
// Everything here is decidable without a renderer and is therefore tested
// natively by `tests/real33d_outfit_selection_tests.cpp`. The panel on top of
// it draws what this decides and nothing more, so "the band was obeyed" and
// "a colour was refused" are statements a test can make rather than something
// only a live session could show.
//
// Two facts from outside this file are repeated in it, and both are pinned
// back to their source by a static_assert inside `Real33DBridge.cpp` -- the
// one file that can see this header and `fusion32/protocol772` at once. That
// is the same arrangement already used for the chat-view kinds and
// `EReal33DEditListType`: a value may be restated for a layer that cannot
// include its origin, never re-decided.
namespace Real33D
{

/**
 * The four colour slots, in wire order.
 *
 * NAMED FROM THE CLASSIC 7.72 CLIENT, NOT FROM FUSION32. The server cannot
 * answer this: `reference/game/src/cr.hh` line 21 declares `uint8 Colors[4]`
 * and every use in the server addresses them by index only, so an earlier
 * draft of this file carried `Colour 1`..`Colour 4` on purpose.
 *
 * The names below are proven, not adopted by convention. The local classic
 * client builds its four colour rows in one loop over i = 0..3 and selects
 * each row's caption through a four-entry jump table at `0x0047D648`, whose
 * entries are literally `"Head"`, `"Body"`, `"Legs"`, `"Feet"` in that index
 * order; the row then stores `i` as the selected slot, a palette click writes
 * `colours[i]`, and the sender writes `colours[0..3]` into consecutive bytes
 * after opcode 211. `tests/verify_outfit_color_mapping.py` asserts all forty
 * of those bytes against the hashed image and prints the mapping; run it
 * before trusting these names.
 *
 * The enum's values are the wire indices, so a cast is the index and no table
 * is needed to convert one to the other.
 */
enum class OutfitSlot : std::uint8_t
{
	Head = 0,
	Body = 1,
	Legs = 2,
	Feet = 3,
};

/** How many slots there are. Four, and the protocol cannot carry a fifth. */
inline constexpr int kOutfitSlotCount = 4;

/** The slot's wire index. */
inline constexpr std::uint8_t OutfitSlotIndex(OutfitSlot Slot)
{
	return static_cast<std::uint8_t>(Slot);
}

/** The slot at a wire index. Out-of-range folds to Head rather than crashing. */
inline constexpr OutfitSlot OutfitSlotAt(int Index)
{
	return Index >= 0 && Index < kOutfitSlotCount
		? static_cast<OutfitSlot>(Index)
		: OutfitSlot::Head;
}

/**
 * The slot's caption, exactly as the classic client spells it.
 *
 * Deliberately the client's own four words and not a prettier set: the point
 * of proving the mapping was to be able to use the real names, and a player
 * who knows the 2D window should read the same labels here.
 */
inline constexpr const char* OutfitSlotName(OutfitSlot Slot)
{
	switch (Slot)
	{
	case OutfitSlot::Head: return "Head";
	case OutfitSlot::Body: return "Body";
	case OutfitSlot::Legs: return "Legs";
	case OutfitSlot::Feet: return "Feet";
	}
	return "Head";
}

/**
 * The highest colour byte Fusion32 accepts, per slot.
 *
 * Source: `reference/game/src/receiving.cc` lines 1440-1443, where CSetOutfit
 * tests `Colors[n] > 132` for each of the four and returns without applying
 * anything if any one of them is over. So 132 is valid and 133 is not, and a
 * single bad slot loses the whole selection.
 *
 * Restated here because this header cannot include ClientCore; pinned to
 * `fusion32::protocol772::kMaxOutfitColor` by a static_assert in
 * `Real33DBridge.cpp`.
 */
inline constexpr std::uint8_t kMaxOutfitColor = 132;

/**
 * A selection being edited, inside the band the server sent.
 *
 * NOT A MODEL OF THE PLAYER'S APPEARANCE. It is what the chooser currently has
 * on screen, which is a different thing: the authoritative appearance arrives
 * as SV_CMD_CREATURE_OUTFIT and lives in WorldState. Nothing here is ever read
 * back as truth about the character.
 *
 * The band is `[First, Last]` as SV_CMD_OUTFIT carried it, and every mutator
 * respects it. 7.72 sends a band and not a list -- only the first and last id,
 * with everything between implied -- so stepping through outfits is arithmetic
 * and there is nothing to enumerate.
 *
 * An unopened draft has an empty band, and `IsUsable` is false for one. That
 * is the state before Fusion32 has answered, and it is why the panel can be
 * built at startup without being able to send anything.
 */
class OutfitDraft
{
public:
	/**
	 * Adopts the band and the current outfit the server just sent.
	 *
	 * AN INVERTED BAND IS KEPT AS GIVEN AND REPORTED AS UNUSABLE rather than
	 * swapped into order. `SendOutfit` computes First and Last from the same
	 * expression every time, so First > Last cannot happen against a healthy
	 * server; if it ever arrives, silently reordering it would hide the
	 * anomaly and offer the player a range the server never granted.
	 *
	 * `OutfitId` is clamped into the band. A character can legitimately be
	 * wearing something outside it -- an illusion, or a stored outfit
	 * `CheckOutfit` has not yet corrected -- and the chooser may only offer
	 * what the band allows.
	 */
	void Open(std::uint16_t FirstOutfit, std::uint16_t LastOutfit,
		std::uint16_t OutfitId, const std::uint8_t (&InColors)[kOutfitSlotCount])
	{
		First = FirstOutfit;
		Last = LastOutfit;
		// Before the clamp, because `Clamp` answers through `IsUsable` and so
		// needs the band and this flag already in place.
		bOpened = true;
		Selected = Clamp(OutfitId);
		for (int Index = 0; Index < kOutfitSlotCount; ++Index)
		{
			Colors[Index] = InColors[Index] <= kMaxOutfitColor ? InColors[Index] : 0;
		}
		ActiveSlot = OutfitSlot::Head;
	}

	/** Back to the unopened state. A closed chooser offers nothing. */
	void Close() { *this = OutfitDraft{}; }

	/** True once a server response has been adopted. */
	bool IsOpen() const { return bOpened; }

	/**
	 * True when the band names at least one outfit, so a selection exists.
	 *
	 * The test a Submit control must use. An open draft over an empty or
	 * inverted band has nothing legitimate to send, and sending the clamped
	 * value anyway would be inventing a selection.
	 */
	bool IsUsable() const { return bOpened && Last >= First; }

	std::uint16_t FirstOutfit() const { return First; }
	std::uint16_t LastOutfit() const { return Last; }

	/** How many outfits the band names. Zero when inverted or unopened. */
	int Count() const
	{
		return IsUsable() ? static_cast<int>(Last - First) + 1 : 0;
	}

	std::uint16_t OutfitId() const { return Selected; }

	/** The colour byte in a slot. */
	std::uint8_t Color(OutfitSlot Slot) const
	{
		return Colors[OutfitSlotIndex(Slot)];
	}

	/** All four, in wire order, for the one place that builds the command. */
	const std::uint8_t (&AllColors() const)[kOutfitSlotCount] { return Colors; }

	/** Which slot the palette is editing. Presentation only. */
	OutfitSlot Slot() const { return ActiveSlot; }
	void SelectSlot(OutfitSlot Next) { ActiveSlot = Next; }

	/** The band's test, which is also CSetOutfit's: `id < First || id > Last`. */
	bool Contains(std::uint16_t OutfitId) const
	{
		return IsUsable() && OutfitId >= First && OutfitId <= Last;
	}

	/** The nearest id the band allows. Returns First for an unusable band. */
	std::uint16_t Clamp(std::uint16_t OutfitId) const
	{
		if (!IsUsable()) return First;
		if (OutfitId < First) return First;
		if (OutfitId > Last) return Last;
		return OutfitId;
	}

	/**
	 * Picks an outfit. Refuses anything the band does not name.
	 *
	 * Refuses rather than clamps, because this is the path a direct choice
	 * takes and a choice the server would drop must not be quietly turned
	 * into a different one. Stepping is what clamping is for.
	 */
	bool SelectOutfit(std::uint16_t OutfitId)
	{
		if (!Contains(OutfitId)) return false;
		Selected = OutfitId;
		return true;
	}

	/**
	 * Steps to the next or previous outfit, wrapping inside the band.
	 *
	 * Wraps because the band is a ring in the 2D client's arrows and because
	 * stopping at the end would leave a one-outfit band with a dead control.
	 * Returns false only when there is nothing to step through.
	 */
	bool StepOutfit(int Delta)
	{
		const int Size = Count();
		if (Size <= 0) return false;
		if (Size == 1) { Selected = First; return true; }
		int Offset = static_cast<int>(Selected) - static_cast<int>(First) + Delta;
		Offset %= Size;
		if (Offset < 0) Offset += Size;
		Selected = static_cast<std::uint16_t>(static_cast<int>(First) + Offset);
		return true;
	}

	/**
	 * Sets one colour byte. Refuses anything above the server's bound.
	 *
	 * Refused here rather than sent and dropped: CSetOutfit answers an
	 * out-of-range colour with silence, so a client that passed one on would
	 * show the player a change that never happened and could not explain it.
	 */
	bool SetColor(OutfitSlot Slot, std::uint8_t Value)
	{
		if (!bOpened || Value > kMaxOutfitColor) return false;
		Colors[OutfitSlotIndex(Slot)] = Value;
		return true;
	}

	/**
	 * Steps one colour byte, wrapping over `0..kMaxOutfitColor`.
	 *
	 * Wrapping over the whole inclusive range, so every value the server
	 * accepts is reachable from any other and no step can land outside it.
	 */
	bool StepColor(OutfitSlot Slot, int Delta)
	{
		if (!bOpened) return false;
		constexpr int Size = static_cast<int>(kMaxOutfitColor) + 1;
		int Value = static_cast<int>(Colors[OutfitSlotIndex(Slot)]) + Delta;
		Value %= Size;
		if (Value < 0) Value += Size;
		Colors[OutfitSlotIndex(Slot)] = static_cast<std::uint8_t>(Value);
		return true;
	}

	/**
	 * Whether this draft may be sent.
	 *
	 * The same two tests CSetOutfit applies, in its order, so a refusal here
	 * is a refusal there. It cannot cover the third case the protocol layer
	 * reports -- an object disguise -- because a draft has no object type to
	 * be one: CL_CMD_SET_OUTFIT is seven fixed bytes with nowhere to put one.
	 * The bridge is what refuses a disguised current outfit, before a draft
	 * is ever opened from it.
	 */
	bool CanSubmit() const
	{
		if (!IsUsable() || !Contains(Selected)) return false;
		for (int Index = 0; Index < kOutfitSlotCount; ++Index)
		{
			if (Colors[Index] > kMaxOutfitColor) return false;
		}
		return true;
	}

private:
	bool bOpened = false;
	std::uint16_t First = 0;
	std::uint16_t Last = 0;
	std::uint16_t Selected = 0;
	std::uint8_t Colors[kOutfitSlotCount] = {};
	OutfitSlot ActiveSlot = OutfitSlot::Head;
};

}  // namespace Real33D
