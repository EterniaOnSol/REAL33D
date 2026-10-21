#include "Real33DOutfitPanel.h"

#include "REAL33D.h"
#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

using Real33D::kMaxOutfitColor;
using Real33D::kOutfitSlotCount;
using Real33D::OutfitDraft;
using Real33D::OutfitSlot;
using Real33D::OutfitSlotAt;
using Real33D::OutfitSlotIndex;
using Real33D::OutfitSlotName;

namespace
{
	const FLinearColor BodyColour(0.92f, 0.90f, 0.82f, 1.0f);
	const FLinearColor NoteColour(0.62f, 0.68f, 0.78f, 1.0f);
	/** The row the palette controls are editing. Presentation only. */
	const FLinearColor ActiveColour(1.0f, 0.86f, 0.46f, 1.0f);
	const FLinearColor PanelColour(0.03f, 0.03f, 0.04f, 0.94f);

	constexpr float kWindowWidth = 380.0f;
	constexpr int32 kTextSize = 9;
	/** Wide enough to be an obvious target; the first live run never found them. */
	// UE 5.8's default button padding consumes all 32 px; steppers override it.
	constexpr float kStepWidth = 32.0f;
}

FText SReal33DOutfitPanel::Title()
{
	// The classic 7.72 client's own window title, at 0x00563C78 in the image
	// `docs/CLASSIC_CLIENT_772.md` records. Not a name invented here.
	return NSLOCTEXT("Real33D", "OutfitTitle", "Select Outfit");
}

FText SReal33DOutfitPanel::RangeText(const FReal33DOutfitDialog& Dialog)
{
	if (!Dialog.bOpen)
	{
		return FText::GetEmpty();
	}
	const int32 Count = Dialog.Count();
	if (Count <= 0)
	{
		// Cannot happen against a healthy server: SendOutfit computes First and
		// Last from one expression. Reported rather than repaired, because
		// reordering it would offer a range the server never granted.
		return FText::FromString(FString::Printf(
			TEXT("Fusion32 granted no selectable outfit (it sent %u..%u)."),
			Dialog.First, Dialog.Last));
	}
	return FText::FromString(FString::Printf(
		TEXT("Fusion32 allows outfits %u to %u (%d)."),
		Dialog.First, Dialog.Last, Count));
}

FText SReal33DOutfitPanel::OutfitText(const OutfitDraft& Draft)
{
	if (!Draft.IsUsable())
	{
		return NSLOCTEXT("Real33D", "OutfitNone", "No outfit is selectable.");
	}
	const int32 Position = int32(Draft.OutfitId()) - int32(Draft.FirstOutfit()) + 1;
	return FText::FromString(FString::Printf(TEXT("Outfit %u  (%d of %d)"),
		Draft.OutfitId(), Position, Draft.Count()));
}

FText SReal33DOutfitPanel::SlotText(const OutfitDraft& Draft, OutfitSlot Slot)
{
	// The slot's own name, proven against the classic client rather than
	// adopted by convention; see Real33DOutfitSelection.h.
	return FText::FromString(FString::Printf(TEXT("%s  %u"),
		ANSI_TO_TCHAR(OutfitSlotName(Slot)), Draft.Color(Slot)));
}

FText SReal33DOutfitPanel::DisguiseNote(const FReal33DOutfitDialog& Dialog)
{
	if (!Dialog.bOpen || !Dialog.Current.bDisguisedAsObject)
	{
		return FText::GetEmpty();
	}
	// CSetOutfit reads a word and then four bytes unconditionally, so the
	// command has nowhere to put an object type. Said plainly instead of
	// quietly turning the disguise into an outfit id.
	return FText::FromString(FString::Printf(
		TEXT("You are disguised as object type %u. That cannot be submitted: "
			 "the 7.72 set-outfit command carries an outfit id and four "
			 "colours and has no room for an object. Choosing below replaces "
			 "the disguise."),
		Dialog.Current.ObjectType));
}

bool SReal33DOutfitPanel::CanSubmit(const FReal33DOutfitDialog& Dialog,
	const OutfitDraft& Draft)
{
	// Both halves, because either alone would be wrong. The dialog must still
	// be the one the server opened, and the draft must pass CSetOutfit's two
	// tests. A selection the server would drop must not reach the wire: it
	// answers one with silence, which this client could not tell from success.
	return Dialog.bOpen && Draft.CanSubmit() && Dialog.Contains(Draft.OutfitId());
}

