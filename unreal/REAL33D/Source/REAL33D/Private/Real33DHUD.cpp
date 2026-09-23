#include "Real33DHUD.h"

#include "Real33DActionBarPanel.h"
#include "Real33DBattlePanel.h"
#include "Real33DContainersPanel.h"
#include "Real33DControlPanel.h"
#include "Real33DCreatureActor.h"
#include "Real33DInventoryPanel.h"
#include "Real33DMinimapPanel.h"
#include "Real33DPanelChrome.h"
#include "Real33DSkillsPanel.h"
#include "Real33DUIStyle.h"
#include "Real33DVitalsPanel.h"
#include "Real33DWorldActor.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	constexpr float SideWidth = FReal33DUIStyle::SidePanelWidth;
	constexpr float ActionWidth = FReal33DUIStyle::ActionColumnWidth;
	// gameinterface.otui: the bottom splitter is 6 tall and the bottom panel
	// starts 200 up from the bottom edge.
	constexpr float SplitterHeight = 6.0f;
	// 150, not the 200 gameinterface.otui reserves. That file's bottom panel
	// holds a console with several channel tabs and a server log beside it;
	// this client has one transcript, and 200px of mostly empty panel was
	// taking a band of the 3D view for nothing.
	constexpr float BottomPanelHeight = 150.0f;
	// healthinfo.otui gives its panel a 32px body; the vitals widget draws the
	// two bars plus the level line, so it needs a little more than that here.
	constexpr float VitalsHeight = 52.0f;
	constexpr int32 TextSize = 8;
}

TSharedRef<SWidget> SReal33DHUD::MakeSideColumn(TSharedRef<SWidget> Contents)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	return SNew(SBox)
		.WidthOverride(SideWidth)
		[
			SNew(SBorder)
			.BorderImage(Style.GetBrush("Real33D.Chrome.SidePanel"))
			// Faded, so a column of panels does not wall off the scene behind
			// it. In the 2D the flanks cover static chrome; here they cover the
			// world the camera is looking at.
			.BorderBackgroundColor(FReal33DUIStyle::PanelTint())
			.Padding(FMargin(0.0f))
			[
				// The 2D's side panels scroll when their windows outgrow the
				// screen. So do these: a stack of mini windows is taller than a
				// short display, and the alternative is panels falling off the
				// bottom with no way to reach them.
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					Contents
				]
			]
		];
}

TSharedRef<SWidget> SReal33DHUD::MakeViewportFrame()
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	// GameMapPanel: a 4px 9-sliced border with the scene inside it. The middle
	// must stay clear -- this is a frame around the 3D view, not a surface over
	// it -- so nothing is placed in the border's slot and the whole widget is
	// transparent to hit testing.
	return SNew(SBorder)
		.BorderImage(Style.GetBrush("Real33D.Chrome.MapPanel"))
		.Padding(FMargin(4.0f))
		.Visibility(EVisibility::HitTestInvisible)
		[
			SNullWidget::NullWidget
		];
}

TSharedRef<SWidget> SReal33DHUD::MakeBottomPanel()
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	return SNew(SVerticalBox)

		// The splitter the 2D drags to resize the console.
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.HeightOverride(SplitterHeight)
			[
				SNew(SImage).Image(Style.GetBrush("Real33D.ActionBar.Splitter"))
			]
		]

		// The bottom action bar, which gameinterface.otui puts between the
		// splitter and the console.
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SReal33DActionBar).SlotCount(12).Vertical(false)
		]

		// The console itself, on the repeating dark background the 2D uses.
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.HeightOverride(BottomPanelHeight)
			[
				SNew(SBorder)
				.BorderImage(Style.GetBrush("Real33D.Chrome.BackgroundDark"))
				.BorderBackgroundColor(FReal33DUIStyle::PanelTint())
				.Padding(FMargin(0.0f))
				[
					SAssignNew(ChatPanel, SReal33DChatPanel)
					.Embedded(true)
				]
			]
		];
}

