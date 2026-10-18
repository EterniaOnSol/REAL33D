// Temporary independent certification diagnostic, never part of the candidate.
// Fixtures exercise real Slate widgets; opt-in live observer only reads server
// event copies and the actual HUD, and never posts gameplay commands or values.
#include "Real33DSkillsPanel.h"
#include "Real33DHUD.h"
#include "Real33DUIStyle.h"
#include "Real33DWorldActor.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/HittestGrid.h"
#include "Layout/Clipping.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"
#include "Rendering/DrawElements.h"
#include "Types/PaintArgs.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SkillsCertification
{
void Find(const TSharedRef<SWidget>& Root, FName Type, TArray<TSharedRef<SWidget>>& Out)
{
    if (Root->GetType() == Type) Out.Add(Root);
    FChildren* Children = Root->GetChildren();
    for (int32 I = 0; I < Children->Num(); ++I) Find(Children->GetChildAt(I), Type, Out);
}

// Observe Slate's real paint clipping, not the production BarFill helper.
// A 100px allocation makes the painted width numerically equal to percent.
float PaintedPercent(const TSharedRef<SWidget>& Widget)
{
    auto Bar = StaticCastSharedRef<SProgressBar>(Widget);
    TSharedRef<SWindow> Window = SNew(SWindow).ClientSize(FVector2D(100, 5));
    FSlateWindowElementList Elements(Window);
    FHittestGrid Grid;
    FPaintArgs Args(nullptr, Grid, FVector2D::ZeroVector, 0.0, 0.0f);
    const FGeometry Geometry = FGeometry::MakeRoot(FVector2f(100, 5), FSlateLayoutTransform());
    Bar->OnPaint(Args, Geometry, FSlateRect(0, 0, 100, 5), Elements, 0, FWidgetStyle(), true);
    const auto& States = Elements.GetClippingManager().GetClippingStates();
    if (States.Num() == 0) return 0.0f;
    if (!States.Last().ScissorRect.IsSet()) return -1.0f;
    const auto& Clip = States.Last().ScissorRect.GetValue();
    return Clip.TopRight.X - Clip.TopLeft.X;
}

FString Values(const TSharedRef<SWidget>& Panel)
{
    TArray<TSharedRef<SWidget>> Texts;
    Find(Panel, "STextBlock", Texts);
    FString Result;
    for (int32 I = 0; I + 1 < Texts.Num(); I += 2)
    {
        Result += StaticCastSharedRef<STextBlock>(Texts[I])->GetText().ToString()
            + TEXT("=") + StaticCastSharedRef<STextBlock>(Texts[I + 1])->GetText().ToString() + TEXT(";");
    }
    return Result;
}

// Runs only with an explicit certification flag; reads genuine live state.
void Observe()
{
    static double LastTime = 0;
    static FString Previous;
    const double Now = FPlatformTime::Seconds();
    if (Now - LastTime < 0.5 || !FSlateApplication::IsInitialized()) return;
    LastTime = Now;
    AReal33DWorld* World = nullptr;
    for (TObjectIterator<AReal33DWorld> It; It; ++It)
        if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->GetWorld() && It->GetWorld()->IsGameWorld()) { World = *It; break; }
    if (!World) return;
    TArray<TSharedRef<SWidget>> Panels;
    for (const auto& Window : FSlateApplication::Get().GetTopLevelWindows()) Find(Window, "SReal33DSkillsPanel", Panels);
    if (Panels.Num() != 1) return;
    const auto& V = World->GetPlayerVitals();
    const auto& S = World->GetPlayerSkills();
    FString State = FString::Printf(TEXT("known=%d/%d vitals=%u,%u,%u,%u,%u,%u,%u,%u,%u skills="),
        V.bKnown, S.bKnown, V.Level, V.LevelPercent, V.Experience, V.Hitpoints,
        V.Mana, V.SoulPoints, V.Capacity, V.MagicLevel, V.MagicLevelPercent);
    for (const FReal33DSkill* Skill : {&S.Fist, &S.Club, &S.Sword, &S.Axe, &S.Distance, &S.Shielding, &S.Fishing})
        State += FString::Printf(TEXT("%u:%u,"), Skill->Level, Skill->Percent);
    State += TEXT(" rendered=") + Values(Panels[0]);
    if (State == Previous) return;
    Previous = State;
    TArray<TSharedRef<SWidget>> Bars;
    Find(Panels[0], "SProgressBar", Bars);
    FString Paint;
    for (const auto& Bar : Bars) Paint += FString::Printf(TEXT("%.2f,"), PaintedPercent(Bar));
    UE_LOG(LogTemp, Display, TEXT("SkillsCert LIVE %s painted_percent=%s"), *State, *Paint);
}

