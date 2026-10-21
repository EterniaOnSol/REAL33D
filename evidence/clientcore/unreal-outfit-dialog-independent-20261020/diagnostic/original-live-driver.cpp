// Temporary independent live diagnostic. Archive and remove before final build.
// Production Slate widgets and real Fusion32 only; no fabricated server events.
#include "Real33DBridge.h"
#include "Real33DHUD.h"
#include "Real33DOutfitPanel.h"
#include "Real33DPanelChrome.h"
#include "Real33DChannelsPanel.h"
#include "Real33DWorldActor.h"
#include "Real33DCreatureActor.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/WidgetPath.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace OutfitCert {
TWeakObjectPtr<AReal33DWorld> World;
TWeakObjectPtr<UReal33DBridge> Bridge;
TSharedPtr<SReal33DHUD> HUD;
TSharedPtr<SReal33DOutfitPanel> Panel;
TSharedPtr<SReal33DChannelsPanel> Channels;
FReal33DOutfit Original, Selected;
FReal33DOutfitDialog Offered;
int32 Step=0, Cycles=0, Walks=0, BeforeGet=0, BeforeSet=0, BeforeUpdate=0;
double Started=0, Stage=0, Last=0;
bool Finished=false, Captured=false;

void Find(const TSharedRef<SWidget>& R,FName Type,TArray<TSharedRef<SWidget>>& Out) {
    if(R->GetType()==Type)Out.Add(R);
    auto* C=R->GetChildren();for(int32 I=0;I<C->Num();++I)Find(C->GetChildAt(I),Type,Out);
}
TArray<FString> Texts(const TSharedRef<SWidget>& R) {
    TArray<TSharedRef<SWidget>> Nodes;Find(R,"STextBlock",Nodes);TArray<FString> Out;
    for(const auto& N:Nodes)Out.Add(StaticCastSharedRef<STextBlock>(N)->GetText().ToString());return Out;
}
bool Click(const TSharedRef<SWidget>& R,const FString& Label,int32 Ordinal=0) {
    TArray<TSharedRef<SWidget>> Nodes;Find(R,"SMenuEntryButton",Nodes);Find(R,"SButton",Nodes);
    for(const auto& N:Nodes)if(N->IsEnabled()&&Texts(N).Contains(Label)&&Ordinal--==0) {
        UE_LOG(LogTemp,Display,TEXT("OUTFIT_CERT_CLICK type=%s label=%s"),*N->GetTypeAsString(),*Label);
        StaticCastSharedRef<SButton>(N)->SimulateClick();return true;
    }return false;
}
bool MenuClick(const TSharedRef<SWindow>& W) {
    if(Click(W,TEXT("Set outfit")))return true;
    for(const auto& C:W->GetChildWindows())if(MenuClick(C))return true;return false;
}
bool HitClick(const TSharedRef<SWidget>& R,const FString& Label) {
    TArray<TSharedRef<SWidget>> Nodes;Find(R,"SButton",Nodes);
    for(const auto& N:Nodes)if(N->IsEnabled()&&Texts(N).Contains(Label)){
        const auto G=N->GetCachedGeometry();const FVector2D P=G.LocalToAbsolute(G.GetLocalSize()*0.5f);
        const auto Path=FSlateApplication::Get().LocateWindowUnderMouse(P,FSlateApplication::Get().GetInteractiveTopLevelWindows());
        for(int32 I=0;I<Path.Widgets.Num();++I)if(Path.Widgets[I].Widget==N){StaticCastSharedRef<SButton>(N)->SimulateClick();return true;}
    }return false;
}
bool Equal(const FReal33DOutfit& A,const FReal33DOutfit& B) {
    if(A.OutfitId!=B.OutfitId||A.bDisguisedAsObject!=B.bDisguisedAsObject||A.ObjectType!=B.ObjectType)return false;
    for(int32 I=0;I<4;++I)if(A.Colors[I]!=B.Colors[I])return false;return true;
}
void Log(const TCHAR* Label,const FReal33DOutfit& O) {
    UE_LOG(LogTemp,Display,TEXT("OUTFIT_CERT_STATE %s id=%u colors=%u,%u,%u,%u disguise=%d object=%u"),
        Label,O.OutfitId,O.Colors[0],O.Colors[1],O.Colors[2],O.Colors[3],O.bDisguisedAsObject,O.ObjectType);
}
void Finish(bool Good) {
    if(Finished)return;Finished=true;
    if(Bridge.IsValid()){const auto S=Bridge->GetStats();UE_LOG(LogTemp,Display,
        TEXT("OUTFIT_CERT_COUNTERS get=%d received=%d set=%d updates=%d refused=%d commands=%d residual=%d unsupported=%d anomalies=%d"),
        S.OutfitDialogsRequested,S.OutfitDialogsReceived,S.OutfitsSubmitted,S.OutfitAppearanceUpdates,
        S.OutfitSelectionsRefused,S.Commands,S.ResidualBytes,S.UnsupportedOpcodes,S.Anomalies);}
    UE_LOG(LogTemp,Display,TEXT("OUTFIT_CERT_RESULT=%s step=%d"),Good?TEXT("PASS"):TEXT("FAILED"),Step);
    if(World.IsValid())World->WriteEvidence(Good?TEXT("OutfitCertComplete"):TEXT("OutfitCertFailed"));
    FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
}
bool Require(bool Good,const TCHAR* Label) {
    UE_LOG(LogTemp,Display,TEXT("OUTFIT_CERT_ASSERT %s %s"),Good?TEXT("PASS"):TEXT("FAILED"),Label);
    if(!Good)Finish(false);return Good;
}
void Next(){++Step;Stage=FPlatformTime::Seconds();Captured=false;}
bool Capture(const TCHAR* Label){if(Captured)return true;World->WriteEvidence(Label);Captured=true;return false;}
bool DraftEquals(const FReal33DOutfit& O) {
    const auto& D=Panel->GetDraft();if(D.OutfitId()!=O.OutfitId)return false;
    for(int32 I=0;I<4;++I)if(D.Color(Real33D::OutfitSlotAt(I))!=O.Colors[I])return false;return true;
}
bool Request() {
    BeforeGet=Bridge->GetStats().OutfitDialogsRequested;
    const auto Revision=Bridge->GetOutfitDialogRevision();
    HUD->ShowSelfContextMenu();bool Clicked=false;
    for(const auto& W:FSlateApplication::Get().GetTopLevelWindows())if(MenuClick(W)){Clicked=true;break;}
    return Require(Clicked&&!Panel->IsShowing()&&!Bridge->GetOutfitDialog().bOpen&&
        Bridge->GetOutfitDialogRevision()==Revision,TEXT("menu request opens nothing before server event"));
}
bool SlotRight(int32 Slot,bool Alt=false) {
    TArray<TSharedRef<SWidget>> Nodes;Find(HUD.ToSharedRef(),"SReal33DSlot",Nodes);
    for(const auto& N:Nodes){auto S=StaticCastSharedRef<SReal33DSlot>(N);const auto R=S->GetLocation();
        if(R.Kind!=FReal33DSlotRef::EKind::Inventory||R.Slot!=Slot||R.TypeId==0)continue;
        TSet<FKey> Keys;Keys.Add(EKeys::RightMouseButton);
        const FVector2D P=S->GetCachedGeometry().GetAbsolutePosition()+FVector2D(8,8);
        FModifierKeysState Mods(false,false,false,false,Alt,false,false,false,false);
        FPointerEvent E(0,P,P,Keys,EKeys::RightMouseButton,0,Mods);
        S->OnMouseButtonDown(S->GetCachedGeometry(),E);S->OnMouseButtonUp(S->GetCachedGeometry(),E);return true;
    }return false;
}
void Tick() {
    if(Finished)return;const double Now=FPlatformTime::Seconds();
    if(!Started)Started=Stage=Now;
    if(Now-Started>360||Now-Stage>45){Require(false,TEXT("live deadline"));return;}
    if(Now-Last<1.0)return;Last=Now;
    if(Step==0){
        if(!FSlateApplication::IsInitialized())return;
        for(TObjectIterator<AReal33DWorld> It;It;++It)if(!It->HasAnyFlags(RF_ClassDefaultObject)&&It->GetWorld()&&It->GetWorld()->IsGameWorld()){World=*It;break;}
        if(!World.IsValid()||!World->GetLocalPlayer()||!World->GetInventory().bKnown)return;
        Bridge=World->GetGameInstance()->GetSubsystem<UReal33DBridge>();if(!Bridge.IsValid()||!Bridge->IsRunning())return;
        TArray<TSharedRef<SWidget>> Nodes;
        for(const auto& W:FSlateApplication::Get().GetTopLevelWindows())Find(W,"SReal33DHUD",Nodes);
        if(Nodes.Num()!=1)return;HUD=StaticCastSharedRef<SReal33DHUD>(Nodes[0]);
        Nodes.Reset();Find(HUD.ToSharedRef(),"SReal33DOutfitPanel",Nodes);
        if(!Require(Nodes.Num()==1,TEXT("one production outfit panel in actual HUD")))return;
        Panel=StaticCastSharedRef<SReal33DOutfitPanel>(Nodes[0]);
        Nodes.Reset();Find(HUD.ToSharedRef(),"SReal33DChannelsPanel",Nodes);
        if(!Require(Nodes.Num()==1,TEXT("actual channels panel")))return;Channels=StaticCastSharedRef<SReal33DChannelsPanel>(Nodes[0]);
        auto& R=HUD->GetPanels();
        if(!Require(R.Num()==10&&R.Find(Real33D::PanelIds::Outfit)==Panel,TEXT("ten panels including exact outfit widget")))return;
        TArray<FName> Ids;R.GetIds(Ids);for(const auto Id:Ids)UE_LOG(LogTemp,Display,TEXT("OUTFIT_CERT_PANEL %s"),*Id.ToString());
        if(!Require(!Panel->IsShowing()&&Panel->GetVisibility()==EVisibility::Collapsed&&!Bridge->GetOutfitDialog().bOpen,TEXT("chooser initially collapsed with no invented state")))return;
        Original=World->GetLocalPlayer()->GetOutfit();Log(TEXT("baseline"),Original);Next();return;
    }
    if(Step==1){
        if(!Capture(TEXT("OutfitCertBeforeInput")))return;
        Next();return;
    }
    if(Step==2){
        auto* PC=World->GetWorld()->GetFirstPlayerController();FHitResult Hit;
        FVector2D P;bool Found=false;
        for(int32 Z=10;Z<=100;Z+=10){
            if(PC->ProjectWorldLocationToScreen(World->GetLocalPlayer()->GetActorLocation()+FVector(0,0,Z),P)&&
                PC->GetHitResultAtScreenPosition(P,ECC_Visibility,true,Hit)&&Hit.GetActor()==World->GetLocalPlayer()){Found=true;break;}
        }
        if(!Require(Found,TEXT("projected screen ray identifies actual own creature")))return;
        FSlateApplication::Get().SetAllUserFocusToGameViewport();
        PC->SetMouseLocation(FMath::RoundToInt(P.X),FMath::RoundToInt(P.Y));
        float MX=0,MY=0;PC->GetMousePosition(MX,MY);
        PC->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility),true,Hit);
        UE_LOG(LogTemp,Display,TEXT("OUTFIT_CERT_CURSOR projected=%.1f,%.1f actual=%.1f,%.1f hit=%s own=%s"),P.X,P.Y,MX,MY,*GetNameSafe(Hit.GetActor()),*World->GetLocalPlayer()->GetName());
        if(!Require(PC->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility),true,Hit)&&Hit.GetActor()==World->GetLocalPlayer(),TEXT("real cursor trace identifies own character")))return;
        const auto Revision=Bridge->GetOutfitDialogRevision();BeforeGet=Bridge->GetStats().OutfitDialogsRequested;
        int32 Bindings=0;
        for(auto Event:{IE_Pressed,IE_Released})for(auto& K:PC->InputComponent->KeyBindings)
            if(K.Chord.Key==EKeys::RightMouseButton&&K.KeyEvent==Event){K.KeyDelegate.Execute(EKeys::RightMouseButton);++Bindings;}
        if(!Require(Bindings==2&&!Bridge->GetOutfitDialog().bOpen,TEXT("production right-click press/release opens self menu only")))return;
        bool Clicked=false;for(const auto& W:FSlateApplication::Get().GetTopLevelWindows())if(MenuClick(W)){Clicked=true;break;}
        if(!Require(Clicked&&!Panel->IsShowing()&&Bridge->GetOutfitDialogRevision()==Revision,TEXT("actual Set outfit menu requests but does not open chooser")))return;
        Next();return;
    }
    if(Step==3){
        if(!Bridge->GetOutfitDialog().bOpen||!Panel->IsShowing())return;
        Offered=Bridge->GetOutfitDialog();Log(TEXT("first server chooser"),Offered.Current);
        if(!Require(Bridge->GetStats().OutfitDialogsRequested==BeforeGet+1&&Equal(Offered.Current,Original)&&DraftEquals(Original),TEXT("GET_OUTFIT has server roundtrip and adopts all five values")))return;
        if(!Require(Panel->GetDraft().FirstOutfit()==Offered.First&&Panel->GetDraft().LastOutfit()==Offered.Last,TEXT("draft uses exact server First Last")))return;
        if(!Require(Panel->GetVisibility()==EVisibility::SelfHitTestInvisible&&HUD->GetPanels().IsOpen(Real33D::PanelIds::Outfit),TEXT("modal does not intercept background and registry reflects open state")))return;
        UE_LOG(LogTemp,Display,TEXT("OUTFIT_CERT_RANGE first=%u last=%u count=%d"),Offered.First,Offered.Last,Offered.Count());
        if(!Capture(TEXT("OutfitCertOpened")))return;
        Cycles=Offered.Count()+2;Next();return;
    }
    if(Step==4){
        if(Cycles-- >0){const auto Old=Panel->GetDraft().OutfitId();const uint16 Expected=Old==Offered.Last?Offered.First:Old+1;
            if(!Require(Click(Panel.ToSharedRef(),TEXT(">"))&&Panel->GetDraft().OutfitId()==Expected&&Offered.Contains(Expected),TEXT("next outfit exact server range with wrap")))return;return;}
        const auto Old=Panel->GetDraft().OutfitId();const uint16 Expected=Old==Offered.First?Offered.Last:Old-1;
        if(!Require(Click(Panel.ToSharedRef(),TEXT("<"))&&Panel->GetDraft().OutfitId()==Expected,TEXT("previous outfit stays in exact server range")))return;
        Next();return;
    }
    if(Step==5){
        if(Captured){Next();return;}
        for(int32 I=0;I<4;++I){auto Before=Panel->GetDraft();if(!Require(Click(Panel.ToSharedRef(),TEXT(">"),I+1),TEXT("actual colour arrow clicked")))return;
            const auto& After=Panel->GetDraft();for(int32 J=0;J<4;++J){const auto Slot=Real33D::OutfitSlotAt(J);const uint8 Expected=I==J?(Before.Color(Slot)+1)%133:Before.Color(Slot);
                if(!Require(After.Color(Slot)==Expected,TEXT("colour arrow changes only its indexed byte")))return;}}
        Selected.OutfitId=Panel->GetDraft().OutfitId();for(int32 I=0;I<4;++I)Selected.Colors[I]=Panel->GetDraft().Color(Real33D::OutfitSlotAt(I));
        Log(TEXT("draft selected"),Selected);
        if(!Require(Equal(World->GetLocalPlayer()->GetOutfit(),Original),TEXT("preview never alters authoritative actor outfit")))return;
        if(!Capture(TEXT("OutfitCertSelected")))return;Next();return;
    }
    if(Step==6){BeforeSet=Bridge->GetStats().OutfitsSubmitted;BeforeUpdate=Bridge->GetStats().OutfitAppearanceUpdates;
        if(!Require(Click(Panel.ToSharedRef(),TEXT("Set Outfit"))&&!Panel->IsShowing()&&!Bridge->GetOutfitDialog().bOpen,TEXT("actual submit closes immediately without claiming success")))return;
        Next();return;}
    if(Step==7){const auto S=Bridge->GetStats();if(S.OutfitsSubmitted<=BeforeSet||S.OutfitAppearanceUpdates<=BeforeUpdate)return;
        if(!Require(S.OutfitsSubmitted==BeforeSet+1&&Equal(World->GetLocalPlayer()->GetOutfit(),Selected),TEXT("SET_OUTFIT followed by authoritative creature update with all colours")))return;
        Log(TEXT("authoritative"),World->GetLocalPlayer()->GetOutfit());if(!Capture(TEXT("OutfitCertAuthoritative")))return;
        if(!Request())return;Next();return;}
    if(Step==8){if(!Bridge->GetOutfitDialog().bOpen||!Panel->IsShowing())return;
        if(!Require(Equal(Bridge->GetOutfitDialog().Current,Selected)&&DraftEquals(Selected),TEXT("reopen restores server outfit and all four colours exactly")))return;
        Log(TEXT("reopened"),Bridge->GetOutfitDialog().Current);
        if(!Capture(TEXT("OutfitCertReopened")))return;
        const uint16 Old=Panel->GetDraft().OutfitId();Click(Panel.ToSharedRef(),TEXT(">"));
        if(!Require(Offered.Count()==1||Panel->GetDraft().OutfitId()!=Old,TEXT("unsent draft can be changed")))return;
        BeforeSet=Bridge->GetStats().OutfitsSubmitted;
        if(!Require(Click(Panel.ToSharedRef(),TEXT("Cancel"))&&!Panel->IsShowing(),TEXT("Cancel closes chooser")))return;Next();return;}
    if(Step==9){if(!Require(Bridge->GetStats().OutfitsSubmitted==BeforeSet&&Equal(World->GetLocalPlayer()->GetOutfit(),Selected),TEXT("Cancel sends no SET and leaves authoritative state")))return;
        if(!Request())return;Next();return;}
    if(Step==10){if(!Bridge->GetOutfitDialog().bOpen||!Panel->IsShowing())return;
        if(!Require(DraftEquals(Selected),TEXT("reopen discards cancelled draft")))return;
        if(!Require(HUD->GetPanels().IsOpen(Real33D::PanelIds::Channels)&&Panel->IsShowing()&&HitClick(Channels.ToSharedRef(),TEXT("Channel List")),TEXT("Slate hit test reaches Channel List through open outfit overlay")))return;Next();return;}
    if(Step==11){TArray<FReal33DChannelEntry> Entries;Bridge->GetChannelList(Entries);if(Entries.Num()==0)return;
        if(!Require(Panel->IsShowing()&&Entries.Num()>0,TEXT("channel list server response and outfit coexist")))return;
        if(!Capture(TEXT("OutfitCertChannels")))return;
        // Return the test account to its initial server-owned outfit through ordinary controls.
        for(int32 I=0;I<Offered.Count()&&Panel->GetDraft().OutfitId()!=Original.OutfitId;++I)Click(Panel.ToSharedRef(),TEXT(">"));
        for(int32 I=0;I<4;++I)Click(Panel.ToSharedRef(),TEXT("<"),I+1);
        if(!Require(DraftEquals(Original),TEXT("restore original outfit through actual controls")))return;
        BeforeSet=Bridge->GetStats().OutfitsSubmitted;BeforeUpdate=Bridge->GetStats().OutfitAppearanceUpdates;
        if(!Require(Click(Panel.ToSharedRef(),TEXT("Set Outfit")),TEXT("submit restoration")))return;Next();return;}
    if(Step==12){if(Bridge->GetStats().OutfitAppearanceUpdates<=BeforeUpdate)return;
        if(!Require(Equal(World->GetLocalPlayer()->GetOutfit(),Original),TEXT("server confirms original outfit restored")))return;
        if(!Request())return;Next();return;}
    if(Step==13){if(!Bridge->GetOutfitDialog().bOpen||!Panel->IsShowing())return;
        if(!Require(DraftEquals(Original),TEXT("final reopen confirms restored outfit and four colours")))return;
        Click(Panel.ToSharedRef(),TEXT("Cancel"));
        if(!Require(SlotRight(3),TEXT("actual backpack right-click regression")))return;Next();return;}
    if(Step==14){if(World->GetContainers().Num()==0)return;
        if(!Require(World->GetContainers()[0].bOpen,TEXT("backpack opens with server contents")))return;
        if(!Require(SlotRight(3,true),TEXT("actual slot Look regression")))return;
        if(!Capture(TEXT("OutfitCertRegression")))return;Next();return;}
    if(Step==15){const auto S=Bridge->GetStats();if(Now-Stage<3)return;
        if(!Require(S.ResidualBytes==0&&S.UnsupportedOpcodes==0&&S.Anomalies==0,TEXT("zero protocol residual unsupported anomalies")))return;
        if(!Require(Equal(World->GetLocalPlayer()->GetOutfit(),Original)&&!Panel->IsShowing(),TEXT("final restored appearance and closed chooser")))return;
        Finish(true);}
}
struct FInstall {
    FDelegateHandle Handle;
    FInstall(){if(FParse::Param(FCommandLine::Get(),TEXT("real33d-cert-outfit")))Handle=FCoreDelegates::OnEndFrame.AddStatic(&Tick);}
    ~FInstall(){if(Handle.IsValid())FCoreDelegates::OnEndFrame.Remove(Handle);}
} Install;
}
#endif
