#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

namespace Real33D
{
/**
 * The ids of the panels the HUD carries today.
 *
 * Named here rather than spelled as literals at each use, so the control
 * strip, the registry and the HUD cannot drift apart over a typo. The values
 * match the names the control strip already emits, which is why they are
 * plain words and not prefixed.
 */
namespace PanelIds
{
	REAL33D_API extern const FName Minimap;
	REAL33D_API extern const FName Health;
	REAL33D_API extern const FName Inventory;
	REAL33D_API extern const FName Skills;
	REAL33D_API extern const FName Battle;
	REAL33D_API extern const FName Containers;
	REAL33D_API extern const FName Channels;
	/**
	 * The book, sign and house-list window.
	 *
	 * Registered like any other panel even though it is a centred modal
	 * rather than a side-column mini window: the registry only manages
	 * visibility, and routing this one through it keeps the HUD with exactly
	 * one way to show and hide something.
	 */
	REAL33D_API extern const FName TextEditor;
	/**
	 * The player-to-player trade window. A centred modal like the text
	 * editor, registered here for the same reason: visibility gets one owner.
	 */
	REAL33D_API extern const FName Trade;
	/**
	 * The outfit chooser. A centred modal like the two above.
	 *
	 * Registered so visibility keeps one owner, but unlike the side-column
	 * panels it is never opened from the control strip: only an SV_CMD_OUTFIT
	 * from Fusion32 may open it, because only the server knows which outfits
	 * this character may wear.
	 */
	REAL33D_API extern const FName Outfit;
	/**
	 * The "how many?" question a Ctrl+drag of a stack opens. A centred modal
	 * like the three above, registered for the same reason.
	 *
	 * Never opened from the control strip either, but for a different reason
	 * from the outfit chooser's: this one describes a move the player has
	 * already aimed, so there is nothing to ask about until a drag has been
	 * released somewhere.
	 */
	REAL33D_API extern const FName MoveAmount;

	// --------------------------------------------------------- before the game
	//
	// The two surfaces of the login screen. They are not HUD panels and they
	// are not in the HUD's registry: they live on their own widget, which is
	// the only thing on screen before a character is entered, and they have
	// their own registry instance there.
	//
	// Registered at all because the rule the registry exists to keep applies
	// here more sharply than anywhere else: exactly one of these two is
	// visible at any moment, and a second owner of that fact -- a bool on the
	// screen widget, say -- is how an account form ends up drawn over a
	// character list.

	/** The account number and password form. */
	REAL33D_API extern const FName Account;
	/** The character list the login service returned. */
	REAL33D_API extern const FName CharacterSelect;
}
}

/**
 * The HUD's list of panels, and the one place that opens and closes them.
 *
 * Before this existed, every panel was wired into `SReal33DHUD` by hand: a
 * member to hold its window, a block in `Construct`, and a branch in a
 * hand-written if-chain that mapped a name to a widget. Adding the seventh
 * panel meant editing all three, so the HUD grew with every milestone and
 * became the file every UI change had to pass through. That is what this
 * removes. A panel now needs its own file and one `AddPanel` call.
 *
 * Ownership. The registry does not own the widgets in the Slate sense -- the
 * widget tree does, through the slots they were added to. It keeps a shared
 * pointer so a lookup stays valid, and it lives as a member of the HUD, so it
 * is destroyed with the HUD and can never outlive the tree it points into.
 *
 * Open state is not cached. `IsOpen` reads the widget's own visibility, so the
 * registry cannot drift out of step with what is actually on screen -- a bool
 * kept here would be a second source of truth for something Slate already
 * knows.
 *
 * Not thread-safe, deliberately: Slate is single-threaded and this is only
 * ever touched from the game thread, like the rest of the HUD.
 */
class REAL33D_API FReal33DPanelRegistry
{
public:
	/**
	 * Records a panel under an id.
	 *
	 * Returns false, and changes nothing, for an empty id, an invalid widget
	 * or an id that is already registered. A duplicate is a programming error
	 * -- two panels answering one name -- so it is refused rather than
	 * silently overwriting the first.
	 *
	 * Registering does not change visibility. A panel keeps whatever state it
	 * was built with.
	 */
	bool Register(FName Id, const TSharedRef<SWidget>& Window);

	/** Forgets a panel. Returns false when the id was not registered. */
	bool Unregister(FName Id);

	/** Forgets every panel. The widgets themselves are untouched. */
	void Reset();

	bool IsRegistered(FName Id) const { return Find(Id).IsValid(); }

	/** How many panels are registered. */
	int32 Num() const { return Entries.Num(); }

	/** The registered ids, in registration order. */
	void GetIds(TArray<FName>& OutIds) const;

	/** The window registered under an id, or null. */
	TSharedPtr<SWidget> Find(FName Id) const;

	/**
	 * Whether a panel is currently showing.
	 *
	 * False for an unknown id: something that is not there is not open.
	 */
	bool IsOpen(FName Id) const;

	/**
	 * Shows or hides a panel. Returns false when the id is not registered.
	 *
	 * Collapsed rather than Hidden, so a closed panel gives its space back to
	 * the column instead of leaving a gap, which is what the hand-written
	 * toggle did and what the 2D client does.
	 */
	bool SetOpen(FName Id, bool bOpen);

	bool Open(FName Id) { return SetOpen(Id, true); }
	bool Close(FName Id) { return SetOpen(Id, false); }

	/** Flips a panel. Returns false when the id is not registered. */
	bool Toggle(FName Id);

private:
	struct FEntry
	{
		FName Id;
		TSharedPtr<SWidget> Window;
	};

	/**
	 * An array, not a map: there are a handful of panels, registration order
	 * is the order a caller would expect to see them listed in, and a linear
	 * scan over six entries is not worth hashing.
	 */
	TArray<FEntry> Entries;
};
