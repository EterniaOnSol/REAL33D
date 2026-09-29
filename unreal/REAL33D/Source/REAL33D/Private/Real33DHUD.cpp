#include "Real33DHUD.h"

#include "REAL33D.h"
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
#include "Real33DWorldOverlay.h"
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
	// Room for a backpack's twenty squares and a bag opened inside it, which is
	// the ordinary case as soon as containers draw their whole capacity. At 200
	// the backpack alone filled the panel and the nested window sat below the
	// fold. The column still scrolls when more than that is open.
	constexpr float ContainersHeight = 420.0f;
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
	// remains transparent, with read-only projected labels and interaction cues.
	// The overlay does not consume gameplay or panel input.
	return SNew(SBorder)
		.BorderImage(Style.GetBrush("Real33D.Chrome.MapPanel"))
		.Padding(FMargin(4.0f))
		.Visibility(EVisibility::HitTestInvisible)
		[
			SAssignNew(WorldOverlay, SReal33DWorldOverlay)
		];
}

TSharedRef<SWidget> SReal33DHUD::MakeBottomPanel(const FReal33DOnTypingChanged& OnTypingChanged)
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
						.Bridge(Bridge)
						.OnTypingChanged(OnTypingChanged)
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
				.Bridge(Bridge)
				.OnDestination(InArgs._OnMinimapDestination)
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
					.OnItemDropped(FReal33DOnItemDropped::CreateSP(
						this, &SReal33DHUD::HandleItemDropped))
					.OnSlotUsed(FReal33DOnSlotUsed::CreateSP(
						this, &SReal33DHUD::HandleSlotUsed))
					.OnSlotPicked(FReal33DOnSlotPicked::CreateSP(
						this, &SReal33DHUD::HandleSlotPicked))
					.OnAttackModeChanged(FReal33DOnAttackModeChanged::CreateSP(
						this, &SReal33DHUD::HandleAttackModeChanged))
					.OnChaseModeChanged(FReal33DOnChaseModeChanged::CreateSP(
						this, &SReal33DHUD::HandleChaseModeChanged))
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
				.OnCreatureTargeted(FReal33DOnCreatureTargeted::CreateSP(
					this, &SReal33DHUD::HandleCreatureTargeted))
			]
		];

	Inner->AddSlot()
		.AutoHeight()
		[
			SNew(SReal33DMiniWindow)
			.Title(FText::FromString(TEXT("Containers")))
			.ContentHeight(ContainersHeight)
			[
				SAssignNew(Containers, SReal33DContainersPanel)
					.Columns(4)
					.OnItemDropped(FReal33DOnItemDropped::CreateSP(
						this, &SReal33DHUD::HandleItemDropped))
					.OnSlotUsed(FReal33DOnSlotUsed::CreateSP(
						this, &SReal33DHUD::HandleSlotUsed))
					.OnSlotPicked(FReal33DOnSlotPicked::CreateSP(
						this, &SReal33DHUD::HandleSlotPicked))
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

	// The crosshair banner. Collapsed until a use-with is waiting for a target,
	// and hit-test invisible so it never eats the click that would aim it.
	TSharedRef<SWidget> Banner = SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.0f, 8.0f, 0.0f, 0.0f))
		[
			SNew(SBorder)
			.BorderImage(Style.GetBrush("Real33D.Chrome.ContainerSlot"))
			.Padding(FMargin(10.0f, 4.0f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(
					TEXT("Choose a target  -  Escape to cancel")))
				.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Readout"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
			]
		];
	TargetingBanner = Banner;
	Banner->SetVisibility(EVisibility::Collapsed);

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
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
				MakeBottomPanel(InArgs._OnTypingChanged)
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
		]
		+ SOverlay::Slot()
		[
			Banner
		]
	];
}

