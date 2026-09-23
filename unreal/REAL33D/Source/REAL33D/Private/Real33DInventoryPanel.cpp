#include "Real33DInventoryPanel.h"

#include "Real33DPanelChrome.h"
#include "Real33DUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// inventory.otui: slots sit 3 apart, the panel is inset 8 from the left,
	// and the header row of buttons is 5 down from the top.
	constexpr float SlotGap = 3.0f;
	/** The 12px header button plus its 3px gap, per inventory.otui. */
	constexpr float ColumnStagger = 15.0f;
	constexpr float PanelInset = 8.0f;
	constexpr float TopInset = 5.0f;
	constexpr float ButtonSize = 20.0f;
	constexpr float ReadoutWidth = FReal33DUIStyle::SlotSize;
	constexpr int32 TextSize = 8;
	constexpr int32 ValueSize = 9;

	// reference/game/src/enums.hh, enum InventorySlot.
	constexpr int32 kSlotHead = 1;
	constexpr int32 kSlotNeck = 2;
	constexpr int32 kSlotBack = 3;
	constexpr int32 kSlotTorso = 4;
	constexpr int32 kSlotShield = 5;
	constexpr int32 kSlotWeapon = 6;
	constexpr int32 kSlotLegs = 7;
	constexpr int32 kSlotFeet = 8;
	constexpr int32 kSlotFinger = 9;
	constexpr int32 kSlotHip = 10;

	/** Each square's placeholder art and label, by slot number. */
	struct FSlotFace { const TCHAR* Brush; const TCHAR* Label; };
	const FSlotFace kSlotFaces[FReal33DInventory::SlotCount] = {
		{ TEXT(""), TEXT("") },  // 0 is not a slot
		{ TEXT("Real33D.Inventory.Slot.head"), TEXT("Helmet") },
		{ TEXT("Real33D.Inventory.Slot.neck"), TEXT("Amulet") },
		{ TEXT("Real33D.Inventory.Slot.back"), TEXT("Backpack") },
		{ TEXT("Real33D.Inventory.Slot.torso"), TEXT("Armor") },
		{ TEXT("Real33D.Inventory.Slot.left_hand"), TEXT("Shield") },
		{ TEXT("Real33D.Inventory.Slot.right_hand"), TEXT("Weapon") },
		{ TEXT("Real33D.Inventory.Slot.legs"), TEXT("Legs") },
		{ TEXT("Real33D.Inventory.Slot.feet"), TEXT("Boots") },
		{ TEXT("Real33D.Inventory.Slot.finger"), TEXT("Ring") },
		{ TEXT("Real33D.Inventory.Slot.hip"), TEXT("Tools") },
	};
}

TSharedRef<SWidget> SReal33DInventoryPanel::MakeSlot(
	int32 Slot, const FName& Placeholder, const FText& Tooltip)
{
	// Hosted in a box rather than placed directly, so SetInventory can replace
	// the square's contents without rebuilding the whole panel around it.
	TSharedRef<SBox> Host = SNew(SBox)
		[
			SNew(SReal33DSlot)
			.SlotBrush("Real33D.Inventory.Slot")
			.PlaceholderBrush(Placeholder)
			.Tooltip(Tooltip)
		];
	SlotHosts[Slot] = Host;
	return Host;
}

void SReal33DInventoryPanel::FillSlot(int32 Slot, const FName& Placeholder,
	const FText& Tooltip, const FReal33DInventory& Inventory)
{
	if (!SlotHosts[Slot].IsValid())
	{
		return;
	}
	const bool bOccupied = Inventory.bKnown && Inventory.bOccupied[Slot];
	const FReal33DItem& Item = Inventory.Items[Slot];

	FReal33DSlotRef Where;
	Where.Kind = FReal33DSlotRef::EKind::Inventory;
	Where.Slot = static_cast<uint8>(Slot);

	SlotHosts[Slot]->SetContent(
		SNew(SReal33DSlot)
		.SlotBrush("Real33D.Inventory.Slot")
		.PlaceholderBrush(Placeholder)
		.TypeId(bOccupied ? Item.TypeId : 0)
		.Count(bOccupied && Item.bHasAmount ? Item.Amount : 0)
		.Location(Where)
		.OnItemDropped(OnItemDropped)
		.OnSlotUsed(OnSlotUsed)
		.OnSlotPicked(OnSlotPicked)
		.Tooltip(bOccupied
			? FText::FromString(Item.bHasAmount
				? FString::Printf(TEXT("%s: object %u x%u"),
					*Tooltip.ToString(), Item.TypeId, Item.Amount)
				: FString::Printf(TEXT("%s: object %u"),
					*Tooltip.ToString(), Item.TypeId))
			: Tooltip));
}