void SReal33DOutfitPanel::Construct(const FArguments& InArgs)
{
	Bridge = InArgs._Bridge;
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	TSharedRef<SVerticalBox> ColorRows = SNew(SVerticalBox);
	for (int32 Index = 0; Index < kOutfitSlotCount; ++Index)
	{
		ColorRows->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 1.0f))
			[
				BuildColorRow(OutfitSlotAt(Index))
			];
	}

	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SBox)
		.WidthOverride(kWindowWidth)
		[
			SNew(SBorder)
			.BorderImage(Style.GetBrush("Real33D.Chrome.WindowBody"))
			.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.96f))
			.Padding(FMargin(0.0f))
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity(PanelColour)
				]
				+ SOverlay::Slot()
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBorder)
						.BorderImage(Style.GetBrush("Real33D.Chrome.WindowHeader"))
						.Padding(FMargin(6.0f, 3.0f))
						[
							SAssignNew(Heading, STextBlock)
							.Text(Title())
							.ColorAndOpacity(FSlateColor(BodyColour))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", kTextSize))
						]
					]

					// The band, in the server's own numbers.
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 4.0f, 8.0f, 0.0f))
					[
						SAssignNew(RangeLabel, STextBlock)
						.ColorAndOpacity(FSlateColor(NoteColour))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.AutoWrapText(true)
					]

					// Why a disguise cannot be sent back, when there is one.
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 2.0f, 8.0f, 0.0f))
					[
						SAssignNew(DisguiseLabel, STextBlock)
						.ColorAndOpacity(FSlateColor(NoteColour))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.AutoWrapText(true)
					]

					// The outfit stepper. A band, not a list, so this is
					// arithmetic over First..Last and there is nothing to
					// enumerate.
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 6.0f, 8.0f, 2.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(kStepWidth)
							[
								SAssignNew(PreviousOutfitButton, SButton)
								.NormalPaddingOverride(FMargin(2.0f, 1.5f))
								.PressedPaddingOverride(FMargin(2.0f, 2.5f, 2.0f, 0.5f))
								.Text(NSLOCTEXT("Real33D", "OutfitPrev", "<"))
								.ToolTipText(NSLOCTEXT("Real33D", "OutfitPrevTip",
									"The previous outfit Fusion32 allows. Wraps "
									"inside the band it granted and never leaves it."))
								.OnClicked(FOnClicked::CreateSP(this,
									&SReal33DOutfitPanel::HandleStepOutfit, -1))
							]
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(FMargin(6.0f, 0.0f))
						.VAlign(VAlign_Center)
						[
							SAssignNew(OutfitLabel, STextBlock)
							.ColorAndOpacity(FSlateColor(BodyColour))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", kTextSize))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(kStepWidth)
							[
								SAssignNew(NextOutfitButton, SButton)
								.NormalPaddingOverride(FMargin(2.0f, 1.5f))
								.PressedPaddingOverride(FMargin(2.0f, 2.5f, 2.0f, 0.5f))
								.Text(NSLOCTEXT("Real33D", "OutfitNext", ">"))
								.ToolTipText(NSLOCTEXT("Real33D", "OutfitNextTip",
									"The next outfit Fusion32 allows. Wraps "
									"inside the band it granted and never leaves it."))
								.OnClicked(FOnClicked::CreateSP(this,
									&SReal33DOutfitPanel::HandleStepOutfit, +1))
							]
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 4.0f, 8.0f, 0.0f))
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("Real33D", "OutfitColours",
							"Colours of the various parts of your body"))
						.ColorAndOpacity(FSlateColor(NoteColour))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 2.0f))
					[
						ColorRows
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 2.0f))
					[
						SAssignNew(Status, STextBlock)
						.ColorAndOpacity(FSlateColor(NoteColour))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.AutoWrapText(true)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 4.0f, 8.0f, 8.0f))
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
							SAssignNew(SubmitButton, SButton)
							.Text(NSLOCTEXT("Real33D", "OutfitSubmit", "Set Outfit"))
							.ToolTipText(NSLOCTEXT("Real33D", "OutfitSubmitTip",
								"Sends the selection to Fusion32 and closes. "
								"Nothing acknowledges it: the server answers a "
								"set-outfit with silence, and the appearance "
								"update it broadcasts only reaches you if you "
								"are already wearing your own outfit."))
							.OnClicked(FOnClicked::CreateSP(
								this, &SReal33DOutfitPanel::HandleSubmit))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(NSLOCTEXT("Real33D", "OutfitClose", "Cancel"))
							.ToolTipText(NSLOCTEXT("Real33D", "OutfitCloseTip",
								"Closes the window and sends nothing. 7.72 has "
								"no command that cancels a chooser, so Fusion32 "
								"is told nothing and keeps no record of it."))
							.OnClicked(FOnClicked::CreateSP(
								this, &SReal33DOutfitPanel::HandleClose))
						]
					]
				]
			]
		]
	];

	// Built collapsed. Fusion32 opens this window, never the client.
	SetVisibility(EVisibility::Collapsed);
}

