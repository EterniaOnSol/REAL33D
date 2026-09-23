#include "Real33DPanelChrome.h"

#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// 30-miniwindow.otui: the header is 15 tall, the title sits 2 down and 20
	// in from the left, and the contents are inset 3 on each side.
	constexpr float HeaderHeight = FReal33DUIStyle::MiniWindowHeaderHeight;
	constexpr float TitleLeft = 20.0f;
	constexpr float ContentInset = 3.0f;
	constexpr float ButtonGap = 1.0f;
	constexpr int32 TitleSize = 9;
}

TSharedRef<SWidget> SReal33DMiniWindow::MakeHeaderButton(const FName& Brush, bool bWired)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();
	const float Size = FReal33DUIStyle::MiniWindowButtonSize;

	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(FCoreStyle::Get(), "NoBorder")
		.ContentPadding(FMargin(0.0f))
		.IsEnabled(bWired)
		.OnClicked(bWired
			? FOnClicked::CreateSP(this, &SReal33DMiniWindow::ToggleCollapsed)
			: FOnClicked())
		[
			SNew(SBox)
			.WidthOverride(Size)
			.HeightOverride(Size)
			[
				SNew(SImage).Image(Style.GetBrush(Brush))
			]
		];

	return Button;
}

void SReal33DMiniWindow::Construct(const FArguments& InArgs)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();
	ContentHeight = InArgs._ContentHeight;

	// The contents, and the dither over them when there is nothing to put there.
	TSharedRef<SOverlay> Contents = SNew(SOverlay)
		+ SOverlay::Slot()
		[
			InArgs._Content.Widget
		];
	if (InArgs._Inert)
	{
		Contents->AddSlot()
		[
			SNew(SImage)
			.Image(Style.GetBrush("Real33D.Chrome.Dither"))
				.ColorAndOpacity(FReal33DUIStyle::DitherTint())
			// Phantom in the 2D's sense: the dither states that the surface is
			// dead, and must not also be the thing that swallows clicks.
			.Visibility(EVisibility::HitTestInvisible)
		];
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(Style.GetBrush("Real33D.Chrome.WindowBody"))
		// The 2D's own PhantomMiniWindow opacity. Only the body is faded; the
		// contents below are separate widgets and keep their own full alpha.
		.BorderBackgroundColor(FReal33DUIStyle::PanelTint())
		.Padding(FMargin(0.0f))
		[
			SNew(SVerticalBox)

			// ------------------------------------------------------- header
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(HeaderHeight)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SImage)
						.Image(Style.GetBrush("Real33D.Chrome.WindowHeader"))
						.ColorAndOpacity(FReal33DUIStyle::PanelTint())
					]
					+ SOverlay::Slot()
					.VAlign(VAlign_Center)
					.Padding(FMargin(4.0f, 0.0f, 3.0f, 0.0f))
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							InArgs._IconBrush.IsNone()
								? SNullWidget::NullWidget
								: StaticCastSharedRef<SWidget>(SNew(SBox)
									.WidthOverride(12.0f)
									.HeightOverride(12.0f)
									[
										SNew(SImage).Image(Style.GetBrush(InArgs._IconBrush))
									])
						]

						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						.Padding(FMargin(
							InArgs._IconBrush.IsNone() ? 0.0f : TitleLeft - 16.0f,
							0.0f, 0.0f, 0.0f))
						[
							SNew(STextBlock)
							.Text(InArgs._Title)
							.ColorAndOpacity(Style.GetSlateColor(InArgs._Inert
								? "Real33D.Text.Disabled" : "Real33D.Text.Title"))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", TitleSize))
						]

						// The header buttons, right to left as the 2D lays them
						// out: context menu, filter, minimise, close.
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin(0.0f, 0.0f, ButtonGap, 0.0f))
						[
							MakeHeaderButton("Real33D.Chrome.ContextMenu", false)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin(0.0f, 0.0f, ButtonGap, 0.0f))
						[
							MakeHeaderButton("Real33D.Chrome.Filter", false)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin(0.0f, 0.0f, ButtonGap, 0.0f))
						[
							// The one live button: collapsing a panel is a
							// local decision and needs nothing from Fusion32.
							MakeHeaderButton("Real33D.Chrome.Minimize", true)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							MakeHeaderButton("Real33D.Chrome.Close", false)
						]
					]
				]
			]

			// The 2px bevel and 1px border the .otui draws under the header.
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(ContentInset, 0.0f, ContentInset, 0.0f))
			[
				SNew(SBox).HeightOverride(1.0f)
				[
					SNew(SImage).ColorAndOpacity(FLinearColor(FColor(0x66, 0x66, 0x66, 0x40)))
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(ContentInset, 0.0f, ContentInset, 0.0f))
			[
				SNew(SBox).HeightOverride(1.0f)
				[
					SNew(SImage).ColorAndOpacity(FLinearColor(FColor(0x2A, 0x2A, 0x2A, 0xFF)))
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
				]
			]

			// ----------------------------------------------------- contents
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(ContentInset, 2.0f, ContentInset, ContentInset))
			[
				SAssignNew(Body, SBox)
				.HeightOverride(ContentHeight)
				[
					Contents
				]
			]
		]
	];
}

FReply SReal33DMiniWindow::ToggleCollapsed()
{
	bCollapsed = !bCollapsed;
	if (Body.IsValid())
	{
		// Collapsed to nothing rather than hidden, so the panels below slide up
		// exactly as they do in the 2D when a miniwindow is rolled away.
		StaticCastSharedPtr<SBox>(Body)->SetHeightOverride(
			bCollapsed ? 0.0f : ContentHeight);
		Body->SetVisibility(bCollapsed ? EVisibility::Collapsed : EVisibility::Visible);
	}
	return FReply::Handled();
}

// ------------------------------------------------------------------- the slot

void SReal33DSlot::Construct(const FArguments& InArgs)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	TSharedRef<SOverlay> Stack = SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(Style.GetBrush(InArgs._SlotBrush))
		];

	if (!InArgs._PlaceholderBrush.IsNone())
	{
		Stack->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(FReal33DUIStyle::SlotIconSize)
			.HeightOverride(FReal33DUIStyle::SlotIconSize)
			[
				SNew(SImage).Image(Style.GetBrush(InArgs._PlaceholderBrush))
			]
		];
	}

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(FReal33DUIStyle::SlotSize)
		.HeightOverride(FReal33DUIStyle::SlotSize)
		.ToolTipText(InArgs._Tooltip)
		[
			Stack
		]
	];
}
