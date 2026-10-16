#include "Real33DUIStyle.h"
#include "Real33DStackSprite.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Styling/SlateBrush.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStackSpriteIndependentTest,
    "REAL33D.Certification.StackableSpriteFiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStackSpriteIndependentTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FReal33DUIStyle::Initialize();
    const FString Root = FPaths::ProjectDir() / TEXT("Resources/UI/Items");
    auto Leaf = [&](const TCHAR* Name) { return Root / Name; };
    auto& Files = IFileManager::Get();
    // Fixture files only; these high IDs never enter Fusion32 or WorldState.
    const TArray<FString> Owned = { Leaf(TEXT("64000.png")), Leaf(TEXT("64000_2.png")),
        Leaf(TEXT("64001.png")), Leaf(TEXT("64001_1.png")), Leaf(TEXT("64001_2.png")),
        Leaf(TEXT("64003.png")) };
    for (const FString& Path : Owned)
        if (Files.FileExists(*Path)) { AddError(TEXT("Fixture path already exists")); return false; }
    if (Files.FileExists(*Leaf(TEXT("64002.png"))))
        { AddError(TEXT("Missing-base fixture already exists")); return false; }
    if (!Files.FileExists(*Leaf(TEXT("3031_5.png"))))
        { AddError(TEXT("Fresh extracted gold frame missing")); return false; }
    for (const FString& Path : Owned)
        TestTrue(TEXT("Create owned PNG fixture"), Files.Copy(*Path, *Leaf(TEXT("3031.png"))) == COPY_OK);

    const FSlateBrush* GoldOne = FReal33DUIStyle::ItemBrush(3031, 1);
    const FSlateBrush* GoldTwelve = FReal33DUIStyle::ItemBrush(3031, 12);
    TestTrue(TEXT("x1 uses frame zero"), GoldOne && GoldOne->GetResourceName().ToString().EndsWith(TEXT("/3031.png")));
    TestTrue(TEXT("x12 uses frame five"), GoldTwelve && GoldTwelve->GetResourceName().ToString().EndsWith(TEXT("/3031_5.png")));
    TestTrue(TEXT("Amount cache entries distinct"), GoldOne != GoldTwelve);
    TestEqual(TEXT("Gold frame count"), FReal33DUIStyle::ItemFrameCount(3031), 8);
    TestEqual(TEXT("Animated worms bounded to eight amount patterns"), FReal33DUIStyle::ItemFrameCount(3492), 8);
    TestTrue(TEXT("Second type x12 resolves generically"), FReal33DUIStyle::ItemBrush(3492, 12)->GetResourceName().ToString().EndsWith(TEXT("/3492_5.png")));
    TestEqual(TEXT("Non-stackable has one picture"), FReal33DUIStyle::ItemFrameCount(3457), 1);
    TestTrue(TEXT("Non-stackable unchanged at arbitrary count"), FReal33DUIStyle::ItemBrush(3457, 1) == FReal33DUIStyle::ItemBrush(3457, 100));
    TestEqual(TEXT("Gap ends contiguous frame range"), FReal33DUIStyle::ItemFrameCount(64000), 1);
    TestTrue(TEXT("Gap falls back to base"), FReal33DUIStyle::ItemBrush(64000, 12)->GetResourceName().ToString().EndsWith(TEXT("/64000.png")));
    TestEqual(TEXT("Short range"), FReal33DUIStyle::ItemFrameCount(64001), 3);
    TestTrue(TEXT("Short range clamps to last available"), FReal33DUIStyle::ItemBrush(64001, 255)->GetResourceName().ToString().EndsWith(TEXT("/64001_2.png")));
    TestEqual(TEXT("Missing base count"), FReal33DUIStyle::ItemFrameCount(64002), 0);
    TestTrue(TEXT("Missing base gives numeric-ID fallback sentinel"), FReal33DUIStyle::ItemBrush(64002, 12) == nullptr);
    TestTrue(TEXT("Reserved empty id"), FReal33DUIStyle::ItemBrush(0, 12) == nullptr);
    TestEqual(TEXT("Probe before disappearance"), FReal33DUIStyle::ItemFrameCount(64003), 1);
    Files.Delete(*Leaf(TEXT("64003.png")), false, false);
    TestTrue(TEXT("Missing selected file after probe gives fallback sentinel"), FReal33DUIStyle::ItemBrush(64003, 12) == nullptr);
    for (int32 Count : {-1000, -1, 0, 1})
        TestEqual(TEXT("Malformed count is safe"), Real33D::StackSpriteFrame(12, Count), 0);
    for (const FString& Path : Owned) Files.Delete(*Path, false, false);
    return !HasAnyErrors();
}
#endif
