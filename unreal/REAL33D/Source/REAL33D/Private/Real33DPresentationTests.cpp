#include "Real33DPresentationPolicy.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

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
#endif
