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
	TestFalse(TEXT("outdoor roof remains"), HideUpperFloors(7, false));
	TestTrue(TEXT("covered room cuts upper floors"), HideUpperFloors(7, true));
	TestTrue(TEXT("underground cannot show surface through missing cover"), HideUpperFloors(8, false));
	const FBox Wall(FVector(100, -50, 0), FVector(120, 50, 220));
	double Entry = -1;
	TestTrue(TEXT("wall across camera ray"), SegmentEntry(FVector(0,0,80), FVector(500,0,80), Wall, 0, Entry));
	TestEqual(TEXT("first contact"), Entry, 0.2);
	TestFalse(TEXT("parallel ray outside wall"), SegmentEntry(FVector(0,80,80), FVector(500,80,80), Wall, 0, Entry));
	TestTrue(TEXT("padding protects near plane"), SegmentEntry(FVector(0,60,80), FVector(500,60,80), Wall, 18, Entry));
	TestTrue(TEXT("inside bounds is detected"), SegmentEntry(FVector(110,0,80), FVector(500,0,80), Wall, 0, Entry));
	TestEqual(TEXT("inside contact is immediate"), Entry, 0.0);
	TestFalse(TEXT("wall beyond camera does not collide"), SegmentEntry(FVector(0,0,80), FVector(50,0,80), Wall, 0, Entry));
	TestTrue(TEXT("near occluder cuts away"), CutAway(FVector(0,0,80), FVector(500,0,80), Wall));
	TestFalse(TEXT("thin ground remains"), CutAway(FVector(0,0,5), FVector(500,0,5), FBox(FVector(100,-50,0), FVector(120,50,10))));
	double Distance = 500;
	LimitCamera(FVector(0,0,80), FVector(500,0,80), Wall, Distance);
	TestEqual(TEXT("camera stops before padded wall"), Distance, 80.0);
	return true;
}
#endif