void SReal33DHUD::Construct(const FArguments& InArgs)
{
	Bridge = InArgs._Bridge;
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	// ----------------------------------------------------- the right column
	//
	// The classic arrangement: the control buttons, then the minimap, the
	// health and mana bars, and the inventory, top to bottom down the outer
	// column, exactly as a 7.x client stacks them.

	TSharedRef<SVerticalBox> Outer = SNew(SVerticalBox);

	Outer->AddSlot()
		.AutoHeight()
		[
			SNew(SBox)
			.HeightOverride(SReal33DControlPanel::PanelHeight)
			[
				SAssignNew(Controls, SReal33DControlPanel)
				.Bridge(Bridge.Get())
				.OnPanelToggled(FReal33DOnPanelToggled::CreateSP(
					this, &SReal33DHUD::HandlePanelToggled))
			]
		];

	Outer->AddSlot()
		.AutoHeight()
		[
			SNew(SReal33DMiniWindow)
			.Title(FText::FromString(TEXT("Minimap")))
			.ContentHeight(SReal33DMinimapPanel::PanelHeight)
			[
				SAssignNew(Minimap, SReal33DMinimapPanel)
			]
		];

	Outer->AddSlot()
		.AutoHeight()
		[
			SNew(SReal33DMiniWindow)
			.Title(FText::FromString(TEXT("Health")))
			.ContentHeight(VitalsHeight)
			[
				SNew(SBox)
				.Padding(FMargin(8.0f, 4.0f, 8.0f, 0.0f))
				[
					SAssignNew(Vitals, SReal33DVitalsPanel)
				]
			]
		];

	Outer->AddSlot()
		.AutoHeight()
		[
			SNew(SReal33DMiniWindow)
			.Title(FText::FromString(TEXT("Inventory")))
			.ContentHeight(SReal33DInventoryPanel::PanelHeight)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SAssignNew(Inventory, SReal33DInventoryPanel)
				]
				// The condition strip sits along the bottom of the inventory
				// panel in inventory.otui, and does here too.
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(8.0f, 2.0f, 8.0f, 4.0f))
				[
					SAssignNew(Conditions, SReal33DConditionStrip)
				]
			]
		];

	// ------------------------------------------------- the inner right column

	TSharedRef<SVerticalBox> Inner = SNew(SVerticalBox);

	Inner->AddSlot()
		.AutoHeight()
		[
			SAssignNew(SkillsWindow, SReal33DMiniWindow)
			.Title(FText::FromString(TEXT("Skills")))
			.IconBrush("Real33D.Control.SkillsIcon")
			.ContentHeight(SReal33DSkillsPanel::DesiredContentHeight())
			[
				SAssignNew(Skills, SReal33DSkillsPanel)
			]
		];

	Inner->AddSlot()
		.AutoHeight()
		[
			SAssignNew(BattleWindow, SReal33DMiniWindow)
			.Title(FText::FromString(TEXT("Battle List")))
			.IconBrush("Real33D.Battle.Icon")
			.ContentHeight(140.0f)
			[
				SAssignNew(Battle, SReal33DBattlePanel)
			]
		];

	Inner->AddSlot()
		.AutoHeight()
		[
			SNew(SReal33DMiniWindow)
			.Title(FText::FromString(TEXT("Container")))
			.ContentHeight(SReal33DContainersPanel::ContentHeightFor(2))
			.Inert(true)
			[
				SNew(SReal33DContainersPanel).Columns(4).Rows(2)
			]
		];

	// ------------------------------------------------------------- assembly
	//
	// No left dock. gameinterface.otui reserves a column there, but in this
	// client it would be empty: the only thing that had been put in it was a
	// docked object-use panel, and that was the wrong shape for REAL33D 3D --
	// those controls belong to the 2D's hotkey configuration dialog, and use
	// here is meant to be mouse-driven with a temporary crosshair mode. An
	// empty 176px strip would cost the player that much of the view for
	// nothing, so the column is not built at all. The left action bar stays.

	ChildSlot
	[
		SNew(SHorizontalBox)

		// The left action column, gameinterface.otui's gameLeftActionPanel.
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(ActionWidth)
			[
				SNew(SReal33DActionBar).SlotCount(8).Vertical(true)
			]
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				MakeViewportFrame()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeBottomPanel()
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(ActionWidth)
			[
				SNew(SReal33DActionBar).SlotCount(8).Vertical(true)
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			MakeSideColumn(Inner)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			MakeSideColumn(Outer)
		]
	];

	(void)Style;
}

void SReal33DHUD::HandlePanelToggled(FName Panel)
{
	// Purely local: showing and hiding a panel asks Fusion32 for nothing.
	const TSharedPtr<SWidget> Target = Panel == TEXT("Skills")
		? SkillsWindow : (Panel == TEXT("Battle") ? BattleWindow : nullptr);
	if (!Target.IsValid())
	{
		return;
	}
	Target->SetVisibility(Target->GetVisibility() == EVisibility::Collapsed
		? EVisibility::Visible : EVisibility::Collapsed);
}

void SReal33DHUD::Refresh(const AReal33DWorld* World)
{
	// Everything below is a server-owned value or an explicit "not known yet".
	// A null world is the second of those, not a reason to keep the last frame:
	// the panels must empty out when the session ends.
	const FReal33DPlayerVitals VitalsNow = World != nullptr
		? World->GetPlayerVitals() : FReal33DPlayerVitals{};
	const FReal33DPlayerSkills SkillsNow = World != nullptr
		? World->GetPlayerSkills() : FReal33DPlayerSkills{};
	const FReal33DConditions ConditionsNow = World != nullptr
		? World->GetConditions() : FReal33DConditions{};

	if (Vitals.IsValid())
	{
		Vitals->SetVitals(VitalsNow);
	}
	if (Skills.IsValid())
	{
		Skills->SetValues(VitalsNow, SkillsNow);
	}
	if (Inventory.IsValid())
	{
		Inventory->SetVitals(VitalsNow);
	}
	if (Conditions.IsValid())
	{
		Conditions->SetConditions(ConditionsNow);
	}
	if (Minimap.IsValid())
	{
		const AReal33DCreature* Self = World != nullptr ? World->GetLocalPlayer() : nullptr;
		Minimap->SetPosition(
			Self != nullptr ? Self->GetLogicalPosition() : Real33D::FMapPosition{},
			Self != nullptr);
	}
	if (Battle.IsValid())
	{
		if (World != nullptr)
		{
			World->GetBattleList(BattleScratch);
		}
		else
		{
			BattleScratch.Reset();
		}
		Battle->SetEntries(BattleScratch);
	}
}
