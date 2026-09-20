#include "Real33DGalleryPanel.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

namespace
{
	void MakeOptions(const TArray<FReal33DExperimentalCatalogEntry>& Catalog,
		TFunctionRef<const FString&(const FReal33DExperimentalCatalogEntry&)> Getter,
		TArray<TSharedPtr<FString>>& Out)
	{
		TSet<FString> Unique;
		for (const FReal33DExperimentalCatalogEntry& Entry : Catalog)
		{
			Unique.Add(Getter(Entry));
		}
		TArray<FString> Sorted = Unique.Array();
		Sorted.Sort();
		Out.Add(MakeShared<FString>(TEXT("ALL")));
		for (const FString& Value : Sorted)
		{
			Out.Add(MakeShared<FString>(Value));
		}
	}
}

void SReal33DGalleryPanel::Construct(const FArguments& InArgs)
{
	Catalog = InArgs._Catalog;
	OnSelection = InArgs._OnSelection;
	if (Catalog == nullptr)
	{
		return;
	}
	MakeOptions(*Catalog, [](const auto& E) -> const FString& { return E.Category; }, Categories);
	MakeOptions(*Catalog, [](const auto& E) -> const FString& { return E.RefinementStatus; }, Refinements);
	MakeOptions(*Catalog, [](const auto& E) -> const FString& { return E.GeometryQuality; }, Qualities);
	SelectedCategory = Categories[0];
	SelectedRefinement = Refinements[0];
	SelectedQuality = Qualities[0];

	ChildSlot
	[
		SNew(SBorder).Padding(10).BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.025f, 0.94f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(STextBlock).Text(FText::FromString(
					TEXT("REAL33D V08 TEST CATALOG — TEST_IMPORTED != APPROVED / READY / INTEGRATED")))
				.ColorAndOpacity(FLinearColor(1.0f, 0.72f, 0.18f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(SSearchBox).HintText(FText::FromString(TEXT("ID or name")))
				.OnTextChanged(this, &SReal33DGalleryPanel::SearchChanged)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1).Padding(0,0,4,0)
				[
					SNew(SComboBox<TSharedPtr<FString>>).OptionsSource(&Categories)
					.InitiallySelectedItem(SelectedCategory)
					.OnGenerateWidget(this, &SReal33DGalleryPanel::ComboRow)
					.OnSelectionChanged(this, &SReal33DGalleryPanel::CategoryChanged)
					[SNew(STextBlock).Text(this, &SReal33DGalleryPanel::CategoryText)]
				]
				+ SHorizontalBox::Slot().FillWidth(1).Padding(4,0)
				[
					SNew(SComboBox<TSharedPtr<FString>>).OptionsSource(&Refinements)
					.InitiallySelectedItem(SelectedRefinement)
					.OnGenerateWidget(this, &SReal33DGalleryPanel::ComboRow)
					.OnSelectionChanged(this, &SReal33DGalleryPanel::RefinementChanged)
					[SNew(STextBlock).Text(this, &SReal33DGalleryPanel::RefinementText)]
				]
				+ SHorizontalBox::Slot().FillWidth(1).Padding(4,0,0,0)
				[
					SNew(SComboBox<TSharedPtr<FString>>).OptionsSource(&Qualities)
					.InitiallySelectedItem(SelectedQuality)
					.OnGenerateWidget(this, &SReal33DGalleryPanel::ComboRow)
					.OnSelectionChanged(this, &SReal33DGalleryPanel::QualityChanged)
					[SNew(STextBlock).Text(this, &SReal33DGalleryPanel::QualityText)]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1)
			[
				SAssignNew(List, SListView<FIndexPtr>).ListItemsSource(&PageRows)
				.OnGenerateRow(this, &SReal33DGalleryPanel::GenerateRow)
				.OnSelectionChanged(this, &SReal33DGalleryPanel::SelectionChanged)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("Previous"))).OnClicked(this, &SReal33DGalleryPanel::PreviousPage)]
				+ SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center)[SAssignNew(PageText, STextBlock)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("Next"))).OnClicked(this, &SReal33DGalleryPanel::NextPage)]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).HeightOverride(170)[SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(Details, STextBlock).AutoWrapText(true)]]
			]
		]
	];
	Refresh();
}

TSharedRef<ITableRow> SReal33DGalleryPanel::GenerateRow(FIndexPtr Index,
	const TSharedRef<STableViewBase>& Owner)
{
	const FReal33DExperimentalCatalogEntry& E = (*Catalog)[*Index];
	return SNew(STableRow<FIndexPtr>, Owner)[SNew(STextBlock).Text(FText::FromString(
		FString::Printf(TEXT("%05u  %s  [%s]"), E.TypeId, *E.Name, *E.ImportStatus)))];
}

