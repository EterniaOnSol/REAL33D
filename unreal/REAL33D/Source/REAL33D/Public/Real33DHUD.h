#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Real33DChatPanel.h"
#include "Real33DPanelChrome.h"
#include "Real33DPanelRegistry.h"
#include "Real33DMinimapPanel.h"
#include "Real33DStackMove.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class AReal33DWorld;
class SReal33DAmountPanel;
class SReal33DBattlePanel;
class SReal33DConditionStrip;
class SReal33DContainersPanel;
class SReal33DControlPanel;
class SReal33DInventoryPanel;
class SReal33DMinimapPanel;
class SReal33DMiniWindow;
class SVerticalBox;
class SReal33DSkillsPanel;
class SReal33DVitalsPanel;
class SBorder;

/**
 * An object released over the 3D scene, with the keys that were down then.
 *
 * The modifiers travel with the drop rather than being re-asked downstream, so
 * the world route reads the gesture at the same instant the slot route does.
 */
DECLARE_DELEGATE_ThreeParams(FReal33DOnWorldItemDropped, FReal33DSlotRef, FVector2D,
	FReal33DDragModifiers);

/**
 * What one completed drag turned into.
 *
 * Three outcomes, not two, and the caller needs to be able to tell them apart
 * for its own log: one move reached the bridge, a question was opened and
 * nothing was sent yet, or nothing happened at all because there was no live
 * bridge to ask.
 */
struct FReal33DStackMoveResult
{
	/** True when a "how many?" question is now on screen. Nothing was sent. */
	bool bAsked = false;
	/** Non-zero when exactly one move request reached the bridge. */
	uint32 MoveId = 0;
	/** The count that was put on the wire, when one was. */
	uint32 Count = 0;
};

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
		SLATE_EVENT(FReal33DOnMinimapDestination, OnMinimapDestination)
		SLATE_EVENT(FReal33DOnWorldItemDropped, OnWorldItemDropped)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual FReply OnDragOver(const FGeometry& Geometry, const FDragDropEvent& Event) override;
	virtual FReply OnDrop(const FGeometry& Geometry, const FDragDropEvent& Event) override;
	bool IsOverWorld(FVector2D DesktopPosition) const;
	FReal33DSlotRef FindItemSlotAt(FVector2D DesktopPosition) const;

	/** Pushes this frame's server-owned values into every live panel. */
	void Refresh(const AReal33DWorld* World);

	/** The chat panel, so the controller can focus it and gate movement. */
	TSharedPtr<SReal33DChatPanel> GetChatPanel() const { return ChatPanel; }

	/**
	 * The HUD's panels, for anything that needs to open, close or ask
	 * after one. Exposed so a future panel can be driven from a hotkey or
	 * a menu without adding another accessor here for each one.
	 */
	FReal33DPanelRegistry& GetPanels() { return Panels; }
	const FReal33DPanelRegistry& GetPanels() const { return Panels; }

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

	/**
	 * Right-clicks the top object on a map field.
	 *
	 * A pending use-with consumes the click as its target. Otherwise this sends
	 * CL_CMD_USE_OBJECT and chooses a free container number exactly as an
	 * inventory/container use does. Returns false only when no live bridge exists.
	 */
	bool UseWorldObject(const Real33D::FMapPosition& Position, uint16 TypeId,
		uint8 StackIndex);

	/** Sends the existing 7.72 look request for a visible map point. */
	bool LookAtWorldPoint(const Real33D::FMapPosition& Position);
	void ShowInteractionRequest(const FString& Text);

	/**
	 * The ctrl+right-click menu for the top object on a map field.
	 *
	 * The world counterpart of `ShowSlotContextMenu`, offered on the same
	 * gesture and for the same reason: Look, Use and Rotate are the three
	 * things Fusion32 can be asked about a map object in this client, and a
	 * right-click without ctrl keeps meaning a plain use so nothing regresses.
	 * Opening the menu sends nothing; each entry is an intent the server rules
	 * on. Called only when a top object exists and no use-with is pending.
	 */
	void ShowWorldObjectContextMenu(const Real33D::FMapPosition& Position,
		uint16 TypeId, uint8 StackIndex);

	// ------------------------------------------------------- stack movement
	//
	// THE ONE ROUTE EVERY DRAG ENDS IN. Slot to slot, slot to ground, ground
	// to slot and ground to ground all call RequestStackMove, so the gesture
	// -- plain drag moves the whole stack, Shift moves one, Ctrl asks -- cannot
	// mean different things in different panels, and the clamp cannot be
	// applied in three places and forgotten in a fourth. The rule itself is in
	// `Real33DStackMove.h` and is tested without Slate.
	//
	// Nothing here mutates inventory, container or world state. A move is a
	// request; the counts the player ends up with are the ones Fusion32 reports
	// afterwards through the ordinary inventory and container commands.

	/**
	 * Turns one completed drag into either one move request or one question.
	 *
	 * @param bStackable  whether the server sent an amount for this object. A
	 *                    non-stackable object moves whole whatever is held
	 *                    down, which is what this client already did.
	 * @param StackCount  the authoritative amount, as last reported. Ignored
	 *                    when `bStackable` is false.
	 */
	FReal33DStackMoveResult RequestStackMove(const UReal33DBridge::FMoveSlot& From,
		uint16 TypeId, uint8 StackIndex, const UReal33DBridge::FMoveSlot& To,
		bool bStackable, uint32 StackCount, FReal33DDragModifiers Modifiers);

	/** True while a "how many?" question is waiting for an answer. */
	bool IsAskingMoveAmount() const { return PendingAmount.bActive; }

	/**
	 * Drops a pending "how many?" question, sending nothing.
	 *
	 * Safe to call when there is none. Bound to the same Escape the other
	 * modals use.
	 */
	void AbandonMoveAmount();