TSharedRef<SWidget> SReal33DInventoryPanel::MakeReadout(
	const FText& Caption, TSharedPtr<STextBlock>& OutValue)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	return SNew(SBox)
		.WidthOverride(ReadoutWidth)
		.HeightOverride(FReal33DUIStyle::SlotSize)
		[
			SNew(SBorder)
			.BorderImage(Style.GetBrush("Real33D.Chrome.ContainerSlot"))
			.Padding(FMargin(1.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(Caption)
					.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Readout"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(OutValue, STextBlock)
					.Text(FText::FromString(TEXT("--")))
					.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Readout"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", ValueSize))
				]
			]
		];
}

TSharedRef<SWidget> SReal33DInventoryPanel::MakeCombatButton(
	int32 Index, const FName& IdleBrush, const FName& ActiveBrush,
	const FText& Tooltip, const FOnClicked& OnClicked)
{
	return SNew(SBox)
		.WidthOverride(ButtonSize)
		.HeightOverride(ButtonSize)
		.ToolTipText(Tooltip)
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.OnClicked(OnClicked)
			[
				SNew(SBox)
				.WidthOverride(ButtonSize)
				.HeightOverride(ButtonSize)
				[
					SNew(SImage)
					.Image_Lambda([this, Index, IdleBrush, ActiveBrush]()
					{
						return FReal33DUIStyle::Get().GetBrush(
							IsCombatButtonActive(Index) ? ActiveBrush : IdleBrush);
					})
				]
			]
		];
}

bool SReal33DInventoryPanel::IsCombatButtonActive(int32 Index) const
{
	// Fusion32 never echoes tactics. Until this client has sent them, drawing
	// an engaged button would claim state the wire has not established.
	if (!LastCombat.bTacticsSent)
	{
		return false;
	}
	switch (Index)
	{
	case 0: return LastCombat.AttackMode == EReal33DAttackMode::Offensive;
	case 1: return LastCombat.AttackMode == EReal33DAttackMode::Balanced;
	case 2: return LastCombat.AttackMode == EReal33DAttackMode::Defensive;
	case 3: return LastCombat.ChaseMode == EReal33DChaseMode::Stand;
	case 4: return LastCombat.ChaseMode == EReal33DChaseMode::Follow;
	default: return false;
	}
}

