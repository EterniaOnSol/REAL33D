#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;
class STextBlock;
class SButton;

/**
 * The player-to-player trade window.
 *
 * Registered in `FReal33DPanelRegistry` under `Real33D::PanelIds::Trade`, so
 * showing and hiding it goes through the one mechanism every other surface
 * uses. A trade itself is opened by Fusion32 and never by this client: the
 * first SV_CMD_TRADE_OFFER_OWN or SV_CMD_TRADE_OFFER_PARTNER starts one and
 * SV_CMD_CLOSE_TRADE ends it.
 *
 * THE PANEL IS DOCKED RATHER THAN A POP-UP, and it does not hide itself when
 * no trade is running -- it shows the idle state instead. A live run proved
 * why a self-hiding window is wrong here: Fusion32 answers the first offer by
 * telling the OTHER player "X wants to trade with you" in prose and sending
 * them no trade command at all, so a surface that appeared only once a trade
 * existed would leave that player with nothing on screen. The trade is
 * STARTED from the object's own ctrl+right-click menu -- Trade with... --
 * which is where the 2D client puts it, not from a button in here.
 *
 * NPC SHOPS ARE NOT THIS WINDOW'S BUSINESS. A shop reaches the same three
 * server commands, but it is driven by TALK and already works; nothing here
 * is shop-shaped and nothing here is needed to buy from Dixi.
 *
 * TWO RULES THIS WINDOW EXISTS TO KEEP:
 *
 *   * it must never report that a trade succeeded. `TPlayer::AcceptTrade`
 *     sends SV_CMD_CLOSE_TRADE to both sides after moving the objects **and**
 *     sends it to both sides when a reach or capacity check fails. The close
 *     carries nothing to tell them apart, so the window says the trade ended
 *     and leaves what changed hands to the inventory Fusion32 sends;
 *   * accepting is not completing. `AcceptTrade` records the flag and returns
 *     while the other side has not accepted, so the window shows that this
 *     client asked and is waiting, never that the trade is done.
 *
 * Pure Slate, built in C++, so the project still carries no .uasset.
 */
class SReal33DTradePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DTradePanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UReal33DBridge>, Bridge)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * Takes this frame's trade from the bridge. Called once a frame by
	 * `SReal33DHUD::Refresh`, never from this widget's own Tick. Slate does not
	 * tick a collapsed widget, and the registry may collapse this one, so a
	 * self-driven version could miss the trade that arrived while it was
	 * hidden. That exact mistake cost a live session in
	 * `UNREAL-EDITORS-UI-001` and is not repeated here.
	 */
	void Refresh();

	/** True while a trade from Fusion32 is on screen. */
	bool IsShowing() const { return bShowing; }

	// --------------------------------------------------------- display rules
	//
	// Static and free of Slate, so each is testable without a renderer.

	/**
	 * The heading for one side of the trade.
	 *
	 * The offering character's name comes from the server; an offer that has
	 * not arrived is named as absent rather than given an empty heading,
	 * because "the partner has offered nothing yet" is a real and common
	 * state in 7.72 -- Fusion32 tells the other side only "X wants to trade
	 * with you" until they put an object up.
	 */
	static FText OfferTitle(const FReal33DTradeOffer& Offer, bool bOwn);

	/**
	 * What stands where the item rows would be when an offer has none.
	 *
	 * Distinguishes "not offered yet" from "offered, and the list was empty",
	 * because only the second is a statement about what the server sent.
	 */
	static FText EmptyOfferText(const FReal33DTradeOffer& Offer, bool bOwn);

	/**
	 * The line describing where the trade stands.
	 *
	 * Never says the trade completed or will complete. It reports what this
	 * client has sent and what Fusion32 has sent, and nothing in between.
	 */
	static FText StatusText(const FReal33DTrade& Trade);

	/** Whether Accept does anything: a trade must be open and both sides in. */
	static bool CanAccept(const FReal33DTrade& Trade);

private:
	void Rebuild();
	TSharedRef<SWidget> BuildOffer(const FReal33DTradeOffer& Offer, bool bOwn);

	FReply HandleAccept();
	FReply HandleReject();
	FReply HandleInspect(bool bPartnerOffer, int32 Index);

	TWeakObjectPtr<UReal33DBridge> Bridge;

	TSharedPtr<SVerticalBox> Body;
	TSharedPtr<STextBlock> Status;
	TSharedPtr<SButton> AcceptButton;

	uint64 DrawnRevision = 0;
	bool bEverDrawn = false;
	bool bShowing = false;
};
