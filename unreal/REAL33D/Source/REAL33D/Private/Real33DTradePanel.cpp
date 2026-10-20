#include "Real33DTradePanel.h"

#include "REAL33D.h"
#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor RowColour(0.87f, 0.87f, 0.87f, 1.0f);
	const FLinearColor NoteColour(0.62f, 0.68f, 0.78f, 1.0f);
	const FLinearColor PanelColour(0.03f, 0.03f, 0.04f, 0.94f);
	constexpr float kWindowWidth = 420.0f;
	constexpr int32 kTextSize = 8;
}

FText SReal33DTradePanel::OfferTitle(const FReal33DTradeOffer& Offer, bool bOwn)
{
	if (!Offer.bKnown)
	{
		return bOwn
			? NSLOCTEXT("Real33D", "TradeOwnNone", "You have offered nothing")
			: NSLOCTEXT("Real33D", "TradePartnerNone", "No offer from the other side yet");
	}
	// The server names the offering character in every offer it sends.
	return FText::FromString(bOwn
		? FString::Printf(TEXT("Your offer (%s)"), *Offer.Name)
		: FString::Printf(TEXT("Offered by %s"), *Offer.Name));
}

FText SReal33DTradePanel::EmptyOfferText(const FReal33DTradeOffer& Offer, bool bOwn)
{
	if (!Offer.bKnown)
	{
		// Not a statement about the server: no offer command has arrived.
		return bOwn
			? NSLOCTEXT("Real33D", "TradeArmHint",
				"Ctrl+right-click an item, choose Trade with, then click the other player.")
			: NSLOCTEXT("Real33D", "TradeWaitPartner",
				"Fusion32 sends their offer only once they put an object up.");
	}
	// The offer arrived and carried no objects, which only the server can say.
	return NSLOCTEXT("Real33D", "TradeOfferEmpty", "Fusion32 listed no objects.");
}

FText SReal33DTradePanel::StatusText(const FReal33DTrade& Trade)
{
	if (!Trade.bOpen)
	{
		return FText::GetEmpty();
	}
	if (Trade.bAcceptSent)
	{
		// Exactly what is true: we asked. AcceptTrade returns silently while
		// the other side has not accepted.
		return NSLOCTEXT("Real33D", "TradeAcceptSent",
			"Accept sent. Fusion32 completes the trade only when both sides have "
			"accepted, and it does not acknowledge an accept on its own.");
	}
	if (!Trade.Partner.bKnown)
	{
		return NSLOCTEXT("Real33D", "TradeHalfOpen",
			"Waiting for the other player to offer an object.");
	}
	return NSLOCTEXT("Real33D", "TradeBothOffered",
		"Both sides have offered. Accepting does not complete the trade on its own.");
}

bool SReal33DTradePanel::CanAccept(const FReal33DTrade& Trade)
{
	// AcceptTrade returns immediately unless this player and the partner both
	// hold a TradeObject, so an accept before both offers reaches the server
	// and does nothing. Offered only when it can mean something.
	return Trade.bOpen && Trade.Own.bKnown && Trade.Partner.bKnown;
}

void SReal33DTradePanel::Construct(const FArguments& InArgs)
{
	Bridge = InArgs._Bridge;
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	ChildSlot
	.Padding(FMargin(6.0f, 2.0f, 6.0f, 4.0f))
	[
		SNew(SVerticalBox)

		// Both offers, rebuilt whenever the trade changes.
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(Body, SVerticalBox)
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 2.0f))
		[
			SAssignNew(Status, STextBlock)
			.ColorAndOpacity(FSlateColor(NoteColour))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", kTextSize))
			.AutoWrapText(true)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(SSpacer)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FMargin(0.0f, 0.0f, 6.0f, 0.0f))
						[
							SAssignNew(AcceptButton, SButton)
							.Text(NSLOCTEXT("Real33D", "TradeAccept", "Accept"))
							.ToolTipText(NSLOCTEXT("Real33D", "TradeAcceptTip",
								"Tells Fusion32 you accept. It completes the trade only "
								"once both sides have, and acknowledges nothing by itself."))
							.OnClicked(FOnClicked::CreateSP(this, &SReal33DTradePanel::HandleAccept))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(NSLOCTEXT("Real33D", "TradeReject", "Reject"))
							.ToolTipText(NSLOCTEXT("Real33D", "TradeRejectTip",
								"Cancels the trade. Fusion32 answers the other player, "
								"not this client, so the window closes here."))
							.OnClicked(FOnClicked::CreateSP(this, &SReal33DTradePanel::HandleReject))
						]
		]
	];

	// Drawn from the start, unlike the text editor. The Offer control is how
	// a trade BEGINS, so it cannot live behind a surface that only appears
	// once a trade already exists -- a live run found exactly that dead end:
	// Fusion32 answers the first offer by telling the other player "X wants
	// to trade with you" as plain prose and nothing else, so B had no window
	// and no way to respond. Reading that sentence to infer a pending trade
	// would be inventing structure from English, which this project does not
	// do, so the panel is simply always reachable.
	Rebuild();
}