void SReal33DGalleryPanel::SelectionChanged(FIndexPtr Index, ESelectInfo::Type)
{
	if (!Index.IsValid()) return;
	const FReal33DExperimentalCatalogEntry& E = (*Catalog)[*Index];
	Details->SetText(FText::FromString(FString::Printf(
		TEXT("ID: %u\nName: %s\nCategory: %s\nRefinement: %s\nGeometry: %s\nGeneration: %s\nImport: %s\nWarnings: %s"),
		E.TypeId, *E.Name, *E.Category, *E.RefinementStatus, *E.GeometryQuality,
		*E.Generation, *E.ImportStatus, E.Warnings.IsEmpty() ? TEXT("none") : *E.Warnings)));
	OnSelection.ExecuteIfBound(E.TypeId);
}

void SReal33DGalleryPanel::Refresh()
{
	Filtered.Reset();
	const FString Lower = Query.ToLower();
	for (int32 Index = 0; Index < Catalog->Num(); ++Index)
	{
		const auto& E = (*Catalog)[Index];
		const bool bCategory = *SelectedCategory == TEXT("ALL") || E.Category == *SelectedCategory;
		const bool bRefinement = *SelectedRefinement == TEXT("ALL") || E.RefinementStatus == *SelectedRefinement;
		const bool bQuality = *SelectedQuality == TEXT("ALL") || E.GeometryQuality == *SelectedQuality;
		const FString Haystack = FString::Printf(TEXT("%u %05u %s"), E.TypeId, E.TypeId, *E.Name).ToLower();
		if (bCategory && bRefinement && bQuality && (Lower.IsEmpty() || Haystack.Contains(Lower)))
		{
			Filtered.Add(Index);
		}
	}
	Page = 0;
	RebuildPage();
}

void SReal33DGalleryPanel::RebuildPage()
{
	PageRows.Reset();
	const int32 Start = Page * PageSize;
	for (int32 Index = Start; Index < FMath::Min(Start + PageSize, Filtered.Num()); ++Index)
	{
		PageRows.Add(MakeShared<int32>(Filtered[Index]));
	}
	if (List.IsValid()) List->RequestListRefresh();
	const int32 Pages = FMath::Max(1, FMath::DivideAndRoundUp(Filtered.Num(), PageSize));
	if (PageText.IsValid()) PageText->SetText(FText::FromString(FString::Printf(
		TEXT("%d assets — page %d / %d"), Filtered.Num(), Page + 1, Pages)));
	if (PageRows.Num() > 0 && List.IsValid()) List->SetSelection(PageRows[0]);
}

FReply SReal33DGalleryPanel::PreviousPage() { Page = FMath::Max(0, Page - 1); RebuildPage(); return FReply::Handled(); }
FReply SReal33DGalleryPanel::NextPage() { const int32 Pages = FMath::Max(1, FMath::DivideAndRoundUp(Filtered.Num(), PageSize)); Page = FMath::Min(Pages - 1, Page + 1); RebuildPage(); return FReply::Handled(); }
void SReal33DGalleryPanel::SearchChanged(const FText& Text) { Query = Text.ToString(); Refresh(); }
void SReal33DGalleryPanel::CategoryChanged(TSharedPtr<FString> V, ESelectInfo::Type) { if (V) { SelectedCategory = V; Refresh(); } }
void SReal33DGalleryPanel::RefinementChanged(TSharedPtr<FString> V, ESelectInfo::Type) { if (V) { SelectedRefinement = V; Refresh(); } }
void SReal33DGalleryPanel::QualityChanged(TSharedPtr<FString> V, ESelectInfo::Type) { if (V) { SelectedQuality = V; Refresh(); } }
TSharedRef<SWidget> SReal33DGalleryPanel::ComboRow(TSharedPtr<FString> V) const { return SNew(STextBlock).Text(FText::FromString(V.IsValid() ? *V : TEXT("ALL"))); }
FText SReal33DGalleryPanel::CategoryText() const { return FText::FromString(SelectedCategory.IsValid() ? *SelectedCategory : TEXT("ALL")); }
FText SReal33DGalleryPanel::RefinementText() const { return FText::FromString(SelectedRefinement.IsValid() ? *SelectedRefinement : TEXT("ALL")); }
FText SReal33DGalleryPanel::QualityText() const { return FText::FromString(SelectedQuality.IsValid() ? *SelectedQuality : TEXT("ALL")); }
