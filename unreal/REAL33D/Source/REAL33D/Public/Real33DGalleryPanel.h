#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Real33DAssetRegistry.h"

class STextBlock;

DECLARE_DELEGATE_OneParam(FReal33DGallerySelection, uint16);

/** Slate catalog browser used only by -real33d-gallery. */
class SReal33DGalleryPanel final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DGalleryPanel) {}
		SLATE_ARGUMENT(const TArray<FReal33DExperimentalCatalogEntry>*, Catalog)
		SLATE_EVENT(FReal33DGallerySelection, OnSelection)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	using FIndexPtr = TSharedPtr<int32>;
	TSharedRef<ITableRow> GenerateRow(FIndexPtr Index,
		const TSharedRef<STableViewBase>& Owner);
	void SelectionChanged(FIndexPtr Index, ESelectInfo::Type SelectInfo);
	void Refresh();
	void RebuildPage();
	FReply PreviousPage();
	FReply NextPage();
	void SearchChanged(const FText& Text);
	void CategoryChanged(TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo);
	void RefinementChanged(TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo);
	void QualityChanged(TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> ComboRow(TSharedPtr<FString> Value) const;
	FText CategoryText() const;
	FText RefinementText() const;
	FText QualityText() const;

	const TArray<FReal33DExperimentalCatalogEntry>* Catalog = nullptr;
	FReal33DGallerySelection OnSelection;
	TArray<int32> Filtered;
	TArray<FIndexPtr> PageRows;
	TArray<TSharedPtr<FString>> Categories;
	TArray<TSharedPtr<FString>> Refinements;
	TArray<TSharedPtr<FString>> Qualities;
	TSharedPtr<FString> SelectedCategory;
	TSharedPtr<FString> SelectedRefinement;
	TSharedPtr<FString> SelectedQuality;
	TSharedPtr<SListView<FIndexPtr>> List;
	TSharedPtr<STextBlock> PageText;
	TSharedPtr<STextBlock> Details;
	FString Query;
	int32 Page = 0;
	static constexpr int32 PageSize = 40;
};
