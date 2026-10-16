// Independent certification probe; removed before the exact merge build.
#include "Real33DHUD.h"
#include "Real33DUIStyle.h"
#include "Widgets/SWidget.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
int32 CountWidgetType(const TSharedRef<SWidget>& Widget, FName Type)
{
    int32 Count = Widget->GetType() == Type ? 1 : 0;
    FChildren* Children = Widget->GetChildren();
    for (int32 Index = 0; Index < Children->Num(); ++Index)
        Count += CountWidgetType(Children->GetChildAt(Index), Type);
    return Count;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DHUDSixPanelsCertification,
    "REAL33D.Presentation.CertificationHUDSixPanels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReal33DHUDSixPanelsCertification::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FReal33DUIStyle::Initialize();
    // The production HUD itself, with no bridge: panel operations must need no server.
    TSharedRef<SReal33DHUD> HUD = SNew(SReal33DHUD);
    auto& Registry = HUD->GetPanels();
    const TArray<FName> Expected = {
        Real33D::PanelIds::Minimap, Real33D::PanelIds::Health,
        Real33D::PanelIds::Inventory, Real33D::PanelIds::Skills,
        Real33D::PanelIds::Battle, Real33D::PanelIds::Containers };
    const TArray<FName> ContentTypes = {
        "SReal33DMinimapPanel", "SReal33DVitalsPanel", "SReal33DInventoryPanel",
        "SReal33DSkillsPanel", "SReal33DBattlePanel", "SReal33DContainersPanel" };
    TArray<FName> Actual;
    Registry.GetIds(Actual);
    TestEqual(TEXT("six registered production mini windows"), Registry.Num(), 6);
    TestTrue(TEXT("all ids and registration order"), Actual == Expected);
    TestEqual(TEXT("HUD contains six mini windows"), CountWidgetType(HUD, "SReal33DMiniWindow"), 6);
    TestEqual(TEXT("one separate chat survives"), CountWidgetType(HUD, "SReal33DChatPanel"), 1);
    TestTrue(TEXT("chat accessor intact"), HUD->GetChatPanel().IsValid());
    TestFalse(TEXT("unknown panel close rejected"), Registry.Close("UnknownCertificationPanel"));
    for (int32 Index = 0; Index < Expected.Num(); ++Index)
    {
        const FName Id = Expected[Index];
        const FString Label = Id.ToString();
        TSharedPtr<SWidget> Window = Registry.Find(Id);
        if (!TestTrue(Label + TEXT(" registered"), Window.IsValid())) continue;
        TestEqual(Label + TEXT(" is mini window"), Window->GetType(), FName("SReal33DMiniWindow"));
        TestEqual(Label + TEXT(" retains exact content type"), CountWidgetType(Window.ToSharedRef(), ContentTypes[Index]), 1);
        TestTrue(Label + TEXT(" starts open"), Registry.IsOpen(Id));
        TestTrue(Label + TEXT(" close returns success"), Registry.Close(Id));
        TestEqual(Label + TEXT(" actually collapsed"), Window->GetVisibility(), EVisibility::Collapsed);
        TestFalse(Label + TEXT(" closed state"), Registry.IsOpen(Id));
        for (const FName Other : Expected)
            if (Other != Id) TestTrue(Label + TEXT(" leaves ") + Other.ToString(), Registry.IsOpen(Other));
        TestTrue(Label + TEXT(" open returns success"), Registry.Open(Id));
        TestEqual(Label + TEXT(" actually visible"), Window->GetVisibility(), EVisibility::Visible);
        TestTrue(Label + TEXT(" toggle closes"), Registry.Toggle(Id));
        TestFalse(Label + TEXT(" toggle closed state"), Registry.IsOpen(Id));
        TestTrue(Label + TEXT(" toggle reopens"), Registry.Toggle(Id));
        TestTrue(Label + TEXT(" toggle reopened state"), Registry.IsOpen(Id));
        TestTrue(Label + TEXT(" retained original widget"), Registry.Find(Id) == Window);
        TestTrue(Label + TEXT(" chat still visible"), HUD->GetChatPanel()->GetVisibility() != EVisibility::Collapsed);
        AddInfo(Label + TEXT(": registered / close / open / toggle twice / independent / original content PASS"));
    }
    return true;
}
#endif
