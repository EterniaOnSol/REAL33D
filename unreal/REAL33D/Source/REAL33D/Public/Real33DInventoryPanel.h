#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Real33DPanelChrome.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class STextBlock;

DECLARE_DELEGATE_OneParam(FReal33DOnAttackModeChanged, EReal33DAttackMode);
DECLARE_DELEGATE_OneParam(FReal33DOnChaseModeChanged, EReal33DChaseMode);

/**
 * The inventory panel, transcribed from `game_inventory/inventory.otui`.
 *
 * Three things live in here and they are not equally alive, which the panel is
 * careful to show rather than hide:
 *
 * The ten equipment slots are shells. Fusion32 sends SV_CMD_SET_INVENTORY and
 * SV_CMD_DELETE_INVENTORY on every equip, and ClientCore decodes both -- but
 * only far enough to walk the frame past them, storing nothing, as
 * `player_state.h` says in as many words. So there is no item to draw. Each
 * slot shows its own 34x34 square and the 32x32 body-part placeholder the 2D
 * shows when a slot is empty, which is exactly what this client knows.
 *
 * Soul and capacity are live. They arrive in SV_CMD_PLAYER_DATA, which is
 * decoded and stored, so those two readouts are server-owned values.
 *
 * The combat and posture buttons are shells. Their state comes from a client
 * command this client does not send and a server that never reports it back,
 * so drawing one as engaged would be this client asserting a fight mode nobody
 * chose. They are drawn in their idle art and disabled.
 */
class SReal33DInventoryPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DInventoryPanel) {}
		/** Fired when an object is dropped on one of the ten squares. */
		SLATE_EVENT(FReal33DOnItemDropped, OnItemDropped)
		/** Right-click use, and shift-right-click use-with. */
		SLATE_EVENT(FReal33DOnSlotUsed, OnSlotUsed)
		/** Offered a left-click so a pending use-with can take its target. */
		SLATE_EVENT(FReal33DOnSlotPicked, OnSlotPicked)
		SLATE_EVENT(FReal33DOnAttackModeChanged, OnAttackModeChanged)
		SLATE_EVENT(FReal33DOnChaseModeChanged, OnChaseModeChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Updates the two live readouts. Cheap to call every frame. */
	void SetVitals(const FReal33DPlayerVitals& Vitals);

	/** Redraws the ten equipment squares. No-ops when nothing changed. */
	void SetInventory(const FReal33DInventory& Inventory);
	void SetCombat(const FReal33DCombat& Combat);

	/** inventory.otui gives the panel a fixed 162px body. */
	static constexpr float PanelHeight = 162.0f;

private:
	/**
	 * One 34x34 equipment square, hosted in a box so its contents can be
	 * swapped when the server says the slot changed.
	 *
	 * `Slot` is the server's own InventorySlot number, which is what
	 * SV_CMD_SET_INVENTORY carries and therefore what the redraw is keyed on.
	 */
	TSharedRef<SWidget> MakeSlot(int32 Slot, const FName& Placeholder,
		const FText& Tooltip);

	/** Rebuilds one square for what the server says is in it. */
	void FillSlot(int32 Slot, const FName& Placeholder, const FText& Tooltip,
		const FReal33DInventory& Inventory);

	/** A `containerslot` box with a caption over a value, as Soul and Cap are. */
	TSharedRef<SWidget> MakeReadout(const FText& Caption, TSharedPtr<STextBlock>& OutValue);

	/** One 20x20 source-backed combat or posture toggle. */
	TSharedRef<SWidget> MakeCombatButton(int32 Index, const FName& IdleBrush,
		const FName& ActiveBrush, const FText& Tooltip, const FOnClicked& OnClicked);
	bool IsCombatButtonActive(int32 Index) const;

	TSharedPtr<STextBlock> SoulValue;
	TSharedPtr<STextBlock> CapacityValue;

	/**
	 * The ten squares, kept so a slot can be rebuilt in place.
	 *
	 * Indexed by the server's own slot number, like everything else that
	 * carries one, so index 0 is unused.
	 */
	TSharedPtr<SBox> SlotHosts[FReal33DInventory::SlotCount];

	FReal33DOnItemDropped OnItemDropped;
	FReal33DOnSlotUsed OnSlotUsed;
	FReal33DOnSlotPicked OnSlotPicked;
	FReal33DOnAttackModeChanged OnAttackModeChanged;
	FReal33DOnChaseModeChanged OnChaseModeChanged;

	FReal33DPlayerVitals Last;
	FReal33DInventory LastInventory;
	FReal33DCombat LastCombat;
	bool bHasDrawnOnce = false;
	bool bHasDrawnInventory = false;
};

/**
 * The condition strip, from the `icons` panel of `inventory.otui`.
 *
 * Live. SV_CMD_PLAYER_STATE carries eight bits and ClientCore stores all eight,
 * so each icon is shown exactly when the server says the condition holds. The
 * strip is the 2D's own `containerslot` inset and the icons are cut from its
 * own `player-state-flags.png` at the indices `gamelib/player.lua` assigns.
 *
 * Only the eight flags 7.72 defines are drawn. The sheet carries dozens more
 * for later protocol versions; Fusion32 cannot raise any of them.
 */
class SReal33DConditionStrip : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DConditionStrip) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetConditions(const FReal33DConditions& Conditions);

private:
	static constexpr int32 FlagCount = 8;

	TSharedPtr<SBox> Icons[FlagCount];
	FReal33DConditions Last;
	bool bHasDrawnOnce = false;
};