TSharedRef<SWidget> SReal33DOutfitPanel::BuildColorRow(OutfitSlot Slot)
{
	FColorRow& Row = Rows[OutfitSlotIndex(Slot)];
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			// PLAIN TEXT, NOT A BUTTON, and the first live run is why. An
			// earlier version made the label a button whose only effect was to
			// highlight the row, because the 2D window's four rows are
			// selectable -- but there they select which row a *shared palette*
			// edits, and this window gives every row its own arrows instead. So
			// the label was a control that looked like it did something and did
			// not, the operator never touched the arrows, and the run submitted
			// the server's default colours twice without anyone noticing.
			SAssignNew(Row.Label, STextBlock)
			.ToolTipText(NSLOCTEXT("Real33D", "OutfitSlotTip",
				"One of the four colour bytes the set-outfit command carries. "
				"Fusion32 never names these slots; the names come from the "
				"classic 7.72 client itself. Use the arrows to change it."))
			.ColorAndOpacity(FSlateColor(BodyColour))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", kTextSize))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(6.0f, 0.0f, 2.0f, 0.0f))
		[
			SNew(SBox)
			.WidthOverride(kStepWidth)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("Real33D", "OutfitColourPrev", "<"))
				.NormalPaddingOverride(FMargin(2.0f, 1.5f))
				.PressedPaddingOverride(FMargin(2.0f, 2.5f, 2.0f, 0.5f))
				.ToolTipText(NSLOCTEXT("Real33D", "OutfitColourPrevTip",
					"The previous colour. Wraps over the whole range Fusion32 "
					"accepts and can never land outside it."))
				.OnClicked(FOnClicked::CreateSP(
					this, &SReal33DOutfitPanel::HandleStepColor, Slot, -1))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(kStepWidth)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("Real33D", "OutfitColourNext", ">"))
				.NormalPaddingOverride(FMargin(2.0f, 1.5f))
				.PressedPaddingOverride(FMargin(2.0f, 2.5f, 2.0f, 0.5f))
				.ToolTipText(NSLOCTEXT("Real33D", "OutfitColourNextTip",
					"The next colour. Wraps over the whole range Fusion32 "
					"accepts and can never land outside it."))
				.OnClicked(FOnClicked::CreateSP(
					this, &SReal33DOutfitPanel::HandleStepColor, Slot, +1))
			]
		];
}

void SReal33DOutfitPanel::Refresh()
{
	const UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr)
	{
		return;
	}
	const uint64 Revision = Live->GetOutfitDialogRevision();
	if (bEverDrawn && Revision == DrawnRevision)
	{
		// Not rebuilt every frame: a rebuild adopts the server's state into the
		// draft, and doing that under the player's hands would throw away the
		// outfit and colours they had just chosen.
		return;
	}
	DrawnRevision = Revision;
	bEverDrawn = true;
	Rebuild();
}

