#pragma once
#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class AReal33DWorld;

/** Read-only projection of live actors and the normal interaction hit. */
class SReal33DWorldOverlay : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DWorldOverlay) {}
	SLATE_END_ARGS()
	void Construct(const FArguments&) { SetVisibility(EVisibility::HitTestInvisible); SetClipping(EWidgetClipping::ClipToBounds); }
	void Refresh(const AReal33DWorld* InWorld, bool bInTargeting);
	void ShowRequest(const FString& Text);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry,
		const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer,
		const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
	TWeakObjectPtr<const AReal33DWorld> World;
	bool bTargeting = false;
	FString RequestText;
	double RequestExpires = 0;
};
