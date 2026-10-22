#include "Real33DContainersPanel.h"

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
	// container.otui: cell-size 34 34, cell-spacing 3, padding 6.
	constexpr float Cell = FReal33DUIStyle::SlotSize;
	constexpr float Gap = FReal33DUIStyle::SlotSpacing;
	constexpr float Padding = 6.0f;
	constexpr float HeaderHeight = 14.0f;
	constexpr int32 TextSize = 8;
}

TSharedRef<SWidget> SReal33DContainersPanel::MakeHeaderButton(
	FName Brush, const FText& Tip, uint8 Number, bool bUp)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();
	const float Size = FReal33DUIStyle::MiniWindowButtonSize;

	// The NUMBER is captured, not the window's position. Closing a container
	// out of the middle renumbers nothing on the server but does move every
	// window below it, and a button bound to a position would then close the
	// wrong container -- which is the one way this control can be silently and
	// expensively wrong.
	return SNew(SBox)
		.WidthOverride(Size)
		.HeightOverride(Size)
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(0.0f))
			.ToolTipText(Tip)
			.OnClicked(bUp
				? FOnClicked::CreateSP(this, &SReal33DContainersPanel::HandleUp, Number)
				: FOnClicked::CreateSP(this, &SReal33DContainersPanel::HandleClose, Number))
			[
				SNew(SImage).Image(Style.GetBrush(Brush))
			]
		];
}

FReply SReal33DContainersPanel::HandleClose(uint8 Number)
{
	// Nothing closes here. The request goes out and this panel keeps drawing
	// the container until Fusion32's own SV_CMD_CLOSE_CONTAINER removes it,
	// exactly as a move keeps drawing an object where the server last put it.
	OnContainerClosed.ExecuteIfBound(Number);
	return FReply::Handled();
}

FReply SReal33DContainersPanel::HandleUp(uint8 Number)
{
	OnContainerUp.ExecuteIfBound(Number);
	return FReply::Handled();
}

FReply SReal33DContainersPanel::HandleToggleCollapsed(uint8 Number)
{
	// Local only. The container stays open on the server, so nothing is sent
	// and the next SV_CMD_CONTAINER still lands in the same window.
	if (Collapsed.Contains(Number))
	{
		Collapsed.Remove(Number);
	}
	else
	{
		Collapsed.Add(Number);
	}
	// Drawn is cleared so SetContainers cannot decide nothing changed: the
	// server state really has not, and only this panel's own view of it has.
	Drawn.Reset();
	SetContainers(Live);
	return FReply::Handled();
}