struct FObserverRegistration
{
    FDelegateHandle Handle;
    FObserverRegistration()
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("real33d-cert-skills-observer")))
            Handle = FCoreDelegates::OnEndFrame.AddStatic(&Observe);
    }
    ~FObserverRegistration() { if (Handle.IsValid()) FCoreDelegates::OnEndFrame.Remove(Handle); }
};
static FObserverRegistration Registration;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DSkillsWidgetCertification,
    "REAL33D.Presentation.CertificationSkillsWidgets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReal33DSkillsWidgetCertification::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace SkillsCertification;
    FReal33DUIStyle::Initialize();
    auto Panel = SNew(SReal33DSkillsPanel);
    TArray<TSharedRef<SWidget>> Texts, Bars;
    Find(Panel, "STextBlock", Texts);
    Find(Panel, "SProgressBar", Bars);
    if (!TestEqual(TEXT("14 actual name/value pairs"), Texts.Num(), 28)
        || !TestEqual(TEXT("9 real percent bars"), Bars.Num(), 9)) return false;
    const TCHAR* Names[] = {TEXT("Fist Fighting"), TEXT("Club Fighting"), TEXT("Sword Fighting"), TEXT("Axe Fighting"), TEXT("Distance Fighting"), TEXT("Shielding"), TEXT("Fishing")};
    for (int32 I = 0; I < 7; ++I)
        TestEqual(TEXT("skill label/order"), StaticCastSharedRef<STextBlock>(Texts[14 + I * 2])->GetText().ToString(), FString(Names[I]));
    auto CheckText = [&](int32 Row, const TCHAR* Expected)
    { TestEqual(FString::Printf(TEXT("row%d actual text"), Row), StaticCastSharedRef<STextBlock>(Texts[Row * 2 + 1])->GetText().ToString(), FString(Expected)); };
    for (int32 Row = 0; Row < 14; ++Row) CheckText(Row, TEXT("--"));
    FReal33DPlayerVitals V;
    FReal33DPlayerSkills S;
    V.Experience = 4000000001U; V.Level = 42; V.Hitpoints = 123; V.Mana = 45;
    V.SoulPoints = 67; V.Capacity = 89; V.LevelPercent = 37; V.MagicLevelPercent = 63;
    FReal33DSkill* Seven[] = {&S.Fist, &S.Club, &S.Sword, &S.Axe, &S.Distance, &S.Shielding, &S.Fishing};
    const uint8 Percents[] = {0, 1, 17, 50, 99, 100, 255};
    for (int32 I = 0; I < 7; ++I) { Seven[I]->Level = static_cast<uint8>(I); Seven[I]->Percent = Percents[I]; }
    Panel->SetValues(V, S);
    for (int32 Row = 0; Row < 14; ++Row) CheckText(Row, TEXT("--"));
    for (const auto& Bar : Bars) TestEqual(TEXT("unknown actual paint empty"), PaintedPercent(Bar), 0.0f);
    V.bKnown = true;
    Panel->SetValues(V, S);
    CheckText(0, TEXT("42")); CheckText(1, TEXT("4000000001")); CheckText(2, TEXT("123"));
    CheckText(3, TEXT("45")); CheckText(4, TEXT("67")); CheckText(5, TEXT("89")); CheckText(6, TEXT("0"));
    for (int32 Row = 7; Row < 14; ++Row) CheckText(Row, TEXT("--"));
    S.bKnown = true;
    Panel->SetValues(V, S);
    TestTrue(TEXT("level actual painted37%"), FMath::IsNearlyEqual(PaintedPercent(Bars[0]), 37.0f, 0.02f));
    TestTrue(TEXT("magic actual painted63%"), FMath::IsNearlyEqual(PaintedPercent(Bars[1]), 63.0f, 0.02f));
    for (int32 I = 0; I < 7; ++I)
    {
        CheckText(I + 7, *FString::FromInt(I));
        TestTrue(TEXT("skill real painted received percentage"), FMath::IsNearlyEqual(PaintedPercent(Bars[I + 2]), FMath::Min(static_cast<float>(Percents[I]), 100.0f), 0.02f));
    }
    S.Sword.Percent = 72; S.Fishing.Level = 18;
    Panel->SetValues(V, S);
    TestTrue(TEXT("percent-only update redraws actual sword bar"), FMath::IsNearlyEqual(PaintedPercent(Bars[4]), 72.0f, 0.02f));
    CheckText(13, TEXT("18"));
    V.bKnown = false;
    Panel->SetValues(V, S);
    for (int32 Row = 0; Row < 7; ++Row) CheckText(Row, TEXT("--"));
    CheckText(7, TEXT("0"));
    S.bKnown = false; Panel->SetValues(V, S);
    for (int32 Row = 0; Row < 14; ++Row) CheckText(Row, TEXT("--"));
    for (const auto& Bar : Bars) TestEqual(TEXT("cleared actual paint empty"), PaintedPercent(Bar), 0.0f);
    auto HUD = SNew(SReal33DHUD);
    auto& Registry = HUD->GetPanels();
    TestEqual(TEXT("registry still has six panels"), Registry.Num(), 6);
    TArray<FName> Ids; Registry.GetIds(Ids);
    for (FName Id : Ids)
    {
        TestTrue(TEXT("close actual panel"), Registry.Close(Id));
        TestFalse(TEXT("actual collapsed"), Registry.IsOpen(Id));
        TestTrue(TEXT("open actual panel"), Registry.Open(Id));
        TestTrue(TEXT("actual visible"), Registry.IsOpen(Id));
        Registry.Toggle(Id); TestFalse(TEXT("toggle closed"), Registry.IsOpen(Id));
        Registry.Toggle(Id); TestTrue(TEXT("toggle reopened"), Registry.IsOpen(Id));
    }
    TestTrue(TEXT("chat retained"), HUD->GetChatPanel().IsValid());
    AddInfo(TEXT("14 rows, 7 named skills, independent known flags, actual zero, large experience, 9 painted bars, percent-only refresh and six-panel registry PASS"));
    return true;
}
#endif