private:
	// The HUD receives bubbled Slate drops, but only its map rectangle is a world target.
	TSharedPtr<SBorder> ViewportFrame;
	FReal33DOnWorldItemDropped OnWorldItemDropped;
	bool IsWorldItemDrop(const FDragDropEvent& Event) const;

	/** A 176px side column carrying a stack of mini windows. */
	TSharedRef<SWidget> MakeSideColumn(TSharedRef<SWidget> Contents);

	/** The map frame with read-only world labels and interaction feedback. */
	TSharedRef<SWidget> MakeViewportFrame();

	/** The bottom panel: the splitter, the bottom action bar and the console. */
	TSharedRef<SWidget> MakeBottomPanel(const FReal33DOnTypingChanged& OnTypingChanged);

	void HandlePanelToggled(FName Panel);

	/**
	 * Adds a panel to a column as a standard mini window and registers it.
	 *
	 * The one call a future panel needs. Everything that used to be
	 * repeated per panel -- the slot, the mini-window chrome, the member
	 * to hold the window, the branch in the toggle -- happens here or in
	 * the registry instead.
	 *
	 * @param IconBrush NAME_None for a window with no header icon.
	 */
	void AddPanel(const TSharedRef<SVerticalBox>& Column, FName Id, const FText& Title,
		FName IconBrush, float ContentHeight, const TSharedRef<SWidget>& Content);

	/** Turns a drop between two slots into a move request on the bridge. */
	void HandleItemDropped(FReal33DSlotRef From, FReal33DSlotRef To,
		FReal33DDragModifiers Modifiers);

	/** Puts exactly one CL_CMD_MOVE_OBJECT on the bridge, and says so. */
	uint32 SendMove(const UReal33DBridge::FMoveSlot& From, uint16 TypeId,
		uint8 StackIndex, const UReal33DBridge::FMoveSlot& To, uint32 Count);

	/** Opens the "how many?" question and remembers the move it is about. */
	void AskMoveAmount(const UReal33DBridge::FMoveSlot& From, uint16 TypeId,
		uint8 StackIndex, const UReal33DBridge::FMoveSlot& To, uint32 Max);

	/** The question was answered. Sends exactly one move, clamped again. */
	void HandleAmountChosen(uint32 Amount);

	/** The question was abandoned. Sends nothing and forgets the move. */
	void HandleAmountAbandoned();

	/** One end of a move, in words, for the question and for the log. */
	static FString DescribeMoveSlot(const UReal33DBridge::FMoveSlot& Slot);

	/** Right-click on a slot: use it, or begin a use-with. */
	void HandleSlotUsed(FReal33DSlotRef Slot, EReal33DSlotAction Action);

	/**
	 * A container header button. Both send one command and change nothing
	 * here; the panel follows the server's answer.
	 *
	 * `Number` is the server's own open-container number, taken from the
	 * container being drawn, so it stays correct when windows move.
	 */
	void HandleContainerClosed(uint8 Number);
	void HandleContainerUp(uint8 Number);

	/** Left-click on a slot, offered to a pending use-with. */
	bool HandleSlotPicked(FReal33DSlotRef Slot);

	/** Battle-list target action; the worker performs the WorldState toggle. */
	void HandleCreatureTargeted(uint32 CreatureId, bool bFollow);
	void HandleAttackModeChanged(EReal33DAttackMode Mode);
	void HandleChaseModeChanged(EReal33DChaseMode Mode);

	/** Shows or hides the "choose a target" banner. */
	void UpdateTargetingBanner();
	void BeginUseTarget(const UReal33DBridge::FMoveSlot& Source, uint16 TypeId, uint8 StackIndex);

	/**
	 * The ctrl+right-click menu for one object, as the 2D client offers it.
	 *
	 * Built from what Fusion32 can actually be asked about this object, and
	 * nothing else: Look always, and Trade with when a trade could be
	 * started. Choosing an entry is what sends a command; opening the menu
	 * sends nothing.
	 */
	void ShowSlotContextMenu(FReal33DSlotRef Slot);

	/** Arms the trade half of the same pick-then-aim gesture. */
	void BeginTradeTarget(const UReal33DBridge::FMoveSlot& Source, uint16 TypeId, uint8 StackIndex);