TSharedRef<SWidget> SReal33DContainersPanel::MakeContainer(
	const FReal33DContainer& Container)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();
	const bool bCollapsed = Collapsed.Contains(Container.Number);

	// The grid is the container's whole capacity, which is what a 7.72 client
	// draws: a bag is eight squares whether or not anything is in them. The
	// empty ones are places to drop something into and claim nothing about the
	// contents, because they draw no object. An earlier version sized the grid
	// to the contents instead, and an operator reading a backpack of four as
	// "5 slots" had no way to see how much room was left except the footer.
	const int32 Count = Container.Objects.Num();

	// Never fewer squares than there are objects: Fusion32 is authoritative
	// about what is inside, and a container holding more than its type's
	// capacity must still show all of it rather than hide the overflow.
	const int32 Cells = FMath::Max(Count, static_cast<int32>(Container.Capacity));
	const int32 Rows = FMath::Max(1, FMath::DivideAndRoundUp(Cells, Columns));

	TSharedRef<SVerticalBox> Grid = SNew(SVerticalBox);
	for (int32 Row = 0; Row < Rows; ++Row)
	{
		TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox);
		for (int32 Column = 0; Column < Columns; ++Column)
		{
			const int32 Index = Row * Columns + Column;
			if (Index >= Cells)
			{
				break;
			}
			if (Index >= Count)
			{
				FReal33DSlotRef Target;
				Target.Kind = FReal33DSlotRef::EKind::Container;
				Target.Container = Container.Number;
				Target.Slot = static_cast<uint8>(Index);
				Line->AddSlot()
					.AutoWidth()
					.Padding(FMargin(0.0f, 0.0f, Gap, 0.0f))
					[
						SNew(SReal33DSlot)
						.SlotBrush("Real33D.Chrome.ItemSlot")
						.Location(Target)
						.OnItemDropped(OnItemDropped)
		.OnSlotUsed(OnSlotUsed)
		.OnSlotPicked(OnSlotPicked)
						.Tooltip(FText::FromString(TEXT("Empty. Drop an object here.")))
					];
				continue;
			}
			const FReal33DItem& Item = Container.Objects[Index];
			FReal33DSlotRef Where;
			Where.Kind = FReal33DSlotRef::EKind::Container;
			Where.Container = Container.Number;
			Where.Slot = static_cast<uint8>(Index);

			Line->AddSlot()
				.AutoWidth()
				.Padding(FMargin(0.0f, 0.0f, Gap, 0.0f))
				[
					SNew(SReal33DSlot)
					.SlotBrush("Real33D.Chrome.ItemSlot")
					.TypeId(Item.TypeId)
					.Count(Item.bHasAmount ? Item.Amount : 0)
					.Stackable(Item.bHasAmount)
					.Location(Where)
					.OnItemDropped(OnItemDropped)
		.OnSlotUsed(OnSlotUsed)
		.OnSlotPicked(OnSlotPicked)
					.Tooltip(FText::FromString(Item.bHasAmount
						? FString::Printf(TEXT("Slot %d: object %u x%u"),
							Index, Item.TypeId, Item.Amount)
						: FString::Printf(TEXT("Slot %d: object %u"),
							Index, Item.TypeId)))
				];
		}
		Grid->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, Gap))
			[
				Line
			];
	}

	return SNew(SBorder)
		.BorderImage(Style.GetBrush("Real33D.Chrome.WindowBody"))
		.BorderBackgroundColor(FReal33DUIStyle::PanelTint())
		.Padding(FMargin(Padding))
		[
			SNew(SVerticalBox)

			// The header: the container's own name, and the up arrow when the
			// server says it sits inside another one.
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 4.0f))
			[
				SNew(SBox)
				.HeightOverride(HeaderHeight)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Container.Name))
						.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Title"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize + 1))
						.Clipping(EWidgetClipping::ClipToBounds)
					]
					// The up arrow, only where the server says there is a parent.
					// CUpContainer has nothing to answer for a container
					// without one, so offering it would be a control that can
					// only be refused.
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(0.0f, 0.0f, 2.0f, 0.0f))
					[
						Container.bHasParent
							? MakeHeaderButton("Real33D.Chrome.ContainerUp",
								NSLOCTEXT("Real33D", "ContainerUpTip",
									"Show the container this one is inside. "
									"Fusion32 answers with the parent, or by "
									"closing this window when the parent is a "
									"map or body container and there is nothing "
									"left to show."),
								Container.Number, true)
							: SNullWidget::NullWidget
					]

					// Minimise. Local only: a rolled-away bag is still open on
					// the server and is a drop target again the instant it is
					// rolled back out.
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(0.0f, 0.0f, 2.0f, 0.0f))
					[
						SNew(SBox)
						.WidthOverride(FReal33DUIStyle::MiniWindowButtonSize)
						.HeightOverride(FReal33DUIStyle::MiniWindowButtonSize)
						[
							SNew(SButton)
							.ButtonStyle(FCoreStyle::Get(), "NoBorder")
							.ContentPadding(FMargin(0.0f))
							.ToolTipText(bCollapsed
								? NSLOCTEXT("Real33D", "ContainerExpandTip",
									"Roll this container back out. It never "
									"closed: nothing was sent and Fusion32 still "
									"has it open.")
								: NSLOCTEXT("Real33D", "ContainerCollapseTip",
									"Roll this container away, keeping it open. "
									"Local only; nothing is sent."))
							.OnClicked(FOnClicked::CreateSP(this,
								&SReal33DContainersPanel::HandleToggleCollapsed,
								Container.Number))
							[
								SNew(SImage).Image(Style.GetBrush(
									"Real33D.Chrome.Minimize"))
							]
						]
					]

					// Close is always offered: every open container can be
					// closed, and CCloseContainer simply clears the slot.
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						MakeHeaderButton("Real33D.Chrome.Close",
							NSLOCTEXT("Real33D", "ContainerCloseTip",
								"Close this container. Asks Fusion32 to clear "
								"the slot; the window goes when the server says "
								"it did, not before."),
							Container.Number, false)
					]
				]
			]

			// Collapsed to nothing rather than hidden, so the bags below slide
			// up -- the same thing SReal33DMiniWindow does for a side panel.
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				bCollapsed ? SNullWidget::NullWidget
					: StaticCastSharedRef<SWidget>(Grid)
			]

			// The footer states how full it is, from the server's own capacity
			// attribute and its own object count. It stays while collapsed:
			// "4 of 20" is exactly what a rolled-away bag is still worth
			// saying, and it keeps the header from reading as an empty strip.
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 2.0f, 0.0f, 0.0f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(
					TEXT("%d of %u"), Count, Container.Capacity)))
				.ColorAndOpacity(Style.GetSlateColor("Real33D.Text.Disabled"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
			]
		];
}