void SReal33DOutfitPanel::Rebuild()
{
	const UReal33DBridge* Live = Bridge.Get();
	const FReal33DOutfitDialog Dialog = Live != nullptr
		? Live->GetOutfitDialog() : FReal33DOutfitDialog();

	bShowing = Dialog.bOpen;
	// SelfHitTestInvisible rather than Visible, for the reason the editor
	// window records: this widget fills its whole overlay slot so the box can
	// be centred, and a plain Visible would swallow every click on the panels
	// behind it for as long as the chooser was open.
	SetVisibility(Dialog.bOpen
		? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);

	if (!Dialog.bOpen)
	{
		Draft.Close();
		return;
	}

	// THE SERVER'S STATE, ADOPTED WHOLE. This is what makes reopening the
	// chooser show what Fusion32 believes rather than what was left on screen
	// last time: a reopen is a fresh SV_CMD_OUTFIT, which bumps the revision
	// and lands here.
	//
	// A disguised current outfit carries no colours of its own -- TOutfit is a
	// union and SendOutfit wrote an object type where the colours would be --
	// so the draft starts from zeroes rather than from four bytes the server
	// never sent. `DisguiseNote` tells the player that is what happened.
	Draft.Open(Dialog.First, Dialog.Last, Dialog.Current.OutfitId,
		Dialog.Current.Colors);

	if (RangeLabel.IsValid()) { RangeLabel->SetText(RangeText(Dialog)); }
	if (DisguiseLabel.IsValid())
	{
		const FText Note = DisguiseNote(Dialog);
		DisguiseLabel->SetText(Note);
		DisguiseLabel->SetVisibility(
			Note.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);
	}
	RefreshControls();

	UE_LOG(LogReal33D, Log,
		TEXT("outfit chooser shown: band=%u..%u current=%u%s colours=%u,%u,%u,%u"),
		Dialog.First, Dialog.Last, Dialog.Current.OutfitId,
		Dialog.Current.bDisguisedAsObject ? TEXT(" [disguise]") : TEXT(""),
		Draft.Color(OutfitSlot::Head), Draft.Color(OutfitSlot::Body),
		Draft.Color(OutfitSlot::Legs), Draft.Color(OutfitSlot::Feet));
}

void SReal33DOutfitPanel::RefreshControls()
{
	const UReal33DBridge* Live = Bridge.Get();
	const FReal33DOutfitDialog Dialog = Live != nullptr
		? Live->GetOutfitDialog() : FReal33DOutfitDialog();

	if (OutfitLabel.IsValid()) { OutfitLabel->SetText(OutfitText(Draft)); }
	if (Status.IsValid())      { Status->SetText(StatusText(Dialog, Draft)); }

	// Dead rather than misleading when the band names nothing: a control that
	// moved a selection the server would refuse would be worse than no control.
	const bool bSteppable = Draft.Count() > 1;
	if (PreviousOutfitButton.IsValid()) { PreviousOutfitButton->SetEnabled(bSteppable); }
	if (NextOutfitButton.IsValid())     { NextOutfitButton->SetEnabled(bSteppable); }
	if (SubmitButton.IsValid())         { SubmitButton->SetEnabled(CanSubmit(Dialog, Draft)); }

	for (int32 Index = 0; Index < kOutfitSlotCount; ++Index)
	{
		const OutfitSlot Slot = OutfitSlotAt(Index);
		FColorRow& Row = Rows[Index];
		if (Row.Label.IsValid())
		{
			Row.Label->SetText(SlotText(Draft, Slot));
			Row.Label->SetColorAndOpacity(FSlateColor(
				Draft.Slot() == Slot ? ActiveColour : BodyColour));
		}
	}
}

FText SReal33DOutfitPanel::StatusText(const FReal33DOutfitDialog& Dialog,
	const OutfitDraft& Draft)
{
	if (!Dialog.bOpen)
	{
		return FText::GetEmpty();
	}
	if (Dialog.Count() <= 0)
	{
		return NSLOCTEXT("Real33D", "OutfitNoBand",
			"Nothing can be sent: Fusion32 granted an empty range of outfits.");
	}
	if (!Dialog.Contains(Draft.OutfitId()))
	{
		// Tested against the DIALOG's band and not the draft's own, because
		// the dialog is the authority: a newer SV_CMD_OUTFIT can narrow the
		// band under a draft that was opened from a wider one, and it is the
		// newer answer that the server will enforce. Unreachable through this
		// window's own controls, which clamp, so it is a statement rather than
		// an assert.
		return NSLOCTEXT("Real33D", "OutfitOutOfBand",
			"The selected outfit is outside the range Fusion32 granted and "
			"will not be sent.");
	}
	// The exact claim this client is entitled to make, and no more. There is no
	// success message because there is no success signal.
	return FText::FromString(FString::Printf(
		TEXT("Each colour may be 0 to %u. Fusion32 does not acknowledge a "
			 "set-outfit, so this window closes when you send; your appearance "
			 "changes when the server broadcasts it."),
		uint32(kMaxOutfitColor)));
}

