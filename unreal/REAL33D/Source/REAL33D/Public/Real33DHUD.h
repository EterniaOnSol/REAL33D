#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Real33DChatPanel.h"
#include "Real33DPanelChrome.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class AReal33DWorld;
class SReal33DBattlePanel;
class SReal33DConditionStrip;
class SReal33DContainersPanel;
class SReal33DControlPanel;
class SReal33DInventoryPanel;
class SReal33DMinimapPanel;
class SReal33DMiniWindow;
class SReal33DSkillsPanel;
class SReal33DVitalsPanel;

/**
 * The whole in-game UI, laid out as `game_interface/gameinterface.otui` lays it.
 *
 * That file describes a frame, not a collection of floating windows: two side
 * columns of a fixed 176px on each flank, a 36px action column inboard of each,
 * a bottom panel under a splitter, and the map filling whatever is left. This
 * reproduces that frame. The centre is deliberately empty -- it is where the 3D
 * scene shows through -- and carries only the `panel_map` border the 2D draws
 * around its own viewport, so the game is framed rather than overlaid.
 *
 * The panels inside it are the modules of that client's own load chain, minus
 * the ones this milestone excludes and plus the action bars, which the 3D
 * client keeps. Which of them carry live values and which are shells is each
 * panel's own business and is documented on each; this class only places them.
 *
 * One owner, one update path: `Refresh` is called once a frame from the player
 * controller with the world actor, and hands each panel the server-owned values
 * it draws. No panel reaches for state on its own and none caches any.
 */
class SReal33DHUD : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DHUD) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UReal33DBridge>, Bridge)
		SLATE_EVENT(FReal33DOnTypingChanged, OnTypingChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Pushes this frame's server-owned values into every live panel. */
	void Refresh(const AReal33DWorld* World);

	/** The chat panel, so the controller can focus it and gate movement. */
	TSharedPtr<SReal33DChatPanel> GetChatPanel() const { return ChatPanel; }

	// --------------------------------------------------------- object use
	//
	// Two shapes, as 7.72 has two. A plain use acts where the object stands
	// and is what opens a container. A use-with needs a second thing, so the
	// client waits: the next click names a creature, an object or a field, and
	// only then does a command go out. Nothing is held on the server in
	// between -- the pending use is entirely local, and Escape discards it.

	/** True while a use-with is waiting for its target. */
	bool IsTargeting() const { return Pending.bActive; }

	/** Drops a pending use-with. Safe to call when there is none. */
	void CancelTargeting();

	/** Completes a pending use-with on a creature. Returns false if none was pending. */
	bool CompleteUseOnCreature(uint32 CreatureId);

	/** Completes a pending use-with on an object lying on a field. */
	bool CompleteUseOnField(const Real33D::FMapPosition& Position, uint16 TypeId,
		uint8 StackIndex);

private:
	/** A 176px side column carrying a stack of mini windows. */
	TSharedRef<SWidget> MakeSideColumn(TSharedRef<SWidget> Contents);

	/** The centre: the 2D's map frame with nothing in it but the 3D scene. */
	TSharedRef<SWidget> MakeViewportFrame();

	/** The bottom panel: the splitter, the bottom action bar and the console. */
	TSharedRef<SWidget> MakeBottomPanel();

	void HandlePanelToggled(FName Panel);

	/** Turns a drop between two slots into a move request on the bridge. */
	void HandleItemDropped(FReal33DSlotRef From, FReal33DSlotRef To);

	/** Right-click on a slot: use it, or begin a use-with. */
	void HandleSlotUsed(FReal33DSlotRef Slot, bool bWithTarget);

	/** Left-click on a slot, offered to a pending use-with. */
	bool HandleSlotPicked(FReal33DSlotRef Slot);

	/** Shows or hides the "choose a target" banner. */
	void UpdateTargetingBanner();

	/**
	 * The lowest open-container number the server is not already using.
	 *
	 * CL_CMD_USE_OBJECT carries the slot a container should be shown in, and
	 * Fusion32 refuses a number outside its table. Reusing a number that is
	 * already open would replace that container rather than open a second one,
	 * which is what makes a bag inside a bag its own window.
	 */
	uint8 FirstFreeContainerNumber() const;

	/** A use-with the player has begun but not yet aimed. */
	struct FPendingUse
	{
		bool bActive = false;
		FReal33DSlotRef Object;
	};
	FPendingUse Pending;

	/** The container numbers currently open, refreshed each frame. */
	TArray<uint8> OpenContainerNumbers;

	/** The "choose a target" strip, shown only while a use-with is pending. */
	TSharedPtr<SWidget> TargetingBanner;

	TSharedPtr<SReal33DChatPanel> ChatPanel;
	TSharedPtr<SReal33DVitalsPanel> Vitals;
	TSharedPtr<SReal33DSkillsPanel> Skills;
	TSharedPtr<SReal33DBattlePanel> Battle;
	TSharedPtr<SReal33DInventoryPanel> Inventory;
	TSharedPtr<SReal33DContainersPanel> Containers;
	TSharedPtr<SReal33DConditionStrip> Conditions;
	TSharedPtr<SReal33DMinimapPanel> Minimap;
	TSharedPtr<SReal33DControlPanel> Controls;

	/** The two windows the control strip can roll away. */
	TSharedPtr<SWidget> SkillsWindow;
	TSharedPtr<SWidget> BattleWindow;

	TWeakObjectPtr<UReal33DBridge> Bridge;

	/** Reused every frame so the battle list does not allocate per tick. */
	TArray<FReal33DBattleEntry> BattleScratch;
};
