// Temporary independent live UI test for UNREAL-EDITORS-UI-001. Not shipped.
//
// Every value asserted here comes from Fusion32 or from the production Slate
// widgets' own painted text. SimulateClick invokes the production button
// callbacks; SetText drives the production input boxes. No private access, no
// injected events, no fabricated packets, no server edit, no implementation
// change. The one non-click entry point used is UReal33DBridge::RequestUseObject,
// which is the exact call SReal33DHUD::HandleSlotUsed makes for a right-click on
// a container slot -- used only to OPEN the window, never to assert anything.
#include "Real33DBridge.h"
#include "Real33DHUD.h"
#include "Real33DChannelsPanel.h"
#include "Real33DChatPanel.h"
#include "Real33DEditorWindow.h"
#include "Real33DPanelRegistry.h"
#include "Real33DWorldActor.h"
#include "Real33DCreatureActor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace EditorsCert
{
TWeakObjectPtr<UReal33DBridge> B;
TWeakObjectPtr<AReal33DWorld> World;
TSharedPtr<SReal33DHUD> HUD;
TSharedPtr<SReal33DEditorWindow> Win;
TSharedPtr<SReal33DChannelsPanel> Chan;
TSharedPtr<SReal33DChatPanel> Chat;
int32 Step = 0;
double Began = 0, StageBegan = 0, LastTick = 0;
bool Finished = false;

// What the scroll held when this run started, and what we write into it.
FString OriginalText, OriginalEditor;
uint32 ScrollObject = 0;
uint16 ScrollType = 0;
uint16 ScrollMax = 0;
uint8 ScrollContainer = 0, ScrollSlot = 0;
bool bScrollInContainer = false;
int32 SubmitsBefore = 0, ListsBefore = 0;
FString LocalToken;

const FString Accepted = TEXT("ECERT20261019 ACCEPTED EDIT");
FString Oversized;  // built at run time, over the server limit

void Find(const TSharedRef<SWidget>& Root, FName Type, TArray<TSharedRef<SWidget>>& Out)
{
	if (Root->GetType() == Type) Out.Add(Root);
	auto* Children = Root->GetChildren();
	for (int32 I = 0; I < Children->Num(); ++I) Find(Children->GetChildAt(I), Type, Out);
}
TArray<FString> Texts(const TSharedRef<SWidget>& Root)
{
	TArray<TSharedRef<SWidget>> Nodes; Find(Root, "STextBlock", Nodes);
	TArray<FString> Result;
	for (const auto& N : Nodes) Result.Add(StaticCastSharedRef<STextBlock>(N)->GetText().ToString());
	return Result;
}
/** Every painted line of the window, joined, so a token can be looked for. */
FString Painted(const TSharedRef<SWidget>& Root)
{
	return FString::Join(Texts(Root), TEXT(" | "));
}
TSharedPtr<SMultiLineEditableTextBox> Body(const TSharedRef<SWidget>& Root)
{
	TArray<TSharedRef<SWidget>> Nodes; Find(Root, "SMultiLineEditableTextBox", Nodes);
	return Nodes.Num() == 1 ? StaticCastSharedPtr<SMultiLineEditableTextBox>(
		TSharedPtr<SWidget>(Nodes[0])) : nullptr;
}
bool ClickText(const TSharedRef<SWidget>& Root, const FString& Text)
{
	TArray<TSharedRef<SWidget>> Buttons; Find(Root, "SButton", Buttons);
	for (const auto& N : Buttons)
		if (Texts(N).Contains(Text)) { StaticCastSharedRef<SButton>(N)->SimulateClick(); return true; }
	return false;
}
void Dump(const TCHAR* Who)
{
	const auto S = B->GetStats();
	UE_LOG(LogTemp, Display,
		TEXT("CERT_COUNTERS %s frames=%d commands=%d residual=%d unsupported=%d anomalies=%d ")
		TEXT("text_open=%d text_submit=%d list_open=%d list_submit=%d notices=%d"),
		Who, S.Frames, S.Commands, S.ResidualBytes, S.UnsupportedOpcodes, S.Anomalies,
		S.EditTextsOpened, S.EditTextsSubmitted, S.EditListsOpened, S.EditListsSubmitted,
		S.SaysRefusedLocally);
	const FReal33DEditor& E = B->GetEditor();
	UE_LOG(LogTemp, Display,
		TEXT("CERT_EDITOR %s open=%d text=%d object=%u type=%u max=%u editor='%s' len=%d"),
		Who, E.bOpen, E.bIsText, E.ObjectId, E.TypeId, E.MaxLength, *E.Editor, E.Text.Len());
}
void Finish(bool Good)
{
	if (Finished) return;
	Finished = true;
	UE_LOG(LogTemp, Display, TEXT("EDITORS_CERT_RESULT=%s step=%d"),
		Good ? TEXT("PASS") : TEXT("FAILED"), Step);
	if (World.IsValid()) World->WriteEvidence(Good ? TEXT("CertComplete") : TEXT("CertFailed"));
	FPlatformMisc::RequestExitWithStatus(false, Good ? 0 : 1);
}
bool Require(bool Good, const TCHAR* Name)
{
	UE_LOG(LogTemp, Display, TEXT("CERT_ASSERT %s %s"), Good ? TEXT("PASS") : TEXT("FAILED"), Name);
	if (!Good) Finish(false);
	return Good;
}
void Advance()
{
	Dump(TEXT("B"));
	UE_LOG(LogTemp, Display, TEXT("CERT_STAGE_COMPLETE %d"), Step);
	++Step; StageBegan = FPlatformTime::Seconds();
}
/** Opens the scroll through the same bridge call a slot right-click makes. */
void UseScroll()
{
	const auto Slot = bScrollInContainer
		? UReal33DBridge::FMoveSlot::InContainer(ScrollContainer, ScrollSlot)
		: UReal33DBridge::FMoveSlot::InInventory(ScrollSlot);
	B->RequestUseObject(Slot, ScrollType, bScrollInContainer ? ScrollSlot : 0, 1);
}

void Tick()
{
	if (Finished) return;
	const double Now = FPlatformTime::Seconds();
	if (Began == 0) { Began = Now; StageBegan = Now; }
	if (Now - Began > 300 || Now - StageBegan > 60) { Require(false, TEXT("live stage deadline")); return; }
	if (Now - LastTick < 1.5) return;
	LastTick = Now;

	// ------------------------------------------------- 0: find the real HUD
	if (Step == 0)
	{
		if (!FSlateApplication::IsInitialized()) return;
		for (TObjectIterator<AReal33DWorld> It; It; ++It)
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->GetWorld() && It->GetWorld()->IsGameWorld())
			{ World = *It; break; }
		if (!World.IsValid() || !World->GetLocalPlayer()) return;
		B = World->GetGameInstance()->GetSubsystem<UReal33DBridge>();
		if (!B.IsValid() || !B->IsRunning() || !World->GetPlayerVitals().bKnown) return;
		TArray<TSharedRef<SWidget>> Nodes;
		for (const auto& W : FSlateApplication::Get().GetTopLevelWindows()) Find(W, "SReal33DHUD", Nodes);
		if (Nodes.Num() != 1) return;
		HUD = StaticCastSharedRef<SReal33DHUD>(Nodes[0]);
		Chat = HUD->GetChatPanel();
		Nodes.Reset(); Find(HUD.ToSharedRef(), "SReal33DEditorWindow", Nodes);
		if (!Require(Nodes.Num() == 1, TEXT("one live editor window widget"))) return;
		Win = StaticCastSharedRef<SReal33DEditorWindow>(Nodes[0]);
		Nodes.Reset(); Find(HUD.ToSharedRef(), "SReal33DChannelsPanel", Nodes);
		if (!Require(Nodes.Num() == 1, TEXT("one live Channels widget"))) return;
		Chan = StaticCastSharedRef<SReal33DChannelsPanel>(Nodes[0]);

		// The registry carries the editor window alongside the seven panels
		// that were there before this milestone. Named, not just counted, so
		// a wrong number says which surface is missing.
		FReal33DPanelRegistry& R = HUD->GetPanels();
		TArray<FName> Ids; R.GetIds(Ids);
		for (const FName& Id : Ids) UE_LOG(LogTemp, Display, TEXT("CERT_PANEL %s"), *Id.ToString());
		if (!Require(R.Num() == 8, TEXT("registry holds eight real HUD surfaces"))) return;
		for (const FName& Id : { Real33D::PanelIds::Minimap, Real33D::PanelIds::Health,
			Real33D::PanelIds::Inventory, Real33D::PanelIds::Skills,
			Real33D::PanelIds::Battle, Real33D::PanelIds::Containers,
			Real33D::PanelIds::Channels, Real33D::PanelIds::TextEditor })
		{
			if (!Require(R.IsRegistered(Id), *FString::Printf(
				TEXT("registry still carries %s"), *Id.ToString()))) return;
		}
		if (!Require(R.IsRegistered(Real33D::PanelIds::TextEditor),
			TEXT("editor window is registered as TextEditor"))) return;
		if (!Require(R.Find(Real33D::PanelIds::TextEditor) == TSharedPtr<SWidget>(Win),
			TEXT("the registered widget is the live editor window"))) return;
		// Closed before Fusion32 opens anything, which is what makes it a
		// server-opened surface rather than a client-opened one.
		if (!Require(!R.IsOpen(Real33D::PanelIds::TextEditor) && !Win->IsShowing()
			&& !B->GetEditor().bOpen, TEXT("editor window starts closed"))) return;
		Advance(); return;
	}

	// ------------------------------------- 1: open the backpack (container)
	if (Step == 1)
	{
		if (World->GetContainers().Num() == 0)
		{
			const FReal33DInventory& Inv = World->GetInventory();
			if (!Inv.bKnown || !Inv.bOccupied[3]) return;
			B->RequestUseObject(UReal33DBridge::FMoveSlot::InInventory(3),
				Inv.Items[3].TypeId, 0, 0);
			return;
		}
		const FReal33DContainer& C = World->GetContainers()[0];
		if (!Require(C.bOpen && C.Objects.Num() > 0,
			TEXT("container regression: backpack opened with contents"))) return;

		// The scroll may be held in a body slot or lie in a container; the
		// server decides where a bought object lands, so look in both rather
		// than assuming. `bInContainer` records which, so the Use below
		// addresses it the same way the HUD's own right-click would.
		for (int32 S = FReal33DInventory::FirstSlot; S <= FReal33DInventory::LastSlot; ++S)
		{
			const FReal33DInventory& Inv = World->GetInventory();
			if (Inv.bOccupied[S] && Inv.Items[S].TypeId == 2815)
			{ ScrollSlot = (uint8)S; ScrollType = 2815; bScrollInContainer = false; }
		}
		if (ScrollType != 2815)
		{
			for (const FReal33DContainer& Open : World->GetContainers())
				for (int32 I = 0; I < Open.Objects.Num(); ++I)
					if (Open.Objects[I].TypeId == 2815)
					{ ScrollContainer = Open.Number; ScrollSlot = (uint8)I; ScrollType = 2815; bScrollInContainer = true; }
		}
		if (!Require(ScrollType == 2815,
			TEXT("a writable scroll (type 2815) is held or carried"))) return;
		UE_LOG(LogTemp, Display, TEXT("CERT_SCROLL in_container=%d container=%u slot=%u"),
			bScrollInContainer, ScrollContainer, ScrollSlot);
		Advance(); return;
	}

	// ------------------------------------------- 2: EDIT_TEXT opens, and shows
	if (Step == 2)
	{
		if (!B->GetEditor().bOpen) { UseScroll(); return; }
		const FReal33DEditor& E = B->GetEditor();
		if (!Require(E.bIsText && E.ObjectId != 0 && E.TypeId == 2815,
			TEXT("SV_CMD_EDIT_TEXT opened for the scroll"))) return;
		if (!Require(Win->IsShowing()
			&& HUD->GetPanels().IsOpen(Real33D::PanelIds::TextEditor),
			TEXT("the production window is showing and the registry agrees"))) return;

		ScrollObject = E.ObjectId; ScrollMax = E.MaxLength;
		OriginalText = E.Text; OriginalEditor = E.Editor;

		// What the operator actually sees, read off the painted widgets.
		const FString Seen = Painted(Win.ToSharedRef());
		UE_LOG(LogTemp, Display, TEXT("CERT_PAINTED %s"), *Seen);
		if (!Require(Seen.Contains(FString::FromInt(E.ObjectId)),
			TEXT("painted window shows the server's object id"))) return;
		if (!Require(Seen.Contains(FString::FromInt(E.TypeId)),
			TEXT("painted window shows the server's type id"))) return;
		if (!Require(Seen.Contains(FString::FromInt(E.MaxLength)) && Seen.Contains(TEXT("Fusion32")),
			TEXT("painted window shows the server's reported max length"))) return;
		// 511 is MAXLENGTH 512 - 1 for a WRITE scroll; the server computed it.
		if (!Require(E.MaxLength == 511, TEXT("max length is the type limit minus one"))) return;

		// Attribution: this scroll was written before, so the named form.
		if (!Require(!E.Editor.IsEmpty(), TEXT("server reports a previous editor"))) return;
		if (!Require(Seen.Contains(FString::Printf(TEXT("Written by %s."), *E.Editor)),
			TEXT("painted window attributes the text to the server's editor"))) return;

		// The body box carries the server's text verbatim.
		TSharedPtr<SMultiLineEditableTextBox> Box = Body(Win.ToSharedRef());
		if (!Require(Box.IsValid() && Box->GetText().ToString() == E.Text,
			TEXT("painted body equals the server's text exactly"))) return;
		Advance(); return;
	}

	// ----------------------- 3: the modal does not block the rest of the HUD
	if (Step == 3)
	{
		if (!Require(Win->GetVisibility() == EVisibility::SelfHitTestInvisible,
			TEXT("open modal is self-hit-test-invisible, not blocking"))) return;
		// Functional proof, not just the flag: click a Channels button that
		// lies behind the modal's slot and require a real server round trip.
		ListsBefore = B->GetStats().ChannelListsReceived;
		if (!Require(ClickText(Chan.ToSharedRef(), TEXT("Channel List")),
			TEXT("Channels button is clickable while the modal is open"))) return;
		Advance(); return;
	}
	if (Step == 4)
	{
		if (B->GetStats().ChannelListsReceived <= ListsBefore) return;
		if (!Require(B->GetEditor().bOpen && Win->IsShowing(),
			TEXT("modal stayed open while unrelated HUD was used"))) return;
		TArray<FReal33DChannelEntry> Rows; B->GetChannelList(Rows);
		if (!Require(Rows.Num() > 0 && B->IsChannelListKnown(),
			TEXT("Channels regression: Fusion32 returned real rows behind the modal"))) return;
		for (const auto& R : Rows) UE_LOG(LogTemp, Display, TEXT("CERT_CHANNEL %d %s"), R.Id, *R.Name);
		Advance(); return;
	}

	// --------------------------------------------- 5: registry behaviour
	if (Step == 5)
	{
		FReal33DPanelRegistry& R = HUD->GetPanels();
		if (!Require(R.Close(Real33D::PanelIds::TextEditor)
			&& !R.IsOpen(Real33D::PanelIds::TextEditor)
			&& Win->GetVisibility() == EVisibility::Collapsed,
			TEXT("registry closes the modal and it collapses"))) return;
		if (!Require(R.Toggle(Real33D::PanelIds::TextEditor)
			&& R.IsOpen(Real33D::PanelIds::TextEditor),
			TEXT("registry toggles the modal back open"))) return;
		// The other panels are untouched by the modal's own id.
		if (!Require(R.IsOpen(Real33D::PanelIds::Channels)
			&& R.IsOpen(Real33D::PanelIds::Minimap),
			TEXT("other registry panels unaffected"))) return;
		if (!Require(!R.Register(Real33D::PanelIds::TextEditor, Win.ToSharedRef()),
			TEXT("duplicate registration refused"))) return;
		Advance(); return;
	}

	// ------------------------------------------- 6: a legitimate edit submit
	if (Step == 6)
	{
		// Restore what the registry test left, then drive the production box.
		Win->Refresh();
		TSharedPtr<SMultiLineEditableTextBox> Box = Body(Win.ToSharedRef());
		if (!Require(Box.IsValid(), TEXT("body box present for submit"))) return;
		SubmitsBefore = B->GetStats().EditTextsSubmitted;
		Box->SetText(FText::FromString(Accepted));
		if (!Require(ClickText(Win.ToSharedRef(), TEXT("Submit")),
			TEXT("production Submit button clicked"))) return;
		Advance(); return;
	}
	if (Step == 7)
	{
		if (B->GetStats().EditTextsSubmitted <= SubmitsBefore) return;
		if (!Require(B->GetStats().EditTextsSubmitted == SubmitsBefore + 1,
			TEXT("exactly one CL_CMD_EDIT_TEXT reached the wire"))) return;
		// Nothing may claim it was kept. Close and ask the server again.
		if (!Require(B->CloseEditor() && !B->GetEditor().bOpen,
			TEXT("window closes locally, server told nothing"))) return;
		Advance(); return;
	}

	// --------------------- 8: re-read proves authoritative persistence
	if (Step == 8)
	{
		if (!B->GetEditor().bOpen) { UseScroll(); return; }
		const FReal33DEditor& E = B->GetEditor();
		if (E.Text == OriginalText) return;  // the re-read may still be in flight
		if (!Require(E.ObjectId == ScrollObject, TEXT("re-read is the same object"))) return;
		if (!Require(E.Text == Accepted,
			TEXT("Fusion32 persisted the submitted text"))) return;
		if (!Require(E.Editor == World->GetLocalPlayer()->GetCreatureName(),
			TEXT("Fusion32 set the editor to this character"))) return;
		const FString Seen = Painted(Win.ToSharedRef());
		if (!Require(Seen.Contains(FString::Printf(TEXT("Written by %s."), *E.Editor)),
			TEXT("painted attribution updated to this character"))) return;
		TSharedPtr<SMultiLineEditableTextBox> Box = Body(Win.ToSharedRef());
		if (!Require(Box.IsValid() && Box->GetText().ToString() == Accepted,
			TEXT("painted body shows the persisted text"))) return;
		UE_LOG(LogTemp, Display, TEXT("CERT_PERSISTED before='%s' by='%s' after='%s' by='%s'"),
			*OriginalText, *OriginalEditor, *E.Text, *E.Editor);
		Advance(); return;
	}

	// ------------------- 9: an oversized submit reaches Fusion32 and is refused
	if (Step == 9)
	{
		Oversized.Reset();
		while (Oversized.Len() <= ScrollMax) Oversized += TEXT("ECERT-OVERSIZE ");
		if (!Require(Oversized.Len() > ScrollMax && Oversized.Len() < 2000,
			TEXT("oversized text is over the server limit and inside the frame limit"))) return;
		TSharedPtr<SMultiLineEditableTextBox> Box = Body(Win.ToSharedRef());
		if (!Require(Box.IsValid(), TEXT("body box present for oversized submit"))) return;
		SubmitsBefore = B->GetStats().EditTextsSubmitted;
		Box->SetText(FText::FromString(Oversized));
		if (!Require(ClickText(Win.ToSharedRef(), TEXT("Submit")),
			TEXT("production Submit clicked with an oversized text"))) return;
		Advance(); return;
	}
	if (Step == 10)
	{
		if (B->GetStats().EditTextsSubmitted <= SubmitsBefore) return;
		if (!Require(B->GetStats().EditTextsSubmitted == SubmitsBefore + 1,
			TEXT("the oversized command reached the wire, it was not dropped"))) return;
		// Fusion32's own refusal, in Local Chat where SV_CMD_MESSAGE lands.
		TArray<FReal33DChatLine> Lines;
		B->GetChatTranscript(Lines);
		bool Refused = false;
		for (const auto& L : Lines) if (L.Line.Contains(TEXT("There is not enough room"))) Refused = true;
		if (!Refused) return;
		if (!Require(true, TEXT("Fusion32 answered NOROOM for the oversized text"))) return;
		if (!Require(B->CloseEditor(), TEXT("close before the refusal re-read"))) return;
		Advance(); return;
	}
	if (Step == 11)
	{
		if (!B->GetEditor().bOpen) { UseScroll(); return; }
		const FReal33DEditor& E = B->GetEditor();
		if (!Require(E.Text == Accepted && E.Text != Oversized,
			TEXT("the refused text was not persisted; the object kept the accepted one"))) return;
		World->WriteEvidence(TEXT("CertEditors"));
		Advance(); return;
	}

	// ---------------------------------- 12: Local Chat regression, then close
	if (Step == 12)
	{
		LocalToken = TEXT("ECERT20261019 LOCAL");
		B->RequestTalk(EReal33DTalkMode::Say, LocalToken);
		Advance(); return;
	}
	if (Step == 13)
	{
		TArray<FReal33DChatLine> Lines; B->GetChatTranscript(Lines);
		bool Heard = false;
		for (const auto& L : Lines) if (L.Line.Contains(LocalToken)) Heard = true;
		if (!Heard) return;
		if (!Require(true, TEXT("Local Chat regression: Fusion32 echoed the say"))) return;

		const auto S = B->GetStats();
		if (!Require(S.ResidualBytes == 0 && S.UnsupportedOpcodes == 0 && S.Anomalies == 0,
			TEXT("protocol residual, unsupported and anomalies all zero"))) return;
		if (!Require(S.EditListsOpened == 0 && S.EditListsSubmitted == 0,
			TEXT("no EDIT_LIST traffic occurred, as expected on this server"))) return;
		Dump(TEXT("FINAL"));
		Advance(); return;
	}
	if (Step == 14 && Now - StageBegan > 3) Finish(true);
}

struct FInstall
{
	FDelegateHandle Handle;
	FInstall() { if (FParse::Param(FCommandLine::Get(), TEXT("real33d-cert-editors"))) Handle = FCoreDelegates::OnEndFrame.AddStatic(&Tick); }
	~FInstall() { if (Handle.IsValid()) FCoreDelegates::OnEndFrame.Remove(Handle); }
};
FInstall Install;
}
#endif
