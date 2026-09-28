#include "Real33DMinimapPanel.h"
#include "Real33DMinimapView.h"
#include "Real33DMinimapPalette.h"
#include "Real33DUIStyle.h"
#include "REAL33D.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

class SReal33DMinimapSurface : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SReal33DMinimapSurface) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UReal33DBridge>, Bridge)
		SLATE_EVENT(FReal33DOnMinimapDestination, OnDestination)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args)
	{
		Bridge = Args._Bridge; OnDestination = Args._OnDestination;
		// Local appearance metadata only. It cannot discover a single map cell.
		FString Text;
		const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Minimap/appearance-palette.txt"));
		if (IFileManager::Get().FileSize(*Path) > 128 * 1024 || !FFileHelper::LoadFileToString(Text, *Path)) return;
		if (!Palette.Load(TCHAR_TO_UTF8(*Text))) return;
		UE_LOG(LogReal33D,Log,TEXT("minimap palette: %u appearance colors; no map input"),static_cast<uint32>(Palette.colors.size()));
	}
	Real33D::MinimapView View;
	FReal33DMinimapState State;
	void Update(const FReal33DMinimapState& Next)
	{
		State = Next;
		View.SetPlayer(Next.Player.X, Next.Player.Y, Next.Player.Z, Next.bPlayerKnown);
		Invalidate(EInvalidateWidgetReason::Paint);
	}
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(113, 109); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&,
		FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
	{
		const FVector2D Size = G.GetLocalSize();
		const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
		Out.PushClip(FSlateClippingZone(G));
		const auto Box = [&](double X, double Y, double W, double H, FLinearColor Color, int32 L)
		{
			FSlateDrawElement::MakeBox(Out, L,
				G.ToPaintGeometry(FVector2f(static_cast<float>(W), static_cast<float>(H)),
					FSlateLayoutTransform(FVector2f(static_cast<float>(X), static_cast<float>(Y)))),
				Brush, ESlateDrawEffect::None, Color);
		};
		Box(0, 0, Size.X, Size.Y, FLinearColor::Black, Layer);
		if (const auto* Core = Bridge.Get())
		{
			TArray<FReal33DMinimapCell> Cells;
			Core->GetMinimapCells(View.floor, View.MapX(0,Size.X)-1, View.MapY(0,Size.Y)-1,
				View.MapX(Size.X,Size.X)+1, View.MapY(Size.Y,Size.Y)+1, Cells);
			for (const auto& Cell : Cells)
			{
				FLinearColor Color = Cell.bStaticObstacle ? FLinearColor(0.64f,0.43f,0.20f)
					: Cell.GroundType ? FLinearColor(0.48f,0.55f,0.60f) : FLinearColor::Black;
				const auto Found = Palette.colors.find(Cell.ColorType ? Cell.ColorType : Cell.GroundType);
				if (Found != Palette.colors.end())
				{
					const uint8 C = Found->second;
					Color = FLinearColor(FColor(static_cast<uint8>((C/36)*51),
						static_cast<uint8>(((C/6)%6)*51),static_cast<uint8>((C%6)*51)));
				}
				Color.A = 1;
				Box(View.PixelX(Cell.Position.X,Size.X)-View.scale/2,
					View.PixelY(Cell.Position.Y,Size.Y)-View.scale/2, View.scale, View.scale, Color, Layer+1);
			}
		}
		if (State.bPlayerKnown && View.floor == State.Player.Z)
		{
			const double PX = View.PixelX(State.Player.X,Size.X), PY = View.PixelY(State.Player.Y,Size.Y);
			Box(PX-4,PY-4,9,9,FLinearColor::Black,Layer+3);
			Box(PX-1,PY-3,3,7,FLinearColor(1,0.9f,0.1f),Layer+4);
			Box(PX-3,PY-1,7,3,FLinearColor(1,0.9f,0.1f),Layer+4);
		}
		Out.PopClip();
		return Layer+4;
	}
	virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override
	{
		if (E.GetEffectingButton() != EKeys::LeftMouseButton && E.GetEffectingButton() != EKeys::RightMouseButton)
			return FReply::Handled();
		Button = E.GetEffectingButton();
		LastMouse = PressMouse = G.AbsoluteToLocal(E.GetScreenSpacePosition());
		bDragged = false;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}
	virtual FReply OnMouseMove(const FGeometry& G, const FPointerEvent& E) override
	{
		if (!HasMouseCapture()) return FReply::Unhandled();
		const FVector2D Now = G.AbsoluteToLocal(E.GetScreenSpacePosition());
		if ((Now-PressMouse).SizeSquared() > 9) bDragged = true;
		if (bDragged || Button == EKeys::RightMouseButton)
		{
			View.Pan(Now.X-LastMouse.X,Now.Y-LastMouse.Y);
			Invalidate(EInvalidateWidgetReason::Paint);
		}
		LastMouse = Now;
		return FReply::Handled();
	}
	virtual FReply OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E) override
	{
		if (HasMouseCapture() && E.GetEffectingButton() == Button)
		{
			const FVector2D P = G.AbsoluteToLocal(E.GetScreenSpacePosition());
			if (!bDragged && Button == EKeys::LeftMouseButton && G.IsUnderLocation(E.GetScreenSpacePosition())
				&& State.bPlayerKnown && View.floor == State.Player.Z)
			{
				const Real33D::FMapPosition Goal{View.MapX(P.X,G.GetLocalSize().X), View.MapY(P.Y,G.GetLocalSize().Y), View.floor};
				UE_LOG(LogReal33D, Log, TEXT("minimap destination: %d,%d,%d"),Goal.X,Goal.Y,Goal.Z);
				OnDestination.ExecuteIfBound(Goal);
			}
			else if (bDragged) UE_LOG(LogReal33D, Log, TEXT("minimap pan: center=%.2f,%.2f floor=%d"),View.center_x,View.center_y,View.floor);
			Button = FKey();
		}
		return FReply::Handled().ReleaseMouseCapture();
	}
	virtual FReply OnMouseWheel(const FGeometry&, const FPointerEvent& E) override
	{
		View.Zoom(E.GetWheelDelta()>0);
		Invalidate(EInvalidateWidgetReason::Paint);
		return FReply::Handled();
	}
	virtual void OnMouseCaptureLost(const FCaptureLostEvent&) override { Button = FKey(); bDragged = false; }