void SReal33DHUD::HandleItemDropped(FReal33DSlotRef From, FReal33DSlotRef To)
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || !Live->IsRunning())
	{
		UE_LOG(LogReal33D, Warning,
			TEXT("not connected; move of object %u not sent"), From.TypeId);
		return;
	}

	const auto ToSlot = [](const FReal33DSlotRef& Ref)
	{
		return Ref.Kind == FReal33DSlotRef::EKind::Container
			? UReal33DBridge::FMoveSlot::InContainer(Ref.Container, Ref.Slot)
			: UReal33DBridge::FMoveSlot::InInventory(Ref.Slot);
	};

	// An intent, exactly like a walk or a say. Nothing on screen changes here:
	// the object stays drawn where the server last said it was, and moves only
	// when the container or inventory commands that follow say it did. A move
	// Fusion32 refuses produces none of those, which is correct.
	const uint32 MoveId = Live->RequestMoveObject(
		ToSlot(From), From.TypeId, From.Slot, ToSlot(To), From.Count);

	UE_LOG(LogReal33D, Log,
		TEXT("move %u: object %u from %s%u slot %u to %s%u slot %u, count %u"),
		MoveId, From.TypeId,
		From.Kind == FReal33DSlotRef::EKind::Container ? TEXT("container ") : TEXT("body "),
		From.Container, From.Slot,
		To.Kind == FReal33DSlotRef::EKind::Container ? TEXT("container ") : TEXT("body "),
		To.Container, To.Slot, From.Count);
}

uint8 SReal33DHUD::FirstFreeContainerNumber() const
{
	// CONTAINER_FIRST..CONTAINER_LAST is sixteen slots wide.
	for (uint8 Number = 0; Number < 16; ++Number)
	{
		if (!OpenContainerNumbers.Contains(Number))
		{
			return Number;
		}
	}
	// All sixteen taken. Fusion32 would refuse a number past its table, so the
	// last legal one is sent and the server decides what to do about it.
	return 15;
}

void SReal33DHUD::HandleSlotUsed(FReal33DSlotRef Slot, EReal33DSlotAction Action)
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || !Live->IsRunning())
	{
		return;
	}

	if (Action == EReal33DSlotAction::Look)
	{
		const auto Point = Slot.Kind == FReal33DSlotRef::EKind::Container
			? UReal33DBridge::FMoveSlot::InContainer(Slot.Container, Slot.Slot)
			: UReal33DBridge::FMoveSlot::InInventory(Slot.Slot);
		const uint32 LookId = Live->RequestLookAtPoint(Point);
		if (LookId != 0 && WorldOverlay.IsValid()) WorldOverlay->ShowRequest(TEXT("Look requested"));
		UE_LOG(LogReal33D, Log, TEXT("look %u at %s%u slot %u"),
			LookId, Slot.Kind == FReal33DSlotRef::EKind::Container
				? TEXT("container ") : TEXT("body "), Slot.Container, Slot.Slot);
		return;
	}

	if (Action == EReal33DSlotAction::UseWith || Live->RequiresUseTarget(Slot.TypeId))
	{
		// Nothing goes out yet. The command needs two ends and only one is
		// known, so the client waits for the click that names the other.
		BeginUseTarget(Slot.Kind == FReal33DSlotRef::EKind::Container
			? UReal33DBridge::FMoveSlot::InContainer(Slot.Container, Slot.Slot)
			: UReal33DBridge::FMoveSlot::InInventory(Slot.Slot), Slot.TypeId, Slot.Slot);
		return;
	}

	const auto ToSlot = [](const FReal33DSlotRef& Ref)
	{
		return Ref.Kind == FReal33DSlotRef::EKind::Container
			? UReal33DBridge::FMoveSlot::InContainer(Ref.Container, Ref.Slot)
			: UReal33DBridge::FMoveSlot::InInventory(Ref.Slot);
	};

	const uint8 OpenAs = FirstFreeContainerNumber();
	const uint32 UseId = Live->RequestUseObject(
		ToSlot(Slot), Slot.TypeId, Slot.Slot, OpenAs);
	if (UseId != 0 && WorldOverlay.IsValid()) WorldOverlay->ShowRequest(TEXT("Use requested"));
	UE_LOG(LogReal33D, Log,
		TEXT("use %u: object %u at %s%u slot %u, would open as container %u"),
		UseId, Slot.TypeId,
		Slot.Kind == FReal33DSlotRef::EKind::Container ? TEXT("container ") : TEXT("body "),
		Slot.Container, Slot.Slot, OpenAs);
}