void SReal33DContainersPanel::Construct(const FArguments& InArgs)
{
	Columns = FMath::Max(1, InArgs._Columns);
	OnItemDropped = InArgs._OnItemDropped;
	OnSlotUsed = InArgs._OnSlotUsed;
	OnSlotPicked = InArgs._OnSlotPicked;
	OnContainerClosed = InArgs._OnContainerClosed;
	OnContainerUp = InArgs._OnContainerUp;

	ChildSlot
	[
		SAssignNew(Windows, SVerticalBox)
	];

	SetContainers({});
}

void SReal33DContainersPanel::SetContainers(const TArray<FReal33DContainer>& Containers)
{
	// Rebuilt only on a real change. This is read every frame off state that
	// changes rarely, so comparing first is what keeps a container window from
	// being re-created sixty times a second.
	bool bSame = Containers.Num() == Drawn.Num();
	for (int32 Index = 0; bSame && Index < Containers.Num(); ++Index)
	{
		const FReal33DContainer& A = Containers[Index];
		const FReal33DContainer& B = Drawn[Index];
		bSame = A.Number == B.Number && A.Name == B.Name
			&& A.Capacity == B.Capacity && A.bHasParent == B.bHasParent
			&& A.Objects.Num() == B.Objects.Num();
		for (int32 Slot = 0; bSame && Slot < A.Objects.Num(); ++Slot)
		{
			bSame = A.Objects[Slot].TypeId == B.Objects[Slot].TypeId
				&& A.Objects[Slot].Amount == B.Objects[Slot].Amount;
		}
	}
	if (bSame && Windows.IsValid() && Windows->NumSlots() > 0)
	{
		return;
	}
	Drawn = Containers;
	Live = Containers;

	// A collapse only means anything while the container is open. Forgetting
	// the ones that have closed is what makes reopening a bag give it back
	// expanded, instead of resurrecting a state the player set minutes ago for
	// a window that has been and gone.
	if (Collapsed.Num() > 0)
	{
		TSet<uint8> StillOpen;
		for (const FReal33DContainer& Each : Containers)
		{
			if (Collapsed.Contains(Each.Number))
			{
				StillOpen.Add(Each.Number);
			}
		}
		Collapsed = MoveTemp(StillOpen);
	}

	if (!Windows.IsValid())
	{
		return;
	}
	Windows->ClearChildren();

	if (Containers.Num() == 0)
	{
		// Saying so beats a blank strip, which reads as a broken panel rather
		// than as a player who has nothing open.
		Windows->AddSlot()
			.AutoHeight()
			.Padding(FMargin(Padding, 4.0f, Padding, 0.0f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("No container open")))
				.ColorAndOpacity(FReal33DUIStyle::Get().GetSlateColor("Real33D.Text.Disabled"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
			];
		return;
	}

	for (const FReal33DContainer& Container : Containers)
	{
		Windows->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 2.0f))
			[
				MakeContainer(Container)
			];
	}
}
