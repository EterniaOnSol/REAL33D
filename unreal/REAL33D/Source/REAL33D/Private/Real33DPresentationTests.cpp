#include "Real33DPresentationPolicy.h"
#include "Real33DPanelRegistry.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Widgets/Layout/SBox.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DPresentationTest,
	"REAL33D.Presentation.CameraAndFloors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReal33DPresentationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace Real33D::Presentation;
	const FBox WaterBorder(FVector(-50,-50,0), FVector(50,50,4.5));
	TestTrue(TEXT("observed water border is a flat surface"), IsFlatSurface(WaterBorder));
	TestFalse(TEXT("wall must not receive terrain layer lift"), IsFlatSurface(FBox(FVector(-50,-15,0), FVector(50,15,220))));
	TestFalse(TEXT("missing mesh bounds are not terrain"), IsFlatSurface(FBox(ForceInit)));
	TestEqual(TEXT("ground retains its original height"), SurfaceLayerLift(0), 0.0);
	TestTrue(TEXT("first border top separates from water top"),
		WaterBorder.Max.Z + SurfaceLayerLift(1) > WaterBorder.Max.Z + SurfaceLayerLift(0));
	TestTrue(TEXT("successive flat overlays preserve render order"), SurfaceLayerLift(2) > SurfaceLayerLift(1));
	TestFalse(TEXT("outdoor roof remains"), HideUpperFloors(7, false));
	TestTrue(TEXT("covered room cuts upper floors"), HideUpperFloors(7, true));
	TestTrue(TEXT("underground cannot show surface through missing cover"), HideUpperFloors(8, false));
	TestTrue(TEXT("temple F9: floor 7 covered by floor 5 despite empty floor 6"),
		HasCoverAbove(7, [](int32 Floor) { return Floor == 5; }));
	TestTrue(TEXT("ordinary immediate cover remains supported"),
		HasCoverAbove(7, [](int32 Floor) { return Floor == 6; }));
	TestFalse(TEXT("outdoors with no observed overhead content stays uncovered"),
		HasCoverAbove(7, [](int32) { return false; }));
	TestFalse(TEXT("current and lower floor content cannot count as overhead"),
		HasCoverAbove(7, [](int32 Floor) { return Floor >= 7; }));
	TestFalse(TEXT("topmost floor has no overhead tiles"),
		HasCoverAbove(0, [](int32) { return true; }));
	const FBox Wall(FVector(100, -50, 0), FVector(120, 50, 220));
	double Entry = -1;
	TestTrue(TEXT("wall across camera ray"), SegmentEntry(FVector(0,0,80), FVector(500,0,80), Wall, 0, Entry));
	TestEqual(TEXT("first contact"), Entry, 0.2);
	TestFalse(TEXT("parallel ray outside wall"), SegmentEntry(FVector(0,80,80), FVector(500,80,80), Wall, 0, Entry));
	TestTrue(TEXT("padding protects near plane"), SegmentEntry(FVector(0,60,80), FVector(500,60,80), Wall, 18, Entry));
	TestTrue(TEXT("inside bounds is detected"), SegmentEntry(FVector(110,0,80), FVector(500,0,80), Wall, 0, Entry));
	TestEqual(TEXT("inside contact is immediate"), Entry, 0.0);
	TestFalse(TEXT("wall beyond camera does not collide"), SegmentEntry(FVector(0,0,80), FVector(50,0,80), Wall, 0, Entry));
	double Distance = 500;
	LimitCamera(FVector(0,0,80), FVector(500,0,80), Wall, Distance);
	TestEqual(TEXT("diagnostic clearance before padded wall"), Distance, 80.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DPanelRegistryTest,
	"REAL33D.Presentation.PanelRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReal33DPanelRegistryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FReal33DPanelRegistry Registry;
	// Real widgets, not SNullWidget: that is a shared singleton and Slate
	// ensures on any attempt to mutate it, so it cannot stand in for a
	// panel that gets shown and hidden.
	const TSharedRef<SWidget> First = SNew(SBox);
	const TSharedRef<SWidget> Second = SNew(SBox);
	// Captured rather than assumed: a widget's default visibility is its
	// own business, and this test only cares that the registry leaves it.
	const EVisibility SecondBuiltWith = Second->GetVisibility();

	// Nothing is registered to begin with, and an unknown id is inert rather
	// than a crash: that is what lets the control strip name a panel the HUD
	// does not carry.
	TestEqual(TEXT("starts empty"), Registry.Num(), 0);
	TestFalse(TEXT("unknown id is not registered"), Registry.IsRegistered(Real33D::PanelIds::Skills));
	TestFalse(TEXT("unknown id is not open"), Registry.IsOpen(Real33D::PanelIds::Skills));
	TestFalse(TEXT("toggling an unknown id reports failure"), Registry.Toggle(Real33D::PanelIds::Skills));
	TestFalse(TEXT("opening an unknown id reports failure"), Registry.Open(Real33D::PanelIds::Skills));

	TestTrue(TEXT("registers"), Registry.Register(Real33D::PanelIds::Skills, First));
	TestEqual(TEXT("counted"), Registry.Num(), 1);
	TestTrue(TEXT("found"), Registry.IsRegistered(Real33D::PanelIds::Skills));
	TestEqual(TEXT("returns the widget it was given"),
		Registry.Find(Real33D::PanelIds::Skills), TSharedPtr<SWidget>(First));

	// A duplicate is refused rather than silently replacing the first.
	TestFalse(TEXT("duplicate id refused"), Registry.Register(Real33D::PanelIds::Skills, Second));
	TestEqual(TEXT("duplicate did not replace"),
		Registry.Find(Real33D::PanelIds::Skills), TSharedPtr<SWidget>(First));
	TestEqual(TEXT("duplicate did not grow the list"), Registry.Num(), 1);
	TestFalse(TEXT("empty id refused"), Registry.Register(NAME_None, Second));

	// Open state is the widget's own visibility, not a cached flag.
	TestTrue(TEXT("a panel built visible reads as open"), Registry.IsOpen(Real33D::PanelIds::Skills));
	TestTrue(TEXT("close succeeds"), Registry.Close(Real33D::PanelIds::Skills));
	TestFalse(TEXT("closed reads as closed"), Registry.IsOpen(Real33D::PanelIds::Skills));
	TestEqual(TEXT("closed collapses so the column reclaims the space"),
		First->GetVisibility(), EVisibility::Collapsed);
	TestTrue(TEXT("toggle reopens"), Registry.Toggle(Real33D::PanelIds::Skills));
	TestTrue(TEXT("reopened reads as open"), Registry.IsOpen(Real33D::PanelIds::Skills));
	TestTrue(TEXT("toggle closes again"), Registry.Toggle(Real33D::PanelIds::Skills));
	TestFalse(TEXT("closed again"), Registry.IsOpen(Real33D::PanelIds::Skills));

	// Visibility set outside the registry is still what the registry reports.
	First->SetVisibility(EVisibility::Visible);
	TestTrue(TEXT("external change is observed, not cached over"),
		Registry.IsOpen(Real33D::PanelIds::Skills));

	// Registration order is the order ids come back in.
	TestTrue(TEXT("second panel registers"), Registry.Register(Real33D::PanelIds::Battle, Second));
	TArray<FName> Ids;
	Registry.GetIds(Ids);
	TestEqual(TEXT("both ids listed"), Ids.Num(), 2);
	TestEqual(TEXT("registration order kept"), Ids[0], Real33D::PanelIds::Skills);
	TestEqual(TEXT("registration order kept"), Ids[1], Real33D::PanelIds::Battle);

	// Panels are independent of one another.
	TestTrue(TEXT("closing one"), Registry.Close(Real33D::PanelIds::Skills));
	TestTrue(TEXT("leaves the other open"), Registry.IsOpen(Real33D::PanelIds::Battle));

	TestTrue(TEXT("unregister"), Registry.Unregister(Real33D::PanelIds::Skills));
	TestFalse(TEXT("unregistering twice reports failure"), Registry.Unregister(Real33D::PanelIds::Skills));
	TestEqual(TEXT("one left"), Registry.Num(), 1);
	Registry.Reset();
	TestEqual(TEXT("reset empties"), Registry.Num(), 0);
	// Reset forgets panels; it does not touch the widgets themselves.
	TestEqual(TEXT("reset left the widget alone"), Second->GetVisibility(), SecondBuiltWith);
	return true;
}
#endif
