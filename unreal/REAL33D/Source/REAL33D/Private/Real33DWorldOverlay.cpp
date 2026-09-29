#include "Real33DWorldOverlay.h"
#include "Real33DWorldActor.h"
#include "Real33DCreatureActor.h"
#include "Real33DTileActor.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SViewport.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

void SReal33DWorldOverlay::Refresh(const AReal33DWorld* InWorld, bool bInTargeting)
{
	World = InWorld;
	bTargeting = bInTargeting;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SReal33DWorldOverlay::ShowRequest(const FString& Text)
{
	RequestText = Text;
	RequestExpires = FPlatformTime::Seconds() + 2.0;
}

int32 SReal33DWorldOverlay::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer,
	const FWidgetStyle& Style, bool bParentEnabled) const
{
	(void)Args; (void)CullingRect; (void)Style; (void)bParentEnabled;
	const AReal33DWorld* Live = World.Get();
	if (!Live || !Live->GetWorld()) return Layer;
	APlayerController* Controller = Live->GetWorld()->GetFirstPlayerController();
	UGameViewportClient* Viewport = Live->GetWorld()->GetGameViewport();
	if (!Controller || !Viewport || !Viewport->GetGameViewportWidget().IsValid()) return Layer;
	// Both geometries must be in window paint space. GetCachedGeometry is in
	// desktop space and adds the OS window position a second time when painted.
	const FGeometry& Full = Viewport->GetGameViewportWidget()->GetPaintSpaceGeometry();
	int32 Width = 0, Height = 0;
	Controller->GetViewportSize(Width, Height);
	if (Width <= 0 || Height <= 0) return Layer;
	const auto Project = [&](const FVector& Point, FVector2D& Out)
	{
		FVector2D Pixel;
		if (!Controller->ProjectWorldLocationToScreen(Point, Pixel)) return false;
		Out = Geometry.AbsoluteToLocal(Full.LocalToAbsolute(Pixel * Full.GetLocalSize() / FVector2D(Width, Height)));
		return true;
	};
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 14);
	const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	const auto Box = [&](FVector2D At, FVector2D Size, FLinearColor Color)
	{
		FSlateDrawElement::MakeBox(Elements, Layer + 1, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(At)), White, ESlateDrawEffect::None, Color);
	};
	const auto Text = [&](const FString& Value, FVector2D At, FLinearColor Color)
	{
		FSlateDrawElement::MakeText(Elements, Layer + 2, Geometry.ToPaintGeometry(FVector2D(1,1), FSlateLayoutTransform(At + FVector2D(1,1))), Value, Font, ESlateDrawEffect::None, FLinearColor::Black);
		FSlateDrawElement::MakeText(Elements, Layer + 3, Geometry.ToPaintGeometry(FVector2D(1,1), FSlateLayoutTransform(At)), Value, Font, ESlateDrawEffect::None, Color);
	};
	const auto Outline = [&](const FVector& Center, FLinearColor Color)
	{
		TArray<FVector2D> Points;
		for (const FVector& Offset : { FVector(-44,-44,5), FVector(44,-44,5), FVector(44,44,5), FVector(-44,44,5), FVector(-44,-44,5) })
		{
			FVector2D Point;
			if (!Project(Center + Offset, Point)) return;
			Points.Add(Point);
		}
		FSlateDrawElement::MakeLines(Elements, Layer + 1, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, 2.0f);
	};
	TArray<AReal33DCreature*> Actors;
	Live->GetPresentationCreatures(Actors);
	TArray<FBox2D> Occupied;
	for (const AReal33DCreature* Creature : Actors)
	{
		FVector2D Anchor;
		if (!Project(Creature->GetNameAnchor(), Anchor)) continue;
		if (Anchor.X < 0 || Anchor.X > Geometry.GetLocalSize().X || Anchor.Y < 0 || Anchor.Y > Geometry.GetLocalSize().Y) continue;
		const bool bTarget = Live->GetCombat().TargetCreatureId == Creature->GetCreatureId();
		const FLinearColor TargetColor = Live->GetCombat().bFollowing ? FLinearColor(0.2f,0.75f,1) : FLinearColor(1,0.25f,0.2f);
		const FString Name = (bTarget ? (Live->GetCombat().bFollowing ? TEXT("[Follow] ") : TEXT("[Attack] ")) : TEXT("")) + Creature->GetCreatureName();
		const FVector2D Size = Measure->Measure(Name, Font);
		FVector2D At = Anchor - FVector2D(Size.X * 0.5, Size.Y + 10);
		At.X = FMath::Clamp(At.X, 3.0, FMath::Max(3.0, Geometry.GetLocalSize().X - Size.X - 3));
		for (int32 Attempt = 0; Attempt < 4; ++Attempt)
		{
			const FBox2D Rect(At - FVector2D(3,2), At + Size + FVector2D(3,9));
			if (!Occupied.ContainsByPredicate([&](const FBox2D& Other) { return Other.Intersect(Rect); })) break;
			At.Y -= Size.Y + 12;
		}
		At.Y = FMath::Max(2.0, At.Y);
		Occupied.Add(FBox2D(At - FVector2D(3,2), At + Size + FVector2D(3,9)));
		const uint8 Health = Creature->GetHealthPercent();
		const FLinearColor HealthColor = Health >= 95 ? FLinearColor(0.25f,1,0.3f) : Health >= 40 ? FLinearColor(1,0.85f,0.1f) : FLinearColor(1,0.25f,0.2f);
		Box(At - FVector2D(3,2), Size + FVector2D(6,10), FLinearColor(0,0,0,0.65f));
		Text(Name, At, bTarget ? TargetColor : HealthColor);
		Box(At + FVector2D(0,Size.Y + 2), FVector2D(Size.X,3), FLinearColor(0.15f,0.15f,0.15f,1));
		Box(At + FVector2D(0,Size.Y + 2), FVector2D(Size.X * FMath::Min<int32>(Health,100) / 100.0,3), HealthColor);
		if (bTarget) Outline(Creature->GetActorLocation(), TargetColor);
		const FString Speech = Creature->GetVisibleSpeech();
		if (!Speech.IsEmpty())
		{
			TArray<FString> Words, Lines;
			Speech.ParseIntoArrayWS(Words);
			FString Line;
			for (const FString& Word : Words)
			{
				if (!Line.IsEmpty() && Line.Len() + Word.Len() > 38) { Lines.Add(Line); Line.Reset(); }
				if (!Line.IsEmpty()) Line += TEXT(" ");
				Line += Word;
			}
			if (!Line.IsEmpty()) Lines.Add(Line);
			const int32 Count = FMath::Min(Lines.Num(), 4);
			for (int32 Index = 0; Index < Count; ++Index)
			{
				FString Shown = Lines[Index];
				if (Index == 3 && Lines.Num() > 4) Shown += TEXT("...");
				const FVector2D Extent = Measure->Measure(Shown, Font);
				const FVector2D Place(FMath::Clamp(Anchor.X - Extent.X / 2, 3.0, FMath::Max(3.0, Geometry.GetLocalSize().X - Extent.X - 3)), FMath::Max(2.0, At.Y - (Count - Index) * (Size.Y + 4) - 5));
				Box(Place - FVector2D(3,2), Extent + FVector2D(6,4), FLinearColor(0.04f,0.04f,0.02f,0.85f));
				Text(Shown, Place, FLinearColor(1,0.95f,0.35f));
			}
		}
	}
	const FVector2D Cursor = FSlateApplication::Get().GetCursorPos();
	// The OS cursor is in desktop space, unlike the geometry passed to OnPaint.
	if (GetTickSpaceGeometry().IsUnderLocation(Cursor))
	{
		FHitResult Hit;
		if (Controller->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, Hit))
		{
			const AReal33DTile* Tile = Cast<AReal33DTile>(Hit.GetActor());
			const AReal33DCreature* Creature = Cast<AReal33DCreature>(Hit.GetActor());
			if (Tile || Creature)
			{
				const bool bLook = Controller->IsInputKeyDown(EKeys::LeftShift) || Controller->IsInputKeyDown(EKeys::RightShift);
				Outline(Hit.GetActor()->GetActorLocation(), bTargeting ? FLinearColor(1,0.8f,0.2f) : FLinearColor(0.65f,0.85f,1));
				const FString Hint = bTargeting ? TEXT("Use With: choose target") : bLook ? TEXT("Look: click") : Creature ? TEXT("Right click: target   Shift+click: Look") : TEXT("Right click: Use   Shift+click: Look");
				Text(Hint, FVector2D(12,Geometry.GetLocalSize().Y - 28), FLinearColor::White);
			}
		}
	}
	if (FPlatformTime::Seconds() < RequestExpires)
		Text(RequestText, FVector2D(12,12), FLinearColor(1,0.9f,0.4f));
	return Layer + 3;
}