bool SReal33DHUD::HandleSlotPicked(FReal33DSlotRef Slot)
{
	if (!Pending.bActive)
	{
		return false;
	}
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || !Live->IsRunning())
	{
		CancelTargeting();
		return true;
	}

	const auto ToSlot = [](const FReal33DSlotRef& Ref)
	{
		return Ref.Kind == FReal33DSlotRef::EKind::Container
			? UReal33DBridge::FMoveSlot::InContainer(Ref.Container, Ref.Slot)
			: UReal33DBridge::FMoveSlot::InInventory(Ref.Slot);
	};

	const uint32 UseId = Live->RequestUseWithObject(
		Pending.Source, Pending.TypeId, Pending.StackIndex,
		ToSlot(Slot), Slot.TypeId, Slot.Slot);
	if (UseId != 0 && WorldOverlay.IsValid()) WorldOverlay->ShowRequest(TEXT("Use With requested"));
	UE_LOG(LogReal33D, Log, TEXT("use-with %u: object %u on object %u in a slot"),
		UseId, Pending.TypeId, Slot.TypeId);

	CancelTargeting();
	return true;
}

void SReal33DHUD::HandleCreatureTargeted(uint32 CreatureId, bool bFollow)
{
	if (CompleteUseOnCreature(CreatureId))
	{
		return;
	}
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || !Live->IsRunning())
	{
		return;
	}
	const uint32 ActionId = bFollow
		? Live->RequestFollow(CreatureId) : Live->RequestAttack(CreatureId);
	UE_LOG(LogReal33D, Log, TEXT("combat input %u from battle list: %s creature %u"),
		ActionId, bFollow ? TEXT("follow") : TEXT("attack"), CreatureId);
}

void SReal33DHUD::HandleAttackModeChanged(EReal33DAttackMode Mode)
{
	if (UReal33DBridge* Live = Bridge.Get(); Live != nullptr && Live->IsRunning())
	{
		const uint32 ActionId = Live->RequestAttackMode(Mode);
		UE_LOG(LogReal33D, Log, TEXT("combat input %u: attack mode %d"),
			ActionId, static_cast<int32>(Mode));
	}
}

void SReal33DHUD::HandleChaseModeChanged(EReal33DChaseMode Mode)
{
	if (UReal33DBridge* Live = Bridge.Get(); Live != nullptr && Live->IsRunning())
	{
		const uint32 ActionId = Live->RequestChaseMode(Mode);
		UE_LOG(LogReal33D, Log, TEXT("combat input %u: chase mode %d"),
			ActionId, static_cast<int32>(Mode));
	}
}

bool SReal33DHUD::CompleteUseOnCreature(uint32 CreatureId)
{
	if (!Pending.bActive)
	{
		return false;
	}
	UReal33DBridge* Live = Bridge.Get();
	if (Live != nullptr && Live->IsRunning())
	{
		const uint32 UseId = Live->RequestUseOnCreature(
			Pending.Source, Pending.TypeId, Pending.StackIndex, CreatureId);
		if (UseId != 0 && WorldOverlay.IsValid()) WorldOverlay->ShowRequest(TEXT("Use With requested"));
		UE_LOG(LogReal33D, Log, TEXT("use-with %u: object %u on creature %u"),
			UseId, Pending.TypeId, CreatureId);
	}
	CancelTargeting();
	return true;
}

bool SReal33DHUD::CompleteUseOnField(const Real33D::FMapPosition& Position,
	uint16 TypeId, uint8 StackIndex)
{
	if (!Pending.bActive)
	{
		return false;
	}
	UReal33DBridge* Live = Bridge.Get();
	if (Live != nullptr && Live->IsRunning())
	{
		const uint32 UseId = Live->RequestUseWithObject(
			Pending.Source, Pending.TypeId, Pending.StackIndex,
			UReal33DBridge::FMoveSlot::OnMap(Position), TypeId, StackIndex);
		if (UseId != 0 && WorldOverlay.IsValid()) WorldOverlay->ShowRequest(TEXT("Use With requested"));
		UE_LOG(LogReal33D, Log,
			TEXT("use-with %u: object %u on object %u at %d,%d,%d stack %u"),
			UseId, Pending.TypeId, TypeId,
			Position.X, Position.Y, Position.Z, StackIndex);
	}
	CancelTargeting();
	return true;
}