FReply SReal33DOutfitPanel::HandleStepOutfit(int32 Delta)
{
	// Clamped and wrapped by the draft, which cannot leave the server's band.
	const uint16 Before = Draft.OutfitId();
	Draft.StepOutfit(Delta);
	RefreshControls();
	// Logged because a live run has to be able to show the band was obeyed,
	// and because the first one could not: nothing recorded what the player
	// touched, so a control that did nothing looked the same in evidence as a
	// control nobody pressed.
	UE_LOG(LogReal33D, Log,
		TEXT("outfit preview: id %u -> %u (band %u..%u)"),
		Before, Draft.OutfitId(), Draft.FirstOutfit(), Draft.LastOutfit());
	return FReply::Handled();
}

FReply SReal33DOutfitPanel::HandleStepColor(OutfitSlot Slot, int32 Delta)
{
	Draft.SelectSlot(Slot);
	const uint8 Before = Draft.Color(Slot);
	Draft.StepColor(Slot, Delta);
	RefreshControls();
	UE_LOG(LogReal33D, Log,
		TEXT("outfit preview: %s colour %u -> %u (max %u); now %u,%u,%u,%u"),
		ANSI_TO_TCHAR(OutfitSlotName(Slot)), Before, Draft.Color(Slot),
		uint32(kMaxOutfitColor),
		Draft.Color(OutfitSlot::Head), Draft.Color(OutfitSlot::Body),
		Draft.Color(OutfitSlot::Legs), Draft.Color(OutfitSlot::Feet));
	return FReply::Handled();
}

FReply SReal33DOutfitPanel::HandleSubmit()
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr)
	{
		return FReply::Handled();
	}
	const FReal33DOutfitDialog Dialog = Live->GetOutfitDialog();
	if (!CanSubmit(Dialog, Draft))
	{
		return FReply::Handled();
	}

	const uint16 OutfitId = Draft.OutfitId();
	const uint32 ActionId = Live->RequestSetOutfit(OutfitId, Draft.AllColors());
	UE_LOG(LogReal33D, Log,
		TEXT("outfit submit %u: id=%u colours=%u,%u,%u,%u"),
		ActionId, OutfitId,
		Draft.Color(OutfitSlot::Head), Draft.Color(OutfitSlot::Body),
		Draft.Color(OutfitSlot::Legs), Draft.Color(OutfitSlot::Feet));

	if (ActionId == 0)
	{
		// Nothing left: the bridge refuses a selection with no chooser behind
		// it, and a local refusal by ClientCore's rules reaches the player as a
		// notice in Local Chat. Either way the window stays so they can see it.
		if (Status.IsValid())
		{
			Status->SetText(NSLOCTEXT("Real33D", "OutfitNotSent",
				"Not sent. The reason is in Local Chat."));
		}
		return FReply::Handled();
	}

	// CLOSED HERE, AND THIS IS NOT A CLAIM OF SUCCESS. CSetOutfit returns void
	// and its CREATURE_OUTFIT_CHANGED broadcast is conditional on the player
	// already wearing their own outfit, so there may be nothing at all to wait
	// for. The classic client closes on OK too, at 0x0047138B. What the player
	// will see, if the server chose to broadcast, is their own creature
	// changing in the world -- which is the authoritative answer and arrives
	// through SV_CMD_CREATURE_OUTFIT like anyone else's.
	Live->CloseOutfitDialog();
	bShowing = false;
	SetVisibility(EVisibility::Collapsed);
	return FReply::Handled();
}

FReply SReal33DOutfitPanel::HandleClose()
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live != nullptr)
	{
		Live->CloseOutfitDialog();
	}
	bShowing = false;
	Draft.Close();
	SetVisibility(EVisibility::Collapsed);
	return FReply::Handled();
}
