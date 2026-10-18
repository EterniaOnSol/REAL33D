#include "Real33DChannelsPanel.h"

#include "REAL33D.h"
#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** Pale, like every other panel's rows. */
	const FLinearColor RowColour(0.87f, 0.87f, 0.87f, 1.0f);
	/** A row this client will not act on. Dim, as the disabled filters are. */
	const FLinearColor InertColour(0.55f, 0.55f, 0.55f, 1.0f);
	constexpr int32 TextSize = 8;
}

FText SReal33DChannelsPanel::RowLabel(const FReal33DChannelEntry& Entry)
{
	if (Entry.bOwnChannelOffer)
	{
		// No number: the 0xFFFF the server wrote is not one, and printing it
		// would invite the player to join channel 65535.
		return FText::FromString(Entry.Name);
	}
	return FText::FromString(FString::Printf(TEXT("%d  %s"), Entry.Id, *Entry.Name));
}

bool SReal33DChannelsPanel::IsJoinable(const FReal33DChannelEntry& Entry)
{
	return !Entry.bOwnChannelOffer
		&& Entry.Id != UReal33DBridge::kRuleViolationsChannel;
}

FText SReal33DChannelsPanel::JoinRefusalReason(const FReal33DChannelEntry& Entry)
{
	if (Entry.bOwnChannelOffer)
	{
		return NSLOCTEXT("Real33D", "ChannelOfferReason",
			"Not a channel. Fusion32 offers to create a private channel here; "
			"this client has no command for that yet.");
	}
	if (Entry.Id == UReal33DBridge::kRuleViolationsChannel)
	{
		return NSLOCTEXT("Real33D", "ChannelQueueReason",
			"This client cannot open this channel: Fusion32 answers it with a "
			"command REAL33D does not decode.");
	}
	return FText::GetEmpty();
}

FText SReal33DChannelsPanel::EmptyListText(bool bRequested, bool bKnown)
{
	if (!bRequested)
	{
		return NSLOCTEXT("Real33D", "ChannelsUnasked",
			"Press Channel List to ask Fusion32 which channels this character has.");
	}
	if (!bKnown)
	{
		return NSLOCTEXT("Real33D", "ChannelsWaiting", "Waiting for Fusion32...");
	}
	// The server answered, and the answer was none. Only here can this client
	// say so, because only here does it know.
	return NSLOCTEXT("Real33D", "ChannelsNone",
		"Fusion32 lists no channels for this character.");
}

void SReal33DChannelsPanel::Construct(const FArguments& InArgs)
{
	Bridge = InArgs._Bridge;

	ChildSlot
	.Padding(FMargin(6.0f, 2.0f, 6.0f, 4.0f))
	[
		SNew(SVerticalBox)

		// Asking. The list is not fetched on its own: a client that sent
		// CL_CMD_GET_CHANNELS by itself would be putting traffic on the wire
		// the player never asked for.
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SButton)
			.Text(NSLOCTEXT("Real33D", "ChannelsRefresh", "Channel List"))
			.ToolTipText(NSLOCTEXT("Real33D", "ChannelsRefreshTip",
				"Asks Fusion32 for the channels this character may join"))
			.OnClicked(FOnClicked::CreateSP(
				this, &SReal33DChannelsPanel::HandleRefreshClicked))
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(FMargin(0.0f, 4.0f))
		[
			SAssignNew(Scroller, SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(Rows, SVerticalBox)
			]
		]

		// A private conversation is opened by name, because
		// SV_CMD_PRIVATE_CHANNEL is addressed by character and carries no
		// number at all.
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SAssignNew(PrivateName, SEditableTextBox)
				.HintText(NSLOCTEXT("Real33D", "ChannelsPrivateHint", "Character name"))
				// CPrivateChannel reads into char Name[30] and readString
				// truncates rather than refusing, so a longer name would
				// arrive silently cut. Stopped at the keyboard instead.
				.MaximumLength(29)
				.OnTextCommitted(FOnTextCommitted::CreateSP(
					this, &SReal33DChannelsPanel::HandlePrivateNameCommitted))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(NSLOCTEXT("Real33D", "ChannelsPrivateOpen", "Message"))
				.ToolTipText(NSLOCTEXT("Real33D", "ChannelsPrivateTip",
					"Opens a private conversation. The character must exist; "
					"sending to them needs them online."))
				.OnClicked(FOnClicked::CreateSP(
					this, &SReal33DChannelsPanel::HandleOpenPrivateClicked))
			]
		]
	];

	RebuildRows();
}

void SReal33DChannelsPanel::Tick(const FGeometry& AllottedGeometry,
	const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	const UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr)
	{
		return;
	}
	// Two revisions, because a row's button depends on both: the list says
	// which channels there are, and the chat views say which of them this
	// player is in.
	const uint64 Revision = Live->GetChannelListRevision();
	const uint64 ChatRevision = Live->GetChatRevision();
	if (bEverDrawn && Revision == DrawnRevision && ChatRevision == DrawnChatRevision)
	{
		return;
	}
	DrawnRevision = Revision;
	DrawnChatRevision = ChatRevision;
	bEverDrawn = true;
	RebuildRows();
}

