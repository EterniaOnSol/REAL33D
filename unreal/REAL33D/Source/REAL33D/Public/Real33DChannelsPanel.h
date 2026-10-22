#pragma once

#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SCompoundWidget.h"

class SScrollBox;
class SVerticalBox;

/**
 * The channel list, and the one place a channel is joined or left.
 *
 * Every row here came off the wire. Fusion32 decides which channels exist for
 * this player -- `operate.cc::ChannelAvailable` gates the guild channel on
 * having a guild and three more on gamemaster rights -- and it names them too,
 * since channel 0's name is the player's own guild. So this panel knows of no
 * channels at all until SV_CMD_CHANNELS answers, and draws none. A client that
 * listed the eight numbers `operate.hh` defines would be offering channels this
 * player may not be allowed to see and naming one of them wrongly.
 *
 * Joining is an intent, like everything else this client sends. Pressing Join
 * does not open a tab: the tab appears when SV_CMD_OPEN_CHANNEL comes back, and
 * for channel 3 it never does -- see `IsJoinable`.
 *
 * Pure Slate, built in C++, so the project still carries no .uasset.
 */
class SReal33DChannelsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DChannelsPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UReal33DBridge>, Bridge)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime,
		const float DeltaTime) override;

	/** Tall enough for the request row, a few channels and the private box. */
	static constexpr float PanelHeight = 186.0f;

	/**
	 * Width when the panel sits beside the console rather than in a column.
	 *
	 * The side columns are 176px and this kept to them. Next to the chat box
	 * it only has to carry the list button and the channel names, and the
	 * console is the thing that wants the width.
	 */
	static constexpr float PanelWidth = 150.0f;

	// ------------------------------------------------------- display rules
	//
	// Static and free of Slate so each can be tested without a renderer and
	// without a server. What a live run then has to prove is that the operator
	// sees these answers on screen.

	/**
	 * What a row reads.
	 *
	 * The server's own name, with its number, because two channels can share a
	 * name -- a guild channel is named after the guild -- and the number is
	 * what the player is actually joining. The offer row has no number to show,
	 * since its 0xFFFF is not one.
	 */
	static FText RowLabel(const FReal33DChannelEntry& Entry);

	/**
	 * Whether this client will send a join for this row.
	 *
	 * False twice, and both times because of this client rather than the
	 * server:
	 *
	 *   * the own-channel offer is not a channel. `SendChannels` writes it as
	 *     an invitation to create one, which takes CL_CMD_OPEN_CHANNEL, a
	 *     command this project has no builder for;
	 *   * channel 3 is answered with SV_CMD_OPEN_REQUEST_QUEUE, which this
	 *     project does not decode. An undecoded command stops the payload
	 *     walk, so asking costs the rest of that packet.
	 *
	 * Everything else is offered. Whether the server accepts is its ruling:
	 * `CJoinChannel` checks the number against `GetNumberOfChannels()`, which
	 * is server state this client has no copy of.
	 */
	static bool IsJoinable(const FReal33DChannelEntry& Entry);

	/** Why a row is not joinable. Empty when it is. */
	static FText JoinRefusalReason(const FReal33DChannelEntry& Entry);

	/**
	 * What stands where the rows would be when there are none.
	 *
	 * Three different states, never one: the list has not been asked for, the
	 * server answered with nothing, or the server has not answered yet. A
	 * client that printed "no channels" for all three would be claiming a fact
	 * about the server in two cases where it has none.
	 */
	static FText EmptyListText(bool bRequested, bool bKnown);

private:
	TSharedRef<SWidget> BuildChannelRow(const FReal33DChannelEntry& Entry);
	void RebuildRows();

	FReply HandleRefreshClicked();
	FReply HandleJoinClicked(int32 Channel);
	FReply HandleLeaveClicked(int32 Channel);
	FReply HandleOpenPrivateClicked();
	void HandlePrivateNameCommitted(const FText& Text, ETextCommit::Type Cause);
	void OpenPrivate();

	TWeakObjectPtr<UReal33DBridge> Bridge;

	TSharedPtr<SVerticalBox> Rows;
	TSharedPtr<SScrollBox> Scroller;
	TSharedPtr<SEditableTextBox> PrivateName;

	/** True once this client has asked. Distinct from having been answered. */
	bool bRequested = false;

	/** Last list revision drawn, so the rows rebuild only on a change. */
	uint64 DrawnRevision = 0;
	/** The chat revision too: joining or leaving changes a row's button. */
	uint64 DrawnChatRevision = 0;
	bool bEverDrawn = false;
};
