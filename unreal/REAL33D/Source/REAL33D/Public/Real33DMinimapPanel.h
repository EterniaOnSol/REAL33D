#pragma once
#include "CoreMinimal.h"
#include "Real33DBridge.h"
#include "Widgets/SCompoundWidget.h"
DECLARE_DELEGATE_OneParam(FReal33DOnMinimapDestination, Real33D::FMapPosition);
class SReal33DMinimapSurface;
class STextBlock;
/** Terrain history, separate from live observation, in the existing HUD. */
class SReal33DMinimapPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DMinimapPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UReal33DBridge>, Bridge)
		SLATE_EVENT(FReal33DOnMinimapDestination, OnDestination)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	void Refresh();
	static constexpr float PanelHeight = 154.0f;
private:
	TWeakObjectPtr<UReal33DBridge> Bridge;
	TSharedPtr<SReal33DMinimapSurface> Surface;
	TSharedPtr<STextBlock> PositionLabel;
	TSharedPtr<STextBlock> FloorLabel;
};
