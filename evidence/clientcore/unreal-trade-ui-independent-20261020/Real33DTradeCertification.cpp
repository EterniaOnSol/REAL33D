// Temporary independent diagnostic; retire into evidence before certification.
// Real sockets and production Slate callbacks only. No synthetic server state.
#include "Real33DBridge.h"
#include "Real33DHUD.h"
#include "Real33DTradePanel.h"
#include "Real33DPanelChrome.h"
#include "Real33DPanelRegistry.h"
#include "Real33DChannelsPanel.h"
#include "Real33DChatPanel.h"
#include "Real33DEditorWindow.h"
#include "Real33DWorldActor.h"
#include "Real33DCreatureActor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"
#include "Subsystems/SubsystemCollection.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace TradeCert
{
TWeakObjectPtr<AReal33DWorld> World;
TWeakObjectPtr<UReal33DBridge> B;
UReal33DBridge* A=nullptr;
FSubsystemCollection<UGameInstanceSubsystem> Peers;
TSharedPtr<SReal33DHUD> HUD;
TSharedPtr<SReal33DTradePanel> BT,AT;
TSharedPtr<SReal33DChannelsPanel> Channels;
TSharedPtr<SReal33DEditorWindow> Editor;
TSharedPtr<SReal33DChatPanel> Chat;
FReal33DInventory AI;
TArray<FReal33DContainer> AC;
uint32 AId=0,BId=0;
uint16 OriginalA=0,OriginalB=0;
FString AName,BName,ALook,BLook;
int32 Step=0,Round=0,InspectBefore=0,LookMessagesBefore=0,UsesBefore=0,OffersBefore=0,BClose=0,AClose=0;
double Started=0,StageStart=0,LastTick=0;
bool Finished=false,Captured=false,FollowSent=false;
bool Resume=false;
const FString ChannelToken=TEXT("TCERT20261020 CHANNEL");
const FString LocalToken=TEXT("TCERT20261020 LOCAL");

void Find(const TSharedRef<SWidget>& R,FName Type,TArray<TSharedRef<SWidget>>& Out)
{
    if(R->GetType()==Type) Out.Add(R);
    auto* C=R->GetChildren();
    for(int32 I=0;I<C->Num();++I) Find(C->GetChildAt(I),Type,Out);
}
TArray<FString> Texts(const TSharedRef<SWidget>& R)
{
    TArray<TSharedRef<SWidget>> N;Find(R,"STextBlock",N);TArray<FString> T;
    for(const auto& W:N) T.Add(StaticCastSharedRef<STextBlock>(W)->GetText().ToString());
    return T;
}
FString Painted(const TSharedRef<SWidget>& R){return FString::Join(Texts(R),TEXT(" | "));}
bool Click(const TSharedRef<SWidget>& R,const FString& Label,int32 Ordinal=0)
{
    TArray<TSharedRef<SWidget>> N;Find(R,"SButton",N);Find(R,"SMenuEntryButton",N);
    for(const auto& W:N) if(W->IsEnabled()&&Texts(W).Contains(Label))
        {if(Ordinal--==0){StaticCastSharedRef<SButton>(W)->SimulateClick();return true;}}
    return false;
}
bool Menu(const TSharedRef<SWindow>& W)
{
    if(Click(W,TEXT("Trade with..."))) return true;
    for(const auto& C:W->GetChildWindows()) if(Menu(C)) return true;
    return false;
}
uint32 View(EReal33DChatViewKind Kind,int32 Channel=0)
{
    TArray<FReal33DChatView> V;B->GetChatViews(V);
    for(const auto& X:V) if(X.Kind==Kind&&(Kind!=EReal33DChatViewKind::Channel||X.Channel==Channel)) return X.ViewId;
    return 0;
}
bool Has(uint32 Id,const FString& Token)
{
    TArray<FReal33DChatLine> L;B->GetChatTranscript(Id,L);
    for(const auto& X:L) if(X.Line.Contains(Token))return true;return false;
}
FString LastLook()
{
    TArray<FReal33DChatLine>L;B->GetChatTranscript(View(EReal33DChatViewKind::Local),L);
    for(int32 I=L.Num()-1;I>=0;--I){int32 P=L[I].Line.Find(TEXT("You see "));if(P!=INDEX_NONE)return L[I].Line.Mid(P);}
    return FString();
}
FString FirstSentence(const FString& S){int32 I;if(S.FindChar('.',I))return S.Left(I+1);return S;}
int32 LookCount()
{
    TArray<FReal33DChatLine> L;B->GetChatTranscript(View(EReal33DChatViewKind::Local),L);
    int32 N=0;for(const auto& X:L)if(X.Line.Contains(TEXT("You see ")))++N;return N;
}
void Ledger(const TCHAR* Who,const FReal33DInventory& Inv,const TArray<FReal33DContainer>& Cs)
{
    for(int32 S=1;S<=10;++S)if(Inv.bOccupied[S])
        UE_LOG(LogTemp,Display,TEXT("CERT_ITEM %s inventory=%d type=%u amount=%u"),Who,S,Inv.Items[S].TypeId,Inv.Items[S].Amount);
    for(const auto& C:Cs)for(int32 S=0;S<C.Objects.Num();++S)
        UE_LOG(LogTemp,Display,TEXT("CERT_ITEM %s container=%u slot=%d type=%u amount=%u"),Who,C.Number,S,C.Objects[S].TypeId,C.Objects[S].Amount);
}
int32 Count(const FReal33DInventory& Inv,const TArray<FReal33DContainer>& Cs,uint16 Type)
{
    int32 N=0;for(int32 S=1;S<=10;++S)if(Inv.bOccupied[S]&&Inv.Items[S].TypeId==Type)++N;
    for(const auto& C:Cs)for(const auto& I:C.Objects)if(I.TypeId==Type)++N;return N;
}
bool Ownership(bool Swapped)
{
    return Count(AI,AC,OriginalA)==(Swapped?0:1)&&Count(AI,AC,OriginalB)==(Swapped?1:0)
        &&Count(World->GetInventory(),World->GetContainers(),OriginalB)==(Swapped?0:1)
        &&Count(World->GetInventory(),World->GetContainers(),OriginalA)==(Swapped?1:0);
}
FReal33DSlotRef Ref(const FReal33DInventory& Inv,const TArray<FReal33DContainer>& Cs,uint16 Type)
{
    FReal33DSlotRef R;R.TypeId=Type;
    for(int32 S=1;S<=10;++S)if(Inv.bOccupied[S]&&Inv.Items[S].TypeId==Type)
        {R.Kind=FReal33DSlotRef::EKind::Inventory;R.Slot=S;return R;}
    for(const auto& C:Cs)for(int32 S=0;S<C.Objects.Num();++S)if(C.Objects[S].TypeId==Type)
        {R.Kind=FReal33DSlotRef::EKind::Container;R.Container=C.Number;R.Slot=S;return R;}
    return R;
}
UReal33DBridge::FMoveSlot Address(const FReal33DSlotRef& R)
{
    return R.Kind==FReal33DSlotRef::EKind::Inventory?UReal33DBridge::FMoveSlot::InInventory(R.Slot):
        UReal33DBridge::FMoveSlot::InContainer(R.Container,R.Slot);
}
TSharedPtr<SReal33DSlot> Slot(const FReal33DSlotRef& R)
{
    TArray<TSharedRef<SWidget>> N;Find(HUD.ToSharedRef(),"SReal33DSlot",N);
    for(const auto& W:N){auto S=StaticCastSharedRef<SReal33DSlot>(W);const auto X=S->GetLocation();
        if(X.Kind==R.Kind&&X.Slot==R.Slot&&X.Container==R.Container&&X.TypeId==R.TypeId)return S;}
    return nullptr;
}
bool Right(const FReal33DSlotRef& R,bool Ctrl=false,bool Shift=false,bool Alt=false)
{
    auto W=Slot(R);if(!W)return false;
    TSet<FKey> Pressed;Pressed.Add(EKeys::RightMouseButton);
    FModifierKeysState Mods(Shift,false,Ctrl,false,Alt,false,false,false,false);
    FVector2D P=W->GetCachedGeometry().GetAbsolutePosition()+FVector2D(10,10);
    FPointerEvent E(0,P,P,Pressed,EKeys::RightMouseButton,0,Mods);
    W->OnMouseButtonDown(W->GetCachedGeometry(),E);
    W->OnMouseButtonUp(W->GetCachedGeometry(),E);return true;
}
void Dump()
{
    for(auto* X:{B.Get(),A}){if(!X)continue;const auto S=X->GetStats();
        UE_LOG(LogTemp,Display,TEXT("CERT_COUNTERS %s frames=%d commands=%d residual=%d unsupported=%d anomalies=%d own=%d partner=%d close=%d offers=%d inspect=%d accept=%d reject=%d uses=%d looks=%d"),
            X==A?TEXT("A"):TEXT("B"),S.Frames,S.Commands,S.ResidualBytes,S.UnsupportedOpcodes,S.Anomalies,
            S.TradeOwnOffersReceived,S.TradePartnerOffersReceived,S.TradeClosesReceived,S.TradeOffersRequested,
            S.TradeInspectsRequested,S.TradeAcceptsRequested,S.TradeRejectsRequested,S.UsesRequested,S.LooksRequested);}
    if(World.IsValid()){Ledger(TEXT("B"),World->GetInventory(),World->GetContainers());Ledger(TEXT("A"),AI,AC);}
}
void Finish(bool Good)
{
    if(Finished)return;Finished=true;Dump();
    UE_LOG(LogTemp,Display,TEXT("TRADE_CERT_RESULT=%s step=%d round=%d"),Good?TEXT("PASS"):TEXT("FAILED"),Step,Round);
    if(World.IsValid())World->WriteEvidence(Good?TEXT("CertComplete"):TEXT("CertFailed"));
    if(A){A->Deinitialize();A->RemoveFromRoot();A=nullptr;}
    FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
}
bool Require(bool Good,const TCHAR* Name)
{
    UE_LOG(LogTemp,Display,TEXT("CERT_ASSERT %s %s"),Good?TEXT("PASS"):TEXT("FAILED"),Name);
    if(!Good)Finish(false);return Good;
}
void Next(int32 Target=-1)
{
    Dump();UE_LOG(LogTemp,Display,TEXT("CERT_STAGE_COMPLETE step=%d round=%d"),Step,Round);
    Step=Target>=0?Target:Step+1;StageStart=FPlatformTime::Seconds();Captured=false;
}
bool Capture(const TCHAR* Name)
{
    if(Captured)return true;World->WriteEvidence(FString::Printf(TEXT("%s%d"),Name,Round));Captured=true;return false;
}
bool RowClick(int32 Id,const FString& Label)
{
    TArray<TSharedRef<SWidget>> N;Find(Channels.ToSharedRef(),"SHorizontalBox",N);
    for(const auto& R:N){auto T=Texts(R);if(T.Num()==2&&T[0].StartsWith(FString::FromInt(Id)+TEXT("  "))&&T[1]==Label)return Click(R,Label);}return false;
}
bool Send(const FString& Token)
{
    auto Input=StaticCastSharedPtr<SEditableTextBox>(Chat->GetInputWidget());if(!Input)return false;
    Input->SetText(FText::FromString(Token));return Click(Chat.ToSharedRef(),TEXT("Send"));
}
bool Select(EReal33DChatViewKind Kind,int32 Channel=0)
{
    TArray<FReal33DChatView> V;B->GetChatViews(V);
    for(const auto& X:V)if(X.Kind==Kind&&(Kind!=EReal33DChatViewKind::Channel||X.Channel==Channel))
        return Click(Chat.ToSharedRef(),SReal33DChatPanel::TabLabel(X).ToString());return false;
}
void Tick()
{
    if(Finished)return;double Now=FPlatformTime::Seconds();
    if(!Started){Started=StageStart=Now;}
    if(Now-Started>480||Now-StageStart>60){Require(false,TEXT("live deadline"));return;}
    if(A)
    {
        TArray<FReal33DEvent> Es;A->DrainEvents(Es);
        for(const auto& E:Es)
        {
            if(E.Kind==EReal33DEventKind::Connected)AName=E.CreatureName;
            if(E.Kind==EReal33DEventKind::LocalPlayerIdentified)AId=E.CreatureId;
            if(E.Kind==EReal33DEventKind::InventoryChanged)AI=E.Inventory;
            if(E.Kind==EReal33DEventKind::ContainerChanged)
            {AC.RemoveAll([&E](const FReal33DContainer& C){return C.Number==E.Container.Number;});if(E.Container.bOpen)AC.Add(E.Container);}
            if(E.Kind==EReal33DEventKind::ServerMessage||E.Kind==EReal33DEventKind::Talk)
            {UE_LOG(LogTemp,Display,TEXT("CERT_PEER_MESSAGE %s"),*E.Detail);int32 P=E.Detail.Find(TEXT("You see "));if(P!=INDEX_NONE)ALook=E.Detail.Mid(P);}
            if(E.Kind==EReal33DEventKind::Diagnostic||E.Kind==EReal33DEventKind::Failed)
            {Require(false,TEXT("peer has no diagnostic/failure"));return;}
        }
        AT->Refresh();
    }
    if(Now-LastTick<1.5)return;LastTick=Now;
    if(Step==0)
    {
        Resume=FParse::Param(FCommandLine::Get(),TEXT("real33d-cert-trade-resume"));
        if(!FSlateApplication::IsInitialized())return;
        for(TObjectIterator<AReal33DWorld> It;It;++It)if(!It->HasAnyFlags(RF_ClassDefaultObject)&&It->GetWorld()&&It->GetWorld()->IsGameWorld()){World=*It;break;}
        if(!World.IsValid()||!World->GetLocalPlayer()||!World->GetInventory().bKnown)return;
        B=World->GetGameInstance()->GetSubsystem<UReal33DBridge>();if(!B.IsValid()||!B->IsRunning())return;
        TArray<TSharedRef<SWidget>> N;
        for(const auto& W:FSlateApplication::Get().GetTopLevelWindows())Find(W,"SReal33DHUD",N);
        if(N.Num()!=1)return;HUD=StaticCastSharedRef<SReal33DHUD>(N[0]);Chat=HUD->GetChatPanel();
        N.Reset();Find(HUD.ToSharedRef(),"SReal33DTradePanel",N);if(!Require(N.Num()==1,TEXT("actual HUD TradePanel")))return;BT=StaticCastSharedRef<SReal33DTradePanel>(N[0]);
        N.Reset();Find(HUD.ToSharedRef(),"SReal33DChannelsPanel",N);if(!Require(N.Num()==1,TEXT("actual Channels widget")))return;Channels=StaticCastSharedRef<SReal33DChannelsPanel>(N[0]);
        N.Reset();Find(HUD.ToSharedRef(),"SReal33DEditorWindow",N);if(!Require(N.Num()==1,TEXT("actual editor widget")))return;Editor=StaticCastSharedRef<SReal33DEditorWindow>(N[0]);
        auto& R=HUD->GetPanels();if(!Require(R.Num()==9,TEXT("nine registered HUD surfaces")))return;
        TArray<FName> Ids;R.GetIds(Ids);
        for(FName Id:Ids){bool Open=R.IsOpen(Id);R.Close(Id);if(!Require(!R.IsOpen(Id),TEXT("registry closes")))return;R.Open(Id);R.Toggle(Id);R.Toggle(Id);if(!Require(R.IsOpen(Id),TEXT("registry opens and toggles")))return;R.SetOpen(Id,Open);UE_LOG(LogTemp,Display,TEXT("CERT_PANEL %s"),*Id.ToString());}
        if(!Require(!B->GetTrade().bOpen&&!BT->IsShowing(),TEXT("no invented initial offer")))return;
        BId=World->GetLocalPlayer()->GetCreatureId();BName=World->GetLocalPlayer()->GetCreatureName();
        FReal33DConnectionConfig C;FParse::Value(FCommandLine::Get(),TEXT("-real33d-runtime="),C.RuntimeDir);C.Account=TEXT("A");
        A=NewObject<UReal33DBridge>(World->GetGameInstance());A->AddToRoot();A->Initialize(Peers);
        if(!Require(A->Connect(C),TEXT("second real account connection")))return;AT=SNew(SReal33DTradePanel).Bridge(A);Next();return;
    }
    if(!B.IsValid()||!A)return;
    if(Step==1)
    {
        if(!AI.bKnown||!AId||AName.IsEmpty())return;
        AReal33DCreature* Peer=nullptr;for(TObjectIterator<AReal33DCreature> It;It;++It)if(!It->HasAnyFlags(RF_ClassDefaultObject)&&It->GetCreatureId()==AId){Peer=*It;break;}
        if(!Peer)return;const auto P=Peer->GetLogicalPosition(),Q=World->GetLocalPlayer()->GetLogicalPosition();
        if(FMath::Abs(int(P.X)-int(Q.X))>2||FMath::Abs(int(P.Y)-int(Q.Y))>2){if(!FollowSent){B->RequestFollow(AId);FollowSent=true;}return;}
        if(FollowSent)B->RequestCancelCombat();
        if(Resume)
        {
            // These types are the recorded received baseline of this same
            // certification's preceding run, not invented current ownership.
            // Step 2 checks their CURRENT location from both real connections.
            OriginalA=3561;OriginalB=3294;Round=3;
        }
        else
        {
            if(!Require(AI.bOccupied[5]&&World->GetInventory().bOccupied[5],TEXT("actual initial single offer items")))return;
            OriginalA=AI.Items[5].TypeId;OriginalB=World->GetInventory().Items[5].TypeId;
        }
        if(!Require(OriginalA&&OriginalB&&OriginalA!=OriginalB,TEXT("distinct received offer types")))return;
        auto BagB=Ref(World->GetInventory(),World->GetContainers(),World->GetInventory().Items[3].TypeId);
        if(!Require(Right(BagB),TEXT("plain right click opens actual backpack")))return;
        A->RequestUseObject(UReal33DBridge::FMoveSlot::InInventory(3),AI.Items[3].TypeId,0,0);
        if(!Resume)
        {
            if(!Require(Right(Ref(World->GetInventory(),World->GetContainers(),OriginalB),false,false,true),TEXT("Alt right click Look gesture")))return;
            A->RequestLookAtPoint(UReal33DBridge::FMoveSlot::InInventory(5));
        }
        Next();return;
    }
    if(Step==2)
    {
        if(AC.Num()!=1||World->GetContainers().Num()!=1)return;
        if(Resume)
        {
            if(!Require(Ownership(true),TEXT("resume confirms previously transferred ownership from fresh server snapshots")))return;
            if(!Capture(TEXT("CertResume")))return;Next();return;
        }
        if(ALook.IsEmpty()||LastLook().IsEmpty())return;
        BLook=LastLook();UE_LOG(LogTemp,Display,TEXT("CERT_LOOK_BASELINE A=%s B=%s"),*ALook,*BLook);
        if(!Require(Ownership(false),TEXT("received inventory plus backpack baseline has exactly one of each")))return;
        if(!Capture(TEXT("CertBaseline")))return;Next();return;
    }
    const uint16 BType=Round==0?OriginalB:OriginalA,AType=Round==0?OriginalA:OriginalB;
    if(Step==3)
    {
        BClose=B->GetStats().TradeClosesReceived;AClose=A->GetStats().TradeClosesReceived;
        UsesBefore=B->GetStats().UsesRequested;OffersBefore=B->GetStats().TradeOffersRequested;
        if(!Require(!A->GetTrade().bOpen&&!B->GetTrade().bOpen,TEXT("both real trade models idle")))return;
        if(!Require(Right(Ref(World->GetInventory(),World->GetContainers(),BType),true),TEXT("Ctrl right click real item slot")))return;
        if(!Require(B->GetStats().TradeOffersRequested==OffersBefore&&B->GetStats().UsesRequested==UsesBefore,TEXT("context menu sends neither Use nor Trade")))return;
        auto Popup=FSlateApplication::Get().GetVisibleMenuWindow();
        UE_LOG(LogTemp,Display,TEXT("CERT_MENU visible=%d text=%s"),FSlateApplication::Get().AnyMenusVisible(),Popup?*Painted(Popup.ToSharedRef()):TEXT("<no window>"));
        bool Found=Popup&&Menu(Popup.ToSharedRef());
        if(!Found)for(const auto& W:FSlateApplication::Get().GetTopLevelWindows())if(Menu(W)){Found=true;break;}
        if(!Require(Found&&HUD->IsTargeting(),TEXT("actual Trade with menu arms target")))return;
        Next(5);return;
    }
    if(Step==4)
    {
        if(!Require(B->GetStats().TradeOffersRequested==OffersBefore&&B->GetStats().UsesRequested==UsesBefore,TEXT("context menu sends neither Use nor Trade")))return;
        if(!Capture(TEXT("CertMenu")))return;
        bool Found=false;for(const auto& W:FSlateApplication::Get().GetTopLevelWindows())if(Menu(W)){Found=true;break;}
        if(!Require(Found&&HUD->IsTargeting(),TEXT("actual Trade with menu arms target")))return;Next();return;
    }
    if(Step==5)
    {
        if(!Require(B->GetStats().TradeOffersRequested==OffersBefore&&!B->GetTrade().bOpen,TEXT("target selection has no optimistic offer")))return;
        TArray<TSharedRef<SWidget>> N;Find(HUD.ToSharedRef(),"SReal33DBattlePanel",N);
        if(!Require(N.Num()==1&&Click(N[0],AName),TEXT("actual player button completes Trade target")))return;
        if(!Require(!HUD->IsTargeting()&&!B->GetTrade().bOpen,TEXT("request clears targeting but waits for server offer")))return;Next();return;
    }
    if(Step==6)
    {
        const auto& T=B->GetTrade();if(!T.Own.bKnown)return;
        if(!Require(T.bOpen&&T.Own.Name==BName&&T.Own.Items.Num()==1&&T.Own.Items[0].TypeId==BType&&!T.Partner.bKnown&&!A->GetTrade().bOpen,TEXT("server own offer; partner prose is not invented offer")))return;
        if(!Require(Painted(BT.ToSharedRef()).Contains(BName)&&Painted(BT.ToSharedRef()).Contains(FString::FromInt(BType)),TEXT("own received name and item painted")))return;
        if(!Capture(TEXT("CertOwn")))return;
        auto R=Ref(AI,AC,AType);if(!Require(R.IsValid()&&A->RequestTradeOffer(Address(R),AType,R.Kind==FReal33DSlotRef::EKind::Inventory?0:R.Slot,BId)>0,TEXT("peer offers its received item normally")))return;Next();return;
    }
    if(Step==7)
    {
        const auto& T=B->GetTrade();const auto& U=A->GetTrade();if(!T.Partner.bKnown||!U.Own.bKnown||!U.Partner.bKnown)return;
        if(!Require(T.Partner.Name==AName&&T.Partner.Items.Num()==1&&T.Partner.Items[0].TypeId==AType&&U.Partner.Items[0].TypeId==BType,TEXT("both reciprocal server offers")))return;
        if(!Require(Painted(BT.ToSharedRef()).Contains(AName)&&Painted(BT.ToSharedRef()).Contains(FString::FromInt(AType)),TEXT("partner name and item painted")))return;
        HUD->GetPanels().Close(Real33D::PanelIds::Trade);BT->Refresh();HUD->GetPanels().Open(Real33D::PanelIds::Trade);
        if(!Require(BT->IsShowing()&&B->GetTrade().Partner.bKnown,TEXT("registry hide reopen preserves received trade")))return;
        if(!Capture(TEXT("CertBoth")))return;
        if(Round==0){InspectBefore=B->GetStats().TradeInspectsRequested;LookMessagesBefore=LookCount();if(!Require(Click(BT.ToSharedRef(),TEXT("Look"),0),TEXT("own actual offer Look button")))return;Next();}
        else Next(10);return;
    }
    if(Step==8)
    {
        if(B->GetStats().TradeInspectsRequested<InspectBefore+1||LookCount()<=LookMessagesBefore)return;
        if(!Require(FirstSentence(LastLook())==FirstSentence(BLook),TEXT("own inspect received matching server description")))return;
        LookMessagesBefore=LookCount();if(!Require(Click(BT.ToSharedRef(),TEXT("Look"),1),TEXT("partner actual offer Look button")))return;Next();return;
    }
    if(Step==9)
    {
        if(B->GetStats().TradeInspectsRequested<InspectBefore+2||LookCount()<=LookMessagesBefore||FirstSentence(LastLook())!=FirstSentence(ALook))return;
        if(!Require(true,TEXT("partner inspect received matching server description")))return;
        if(!Capture(TEXT("CertInspect")))return;Next();return;
    }
    if(Step==10)
    {
        if(Round==1){if(!Require(Click(BT.ToSharedRef(),TEXT("Reject"))&&!B->GetTrade().bOpen,TEXT("B Reject actual button clears locally")))return;Next(13);return;}
        if(Round==2){if(!Require(Click(AT.ToSharedRef(),TEXT("Reject"))&&!A->GetTrade().bOpen,TEXT("A Reject actual peer button")))return;Next(13);return;}
        if(!Require(Click(BT.ToSharedRef(),TEXT("Accept"))&&B->GetTrade().bOpen&&B->GetTrade().bAcceptSent,TEXT("actual Accept records pending without completion")))return;Next();return;
    }
    if(Step==11)
    {
        if(Now-StageStart<4)return;
        if(!Require(B->GetTrade().bOpen&&A->GetTrade().bOpen&&!A->GetTrade().bAcceptSent&&Ownership(Round!=0)&&B->GetStats().TradeClosesReceived==BClose,TEXT("one accept waits; received ownership unchanged")))return;
        if(!Capture(TEXT("CertWaiting")))return;
        if(!Require(Click(AT.ToSharedRef(),TEXT("Accept")),TEXT("second real account actual Accept button")))return;Next();return;
    }
    if(Step==12)
    {
        if(A->GetTrade().bOpen||B->GetTrade().bOpen||!Ownership(Round==0))return;
        if(!Require(A->GetStats().TradeClosesReceived==AClose+1&&B->GetStats().TradeClosesReceived==BClose+1,TEXT("both close received")))return;
        if(!Require(Ownership(Round==0),Round==0?TEXT("AUTHORITATIVE TRANSFER: both owners changed in received inventory/container deltas"):TEXT("reciprocal trade restores original ownership")))return;
        if(!Capture(TEXT("CertTransfer")))return;
        if(Round==0){Round=1;Next(3);}else Next(14);return;
    }
    if(Step==13)
    {
        if(A->GetTrade().bOpen||B->GetTrade().bOpen)return;
        if(!Require(Ownership(true),TEXT("cancel preserves authoritative item ownership")))return;
        if(!Require(B->GetStats().TradeClosesReceived==BClose+(Round==2?1:0)&&A->GetStats().TradeClosesReceived==AClose+(Round==1?1:0),TEXT("only partner receives CLOSE_TRADE on reject")))return;
        if(!Capture(TEXT("CertCancel")))return;++Round;Next(3);return;
    }
    if(Step==14)
    {
        OffersBefore=B->GetStats().TradeOffersRequested;UsesBefore=B->GetStats().UsesRequested;
        auto R=Ref(World->GetInventory(),World->GetContainers(),3003);
        if(!Require(R.IsValid()&&Right(R,false,true),TEXT("Shift right click existing UseWith gesture")))return;
        if(!Require(HUD->IsTargeting(),TEXT("UseWith arms target")))return;
        TArray<TSharedRef<SWidget>> N;Find(HUD.ToSharedRef(),"SReal33DBattlePanel",N);
        if(!Require(N.Num()==1&&Click(N[0],AName)&&!HUD->IsTargeting(),TEXT("UseWith targets real player")))return;Next();return;
    }
    if(Step==15)
    {
        if(B->GetStats().UsesRequested<UsesBefore+1)return;
        if(!Require(B->GetStats().TradeOffersRequested==OffersBefore,TEXT("UseWith still routes to Use; no trade command")))return;
        if(!Require(Click(Channels.ToSharedRef(),TEXT("Channel List")),TEXT("Channels list actual button")))return;Next();return;
    }
    if(Step==16)
    {
        if(!B->IsChannelListKnown())return;
        if(!Require(RowClick(4,TEXT("Join"))&&!B->IsChannelJoined(4),TEXT("offered channel Join waits for server")))return;Next();return;
    }
    if(Step==17)
    {
        if(!B->IsChannelJoined(4))return;
        if(!Require(Select(EReal33DChatViewKind::Channel,4)&&Send(ChannelToken),TEXT("actual channel tab and Send")))return;Next();return;
    }
    if(Step==18)
    {
        if(!Has(View(EReal33DChatViewKind::Channel,4),ChannelToken))return;
        if(!Require(!Has(View(EReal33DChatViewKind::Local),ChannelToken)&&Painted(Chat.ToSharedRef()).Contains(ChannelToken),TEXT("channel real echo isolated and painted")))return;
        if(!Capture(TEXT("CertChannel")))return;
        if(!Require(Select(EReal33DChatViewKind::Local)&&Send(LocalToken),TEXT("Local Chat Send")))return;Next();return;
    }
    if(Step==19)
    {
        if(!Has(View(EReal33DChatViewKind::Local),LocalToken))return;
        if(!Require(!Has(View(EReal33DChatViewKind::Channel,4),LocalToken)&&Painted(Chat.ToSharedRef()).Contains(LocalToken),TEXT("Local Chat real echo isolated and painted")))return;
        if(!Require(RowClick(4,TEXT("Leave")),TEXT("actual channel Leave")))return;
        if(!Require(Right(Ref(World->GetInventory(),World->GetContainers(),2815)),TEXT("plain right click existing scroll")))return;Next();return;
    }
    if(Step==20)
    {
        const auto& E=B->GetEditor();if(!E.bOpen)return;
        TArray<TSharedRef<SWidget>> N;Find(Editor.ToSharedRef(),"SMultiLineEditableTextBox",N);
        if(!Require(E.bIsText&&E.TypeId==2815&&Editor->IsShowing()&&N.Num()==1&&StaticCastSharedRef<SMultiLineEditableTextBox>(N[0])->GetText().ToString()==E.Text,TEXT("text editor renders exact server body")))return;
        if(!Capture(TEXT("CertEditor")))return;
        if(!Require(Click(Editor.ToSharedRef(),TEXT("Close"))&&!B->GetEditor().bOpen,TEXT("text editor Close")))return;Next();return;
    }
    if(Step==21)
    {
        if(Now-StageStart<4)return;
        for(auto* X:{B.Get(),A}){auto S=X->GetStats();if(!Require(S.ResidualBytes==0&&S.UnsupportedOpcodes==0&&S.Anomalies==0,TEXT("zero protocol residual unsupported anomalies")))return;}
        if(!Require(!A->GetTrade().bOpen&&!B->GetTrade().bOpen&&Ownership(false),TEXT("final original ownership and idle trade")))return;
        if(!Capture(TEXT("CertFinal")))return;Finish(true);return;
    }
}
struct FInstall
{
    FDelegateHandle Handle;
    FInstall(){if(FParse::Param(FCommandLine::Get(),TEXT("real33d-cert-trade")))Handle=FCoreDelegates::OnEndFrame.AddStatic(&Tick);}
    ~FInstall(){if(Handle.IsValid())FCoreDelegates::OnEndFrame.Remove(Handle);}
} Install;
}
#endif