bool SReal33DHUD::UseWorldObject(const Real33D::FMapPosition& Position,
	uint16 TypeId, uint8 StackIndex)
{
	if (CompleteUseOnField(Position, TypeId, StackIndex))
	{
		return true;
	}
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || !Live->IsRunning())
	{
		return false;
	}
	if (Live->RequiresUseTarget(TypeId))
	{
		BeginUseTarget(UReal33DBridge::FMoveSlot::OnMap(Position), TypeId, StackIndex);
		return true;
	}
	const uint8 OpenAs = FirstFreeContainerNumber();
	const uint32 UseId = Live->RequestUseObject(
		UReal33DBridge::FMoveSlot::OnMap(Position), TypeId, StackIndex, OpenAs);
	if (UseId != 0 && WorldOverlay.IsValid()) WorldOverlay->ShowRequest(TEXT("Use requested"));

	UE_LOG(LogReal33D, Log,
		TEXT("use %u from world: object %u at %d,%d,%d stack %u, would open as container %u"),
		UseId, TypeId, Position.X, Position.Y, Position.Z, StackIndex, OpenAs);
	return true;
}

bool SReal33DHUD::LookAtWorldPoint(const Real33D::FMapPosition& Position)
{
	UReal33DBridge* Live = Bridge.Get();
	if (Live == nullptr || !Live->IsRunning()) return false;
	const uint32 LookId = Live->RequestLookAtPoint(UReal33DBridge::FMoveSlot::OnMap(Position));
	if (LookId != 0 && WorldOverlay.IsValid()) WorldOverlay->ShowRequest(TEXT("Look requested"));

	UE_LOG(LogReal33D, Log, TEXT("look %u at world point %d,%d,%d"),
		LookId, Position.X, Position.Y, Position.Z);
	return LookId != 0;
}

void SReal33DHUD::BeginUseTarget(const UReal33DBridge::FMoveSlot& Source,
	uint16 TypeId, uint8 StackIndex)
{
	Pending.bActive = true;
	Pending.Source = Source;
	Pending.TypeId = TypeId;
	Pending.StackIndex = StackIndex;
	UpdateTargetingBanner();
	UE_LOG(LogReal33D, Log, TEXT("use-with begun with object %u; waiting for a target"), TypeId);
}

void SReal33DHUD::CancelTargeting()
{
	if (!Pending.bActive)
	{
		return;
	}
	Pending = FPendingUse{};
	UpdateTargetingBanner();
}

void SReal33DHUD::UpdateTargetingBanner()
{
	// Slots are Slate widgets; their inherited cursor must match the world view.
	SetCursor(Pending.bActive ? TOptional<EMouseCursor::Type>(EMouseCursor::Crosshairs)
		: TOptional<EMouseCursor::Type>());
	if (TargetingBanner.IsValid())
	{
		TargetingBanner->SetVisibility(Pending.bActive
			? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	}
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
	if (WorldOverlay.IsValid()) WorldOverlay->Refresh(World, Pending.bActive);
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
		Inventory->SetInventory(World != nullptr
			? World->GetInventory() : FReal33DInventory{});
		Inventory->SetCombat(World != nullptr
			? World->GetCombat() : FReal33DCombat{});
	}
	if (Containers.IsValid())
	{
		static const TArray<FReal33DContainer> None;
		const TArray<FReal33DContainer>& Open =
			World != nullptr ? World->GetContainers() : None;
		Containers->SetContainers(Open);

		// Which numbers the server has in use, so the next container the player
		// opens gets a free one rather than replacing an open window.
		OpenContainerNumbers.Reset(Open.Num());
		for (const FReal33DContainer& Each : Open)
		{
			OpenContainerNumbers.Add(Each.Number);
		}
	}

	// A pending use-with cannot outlive the session that began it.
	if (Pending.bActive && World == nullptr)
	{
		CancelTargeting();
	}
	if (Conditions.IsValid())
	{
		Conditions->SetConditions(ConditionsNow);
	}
	if (Minimap.IsValid())
	{
		Minimap->Refresh();
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
