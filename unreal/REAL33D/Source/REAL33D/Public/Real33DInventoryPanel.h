#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class STextBlock;

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
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Updates the two live readouts. Cheap to call every frame. */
	void SetVitals(const FReal33DPlayerVitals& Vitals);

	/** inventory.otui gives the panel a fixed 162px body. */
	static constexpr float PanelHeight = 162.0f;

private:
	/** One 34x34 equipment square with its body-part placeholder. */
	TSharedRef<SWidget> MakeSlot(const FName& Placeholder, const FText& Tooltip);

	/** A `containerslot` box with a caption over a value, as Soul and Cap are. */
	TSharedRef<SWidget> MakeReadout(const FText& Caption, TSharedPtr<STextBlock>& OutValue);

	/** One 20x20 combat or posture toggle, drawn idle and inert. */
	TSharedRef<SWidget> MakeCombatButton(const FName& Brush, const FText& Tooltip);

	TSharedPtr<STextBlock> SoulValue;
	TSharedPtr<STextBlock> CapacityValue;

	FReal33DPlayerVitals Last;
	bool bHasDrawnOnce = false;
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
