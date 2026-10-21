#include "Real33DAmountPanel.h"

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

using Real33D::ClampStackAmount;
using Real33D::kStackAmountCoarseStep;
using Real33D::kStackAmountFineStep;
using Real33D::StepStackAmount;

namespace
{
	const FLinearColor BodyColour(0.92f, 0.90f, 0.82f, 1.0f);
	const FLinearColor NoteColour(0.62f, 0.68f, 0.78f, 1.0f);
	const FLinearColor PanelColour(0.03f, 0.03f, 0.04f, 0.94f);

	constexpr float kWindowWidth = 320.0f;
	constexpr int32 kTextSize = 9;
	// The outfit milestone's lesson, kept: UE 5.8's default button padding eats
	// a 32px box whole, so a stepper overrides its own padding and is wide
	// enough to be an obvious target. The first live outfit run found neither.
	constexpr float kStepWidth = 34.0f;
}

FText SReal33DAmountPanel::Title()
{
	// The classic 7.72 client's own window title, at 0x00563C00 in the image
	// `docs/CLASSIC_CLIENT_772.md` records. Not a name invented here.
	return NSLOCTEXT("Real33D", "AmountTitle", "Move Objects");
}

FText SReal33DAmountPanel::AmountText(uint32 InAmount, uint32 InMax)
{
	// Both numbers, always. The bound is what makes the answer meaningful, and
	// a live run has to be able to read the stack the question was opened for
	// straight off the screen.
	return FText::FromString(FString::Printf(TEXT("%u  of %u"),
		ClampStackAmount(static_cast<int64>(InAmount), InMax),
		ClampStackAmount(static_cast<int64>(InMax), InMax)));
}

FText SReal33DAmountPanel::NoteText()
{
	return NSLOCTEXT("Real33D", "AmountNote",
		"Ok asks Fusion32 to move this many. Nothing has been sent yet, and "
		"Cancel sends nothing at all. The counts you end up with are the "
		"server's answer, not this window's.");
}

void SReal33DAmountPanel::Construct(const FArguments& InArgs)
{
	OnChosen = InArgs._OnChosen;
	OnAbandoned = InArgs._OnAbandoned;
	const ISlateStyle& Style = FReal33DUIStyle::Get();

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
							SNew(STextBlock)
							.Text(Title())
							.ColorAndOpacity(FSlateColor(BodyColour))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", kTextSize))
						]
					]

					// Which object, and where it is going. Named so a live run
					// can match the window against the move that follows it.
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 5.0f, 8.0f, 0.0f))
					[
						SAssignNew(SubjectLabel, STextBlock)
						.ColorAndOpacity(FSlateColor(NoteColour))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.AutoWrapText(true)
					]

					// The answer, with the coarse and fine steppers either side.
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 6.0f, 8.0f, 2.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							MakeStep(NSLOCTEXT("Real33D", "AmountMuchLess", "<<"),
								NSLOCTEXT("Real33D", "AmountMuchLessTip",
									"Ten fewer. Stops at one rather than wrapping."),
								-kStackAmountCoarseStep)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FMargin(2.0f, 0.0f, 0.0f, 0.0f))
						[
							MakeStep(NSLOCTEXT("Real33D", "AmountLess", "<"),
								NSLOCTEXT("Real33D", "AmountLessTip",
									"One fewer. Stops at one rather than wrapping."),
								-kStackAmountFineStep)
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(FMargin(6.0f, 0.0f))
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SAssignNew(AmountLabel, STextBlock)
							.ColorAndOpacity(FSlateColor(BodyColour))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FMargin(0.0f, 0.0f, 2.0f, 0.0f))
						[
							MakeStep(NSLOCTEXT("Real33D", "AmountMore", ">"),
								NSLOCTEXT("Real33D", "AmountMoreTip",
									"One more. Stops at the whole stack rather "
									"than wrapping."),
								+kStackAmountFineStep)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							MakeStep(NSLOCTEXT("Real33D", "AmountMuchMore", ">>"),
								NSLOCTEXT("Real33D", "AmountMuchMoreTip",
									"Ten more. Stops at the whole stack rather "
									"than wrapping."),
								+kStackAmountCoarseStep)
						]
					]

					// The two ends of the range, reachable in one click each.
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 2.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(NSLOCTEXT("Real33D", "AmountOne", "One"))
							.ToolTipText(NSLOCTEXT("Real33D", "AmountOneTip",
								"A single object. The same amount Shift+drag "
								"sends without asking."))
							.OnClicked(FOnClicked::CreateSP(
								this, &SReal33DAmountPanel::HandleOne))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FMargin(6.0f, 0.0f, 0.0f, 0.0f))
						[
							SNew(SButton)
							.Text(NSLOCTEXT("Real33D", "AmountAll", "All"))
							.ToolTipText(NSLOCTEXT("Real33D", "AmountAllTip",
								"The whole stack, as the server last reported "
								"it. The same amount a plain drag sends without "
								"asking."))
							.OnClicked(FOnClicked::CreateSP(
								this, &SReal33DAmountPanel::HandleAll))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(SSpacer)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(8.0f, 2.0f))
					[
						SNew(STextBlock)
						.Text(NoteText())
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
							SAssignNew(OkButton, SButton)
							// "Ok" is the generic dialog caption in the same
							// image, at 0x00563738.
							.Text(NSLOCTEXT("Real33D", "AmountOk", "Ok"))
							.ToolTipText(NSLOCTEXT("Real33D", "AmountOkTip",
								"Sends one move request for this many and "
								"closes. Fusion32 rules on it; nothing moves "
								"on screen until the server says it did."))
							.OnClicked(FOnClicked::CreateSP(
								this, &SReal33DAmountPanel::HandleOk))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(NSLOCTEXT("Real33D", "AmountCancel", "Cancel"))
							.ToolTipText(NSLOCTEXT("Real33D", "AmountCancelTip",
								"Closes the window and sends nothing. No move "
								"was ever offered to Fusion32, so there is "
								"nothing to withdraw and the server is told "
								"nothing at all."))
							.OnClicked(FOnClicked::CreateSP(
								this, &SReal33DAmountPanel::HandleCancel))
						]
					]
				]
			]
		]
	];

	// Built collapsed. A drag opens this window, never the client on its own.
	SetVisibility(EVisibility::Collapsed);
}