TSharedRef<SWidget> SReal33DChannelsPanel::BuildChannelRow(const FReal33DChannelEntry& Entry)
{
	const UReal33DBridge* Live = Bridge.Get();
	const bool bJoinable = IsJoinable(Entry);
	const bool bJoined = bJoinable && Live != nullptr && Live->IsChannelJoined(Entry.Id);
	const FText Reason = JoinRefusalReason(Entry);

	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(RowLabel(Entry))
			.ToolTipText(Reason)
			.ColorAndOpacity(FSlateColor(bJoinable ? RowColour : InertColour))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
		];

	if (!bJoinable)
	{
		// The reason stands where the button would: a row with neither reads as
		// something the client simply failed to draw.
		Row->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("Real33D", "ChannelsNotJoinable", "unavailable"))
				.ToolTipText(Reason)
				.ColorAndOpacity(FSlateColor(InertColour))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
			];
		return Row;
	}

	Row->AddSlot()
		.AutoWidth()
		[
			SNew(SButton)
			.Text(bJoined
				? NSLOCTEXT("Real33D", "ChannelsLeave", "Leave")
				: NSLOCTEXT("Real33D", "ChannelsJoin", "Join"))
			.ToolTipText(bJoined
				? NSLOCTEXT("Real33D", "ChannelsLeaveTip",
					"Leaves the channel and closes its tab")
				: NSLOCTEXT("Real33D", "ChannelsJoinTip",
					"Asks Fusion32 to open this channel; the tab appears when it answers"))
			.OnClicked(bJoined
				? FOnClicked::CreateSP(this, &SReal33DChannelsPanel::HandleLeaveClicked,
					Entry.Id)
				: FOnClicked::CreateSP(this, &SReal33DChannelsPanel::HandleJoinClicked,
					Entry.Id))
		];
	return Row;
}

void SReal33DChannelsPanel::RebuildRows()
{
	if (!Rows.IsValid())
	{
		return;
	}
	Rows->ClearChildren();

	const UReal33DBridge* Live = Bridge.Get();
	TArray<FReal33DChannelEntry> Channels;
	if (Live != nullptr)
	{
		Live->GetChannelList(Channels);
	}

	if (Channels.Num() == 0)
	{
		Rows->AddSlot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(EmptyListText(bRequested,
					Live != nullptr && Live->IsChannelListKnown()))
				.ColorAndOpacity(FSlateColor(InertColour))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
				.AutoWrapText(true)
			];
		return;
	}

	// In the server's own order. SendChannels walks the channels in ascending
	// number with the offer row first, and reordering them would make the list
	// this client shows differ from the list Fusion32 sent.
	for (const FReal33DChannelEntry& Each : Channels)
	{
		Rows->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 1.0f))
			[
				BuildChannelRow(Each)
			];
	}
}

FReply SReal33DChannelsPanel::HandleRefreshClicked()
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || !Live->IsRunning())
	{
		UE_LOG(LogReal33D, Warning, TEXT("not connected; no channel list requested"));
		return FReply::Handled();
	}
	bRequested = true;
	const uint32 ActionId = Live->RequestChannelList();
	UE_LOG(LogReal33D, Log, TEXT("channel list %u requested"), ActionId);
	// Redrawn now so the panel says it is waiting rather than still saying the
	// list has not been asked for.
	RebuildRows();
	return FReply::Handled();
}

FReply SReal33DChannelsPanel::HandleJoinClicked(int32 Channel)
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr)
	{
		return FReply::Handled();
	}
	// Nothing is opened here. The tab appears when Fusion32 answers.
	const uint32 ActionId = Live->RequestJoinChannel(Channel);
	UE_LOG(LogReal33D, Log, TEXT("join of channel %d handed to Fusion32 as %u"),
		Channel, ActionId);
	return FReply::Handled();
}

FReply SReal33DChannelsPanel::HandleLeaveClicked(int32 Channel)
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr)
	{
		return FReply::Handled();
	}
	// Not logged here. The bridge logs it, because the tab's close box leaves a
	// channel too and a line at this button would make that path invisible.
	Live->RequestLeaveChannel(Channel);
	return FReply::Handled();
}

FReply SReal33DChannelsPanel::HandleOpenPrivateClicked()
{
	OpenPrivate();
	return FReply::Handled();
}

void SReal33DChannelsPanel::HandlePrivateNameCommitted(const FText& Text,
	ETextCommit::Type Cause)
{
	if (Cause == ETextCommit::OnEnter)
	{
		OpenPrivate();
	}
}

void SReal33DChannelsPanel::OpenPrivate()
{
	if (!PrivateName.IsValid())
	{
		return;
	}
	const FString Name = PrivateName->GetText().ToString().TrimStartAndEnd();
	if (Name.IsEmpty())
	{
		// CPrivateChannel returns an empty name unsent, so this is not a
		// request at all.
		return;
	}
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || !Live->IsRunning())
	{
		UE_LOG(LogReal33D, Warning, TEXT("not connected; private channel not requested"));
		return;
	}
	// An intent. No tab opens here: SV_CMD_PRIVATE_CHANNEL does that, and an
	// unknown name is answered with PLAYERNOTEXISTING instead.
	const uint32 ActionId = Live->RequestOpenPrivateChannel(Name);
	UE_LOG(LogReal33D, Log, TEXT("private channel with \"%s\" requested as %u"),
		*Name, ActionId);
	PrivateName->SetText(FText::GetEmpty());
}
