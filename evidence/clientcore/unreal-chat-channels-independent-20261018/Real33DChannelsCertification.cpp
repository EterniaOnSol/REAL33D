// Temporary independent live UI test. Not shipped. All state comes from Fusion32.
// SimulateClick invokes the production Slate button callback; no private access,
// injected events, server edits, packet fabrication or implementation replacement.
#include "Real33DBridge.h"
#include "Real33DHUD.h"
#include "Real33DChannelsPanel.h"
#include "Real33DChatPanel.h"
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
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace ChannelsCert
{
TWeakObjectPtr<UReal33DBridge> B;
UReal33DBridge* A = nullptr;
TWeakObjectPtr<AReal33DWorld> World;
TSharedPtr<SReal33DHUD> HUD;
TSharedPtr<SReal33DChannelsPanel> BC, AC;
TSharedPtr<SReal33DChatPanel> BT, AT;
FSubsystemCollection<UGameInstanceSubsystem> PeerCollection;
FString AName, BName;
int32 Step = 0;
double Began = 0, StageBegan = 0, LastTick = 0;
bool Finished = false;
bool StageCaptured = false;
int32 PeerChannelBeforeLeave = 0;
const FString GameText = TEXT("CCERT20261018 B GAME");
const FString RLText = TEXT("CCERT20261018 B RL");
const FString LocalText = TEXT("CCERT20261018 B LOCAL");
const FString BA = TEXT("CCERT20261018 PRIVATE B TO A");
const FString AB = TEXT("CCERT20261018 PRIVATE A TO B");
const FString AfterLeave = TEXT("CCERT20261018 AFTER A LEAVE");
const FString AfterTab = TEXT("CCERT20261018 AFTER A TAB CLOSE");

void Find(const TSharedRef<SWidget>& Root, FName Type, TArray<TSharedRef<SWidget>>& Out)
{
    if (Root->GetType() == Type) Out.Add(Root);
    auto* Children = Root->GetChildren();
    for (int32 I=0; I<Children->Num(); ++I) Find(Children->GetChildAt(I), Type, Out);
}
TArray<FString> Texts(const TSharedRef<SWidget>& Root)
{
    TArray<TSharedRef<SWidget>> Nodes; Find(Root, "STextBlock", Nodes);
    TArray<FString> Result;
    for (const auto& N:Nodes) Result.Add(StaticCastSharedRef<STextBlock>(N)->GetText().ToString());
    return Result;
}
void Finish(bool Good)
{
    if (Finished) return;
    Finished=true;
    UE_LOG(LogTemp, Display, TEXT("CHANNELS_CERT_RESULT=%s step=%d"), Good?TEXT("PASS"):TEXT("FAILED"), Step);
    if (World.IsValid()) World->WriteEvidence(Good?TEXT("CertComplete"):TEXT("CertFailed"));
    if (A) { A->Deinitialize(); A->RemoveFromRoot(); A=nullptr; }
    FPlatformMisc::RequestExitWithStatus(false, Good?0:1);
}
bool Require(bool Good, const TCHAR* Name)
{
    UE_LOG(LogTemp, Display, TEXT("CERT_ASSERT %s %s"), Good?TEXT("PASS"):TEXT("FAILED"), Name);
    if (!Good) Finish(false);
    return Good;
}
bool ClickText(const TSharedRef<SWidget>& Root, const FString& Text)
{
    TArray<TSharedRef<SWidget>> Buttons; Find(Root,"SButton",Buttons);
    for (const auto& N:Buttons)
        if (Texts(N).Contains(Text)) { StaticCastSharedRef<SButton>(N)->SimulateClick(); return true; }
    return false;
}
bool RowClick(const TSharedRef<SWidget>& Panel, int32 Id, const FString& Button)
{
    TArray<TSharedRef<SWidget>> Rows; Find(Panel,"SHorizontalBox",Rows);
    for (const auto& Row:Rows)
    {
        const auto T=Texts(Row);
        if (T.Num()==2 && T[0].StartsWith(FString::FromInt(Id)+TEXT("  ")) && T[1]==Button)
            return ClickText(Row,Button);
    }
    return false;
}
uint32 View(UReal33DBridge* Bridge, EReal33DChatViewKind Kind, int32 Channel=0)
{
    TArray<FReal33DChatView> Views; Bridge->GetChatViews(Views);
    for (const auto& V:Views) if (V.Kind==Kind && (Kind!=EReal33DChatViewKind::Channel || V.Channel==Channel)) return V.ViewId;
    return 0;
}
bool Has(UReal33DBridge* Bridge, uint32 Id, const FString& Token)
{
    TArray<FReal33DChatLine> Lines; Bridge->GetChatTranscript(Id,Lines);
    for (const auto& L:Lines) if (L.Line.Contains(Token)) return true;
    return false;
}
bool Visible(const TSharedRef<SWidget>& Panel, const FString& Token)
{
    for (const auto& T:Texts(Panel)) if (T.Contains(Token)) return true;
    return false;
}
bool Select(UReal33DBridge* Bridge, const TSharedRef<SWidget>& Panel, EReal33DChatViewKind Kind, int32 Channel=0)
{
    TArray<FReal33DChatView> Views; Bridge->GetChatViews(Views);
    for (const auto& V:Views)
        if (V.Kind==Kind && (Kind!=EReal33DChatViewKind::Channel || V.Channel==Channel))
            return ClickText(Panel,SReal33DChatPanel::TabLabel(V).ToString());
    return false;
}
bool Send(const TSharedRef<SReal33DChatPanel>& Panel, const FString& Text)
{
    auto Input=StaticCastSharedPtr<SEditableTextBox>(Panel->GetInputWidget());
    if (!Input.IsValid()) return false;
    Input->SetText(FText::FromString(Text));
    return ClickText(Panel,TEXT("Send"));
}
bool OpenPrivate(const TSharedRef<SReal33DChannelsPanel>& Panel, const FString& Name)
{
    TArray<TSharedRef<SWidget>> Inputs; Find(Panel,"SEditableTextBox",Inputs);
    if (Inputs.Num()!=1) return false;
    StaticCastSharedRef<SEditableTextBox>(Inputs[0])->SetText(FText::FromString(Name));
    return ClickText(Panel,TEXT("Message"));
}
bool CloseTab(const TSharedRef<SWidget>& Panel, const FString& Name)
{
    TArray<TSharedRef<SWidget>> Buttons; Find(Panel,"SButton",Buttons);
    for (const auto& Tab:Buttons)
    {
        if (!Texts(Tab).Contains(Name)) continue;
        TArray<TSharedRef<SWidget>> Children; Find(Tab,"SButton",Children);
        for (const auto& C:Children)
            if (C!=Tab && Texts(C).Num()==0) { StaticCastSharedRef<SButton>(C)->SimulateClick(); return true; }
    }
    return false;
}
FString ChannelName(UReal33DBridge* Bridge,int32 Channel)
{
    TArray<FReal33DChannelEntry> List; Bridge->GetChannelList(List);
    for (const auto& E:List) if (!E.bOwnChannelOffer && E.Id==Channel) return E.Name;
    return FString();
}
void Dump(UReal33DBridge* Bridge,const TCHAR* Who)
{
    const auto S=Bridge->GetStats();
    UE_LOG(LogTemp,Display,TEXT("CERT_COUNTERS %s frames=%d commands=%d residual=%d unsupported=%d anomalies=%d lists=%d/%d joins=%d opens=%d leaves=%d channel_send=%d channel_receive=%d private_send=%d private_receive=%d"),
        Who,S.Frames,S.Commands,S.ResidualBytes,S.UnsupportedOpcodes,S.Anomalies,S.ChannelListsRequested,S.ChannelListsReceived,S.ChannelJoinsRequested,S.ChannelsOpened,S.ChannelLeavesRequested,S.ChannelSaysRequested,S.ChannelTalkReceived,S.PrivateSaysRequested,S.PrivateTalkReceived);
    TArray<FReal33DChatView> Views; Bridge->GetChatViews(Views);
    for (const auto& V:Views)
    {
        UE_LOG(LogTemp,Display,TEXT("CERT_VIEW %s id=%u kind=%d channel=%d name=%s selected=%d"),Who,V.ViewId,static_cast<int32>(V.Kind),V.Channel,*V.Name,Bridge->GetSelectedChatView()==V.ViewId);
        TArray<FReal33DChatLine> Lines; Bridge->GetChatTranscript(V.ViewId,Lines);
        for (const auto& L:Lines) UE_LOG(LogTemp,Display,TEXT("CERT_LINE %s view=%u %s"),Who,V.ViewId,*L.Line);
    }
}
void Advance()
{
    Dump(B.Get(),TEXT("B")); Dump(A,TEXT("A"));
    UE_LOG(LogTemp,Display,TEXT("CERT_STAGE_COMPLETE %d"),Step);
    ++Step; StageBegan=FPlatformTime::Seconds(); StageCaptured=false;
}
void Tick()
{
    if (Finished) return;
    const double Now=FPlatformTime::Seconds();
    if (Began==0) { Began=Now; StageBegan=Now; }
    if (Now-Began>240 || Now-StageBegan>60) { Require(false,TEXT("live stage deadline")); return; }
    if (A)
    {
        TArray<FReal33DEvent> Events; A->DrainEvents(Events);
        for (const auto& E:Events)
        {
            if (E.Kind==EReal33DEventKind::Connected) AName=E.CreatureName;
            if (E.Kind==EReal33DEventKind::ChannelList || E.Kind==EReal33DEventKind::ChannelOpened || E.Kind==EReal33DEventKind::PrivateChannelOpened || E.Kind==EReal33DEventKind::Talk)
                UE_LOG(LogTemp,Display,TEXT("CERT_PEER_EVENT kind=%d channel=%d name=%s speaker=%s detail=%s"),static_cast<int32>(E.Kind),E.Channel,*E.ChannelName,*E.Speaker,*E.Detail);
        }
        AC->Tick(FGeometry(),Now,0.1f); AT->Tick(FGeometry(),Now,0.1f);
    }
    if (Now-LastTick<1.5) return;
    LastTick=Now;
    if (Step==0)
    {
        if (!FSlateApplication::IsInitialized()) return;
        for (TObjectIterator<AReal33DWorld> It; It; ++It)
            if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->GetWorld() && It->GetWorld()->IsGameWorld()) { World=*It; break; }
        if (!World.IsValid() || !World->GetLocalPlayer()) return;
        B=World->GetGameInstance()->GetSubsystem<UReal33DBridge>();
        if (!B.IsValid() || !B->IsRunning() || !World->GetPlayerVitals().bKnown) return;
        TArray<TSharedRef<SWidget>> Nodes;
        for (const auto& W:FSlateApplication::Get().GetTopLevelWindows()) Find(W,"SReal33DHUD",Nodes);
        if (Nodes.Num()!=1) return;
        HUD=StaticCastSharedRef<SReal33DHUD>(Nodes[0]); BT=HUD->GetChatPanel();
        Nodes.Reset(); Find(HUD.ToSharedRef(),"SReal33DChannelsPanel",Nodes);
        if (!Require(Nodes.Num()==1,TEXT("one live Channels widget"))) return;
        BC=StaticCastSharedRef<SReal33DChannelsPanel>(Nodes[0]);
        BName=World->GetLocalPlayer()->GetCreatureName();
        if (!Require(HUD->GetPanels().Num()==7,TEXT("registry seven real HUD panels"))) return;
        TArray<FName> Ids; HUD->GetPanels().GetIds(Ids);
        for (FName Id:Ids)
        {
            HUD->GetPanels().Close(Id);
            if (!Require(!HUD->GetPanels().IsOpen(Id),TEXT("registry closes real window"))) return;
            HUD->GetPanels().Open(Id); HUD->GetPanels().Toggle(Id); HUD->GetPanels().Toggle(Id);
            if (!Require(HUD->GetPanels().IsOpen(Id),TEXT("registry reopens and toggles real window"))) return;
        }
        FReal33DConnectionConfig Config;
        FParse::Value(FCommandLine::Get(),TEXT("-real33d-runtime="),Config.RuntimeDir);
        Config.Account=TEXT("A");
        A=NewObject<UReal33DBridge>(World->GetGameInstance()); A->AddToRoot(); A->Initialize(PeerCollection);
        if (!Require(A->Connect(Config),TEXT("independent real peer A connection"))) return;
        AC=SNew(SReal33DChannelsPanel).Bridge(A);
        AT=SNew(SReal33DChatPanel).Bridge(A).Embedded(true);
        if (!Require(!B->IsChannelListKnown(),TEXT("no invented initial channel list"))) return;
        if (!Require(ClickText(BC.ToSharedRef(),TEXT("Channel List")),TEXT("B actual Channel List button"))) return;
        Advance(); return;
    }
    if (!B.IsValid() || !A) return;
    const uint32 BL=View(B.Get(),EReal33DChatViewKind::Local), AL=View(A,EReal33DChatViewKind::Local);
    const uint32 BG=View(B.Get(),EReal33DChatViewKind::Channel,4), AG=View(A,EReal33DChatViewKind::Channel,4);
    const uint32 BR=View(B.Get(),EReal33DChatViewKind::Channel,6), AR=View(A,EReal33DChatViewKind::Channel,6);
    const uint32 BP=View(B.Get(),EReal33DChatViewKind::Private), AP=View(A,EReal33DChatViewKind::Private);
    if (Step==1)
    {
        if (AName.IsEmpty() || !B->IsChannelListKnown()) return;
        if (!A->IsChannelListKnown()) { ClickText(AC.ToSharedRef(),TEXT("Channel List")); return; }
        TArray<FReal33DChannelEntry> List; B->GetChannelList(List);
        for (const auto& E:List)
            if (!Require(Visible(BC.ToSharedRef(),SReal33DChannelsPanel::RowLabel(E).ToString()),TEXT("actual list row matches received server name"))) return;
        if (!Require(!ChannelName(B.Get(),4).IsEmpty() && !ChannelName(B.Get(),6).IsEmpty(),TEXT("server offered test channels4and6"))) return;
        if (!Require(RowClick(BC.ToSharedRef(),4,TEXT("Join")) && RowClick(AC.ToSharedRef(),4,TEXT("Join")),TEXT("both actual Join buttons"))) return;
        if (!Require(!B->IsChannelJoined(4) && !A->IsChannelJoined(4),TEXT("join request alone creates no tab"))) return;
        Advance(); return;
    }
    if (Step==2)
    {
        if (!BG || !AG) return;
        if (!Require(B->GetStats().ChannelsOpened==1 && A->GetStats().ChannelsOpened==1,TEXT("OPEN_CHANNEL received before joined tabs"))) return;
        if (!Require(Visible(BT.ToSharedRef(),ChannelName(B.Get(),4)),TEXT("real joined tab visible"))) return;
        if (!Require(RowClick(BC.ToSharedRef(),6,TEXT("Join")) && RowClick(AC.ToSharedRef(),6,TEXT("Join")),TEXT("second channel actual joins"))) return;
        Advance(); return;
    }
    if (Step==3)
    {
        if (!BR || !AR) return;
        if (!Require(Select(B.Get(),BT.ToSharedRef(),EReal33DChatViewKind::Channel,4) && Send(BT.ToSharedRef(),GameText),TEXT("B selects game tab and sends through real Send button"))) return;
        if (!Require(!Has(B.Get(),BG,GameText),TEXT("no optimistic sent channel echo"))) return;
        Advance(); return;
    }
    if (Step==4)
    {
        if (!Has(B.Get(),BG,GameText) || !Has(A,AG,GameText)) return;
        if (!Require(Visible(BT.ToSharedRef(),GameText) && !Has(B.Get(),BL,GameText) && !Has(B.Get(),BR,GameText),TEXT("server channel echo rendered only in game tab"))) return;
        if (!StageCaptured) { World->WriteEvidence(TEXT("CertGame")); StageCaptured=true; return; }
        if (!Require(Select(B.Get(),BT.ToSharedRef(),EReal33DChatViewKind::Channel,6) && Send(BT.ToSharedRef(),RLText),TEXT("B selects RL tab and sends"))) return;
        Advance(); return;
    }
    if (Step==5)
    {
        if (!Has(B.Get(),BR,RLText) || !Has(A,AR,RLText)) return;
        if (!Require(Visible(BT.ToSharedRef(),RLText) && !Visible(BT.ToSharedRef(),GameText) && !Has(B.Get(),BG,RLText) && !Has(B.Get(),BL,RLText),TEXT("separate real channel transcripts and tabs"))) return;
        if (!StageCaptured) { World->WriteEvidence(TEXT("CertRL")); StageCaptured=true; return; }
        if (!Require(Select(B.Get(),BT.ToSharedRef(),EReal33DChatViewKind::Local) && Send(BT.ToSharedRef(),LocalText),TEXT("Local Chat real Send button"))) return;
        Advance(); return;
    }
    if (Step==6)
    {
        if (!Has(B.Get(),BL,LocalText)) return;
        if (!Require(Visible(BT.ToSharedRef(),LocalText) && !Visible(BT.ToSharedRef(),GameText) && !Visible(BT.ToSharedRef(),RLText) && !Has(B.Get(),BG,LocalText) && !Has(B.Get(),BR,LocalText),TEXT("Local Chat isolation live"))) return;
        if (!StageCaptured) { World->WriteEvidence(TEXT("CertLocal")); StageCaptured=true; return; }
        if (!Require(OpenPrivate(BC.ToSharedRef(),AName),TEXT("private name and Message button"))) return;
        if (!Require(View(B.Get(),EReal33DChatViewKind::Private)==0,TEXT("private open waits for server"))) return;
        Advance(); return;
    }
    if (Step==7)
    {
        if (!BP || B->GetStats().PrivateChannelsOpened==0) return;
        if (!Require(Select(B.Get(),BT.ToSharedRef(),EReal33DChatViewKind::Private) && Send(BT.ToSharedRef(),BA),TEXT("B to A real private Send"))) return;
        Advance(); return;
    }
    if (Step==8)
    {
        if (!AP || !Has(A,AP,BA)) return;
        if (!Require(!Has(A,AL,BA) && !Has(A,AG,BA) && !Has(B.Get(),BP,BA),TEXT("private delivered only to recipient; sender has no fabricated echo"))) return;
        if (!Require(Select(A,AT.ToSharedRef(),EReal33DChatViewKind::Private) && Send(AT.ToSharedRef(),AB),TEXT("A to B real peer private Send"))) return;
        Advance(); return;
    }
    if (Step==9)
    {
        if (!Has(B.Get(),BP,AB)) return;
        if (!Require(Visible(BT.ToSharedRef(),AB) && !Has(B.Get(),BL,AB) && !Has(B.Get(),BG,AB) && !Has(A,AP,AB),TEXT("private A to B rendered in private tab only"))) return;
        if (!StageCaptured) { World->WriteEvidence(TEXT("CertPrivate")); StageCaptured=true; return; }
        PeerChannelBeforeLeave=A->GetStats().ChannelTalkReceived;
        if (!Require(RowClick(AC.ToSharedRef(),4,TEXT("Leave")) && !A->IsChannelJoined(4),TEXT("peer real Leave button removes channel"))) return;
        Advance(); return;
    }
    if (Step==10)
    {
        if (A->GetStats().ChannelLeavesRequested!=1) return;
        if (!Require(Select(B.Get(),BT.ToSharedRef(),EReal33DChatViewKind::Channel,4) && Send(BT.ToSharedRef(),AfterLeave),TEXT("post-leave real channel message"))) return;
        Advance(); return;
    }
    if (Step==11)
    {
        if (!Has(B.Get(),BG,AfterLeave) || Now-StageBegan<4) return;
        bool NoPeerMessage=true;
        TArray<FReal33DChatView> Views; A->GetChatViews(Views);
        for (const auto& V:Views) NoPeerMessage &= !Has(A,V.ViewId,AfterLeave);
        if (!Require(NoPeerMessage && A->GetStats().ChannelTalkReceived==PeerChannelBeforeLeave,TEXT("Fusion32 stopped delivery after Leave button"))) return;
        if (!Require(RowClick(BC.ToSharedRef(),4,TEXT("Leave")) && !B->IsChannelJoined(4),TEXT("primary live Leave button"))) return;
        if (!Require(Select(B.Get(),BT.ToSharedRef(),EReal33DChatViewKind::Channel,6) && Select(A,AT.ToSharedRef(),EReal33DChatViewKind::Channel,6),TEXT("select RL before tab close"))) return;
        if (!Require(CloseTab(AT.ToSharedRef(),ChannelName(A,6)) && !A->IsChannelJoined(6),TEXT("peer real tab close removes joined channel"))) return;
        Advance(); return;
    }
    if (Step==12)
    {
        if (A->GetStats().ChannelLeavesRequested!=2) return;
        if (!Require(Send(BT.ToSharedRef(),AfterTab),TEXT("post-tab-close real RL channel message"))) return;
        Advance(); return;
    }
    if (Step==13)
    {
        if (!Has(B.Get(),BR,AfterTab) || Now-StageBegan<4) return;
        bool NoPeerMessage=true;
        TArray<FReal33DChatView> Views; A->GetChatViews(Views);
        for (const auto& V:Views) NoPeerMessage &= !Has(A,V.ViewId,AfterTab);
        if (!Require(NoPeerMessage && A->GetStats().ChannelTalkReceived==PeerChannelBeforeLeave,TEXT("Fusion32 stopped delivery after tab close"))) return;
        if (!Require(CloseTab(BT.ToSharedRef(),ChannelName(B.Get(),6)) && !B->IsChannelJoined(6),TEXT("primary real tab close removes joined channel"))) return;
        Advance(); return;
    }
    if (Step==14)
    {
        if (B->GetStats().ChannelLeavesRequested!=2 || A->GetStats().ChannelLeavesRequested!=2) return;
        if (!Require(B->GetSelectedChatView()==BL && A->GetSelectedChatView()==AL && !B->CloseChatView(BL),TEXT("tab close returns to permanent Local Chat"))) return;
        const auto BS=B->GetStats(), AS=A->GetStats();
        if (!Require(BS.ResidualBytes==0 && BS.UnsupportedOpcodes==0 && BS.Anomalies==0 && AS.ResidualBytes==0 && AS.UnsupportedOpcodes==0 && AS.Anomalies==0,TEXT("both real sessions protocol zero"))) return;
        if (!Require(BS.PrivateTalkReceived==1 && AS.PrivateTalkReceived==1 && BS.PrivateSaysRequested==1 && AS.PrivateSaysRequested==1,TEXT("bidirectional private request and arrival counters"))) return;
        Dump(B.Get(),TEXT("B_FINAL")); Dump(A,TEXT("A_FINAL"));
        World->WriteEvidence(TEXT("CertLeaves"));
        Advance(); return;
    }
    if (Step==15 && Now-StageBegan>4) Finish(true);
}
struct FInstall
{
    FDelegateHandle Handle;
    FInstall() { if (FParse::Param(FCommandLine::Get(),TEXT("real33d-cert-channels"))) Handle=FCoreDelegates::OnEndFrame.AddStatic(&Tick); }
    ~FInstall() { if (Handle.IsValid()) FCoreDelegates::OnEndFrame.Remove(Handle); }
};
FInstall Install;
}
#endif