private:
	TWeakObjectPtr<UReal33DBridge> Bridge;
	Real33D::MinimapPalette Palette;
	FReal33DOnMinimapDestination OnDestination;
	FVector2D PressMouse, LastMouse;
	FKey Button;
	bool bDragged = false;
};

void SReal33DMinimapPanel::Construct(const FArguments& Args)
{
	Bridge = Args._Bridge;
	const auto Font = FCoreStyle::GetDefaultFontStyle("Regular", 8);
	const auto Button = [&](const TCHAR* Text, const TCHAR* Tip, TFunction<void()> Action) -> TSharedRef<SWidget>
	{
		return SNew(SBox).HeightOverride(20).WidthOverride(34)
		[
			SNew(SButton).ContentPadding(1).ToolTipText(FText::FromString(Tip))
			.OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })
			[SNew(STextBlock).Text(FText::FromString(Text)).Font(Font).Justification(ETextJustify::Center)]
		];
	};
	ChildSlot.Padding(FMargin(8,3,7,2))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[SAssignNew(PositionLabel,STextBlock).Font(Font).ColorAndOpacity(FLinearColor::White)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0,2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(115).HeightOverride(111)
				[
					SNew(SBorder).BorderImage(FReal33DUIStyle::Get().GetBrush("Real33D.Chrome.SunkenFrame")).Padding(1)
					[SAssignNew(Surface,SReal33DMinimapSurface).Bridge(Bridge).OnDestination(Args._OnDestination)
						.ToolTipText(FText::FromString(TEXT("North up. Explored terrain. Drag to pan; wheel to zoom. Click a position to walk; unexplored routes continue as terrain is discovered.")))]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(3,0,0,0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[Button(TEXT("+"),TEXT("Zoom in"),[this]() { Surface->View.Zoom(true); })]
				+ SVerticalBox::Slot().AutoHeight()[Button(TEXT("-"),TEXT("Zoom out"),[this]() { Surface->View.Zoom(false); })]
				+ SVerticalBox::Slot().AutoHeight()[Button(TEXT("Up"),TEXT("View floor above"),[this]() { Surface->View.BrowseFloor(-1); })]
				+ SVerticalBox::Slot().AutoHeight()[Button(TEXT("Dn"),TEXT("View floor below"),[this]() { Surface->View.BrowseFloor(1); })]
				+ SVerticalBox::Slot().AutoHeight()[Button(TEXT("Home"),TEXT("Recenter on player and current floor"),[this]() { Surface->View.Recenter(); UE_LOG(LogReal33D,Log,TEXT("minimap recenter")); })]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[SAssignNew(FloorLabel,STextBlock).Font(Font).ColorAndOpacity(FLinearColor(0.75f,0.85f,0.9f))]
	];
	Refresh();
}
void SReal33DMinimapPanel::Refresh()
{
	const auto State = Bridge.IsValid() ? Bridge->GetMinimapState() : FReal33DMinimapState{};
	Surface->Update(State);
	PositionLabel->SetText(FText::FromString(State.bPlayerKnown
		? FString::Printf(TEXT("%d, %d  Z %d"),State.Player.X,State.Player.Y,State.Player.Z) : TEXT("Offline - terrain history")));
	FloorLabel->SetText(FText::FromString(FString::Printf(TEXT("Floor %d  %s  N ^"),Surface->View.floor,
		Surface->View.follow ? TEXT("follow") : TEXT("browse"))));
}