void SReal33DTradePanel::Refresh()
{
	const UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr)
	{
		return;
	}
	const uint64 Revision = Live->GetTradeRevision();
	if (bEverDrawn && Revision == DrawnRevision)
	{
		return;
	}
	DrawnRevision = Revision;
	bEverDrawn = true;
	Rebuild();
}

TSharedRef<SWidget> SReal33DTradePanel::BuildOffer(const FReal33DTradeOffer& Offer, bool bOwn)
{
	TSharedRef<SVerticalBox> Column = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 4.0f, 0.0f, 2.0f))
		[
			SNew(STextBlock)
			.Text(OfferTitle(Offer, bOwn))
			.ColorAndOpacity(FSlateColor(RowColour))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", kTextSize))
		];

	if (!Offer.bKnown || Offer.Items.Num() == 0)
	{
		Column->AddSlot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(EmptyOfferText(Offer, bOwn))
				.ColorAndOpacity(FSlateColor(NoteColour))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", kTextSize))
				.AutoWrapText(true)
			];
		return Column;
	}

	// The server's own flat list, in the server's own order. A bag and its
	// contents arrive as consecutive objects with nothing marking the nesting,
	// so they are listed exactly as sent rather than arranged into a tree this
	// client would have had to invent.
	for (int32 Index = 0; Index < Offer.Items.Num(); ++Index)
	{
		const FReal33DItem& Item = Offer.Items[Index];
		FString Label = FString::Printf(TEXT("%2d.  object %u"), Index, Item.TypeId);
		if (Item.bHasAmount)
		{
			Label += FString::Printf(TEXT("  x%u"), Item.Amount);
		}
		Column->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 1.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Label))
					.ColorAndOpacity(FSlateColor(RowColour))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", kTextSize))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(NSLOCTEXT("Real33D", "TradeInspect", "Look"))
					.ToolTipText(NSLOCTEXT("Real33D", "TradeInspectTip",
						"Asks Fusion32 to describe this object. The description "
						"arrives in Local Chat, like any other Look."))
					.OnClicked(FOnClicked::CreateSP(
						this, &SReal33DTradePanel::HandleInspect, !bOwn, Index))
				]
			];
	}
	return Column;
}

void SReal33DTradePanel::Rebuild()
{
	const UReal33DBridge* Live = Bridge.Get();
	const FReal33DTrade Trade = Live != nullptr ? Live->GetTrade() : FReal33DTrade();

	bShowing = Trade.bOpen;
	// Visibility is NOT touched here. This is a docked panel and the registry
	// owns whether it is on screen; hiding it when no trade is open would put
	// the Offer control -- the only way to START a trade -- out of reach.
	if (!Trade.bOpen)
	{
	}

	if (Body.IsValid())
	{
		Body->ClearChildren();
		Body->AddSlot().AutoHeight()[ BuildOffer(Trade.Own, true) ];
		Body->AddSlot().AutoHeight()[ BuildOffer(Trade.Partner, false) ];
	}
	if (Status.IsValid())
	{
		Status->SetText(StatusText(Trade));
	}
	if (AcceptButton.IsValid())
	{
		AcceptButton->SetEnabled(CanAccept(Trade));
	}

	UE_LOG(LogReal33D, Log,
		TEXT("trade window: own=%d(%d items) partner=%d(%d items) accept_sent=%d"),
		Trade.Own.bKnown, Trade.Own.Items.Num(),
		Trade.Partner.bKnown, Trade.Partner.Items.Num(), Trade.bAcceptSent);
}

FReply SReal33DTradePanel::HandleAccept()
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || !CanAccept(Live->GetTrade()))
	{
		return FReply::Handled();
	}
	const uint32 ActionId = Live->RequestTradeAccept();
	UE_LOG(LogReal33D, Log,
		TEXT("trade accept %u sent; completion needs the other side too"), ActionId);
	// The window stays open. Closing it here would look like the completion
	// Fusion32 has not announced.
	return FReply::Handled();
}

FReply SReal33DTradePanel::HandleReject()
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live != nullptr)
	{
		Live->RequestTradeReject();
	}
	bShowing = false;
	// The panel stays on screen; only the trade ends. RequestTradeReject has
	// already cleared the bridge's copy, so the next Refresh draws the idle
	// state.
	return FReply::Handled();
}

FReply SReal33DTradePanel::HandleInspect(bool bPartnerOffer, int32 Index)
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || Index < 0 || Index > 255)
	{
		return FReply::Handled();
	}
	const uint32 ActionId = Live->RequestTradeInspect(
		bPartnerOffer, static_cast<uint8>(Index));
	UE_LOG(LogReal33D, Log, TEXT("trade inspect %u: %s offer index %d"),
		ActionId, bPartnerOffer ? TEXT("partner") : TEXT("own"), Index);
	return FReply::Handled();
}
