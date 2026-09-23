#include "Real33DControlPanel.h"

#include "REAL33D.h"
#include "Real33DBridge.h"
#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"

namespace
{
	// mainoptionspanel.otui: a grid of 20x20 cells, 2 apart, inset 8.
	constexpr float Button = FReal33DUIStyle::ControlButtonSize;
	constexpr float Gap = 2.0f;
	constexpr float Inset = 8.0f;
	constexpr float TopInset = 8.0f;
}

TSharedRef<SWidget> SReal33DControlPanel::MakeButton(
	const FName& Brush, const FText& Tooltip, FName Panel, bool bLogout)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();
	const bool bWired = bLogout || !Panel.IsNone();

	TSharedRef<SOverlay> Face = SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(Style.GetBrush(Brush))
		];
	if (!bWired)
	{
		Face->AddSlot()
		[
			SNew(SImage)
			.Image(Style.GetBrush("Real33D.Chrome.Dither"))
				.ColorAndOpacity(FReal33DUIStyle::DitherTint())
			.Visibility(EVisibility::HitTestInvisible)
		];
	}

	return SNew(SBox)
		.WidthOverride(Button)
		.HeightOverride(Button)
		.ToolTipText(Tooltip)
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(0.0f))
			.IsEnabled(bWired)
			.OnClicked(bLogout
				? FOnClicked::CreateSP(this, &SReal33DControlPanel::HandleLogout)
				: (Panel.IsNone()
					? FOnClicked()
					: FOnClicked::CreateSP(this, &SReal33DControlPanel::HandleToggle, Panel)))
			[
				Face
			]
		];
}

void SReal33DControlPanel::Construct(const FArguments& InArgs)
{
	Bridge = InArgs._Bridge;
	OnPanelToggled = InArgs._OnPanelToggled;

	ChildSlot
	.Padding(FMargin(Inset, TopInset, Inset, 0.0f))
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.0f, 0.0f, Gap, 0.0f))
		[
			MakeButton("Real33D.Control.Skills",
				FText::FromString(TEXT("Skills")), TEXT("Skills"), false)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.0f, 0.0f, Gap, 0.0f))
		[
			MakeButton("Real33D.Control.Battle",
				FText::FromString(TEXT("Battle List")), TEXT("Battle"), false)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.0f, 0.0f, Gap, 0.0f))
		[
			MakeButton("Real33D.Control.Vip",
				FText::FromString(TEXT("VIP List (not available)")), NAME_None, false)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.0f, 0.0f, Gap, 0.0f))
		[
			MakeButton("Real33D.Control.Control",
				FText::FromString(TEXT("Manage control buttons (not available)")),
				NAME_None, false)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.0f, 0.0f, Gap, 0.0f))
		[
			MakeButton("Real33D.Control.Options",
				FText::FromString(TEXT("Options (not available)")), NAME_None, false)
		]

		+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right)
		[
			MakeButton("Real33D.Control.Logout",
				FText::FromString(TEXT("Exit")), NAME_None, true)
		]
	];
}

FReply SReal33DControlPanel::HandleToggle(FName Panel)
{
	OnPanelToggled.ExecuteIfBound(Panel);
	return FReply::Handled();
}

FReply SReal33DControlPanel::HandleLogout()
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || !Live->IsRunning())
	{
		UE_LOG(LogReal33D, Log, TEXT("Exit pressed while not connected; nothing to do"));
		return FReply::Handled();
	}
	// Disconnect already sends BuildLogoutCommand before closing the socket, so
	// this is a real logout and not a dropped connection. Fusion32 refuses one
	// during a fight -- that is the LogoutBlocked condition -- and this client
	// does not second-guess it: the request goes, the server decides.
	UE_LOG(LogReal33D, Log, TEXT("Exit pressed; logging out"));
	Live->Disconnect();
	return FReply::Handled();
}