void SReal33DInventoryPanel::Construct(const FArguments& InArgs)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();
	OnItemDropped = InArgs._OnItemDropped;
	OnSlotUsed = InArgs._OnSlotUsed;
	OnSlotPicked = InArgs._OnSlotPicked;
	OnAttackModeChanged = InArgs._OnAttackModeChanged;
	OnChaseModeChanged = InArgs._OnChaseModeChanged;

	// The three slot columns of inventory.otui, in its own order. Column one is
	// amulet, sword, ring and then Soul; column two helmet, armor, legs, boots;
	// column three backpack, shield, tools and then Cap.
	TSharedRef<SVerticalBox> Left = SNew(SVerticalBox);
	TSharedRef<SVerticalBox> Middle = SNew(SVerticalBox);
	TSharedRef<SVerticalBox> Right = SNew(SVerticalBox);

	const auto Stack = [](TSharedRef<SVerticalBox> Column, TSharedRef<SWidget> Child)
	{
		Column->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, SlotGap))
			[
				Child
			];
	};

	// The slot numbers are Fusion32's own InventorySlot values, which is what
	// inventory.otui also carries in each square's `slotPosition.y` and what
	// SV_CMD_SET_INVENTORY addresses: head 1, neck 2, back 3, torso 4,
	// shield 5, weapon 6, legs 7, feet 8, finger 9, hip 10.
	Stack(Left, MakeSlot(kSlotNeck, "Real33D.Inventory.Slot.neck",
		FText::FromString(TEXT("Amulet"))));
	Stack(Left, MakeSlot(kSlotWeapon, "Real33D.Inventory.Slot.right_hand",
		FText::FromString(TEXT("Weapon"))));
	Stack(Left, MakeSlot(kSlotFinger, "Real33D.Inventory.Slot.finger",
		FText::FromString(TEXT("Ring"))));
	Stack(Left, MakeReadout(FText::FromString(TEXT("Soul")), SoulValue));

	Stack(Middle, MakeSlot(kSlotHead, "Real33D.Inventory.Slot.head",
		FText::FromString(TEXT("Helmet"))));
	Stack(Middle, MakeSlot(kSlotTorso, "Real33D.Inventory.Slot.torso",
		FText::FromString(TEXT("Armor"))));
	Stack(Middle, MakeSlot(kSlotLegs, "Real33D.Inventory.Slot.legs",
		FText::FromString(TEXT("Legs"))));
	Stack(Middle, MakeSlot(kSlotFeet, "Real33D.Inventory.Slot.feet",
		FText::FromString(TEXT("Boots"))));

	Stack(Right, MakeSlot(kSlotBack, "Real33D.Inventory.Slot.back",
		FText::FromString(TEXT("Backpack"))));
	Stack(Right, MakeSlot(kSlotShield, "Real33D.Inventory.Slot.left_hand",
		FText::FromString(TEXT("Shield"))));
	Stack(Right, MakeSlot(kSlotHip, "Real33D.Inventory.Slot.hip",
		FText::FromString(TEXT("Tools"))));
	Stack(Right, MakeReadout(FText::FromString(TEXT("Cap")), CapacityValue));

	// The combat column the 2D puts down the right-hand edge.
	TSharedRef<SVerticalBox> Combat = SNew(SVerticalBox);
	const auto Toggle = [&](int32 Index, const FName& Brush, const FName& ActiveBrush,
		const TCHAR* Label, const FOnClicked& OnClicked)
	{
		Combat->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 4.0f))
			[
				MakeCombatButton(Index, Brush, ActiveBrush,
					FText::FromString(Label), OnClicked)
			];
	};
	Toggle(0, "Real33D.Inventory.Attack", "Real33D.Inventory.AttackOn", TEXT("Offensive"),
		FOnClicked::CreateLambda([this]()
		{
			OnAttackModeChanged.ExecuteIfBound(EReal33DAttackMode::Offensive);
			return FReply::Handled();
		}));
	Toggle(1, "Real33D.Inventory.Balanced", "Real33D.Inventory.BalancedOn", TEXT("Balanced"),
		FOnClicked::CreateLambda([this]()
		{
			OnAttackModeChanged.ExecuteIfBound(EReal33DAttackMode::Balanced);
			return FReply::Handled();
		}));
	Toggle(2, "Real33D.Inventory.Defend", "Real33D.Inventory.DefendOn", TEXT("Defensive"),
		FOnClicked::CreateLambda([this]()
		{
			OnAttackModeChanged.ExecuteIfBound(EReal33DAttackMode::Defensive);
			return FReply::Handled();
		}));
	Toggle(3, "Real33D.Inventory.Stand", "Real33D.Inventory.StandOn", TEXT("Stand while fighting"),
		FOnClicked::CreateLambda([this]()
		{
			OnChaseModeChanged.ExecuteIfBound(EReal33DChaseMode::Stand);
			return FReply::Handled();
		}));
	Toggle(4, "Real33D.Inventory.Follow", "Real33D.Inventory.FollowOn", TEXT("Chase opponent"),
		FOnClicked::CreateLambda([this]()
		{
			OnChaseModeChanged.ExecuteIfBound(EReal33DChaseMode::Follow);
			return FReply::Handled();
		}));

	ChildSlot
	.Padding(FMargin(PanelInset, TopInset, PanelInset, 0.0f))
	[
		SNew(SHorizontalBox)
		// The outer columns start 15px lower than the middle one. That is the
		// classic cross: inventory.otui anchors the helmet to `changeSize.top`
		// but the amulet and the backpack to `changeSize.bottom` plus 3, and
		// the 12px button between them is where the offset comes from. Lining
		// all three columns up flat loses the shape the layout is known by.
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(0.0f, ColumnStagger, SlotGap, 0.0f))
		[
			Left
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(0.0f, 0.0f, SlotGap, 0.0f))
		[
			Middle
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(0.0f, ColumnStagger, 0.0f, 0.0f))
		[
			Right
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Right)
		[
			Combat
		]
	];

	(void)Style;
}