public:
	/**
	 * The right-click menu for the player's own character.
	 *
	 * THE GESTURE THE CLASSIC CLIENT ITSELF DOCUMENTS. Its help text, at
	 * `0x00565734` in the image `docs/CLASSIC_CLIENT_772.md` records, reads
	 * *"To change your outfit, right-click on your character and select 'Set
	 * outfit' from the context menu."* So the entry point is not a guess and
	 * not a button invented for this client; the wording of the entry below is
	 * the client's own.
	 *
	 * Right-clicking your own character did nothing before this:
	 * `InteractUnderCursor` returns early for the local player, because
	 * attacking yourself is not a thing Fusion32 offers. That no-op is what
	 * this fills.
	 *
	 * Opening the menu sends nothing, and choosing the entry sends only
	 * CL_CMD_GET_OUTFIT. The chooser appears if, and only if, Fusion32 answers
	 * with SV_CMD_OUTFIT.
	 */
	void ShowSelfContextMenu();

private:

	/**
	 * The lowest open-container number the server is not already using.
	 *
	 * CL_CMD_USE_OBJECT carries the slot a container should be shown in, and
	 * Fusion32 refuses a number outside its table. Reusing a number that is
	 * already open would replace that container rather than open a second one,
	 * which is what makes a bag inside a bag its own window.
	 */
	uint8 FirstFreeContainerNumber() const;

	/**
	 * Something the player has picked but not yet aimed.
	 *
	 * Two kinds share this one slot because they share the whole gesture:
	 * pick an object, then click a target. A trade offer is aimed at a
	 * creature just as a use-with is, and keeping one pending target means
	 * there is one banner, one Escape and one place that can be stale.
	 */
	struct FPendingUse
	{
		enum class EKind : uint8 { UseWith, TradeOffer };
		EKind Kind = EKind::UseWith;
		bool bActive = false;
		UReal33DBridge::FMoveSlot Source;
		uint16 TypeId = 0;
		uint8 StackIndex = 0;
	};
	FPendingUse Pending;

	/**
	 * A move the player has aimed but not yet sized.
	 *
	 * Held here rather than in the question window because this is the layer
	 * that may speak to the bridge, and because the window must be able to be
	 * replaced or torn down without a half-described move surviving it. While
	 * this is active NOTHING has been sent: the drag produced a question, and
	 * only an answer produces a command.
	 *
	 * `Max` is the stack count as the server last reported it, captured when
	 * the drag was released. It bounds the answer. It is deliberately not
	 * re-read when the answer arrives: re-reading would make the bound depend
	 * on when the player happened to click, and Fusion32 rules on the count
	 * either way -- a stack that shrank meanwhile simply yields a refused
	 * move, which is the correct outcome and changes nothing on screen.
	 */
	struct FPendingAmountMove
	{
		bool bActive = false;
		UReal33DBridge::FMoveSlot From;
		uint16 TypeId = 0;
		uint8 StackIndex = 0;
		UReal33DBridge::FMoveSlot To;
		uint32 Max = Real33D::kMinStackCount;
	};
	FPendingAmountMove PendingAmount;

	/** The container numbers currently open, refreshed each frame. */
	TArray<uint8> OpenContainerNumbers;

	/** The "choose a target" strip, shown only while a use-with is pending. */
	TSharedPtr<SWidget> TargetingBanner;

	TSharedPtr<class SReal33DWorldOverlay> WorldOverlay;
	TSharedPtr<SReal33DChatPanel> ChatPanel;
	TSharedPtr<SReal33DVitalsPanel> Vitals;
	TSharedPtr<SReal33DSkillsPanel> Skills;
	TSharedPtr<SReal33DBattlePanel> Battle;
	TSharedPtr<SReal33DInventoryPanel> Inventory;
	TSharedPtr<SReal33DContainersPanel> Containers;
	/**
	 * The channel list. Held like the others, though nothing pushes into it:
	 * it reads the bridge in its own Tick, because the channel list is bridge
	 * state rather than world state and `Refresh` carries the world actor.
	 */
	TSharedPtr<class SReal33DChannelsPanel> Channels;
	/**
	 * The book, sign and house-list window. Centred over the map rather than
	 * docked, and registered in `Panels` like everything else, so the HUD
	 * keeps one way to show and hide a surface.
	 */
	TSharedPtr<class SReal33DEditorWindow> TextEditor;
	/** The player-to-player trade window. Centred modal, registry-managed. */
	TSharedPtr<class SReal33DTradePanel> Trade;
	/** The outfit chooser. Centred modal, registry-managed, server-opened. */
	TSharedPtr<class SReal33DOutfitPanel> Outfit;
	/**
	 * The "how many?" question. Centred modal, registry-managed, drag-opened.
	 *
	 * The only one of the four modals this client opens itself, because it is
	 * the only one that describes something the player has already done rather
	 * than something the server offered.
	 */
	TSharedPtr<SReal33DAmountPanel> AmountQuestion;
	TSharedPtr<SReal33DConditionStrip> Conditions;
	TSharedPtr<SReal33DMinimapPanel> Minimap;
	TSharedPtr<SReal33DControlPanel> Controls;

	/**
	 * Every mini window on the HUD, by id.
	 *
	 * Replaces the per-panel members and the if-chain that mapped a name
	 * to one of them. Declared after the panel pointers so it is
	 * destroyed before them.
	 */
	FReal33DPanelRegistry Panels;

	TWeakObjectPtr<UReal33DBridge> Bridge;

	/** Reused every frame so the battle list does not allocate per tick. */
	TArray<FReal33DBattleEntry> BattleScratch;
};
