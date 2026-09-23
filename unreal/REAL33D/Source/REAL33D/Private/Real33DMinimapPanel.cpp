#include "Real33DMinimapPanel.h"

#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// minimap.otui: the frame is 115x111, 5 down and 8 in; the layer strip is
	// 20x68 against the right edge; the rose is 43x43; buttons are 20x20.
	constexpr float FrameWidth = FReal33DUIStyle::MinimapFrameWidth;
	constexpr float FrameHeight = FReal33DUIStyle::MinimapFrameHeight;
	constexpr float FrameTop = 5.0f;
	constexpr float FrameLeft = 8.0f;
	constexpr float RoseSize = 43.0f;
	constexpr float ButtonSize = 20.0f;
	constexpr float LayerWidth = 14.0f;
	constexpr float LayerHeight = 67.0f;
	constexpr int32 TextSize = 8;
}

TSharedRef<SWidget> SReal33DMinimapPanel::MakeButton(const FName& Brush, const FText& Tooltip)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	return SNew(SBox)
		.WidthOverride(ButtonSize)
		.HeightOverride(ButtonSize)
		.ToolTipText(Tooltip)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage).Image(Style.GetBrush(Brush))
			]
			+ SOverlay::Slot()
			[
				// Zooming and recentring act on a drawn map. There is none.
				SNew(SImage)
				.Image(Style.GetBrush("Real33D.Chrome.Dither"))
				.ColorAndOpacity(FReal33DUIStyle::DitherTint())
				.Visibility(EVisibility::HitTestInvisible)
			]
		];
}

void SReal33DMinimapPanel::Construct(const FArguments& InArgs)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	ChildSlot
	.Padding(FMargin(FrameLeft, FrameTop, 7.0f, 0.0f))
	[
		SNew(SHorizontalBox)

		// ------------------------------------------------- the map surface
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(FrameWidth)
			.HeightOverride(FrameHeight)
			[
				SNew(SBorder)
				.BorderImage(Style.GetBrush("Real33D.Chrome.SunkenFrame"))
				.Padding(FMargin(1.0f))
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						// Faded with the rest of the chrome. There is no automap
						// to hide behind it, so a solid black square would only
						// be blocking the view for nothing.
						SNew(SImage)
						.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.ColorAndOpacity(FLinearColor(0.04f, 0.04f, 0.05f,
							FReal33DUIStyle::PanelOpacity))
					]
					// The live half: where the server says the player is.
					+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SAssignNew(PositionLabel, STextBlock)
							.Text(FText::FromString(TEXT("--, --")))
							.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Readout"))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize + 1))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SAssignNew(FloorLabel, STextBlock)
							.Text(FText::FromString(TEXT("Floor --")))
							.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Readout"))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(FMargin(0.0f, 6.0f, 0.0f, 0.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Automap not recorded")))
							.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Disabled"))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
						]
					]
				]
			]
		]

		// ------------------------------------------- rose, zoom and floors
		//
		// No gap here. minimap.otui budgets 8 + 115 frame + 43 rose + 7 = 173
		// inside a 176 column, and the 4px I had added pushed the total to 177:
		// the rose and the zoom buttons were being clipped off the right edge.
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			[
				SNew(SBox)
				.WidthOverride(RoseSize)
				.HeightOverride(RoseSize)
				.ToolTipText(FText::FromString(TEXT("Compass (not available)")))
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SImage).Image(Style.GetBrush("Real33D.Automap.Rose"))
					]
					+ SOverlay::Slot()
					[
						SNew(SImage)
						.Image(Style.GetBrush("Real33D.Chrome.Dither"))
				.ColorAndOpacity(FReal33DUIStyle::DitherTint())
						.Visibility(EVisibility::HitTestInvisible)
					]
				]
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Bottom)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.0f, 0.0f, 0.0f, 2.0f))
					[
						MakeButton("Real33D.Automap.ZoomOut",
							FText::FromString(TEXT("Zoom out (not available)")))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.0f, 0.0f, 0.0f, 2.0f))
					[
						MakeButton("Real33D.Automap.ZoomIn",
							FText::FromString(TEXT("Zoom in (not available)")))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeButton("Real33D.Automap.FullMap",
							FText::FromString(TEXT("Open map (not available)")))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Bottom)
				.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
				[
					SNew(SBox)
					.WidthOverride(LayerWidth)
					.HeightOverride(LayerHeight)
					.ToolTipText(FText::FromString(TEXT("Floor selector (not available)")))
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SImage).Image(Style.GetBrush("Real33D.Automap.Layers"))
						]
						+ SOverlay::Slot()
						[
							SNew(SImage)
							.Image(Style.GetBrush("Real33D.Chrome.Dither"))
				.ColorAndOpacity(FReal33DUIStyle::DitherTint())
							.Visibility(EVisibility::HitTestInvisible)
						]
					]
				]
			]
		]
	];
}

void SReal33DMinimapPanel::SetPosition(const Real33D::FMapPosition& Position, bool bKnown)
{
	if (bHasDrawnOnce && bLastKnown == bKnown
		&& Last.X == Position.X && Last.Y == Position.Y && Last.Z == Position.Z)
	{
		return;
	}
	Last = Position;
	bLastKnown = bKnown;
	bHasDrawnOnce = true;

	if (PositionLabel.IsValid())
	{
		PositionLabel->SetText(bKnown
			? FText::FromString(FString::Printf(TEXT("%d, %d"), Position.X, Position.Y))
			: FText::FromString(TEXT("--, --")));
	}
	if (FloorLabel.IsValid())
	{
		FloorLabel->SetText(bKnown
			? FText::FromString(FString::Printf(TEXT("Floor %d"), Position.Z))
			: FText::FromString(TEXT("Floor --")));
	}
}