void SReal33DInventoryPanel::SetCombat(const FReal33DCombat& Combat)
{
	LastCombat = Combat;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SReal33DInventoryPanel::SetVitals(const FReal33DPlayerVitals& Vitals)
{
	if (bHasDrawnOnce
		&& Last.bKnown == Vitals.bKnown
		&& Last.SoulPoints == Vitals.SoulPoints
		&& Last.Capacity == Vitals.Capacity)
	{
		return;
	}
	Last = Vitals;
	bHasDrawnOnce = true;

	const FText Unknown = FText::FromString(TEXT("--"));
	if (SoulValue.IsValid())
	{
		SoulValue->SetText(Vitals.bKnown
			? FText::FromString(FString::Printf(TEXT("%d"),
				static_cast<int32>(Vitals.SoulPoints)))
			: Unknown);
	}
	if (CapacityValue.IsValid())
	{
		CapacityValue->SetText(Vitals.bKnown
			? FText::FromString(FString::Printf(TEXT("%d"),
				static_cast<int32>(Vitals.Capacity)))
			: Unknown);
	}
}

void SReal33DInventoryPanel::SetInventory(const FReal33DInventory& Inventory)
{
	if (bHasDrawnInventory && LastInventory.bKnown == Inventory.bKnown)
	{
		bool bSame = true;
		for (int32 Slot = FReal33DInventory::FirstSlot;
			bSame && Slot <= FReal33DInventory::LastSlot; ++Slot)
		{
			bSame = LastInventory.bOccupied[Slot] == Inventory.bOccupied[Slot]
				&& LastInventory.Items[Slot].TypeId == Inventory.Items[Slot].TypeId
				&& LastInventory.Items[Slot].Amount == Inventory.Items[Slot].Amount;
		}
		if (bSame)
		{
			return;
		}
	}
	LastInventory = Inventory;
	bHasDrawnInventory = true;

	for (int32 Slot = FReal33DInventory::FirstSlot;
		Slot <= FReal33DInventory::LastSlot; ++Slot)
	{
		FillSlot(Slot, FName(kSlotFaces[Slot].Brush),
			FText::FromString(kSlotFaces[Slot].Label), Inventory);
	}
}

// ------------------------------------------------------------ condition strip

void SReal33DConditionStrip::Construct(const FArguments& InArgs)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();
	const float Size = FReal33DUIStyle::ConditionIconSize;

	static const TCHAR* const Names[FlagCount] = {
		TEXT("Poisoned"), TEXT("Burning"), TEXT("Electrified"), TEXT("Drunk"),
		TEXT("ManaShield"), TEXT("Slowed"), TEXT("Hasted"), TEXT("LogoutBlocked") };
	static const TCHAR* const Tips[FlagCount] = {
		TEXT("You are poisoned"), TEXT("You are burning"),
		TEXT("You are electrified"), TEXT("You are drunk"),
		TEXT("You are protected by a magic shield"), TEXT("You are slowed"),
		TEXT("You are hasted"), TEXT("You may not logout during a fight") };

	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < FlagCount; ++Index)
	{
		Row->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(Index == 0 ? 0.0f : 2.0f, 0.0f, 0.0f, 0.0f))
			[
				SAssignNew(Icons[Index], SBox)
				.WidthOverride(Size)
				.HeightOverride(Size)
				.ToolTipText(FText::FromString(Tips[Index]))
				// Hidden rather than collapsed, so the icons keep their places
				// as conditions come and go instead of shuffling left and right.
				.Visibility(EVisibility::Hidden)
				[
					SNew(SImage).Image(Style.GetBrush(
						FName(*FString::Printf(TEXT("Real33D.Condition.%s"), Names[Index]))))
				]
			];
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(Style.GetBrush("Real33D.Chrome.ContainerSlot"))
		.Padding(FMargin(2.0f))
		[
			Row
		]
	];
}

void SReal33DConditionStrip::SetConditions(const FReal33DConditions& Conditions)
{
	if (bHasDrawnOnce && Last.bKnown == Conditions.bKnown
		&& Last.Flags == Conditions.Flags)
	{
		return;
	}
	Last = Conditions;
	bHasDrawnOnce = true;

	for (int32 Index = 0; Index < FlagCount; ++Index)
	{
		if (!Icons[Index].IsValid())
		{
			continue;
		}
		// Bit n of the byte is icon n: the order of FReal33DConditions::EFlag is
		// the order of the strip, pinned to Fusion32's own bit values by a
		// static_assert in the bridge.
		const uint8 Bit = static_cast<uint8>(1u << Index);
		const bool bRaised = Conditions.bKnown && (Conditions.Flags & Bit) != 0;
		Icons[Index]->SetVisibility(bRaised ? EVisibility::Visible : EVisibility::Hidden);
	}
}
