#include "Real33DContainersPanel.h"

#include "Real33DPanelChrome.h"
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
	// container.otui: cell-size 34 34, cell-spacing 3, padding 6.
	constexpr float Cell = FReal33DUIStyle::SlotSize;
	constexpr float Gap = FReal33DUIStyle::SlotSpacing;
	constexpr float Padding = 6.0f;
	constexpr float HeaderHeight = 14.0f;
	constexpr int32 TextSize = 8;
}

TSharedRef<SWidget> SReal33DContainersPanel::MakeContainer(
	const FReal33DContainer& Container)
{
	const ISlateStyle& Style = FReal33DUIStyle::Get();

	// The grid is exactly as many cells as the server said the container
	// holds, rounded up to whole rows. Not the capacity attribute: an eight
	// slot bag holding two objects shows two, because two is what is in it and
	// six empty squares would be six claims about nothing.
	const int32 Count = Container.Objects.Num();

	// One empty square past the contents when the container still has room.
	// It is a place to drop something into, not a claim that anything is
	// there: it draws no object, because there is none. Without it an empty
	// container would have no target at all and nothing could be put in it.
	const bool bHasRoom = Count < static_cast<int32>(Container.Capacity);
	const int32 Cells = Count + (bHasRoom ? 1 : 0);
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
					.Location(Where)
					.OnItemDropped(OnItemDropped)
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
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						Container.bHasParent
							? StaticCastSharedRef<SWidget>(SNew(SBox)
								.WidthOverride(FReal33DUIStyle::MiniWindowButtonSize)
								.HeightOverride(FReal33DUIStyle::MiniWindowButtonSize)
								.ToolTipText(FText::FromString(
									TEXT("Go to parent container (not available)")))
								[
									SNew(SOverlay)
									+ SOverlay::Slot()
									[
										SNew(SImage).Image(
											Style.GetBrush("Real33D.Chrome.ContainerUp"))
									]
									+ SOverlay::Slot()
									[
										// CL_CMD_UP_CONTAINER is not built yet.
										SNew(SImage)
										.Image(Style.GetBrush("Real33D.Chrome.Dither"))
										.ColorAndOpacity(FReal33DUIStyle::DitherTint())
										.Visibility(EVisibility::HitTestInvisible)
									]
								])
							: SNullWidget::NullWidget
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				Grid
			]

			// The footer states how full it is, from the server's own capacity
			// attribute and its own object count.
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