TSharedRef<SWidget> SReal33DAmountPanel::MakeStep(const FText& Caption, const FText& Tip,
	int32 Delta)
{
	return SNew(SBox)
		.WidthOverride(kStepWidth)
		[
			SNew(SButton)
			.Text(Caption)
			.ToolTipText(Tip)
			.NormalPaddingOverride(FMargin(2.0f, 1.5f))
			.PressedPaddingOverride(FMargin(2.0f, 2.5f, 2.0f, 0.5f))
			.OnClicked(FOnClicked::CreateSP(this, &SReal33DAmountPanel::HandleStep, Delta))
		];
}

void SReal33DAmountPanel::Ask(uint32 InMax, const FText& Subject)
{
	Max = ClampStackAmount(static_cast<int64>(InMax), InMax);
	// Opens on the whole stack, so Ok alone equals the unmodified drag.
	Amount = Max;
	bShowing = true;
	if (SubjectLabel.IsValid())
	{
		SubjectLabel->SetText(Subject);
	}
	// SelfHitTestInvisible rather than Visible, for the reason the editor
	// window and the outfit chooser both record: this widget fills its whole
	// overlay slot so the box can be centred, and a plain Visible would
	// swallow every click on the panels behind it while the question was open.
	SetVisibility(EVisibility::SelfHitTestInvisible);
	RefreshControls();
	UE_LOG(LogReal33D, Log, TEXT("move amount asked: 1..%u, opening on %u"), Max, Amount);
}

void SReal33DAmountPanel::Abandon()
{
	if (!bShowing)
	{
		return;
	}
	bShowing = false;
	SetVisibility(EVisibility::Collapsed);
	UE_LOG(LogReal33D, Log, TEXT("move amount abandoned; nothing was sent"));
	OnAbandoned.ExecuteIfBound();
}

void SReal33DAmountPanel::RefreshControls()
{
	if (AmountLabel.IsValid())
	{
		AmountLabel->SetText(AmountText(Amount, Max));
	}
	if (OkButton.IsValid())
	{
		// Always live. Every reachable answer is one CMoveObject accepts,
		// because every control goes through ClampStackAmount, so there is no
		// state in which Ok would send something the server would refuse.
		OkButton->SetEnabled(bShowing);
	}
}

FReply SReal33DAmountPanel::HandleStep(int32 Delta)
{
	Amount = StepStackAmount(Amount, Delta, Max);
	RefreshControls();
	return FReply::Handled();
}

FReply SReal33DAmountPanel::HandleAll()
{
	Amount = Max;
	RefreshControls();
	return FReply::Handled();
}

FReply SReal33DAmountPanel::HandleOne()
{
	Amount = Real33D::kMinStackCount;
	RefreshControls();
	return FReply::Handled();
}

FReply SReal33DAmountPanel::HandleOk()
{
	if (!bShowing)
	{
		return FReply::Handled();
	}
	// Clamped once more on the way out. The controls cannot produce an
	// out-of-range answer, and this is still here because the one value that
	// reaches the wire should be bounded at the point it leaves, not only
	// wherever it was last edited.
	const uint32 Chosen = ClampStackAmount(static_cast<int64>(Amount), Max);
	bShowing = false;
	SetVisibility(EVisibility::Collapsed);
	UE_LOG(LogReal33D, Log, TEXT("move amount chosen: %u of %u"), Chosen, Max);
	OnChosen.ExecuteIfBound(Chosen);
	return FReply::Handled();
}

FReply SReal33DAmountPanel::HandleCancel()
{
	Abandon();
	return FReply::Handled();
}
