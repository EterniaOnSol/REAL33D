#include "Real33DPresentationPolicy.h"
#include "Real33DAmountPanel.h"
#include "Real33DChannelsPanel.h"
#include "Real33DChatPanel.h"
#include "Real33DChatViews.h"
#include "Real33DEditorWindow.h"
#include "Real33DOutfitPanel.h"
#include "Real33DPanelRegistry.h"
#include "Real33DSkillsPanel.h"
#include "Real33DTradePanel.h"
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

	// The channels panel is a registry panel like every other, and its id is
	// its own: the HUD was not edited to hold it by hand.
	const TSharedRef<SWidget> Third = SNew(SBox);
	TestFalse(TEXT("channels is not registered before it is added"),
		Registry.IsRegistered(Real33D::PanelIds::Channels));
	TestTrue(TEXT("channels registers"),
		Registry.Register(Real33D::PanelIds::Channels, Third));
	TestEqual(TEXT("channels is a distinct id"), Registry.Num(), 3);
	TestTrue(TEXT("channels closes"), Registry.Close(Real33D::PanelIds::Channels));
	TestFalse(TEXT("channels reads as closed"), Registry.IsOpen(Real33D::PanelIds::Channels));
	TestTrue(TEXT("channels toggles back open"), Registry.Toggle(Real33D::PanelIds::Channels));
	TestTrue(TEXT("channels reads as open"), Registry.IsOpen(Real33D::PanelIds::Channels));
	TestTrue(TEXT("closing channels leaves battle alone"),
		Registry.Close(Real33D::PanelIds::Channels) && Registry.IsOpen(Real33D::PanelIds::Battle));
	TestTrue(TEXT("channels unregisters"), Registry.Unregister(Real33D::PanelIds::Channels));
	TestEqual(TEXT("back to two"), Registry.Num(), 2);

	// The editor window is a centred modal, not a column panel, and the
	// registry handles it exactly the same way: it only manages visibility.
	const TSharedRef<SWidget> Modal = SNew(SBox);
	TestTrue(TEXT("the editor window registers"),
		Registry.Register(Real33D::PanelIds::TextEditor, Modal));
	TestTrue(TEXT("a modal closes through the registry"),
		Registry.Close(Real33D::PanelIds::TextEditor));
	TestEqual(TEXT("a closed modal collapses like any other panel"),
		Modal->GetVisibility(), EVisibility::Collapsed);
	TestTrue(TEXT("and opens again"), Registry.Open(Real33D::PanelIds::TextEditor));
	TestTrue(TEXT("reading as open"), Registry.IsOpen(Real33D::PanelIds::TextEditor));
	TestTrue(TEXT("the editor id is distinct from the channels id"),
		Real33D::PanelIds::TextEditor != Real33D::PanelIds::Channels);
	TestTrue(TEXT("the editor window unregisters"),
		Registry.Unregister(Real33D::PanelIds::TextEditor));
	TestEqual(TEXT("two again"), Registry.Num(), 2);

	// The trade window is a distinct surface with its own id.
	const TSharedRef<SWidget> TradeModal = SNew(SBox);
	TestTrue(TEXT("the trade window registers"),
		Registry.Register(Real33D::PanelIds::Trade, TradeModal));
	TestTrue(TEXT("trade id differs from the editor id"),
		Real33D::PanelIds::Trade != Real33D::PanelIds::TextEditor);
	TestTrue(TEXT("the trade window closes through the registry"),
		Registry.Close(Real33D::PanelIds::Trade));
	TestEqual(TEXT("and collapses"), TradeModal->GetVisibility(), EVisibility::Collapsed);
	TestTrue(TEXT("and reopens"), Registry.Toggle(Real33D::PanelIds::Trade));
	TestTrue(TEXT("the trade window unregisters"),
		Registry.Unregister(Real33D::PanelIds::Trade));
	TestEqual(TEXT("two once more"), Registry.Num(), 2);

	TestTrue(TEXT("unregister"), Registry.Unregister(Real33D::PanelIds::Skills));
	TestFalse(TEXT("unregistering twice reports failure"), Registry.Unregister(Real33D::PanelIds::Skills));
	TestEqual(TEXT("one left"), Registry.Num(), 1);
	Registry.Reset();
	TestEqual(TEXT("reset empties"), Registry.Num(), 0);
	// Reset forgets panels; it does not touch the widgets themselves.
	TestEqual(TEXT("reset left the widget alone"), Second->GetVisibility(), SecondBuiltWith);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DSkillsPanelTest,
	"REAL33D.Presentation.SkillsPanel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReal33DSkillsPanelTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// The rule this panel exists to keep: nothing is shown until Fusion32 has
	// said it. SV_CMD_PLAYER_DATA and SV_CMD_PLAYER_SKILLS set bKnown; before
	// either arrives every row reads "--", because a 0 drawn there would be a
	// number this client invented.
	const FText Unknown = FText::FromString(TEXT("--"));
	for (int64 Value : {int64(0), int64(1), int64(10), int64(4200), int64(-1)})
	{
		TestEqual(TEXT("an unsent value is never printed"),
			SReal33DSkillsPanel::RowText(Value, false).ToString(), Unknown.ToString());
	}

	// A zero the server really sent is a fact and is shown as 0, not hidden
	// behind the same dashes. Magic level 0 is the ordinary case on Rookgaard.
	TestEqual(TEXT("a server-sent zero is shown"),
		SReal33DSkillsPanel::RowText(0, true).ToString(), FString(TEXT("0")));

	// The seven fighting skills are small numbers; experience is not. Both go
	// through the same formatter, so the wide one is checked too.
	TestEqual(TEXT("skill level"), SReal33DSkillsPanel::RowText(11, true).ToString(),
		FString(TEXT("11")));
	TestEqual(TEXT("experience"), SReal33DSkillsPanel::RowText(441, true).ToString(),
		FString(TEXT("441")));
	TestEqual(TEXT("a value wider than 32 bits still formats"),
		SReal33DSkillsPanel::RowText(4294967296LL, true).ToString(),
		FString(TEXT("4294967296")));

	// Percent bars. Unknown is empty for the same reason as the text.
	TestEqual(TEXT("unknown percent draws empty"),
		SReal33DSkillsPanel::BarFill(57, false), 0.0f);
	TestEqual(TEXT("zero percent"), SReal33DSkillsPanel::BarFill(0, true), 0.0f);
	TestEqual(TEXT("half"), SReal33DSkillsPanel::BarFill(50, true), 0.5f);
	TestEqual(TEXT("full"), SReal33DSkillsPanel::BarFill(100, true), 1.0f);

	// Clamped at both ends: a percentage cannot overrun a bar, and the wire
	// carries a byte this client does not get to trust blindly.
	TestEqual(TEXT("above full is clamped"),
		SReal33DSkillsPanel::BarFill(101, true), 1.0f);
	TestEqual(TEXT("far above full is clamped"),
		SReal33DSkillsPanel::BarFill(255, true), 1.0f);
	TestEqual(TEXT("below zero is clamped"),
		SReal33DSkillsPanel::BarFill(-1, true), 0.0f);

	// Monotonic across the whole byte range, and always a usable fill.
	float Previous = -1.0f;
	for (int32 Percent = 0; Percent <= 255; ++Percent)
	{
		const float Fill = SReal33DSkillsPanel::BarFill(Percent, true);
		TestTrue(TEXT("fill stays in range"), Fill >= 0.0f && Fill <= 1.0f);
		TestTrue(TEXT("fill never decreases"), Fill >= Previous);
		Previous = Fill;
	}

	// The window sizes itself to its rows; a non-positive height would
	// collapse the panel the registry shows.
	TestTrue(TEXT("panel asks for a real height"),
		SReal33DSkillsPanel::DesiredContentHeight() > 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DChannelsPanelTest,
	"REAL33D.Presentation.ChannelsPanel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReal33DChannelsPanelTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// The rule this panel exists to keep: every channel on screen came off the
	// wire. Before SV_CMD_CHANNELS answers there is nothing to show, and the
	// three empty states are told apart, because two of them are statements
	// about this client and only the third is a statement about the server.
	const FString Unasked = SReal33DChannelsPanel::EmptyListText(false, false).ToString();
	const FString Waiting = SReal33DChannelsPanel::EmptyListText(true, false).ToString();
	const FString None = SReal33DChannelsPanel::EmptyListText(true, true).ToString();
	TestTrue(TEXT("an unasked list does not claim the server sent nothing"),
		!Unasked.IsEmpty() && Unasked != None);
	TestTrue(TEXT("an unanswered list does not claim the server sent nothing"),
		!Waiting.IsEmpty() && Waiting != None);
	TestTrue(TEXT("the three empty states are distinct"),
		Unasked != Waiting && Waiting != None);
	// Asking before a request cannot be reported as an answer.
	TestEqual(TEXT("not asked outranks not answered"),
		SReal33DChannelsPanel::EmptyListText(false, true).ToString(), Unasked);

	// A row shows the server's name and the number the player is joining.
	FReal33DChannelEntry Help;
	Help.Id = 7;
	Help.Name = TEXT("Help");
	TestEqual(TEXT("a row carries the number and the server's own name"),
		SReal33DChannelsPanel::RowLabel(Help).ToString(), FString(TEXT("7  Help")));
	TestTrue(TEXT("an ordinary channel is joinable"),
		SReal33DChannelsPanel::IsJoinable(Help));
	TestTrue(TEXT("a joinable row needs no refusal"),
		SReal33DChannelsPanel::JoinRefusalReason(Help).IsEmpty());

	// Channel 0's name is the player's own guild, so the name is never local.
	FReal33DChannelEntry Guild;
	Guild.Id = 0;
	Guild.Name = TEXT("Some Guild");
	TestEqual(TEXT("channel zero is the guild, named by the server"),
		SReal33DChannelsPanel::RowLabel(Guild).ToString(),
		FString(TEXT("0  Some Guild")));
	TestTrue(TEXT("channel zero is joinable"), SReal33DChannelsPanel::IsJoinable(Guild));

	// The own-channel offer is not a channel. Its 0xFFFF is not a number to
	// show and joining it would be meaningless.
	FReal33DChannelEntry Offer;
	Offer.Id = 0;
	Offer.Name = TEXT("Private Chat Channel");
	Offer.bOwnChannelOffer = true;
	TestEqual(TEXT("the offer row shows no channel number"),
		SReal33DChannelsPanel::RowLabel(Offer).ToString(),
		FString(TEXT("Private Chat Channel")));
	TestFalse(TEXT("the offer row is not joinable"),
		SReal33DChannelsPanel::IsJoinable(Offer));
	TestFalse(TEXT("and says why"),
		SReal33DChannelsPanel::JoinRefusalReason(Offer).IsEmpty());

	// Channel 3 is answered with a command this project does not decode, so
	// this client refuses it -- and reports that as its own limitation.
	FReal33DChannelEntry Violations;
	Violations.Id = UReal33DBridge::kRuleViolationsChannel;
	Violations.Name = TEXT("Rule Violations");
	TestEqual(TEXT("the refused channel is Fusion32's channel 3"),
		UReal33DBridge::kRuleViolationsChannel, 3);
	TestFalse(TEXT("rule violations is not joinable"),
		SReal33DChannelsPanel::IsJoinable(Violations));
	TestFalse(TEXT("and says why"),
		SReal33DChannelsPanel::JoinRefusalReason(Violations).IsEmpty());
	TestTrue(TEXT("the two refusals give different reasons"),
		SReal33DChannelsPanel::JoinRefusalReason(Violations).ToString()
			!= SReal33DChannelsPanel::JoinRefusalReason(Offer).ToString());
	// Its neighbours are unaffected: only 3 is refused.
	for (const int32 Id : {0, 1, 2, 4, 5, 6, 7})
	{
		FReal33DChannelEntry Each;
		Each.Id = Id;
		Each.Name = TEXT("Named by the server");
		TestTrue(TEXT("every channel but 3 is offered"),
			SReal33DChannelsPanel::IsJoinable(Each));
	}

	// The window has to have a height, or the registry would show a panel with
	// nothing in it.
	TestTrue(TEXT("panel asks for a real height"),
		SReal33DChannelsPanel::PanelHeight > 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DChatTabsTest,
	"REAL33D.Presentation.ChatTabs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReal33DChatTabsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// Local Chat is preserved: a default view is it, it is labelled, and it
	// cannot be closed, because it is where every unaddressed line goes.
	const FReal33DChatView Local;
	TestEqual(TEXT("a default view is Local Chat"),
		static_cast<int32>(Local.Kind), static_cast<int32>(EReal33DChatViewKind::Local));
	TestEqual(TEXT("Local Chat keeps its name"),
		SReal33DChatPanel::TabLabel(Local).ToString(), FString(TEXT("Local Chat")));
	TestFalse(TEXT("Local Chat cannot be closed"), SReal33DChatPanel::IsClosable(Local));
	// And it has no destination of its own: the mode selector decides there.
	TestTrue(TEXT("Local Chat offers the mode choice instead of a destination"),
		SReal33DChatPanel::DestinationLabel(Local).IsEmpty());

	// A channel tab is labelled with the server's name, not with its number and
	// not with a local table's idea of what channel 7 is called.
	FReal33DChatView Channel;
	Channel.ViewId = 12;
	Channel.Kind = EReal33DChatViewKind::Channel;
	Channel.Channel = 7;
	Channel.Name = TEXT("Help");
	TestEqual(TEXT("a channel tab wears the server's channel name"),
		SReal33DChatPanel::TabLabel(Channel).ToString(), FString(TEXT("Help")));
	TestTrue(TEXT("a channel tab can be closed"), SReal33DChatPanel::IsClosable(Channel));
	const FString ChannelDestination =
		SReal33DChatPanel::DestinationLabel(Channel).ToString();
	TestTrue(TEXT("a channel tab says where the line goes"),
		ChannelDestination.Contains(TEXT("Help")) && ChannelDestination.Contains(TEXT("7")));

	// A guild channel is named after the guild, which is exactly why the label
	// is never derived from the number.
	FReal33DChatView GuildChannel = Channel;
	GuildChannel.Channel = 0;
	GuildChannel.Name = TEXT("Some Guild");
	TestEqual(TEXT("channel zero wears the guild's name"),
		SReal33DChatPanel::TabLabel(GuildChannel).ToString(), FString(TEXT("Some Guild")));

	// A private tab is the character, and carries no channel number because
	// SV_CMD_PRIVATE_CHANNEL carries none.
	FReal33DChatView Private;
	Private.ViewId = 13;
	Private.Kind = EReal33DChatViewKind::Private;
	Private.Name = TEXT("Rook Sample");
	TestEqual(TEXT("a private tab wears the character's name"),
		SReal33DChatPanel::TabLabel(Private).ToString(), FString(TEXT("Rook Sample")));
	TestTrue(TEXT("a private tab can be closed"), SReal33DChatPanel::IsClosable(Private));
	TestTrue(TEXT("a private tab says who the line goes to"),
		SReal33DChatPanel::DestinationLabel(Private).ToString()
			.Contains(TEXT("Rook Sample")));
	// The two destinations cannot read alike: one reaches a whole channel.
	TestTrue(TEXT("a channel and a private destination are told apart"),
		SReal33DChatPanel::DestinationLabel(Private).ToString() != ChannelDestination);

	// The Unreal-side kinds and the Slate-free ones agree. The bridge pins this
	// with static_asserts; checked here too, because a silent drift would route
	// a line to the wrong conversation.
	TestEqual(TEXT("Local matches"), static_cast<int32>(EReal33DChatViewKind::Local),
		static_cast<int32>(Real33D::ChatViewKind::Local));
	TestEqual(TEXT("Channel matches"), static_cast<int32>(EReal33DChatViewKind::Channel),
		static_cast<int32>(Real33D::ChatViewKind::Channel));
	TestEqual(TEXT("Private matches"), static_cast<int32>(EReal33DChatViewKind::Private),
		static_cast<int32>(Real33D::ChatViewKind::Private));

	// The routing model itself, which the native suite covers in full. Repeated
	// here only to prove the header the module compiles is the one that was
	// tested: Local Chat is present from the start and nothing is dropped.
	Real33D::ChatViewSet Views;
	TestEqual(TEXT("one surface to begin with"), static_cast<int32>(Views.size()), 1);
	TestEqual(TEXT("an unjoined channel's line still has somewhere to go"),
		static_cast<int32>(Views.RouteChannel(7)), static_cast<int32>(Views.LocalId()));
	const std::uint32_t Joined = Views.OpenChannel(7, "Help", false);
	TestEqual(TEXT("a joined channel's line goes to its own surface"),
		static_cast<int32>(Views.RouteChannel(7)), static_cast<int32>(Joined));
	TestTrue(TEXT("leaving removes it"), Views.CloseChannel(7));
	TestEqual(TEXT("and its traffic falls back to Local Chat"),
		static_cast<int32>(Views.RouteChannel(7)), static_cast<int32>(Views.LocalId()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DEditorWindowTest,
	"REAL33D.Presentation.EditorWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReal33DEditorWindowTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// Nothing is drawn until Fusion32 opens a window. A default editor is a
	// closed one, and a closed one says nothing at all.
	const FReal33DEditor Closed;
	TestFalse(TEXT("a default editor is closed"), Closed.bOpen);
	TestTrue(TEXT("a closed window has no title"),
		SReal33DEditorWindow::Title(Closed).IsEmpty());
	TestTrue(TEXT("a closed window attributes nothing"),
		SReal33DEditorWindow::Attribution(Closed).IsEmpty());
	TestTrue(TEXT("a closed window states no limit"),
		SReal33DEditorWindow::LengthNote(Closed).IsEmpty());
	TestFalse(TEXT("a closed window cannot submit"),
		SReal33DEditorWindow::CanSubmit(Closed));

	// ---------------------------------------------------------------- text
	FReal33DEditor Sign;
	Sign.bOpen = true;
	Sign.bIsText = true;
	Sign.ObjectId = 4242;
	Sign.TypeId = 2602;
	Sign.MaxLength = 71;
	Sign.Text = TEXT("Be polite and respectful to other people you meet.");

	// The wire carries no name for a text object, so the heading is generic
	// rather than an item name this client would have had to invent.
	TestEqual(TEXT("a text window is headed generically"),
		SReal33DEditorWindow::Title(Sign).ToString(), FString(TEXT("Text")));

	// The attribution has three states and the empty one is a fact, not a
	// missing value: EDITOR is unset until somebody writes.
	TestEqual(TEXT("an unwritten object says so explicitly"),
		SReal33DEditorWindow::Attribution(Sign).ToString(),
		FString(TEXT("Nobody has written on this.")));
	Sign.Editor = TEXT("Test Player B");
	TestEqual(TEXT("a written object names its editor"),
		SReal33DEditorWindow::Attribution(Sign).ToString(),
		FString(TEXT("Written by Test Player B.")));
	// The name is the server's, carried through untouched.
	Sign.Editor = TEXT("A Name With Spaces");
	TestTrue(TEXT("the editor's name is carried verbatim"),
		SReal33DEditorWindow::Attribution(Sign).ToString()
			.Contains(TEXT("A Name With Spaces")));

	// The length note is phrased as the server's report, never as proof the
	// object can be written.
	const FString Note = SReal33DEditorWindow::LengthNote(Sign).ToString();
	TestTrue(TEXT("the limit is attributed to Fusion32"),
		Note.Contains(TEXT("Fusion32")) && Note.Contains(TEXT("71")));

	// THE RULE THIS WINDOW EXISTS TO KEEP. A read-only object is reported by
	// SendEditText with max_length == strlen(text); a writable one is
	// reported with the type's limit minus one. Neither is distinguishable
	// here, so Submit is offered in both cases and Fusion32 rules.
	FReal33DEditor ReadOnly = Sign;
	ReadOnly.Text = TEXT("hello");
	ReadOnly.MaxLength = 5;              // exactly strlen, the read-only shape
	TestTrue(TEXT("a read-only-shaped object still offers Submit"),
		SReal33DEditorWindow::CanSubmit(ReadOnly));
	FReal33DEditor Writable = Sign;
	Writable.Text = TEXT("hello");
	Writable.MaxLength = 199;            // a WRITE type's limit minus one
	TestTrue(TEXT("a writable-shaped object offers Submit"),
		SReal33DEditorWindow::CanSubmit(Writable));
	TestEqual(TEXT("and the two are treated identically"),
		SReal33DEditorWindow::CanSubmit(ReadOnly),
		SReal33DEditorWindow::CanSubmit(Writable));

	// An empty text is a legitimate book, and a zero limit is what the server
	// reports for one. Neither closes the window or blocks a submission.
	FReal33DEditor Empty = Sign;
	Empty.Text = FString();
	Empty.MaxLength = 0;
	TestTrue(TEXT("an empty object still submits"),
		SReal33DEditorWindow::CanSubmit(Empty));

	// Submit is refused only when there is nothing to submit against.
	FReal33DEditor NoObject = Sign;
	NoObject.ObjectId = 0;
	TestFalse(TEXT("no object id means nothing to submit against"),
		SReal33DEditorWindow::CanSubmit(NoObject));

	// ---------------------------------------------------------------- lists
	FReal33DEditor Guests;
	Guests.bOpen = true;
	Guests.bIsText = false;
	Guests.ListType = EReal33DEditListType::Guest;
	Guests.ListId = 10001;
	Guests.Text = TEXT("# Guests of Spiritkeep\n");

	// A list IS named by the wire: the type byte says which one.
	TestEqual(TEXT("a guest list is named"),
		SReal33DEditorWindow::Title(Guests).ToString(), FString(TEXT("Guest list")));
	Guests.ListType = EReal33DEditListType::Subowner;
	TestEqual(TEXT("a subowner list is named"),
		SReal33DEditorWindow::Title(Guests).ToString(), FString(TEXT("Subowner list")));
	Guests.ListType = EReal33DEditListType::Door;
	TestEqual(TEXT("a door list is named"),
		SReal33DEditorWindow::Title(Guests).ToString(), FString(TEXT("Door list")));
	TestTrue(TEXT("a named list submits"), SReal33DEditorWindow::CanSubmit(Guests));

	// SendEditList writes no editor field, so a list attributes nothing
	// rather than claiming nobody wrote it.
	TestTrue(TEXT("a list attributes nothing"),
		SReal33DEditorWindow::Attribution(Guests).IsEmpty());
	TestTrue(TEXT("a list states no character limit"),
		SReal33DEditorWindow::LengthNote(Guests).IsEmpty());

	// A type byte the server never named has nowhere to go: CEditList logs
	// "Unbekannter Typ" and drops it, so submitting would be traffic with no
	// possible effect.
	FReal33DEditor Unnamed = Guests;
	Unnamed.ListType = EReal33DEditListType::Unknown;
	TestFalse(TEXT("an unnamed list cannot submit"),
		SReal33DEditorWindow::CanSubmit(Unnamed));
	TestFalse(TEXT("and is still headed, not blank"),
		SReal33DEditorWindow::Title(Unnamed).IsEmpty());

	// (trade rules live in their own suite below)

	// The three list values are enums.hh's own. The bridge pins them with
	// static_asserts; checked here too, because submitting a guest list under
	// the subowner byte would edit a different list entirely.
	TestEqual(TEXT("GUESTLIST is 1"), static_cast<int32>(EReal33DEditListType::Guest), 1);
	TestEqual(TEXT("SUBOWNERLIST is 2"), static_cast<int32>(EReal33DEditListType::Subowner), 2);
	TestEqual(TEXT("DOORLIST is 3"), static_cast<int32>(EReal33DEditListType::Door), 3);

	// The real widget, not just its rules. It must be built collapsed, so the
	// registry reports it closed and it takes no space before Fusion32 opens
	// anything -- and it must be driven from outside.
	//
	// A live run showed why that second half matters: the window drove itself
	// from Tick, Slate does not tick a collapsed widget, and fifteen decoded
	// SV_CMD_EDIT_TEXT commands produced no window at all. `Refresh` is now
	// called by SReal33DHUD::Refresh once a frame.
	const TSharedRef<SReal33DEditorWindow> Window = SNew(SReal33DEditorWindow);
	TestEqual(TEXT("the window is built collapsed"),
		Window->GetVisibility(), EVisibility::Collapsed);
	TestFalse(TEXT("and shows nothing"), Window->IsShowing());
	// Driven with no bridge: harmless, and still closed. This is the path a
	// gallery or headless session takes every frame.
	Window->Refresh();
	TestEqual(TEXT("a refresh without a bridge leaves it collapsed"),
		Window->GetVisibility(), EVisibility::Collapsed);
	TestFalse(TEXT("and still showing nothing"), Window->IsShowing());

	// The trade panel is DOCKED, not a modal: its Offer control is what
	// starts a trade, so it must be reachable before one exists. It is drawn
	// from the start, shows no trade, and is not armed.
	const TSharedRef<SReal33DTradePanel> TradePanel = SNew(SReal33DTradePanel);
	TestFalse(TEXT("a fresh trade panel shows no trade"), TradePanel->IsShowing());
	TestNotEqual(TEXT("and is not hidden, unlike the editor modal"),
		TradePanel->GetVisibility(), EVisibility::Collapsed);
	TradePanel->Refresh();
	TestFalse(TEXT("a refresh without a bridge still shows no trade"),
		TradePanel->IsShowing());

	// Each list type is labelled distinctly, so a window cannot be mistaken
	// for the list next to it.
	TestTrue(TEXT("the three list labels differ"),
		FString(Real33DEditListTypeLabel(EReal33DEditListType::Guest))
			!= FString(Real33DEditListTypeLabel(EReal33DEditListType::Subowner))
		&& FString(Real33DEditListTypeLabel(EReal33DEditListType::Subowner))
			!= FString(Real33DEditListTypeLabel(EReal33DEditListType::Door)));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DTradePanelTest,
	"REAL33D.Presentation.TradePanel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReal33DTradePanelTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// Nothing is shown until Fusion32 sends an offer. A default trade is
	// closed, and a closed trade says nothing about where anything stands.
	const FReal33DTrade Closed;
	TestFalse(TEXT("a default trade is closed"), Closed.bOpen);
	TestTrue(TEXT("a closed trade has no status line"),
		SReal33DTradePanel::StatusText(Closed).IsEmpty());
	TestFalse(TEXT("a closed trade cannot be accepted"),
		SReal33DTradePanel::CanAccept(Closed));

	// THE RULE THIS WINDOW EXISTS TO KEEP. Accepting is not completing:
	// TPlayer::AcceptTrade records the flag and returns while the other side
	// has not accepted. The status line may never say the trade is done.
	FReal33DTrade Both;
	Both.bOpen = true;
	Both.Own.bKnown = true;  Both.Own.Name = TEXT("Test Player B");
	Both.Partner.bKnown = true; Both.Partner.Name = TEXT("Test Player A");
	TestTrue(TEXT("both offers present means accept is offered"),
		SReal33DTradePanel::CanAccept(Both));
	const FString BothText = SReal33DTradePanel::StatusText(Both).ToString();
	TestFalse(TEXT("the status never claims completion"),
		BothText.Contains(TEXT("complete")) && !BothText.Contains(TEXT("not")));
	Both.bAcceptSent = true;
	const FString SentText = SReal33DTradePanel::StatusText(Both).ToString();
	TestTrue(TEXT("an accept that was sent is reported as sent, not as done"),
		SentText.Contains(TEXT("Accept sent")));
	TestTrue(TEXT("and says the server needs both sides"),
		SentText.Contains(TEXT("both")));

	// Half-open is the ordinary 7.72 state: Fusion32 tells the other player
	// only "X wants to trade with you" until they offer an object, so there
	// is no partner offer to show and accepting cannot mean anything yet.
	FReal33DTrade Half;
	Half.bOpen = true;
	Half.Own.bKnown = true; Half.Own.Name = TEXT("Test Player B");
	TestFalse(TEXT("accept is withheld until both sides have offered"),
		SReal33DTradePanel::CanAccept(Half));
	TestTrue(TEXT("and the status says what is being waited for"),
		SReal33DTradePanel::StatusText(Half).ToString().Contains(TEXT("Waiting")));

	// An offer that has not arrived is named as absent on both sides, and is
	// never confused with an offer that arrived carrying no objects.
	const FReal33DTradeOffer Absent;
	FReal33DTradeOffer Empty;  Empty.bKnown = true;  Empty.Name = TEXT("Test Player A");
	TestTrue(TEXT("an absent own offer says so"),
		SReal33DTradePanel::OfferTitle(Absent, true).ToString().Contains(TEXT("nothing")));
	TestTrue(TEXT("an absent partner offer says so"),
		SReal33DTradePanel::OfferTitle(Absent, false).ToString().Contains(TEXT("No offer")));
	TestTrue(TEXT("a present offer is titled with the server's name"),
		SReal33DTradePanel::OfferTitle(Empty, false).ToString().Contains(TEXT("Test Player A")));
	TestTrue(TEXT("absent and empty read differently"),
		SReal33DTradePanel::EmptyOfferText(Absent, false).ToString()
			!= SReal33DTradePanel::EmptyOfferText(Empty, false).ToString());
	TestTrue(TEXT("only the arrived-and-empty case attributes it to Fusion32"),
		SReal33DTradePanel::EmptyOfferText(Empty, false).ToString().Contains(TEXT("Fusion32")));

	// The own side's absent text tells the player how to START a trade,
	// because this client never opens one by itself and the entry point is
	// the object's own ctrl+right-click menu rather than anything in here.
	const FString Hint = SReal33DTradePanel::EmptyOfferText(Absent, true).ToString();
	TestTrue(TEXT("the own absent text names the menu gesture"),
		Hint.Contains(TEXT("right-click")));
	TestTrue(TEXT("and names the entry that starts a trade"),
		Hint.Contains(TEXT("Trade with")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DOutfitPanelTest,
	"REAL33D.Presentation.OutfitPanel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReal33DOutfitPanelTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using Real33D::OutfitDraft;
	using Real33D::OutfitSlot;

	// Nothing is on screen and nothing is submittable before the server has
	// offered a chooser. This is the state the window is BUILT in, because the
	// band depends on facts the game protocol never carries.
	const FReal33DOutfitDialog Closed;
	const OutfitDraft Fresh;
	TestTrue(TEXT("a closed chooser has no range text"),
		SReal33DOutfitPanel::RangeText(Closed).IsEmpty());
	TestTrue(TEXT("a closed chooser has no status"),
		SReal33DOutfitPanel::StatusText(Closed, Fresh).IsEmpty());
	TestFalse(TEXT("a closed chooser cannot submit"),
		SReal33DOutfitPanel::CanSubmit(Closed, Fresh));
	TestTrue(TEXT("the heading is the classic client's own title"),
		SReal33DOutfitPanel::Title().ToString() == TEXT("Select Outfit"));

	// An ordinary male non-premium chooser: SendOutfit's 128..131.
	FReal33DOutfitDialog Dialog;
	Dialog.bOpen = true;
	Dialog.First = 128;
	Dialog.Last = 131;
	Dialog.Current.OutfitId = 129;
	Dialog.Current.Colors[0] = 78;
	Dialog.Current.Colors[1] = 69;
	Dialog.Current.Colors[2] = 58;
	Dialog.Current.Colors[3] = 76;
	TestEqual(TEXT("the band's count is the server's"), Dialog.Count(), 4);

	OutfitDraft Draft;
	Draft.Open(Dialog.First, Dialog.Last, Dialog.Current.OutfitId, Dialog.Current.Colors);
	TestTrue(TEXT("an open chooser over a real band can submit"),
		SReal33DOutfitPanel::CanSubmit(Dialog, Draft));

	// The range is stated in the server's own ids, because that is what the
	// evidence for a live run has to match against SV_CMD_OUTFIT.
	const FString Range = SReal33DOutfitPanel::RangeText(Dialog).ToString();
	TestTrue(TEXT("the range names the first id"), Range.Contains(TEXT("128")));
	TestTrue(TEXT("the range names the last id"), Range.Contains(TEXT("131")));
	TestTrue(TEXT("the range credits Fusion32 for it"), Range.Contains(TEXT("Fusion32")));

	const FString Outfit = SReal33DOutfitPanel::OutfitText(Draft).ToString();
	TestTrue(TEXT("the selection names the chosen id"), Outfit.Contains(TEXT("129")));
	TestTrue(TEXT("and its position within the band"), Outfit.Contains(TEXT("2 of 4")));

	// The four slot names are the proven ones, and each row shows its own byte.
	TestEqual(TEXT("slot 0 is Head with its own colour"),
		SReal33DOutfitPanel::SlotText(Draft, OutfitSlot::Head).ToString(),
		FString(TEXT("Head  78")));
	TestEqual(TEXT("slot 1 is Body with its own colour"),
		SReal33DOutfitPanel::SlotText(Draft, OutfitSlot::Body).ToString(),
		FString(TEXT("Body  69")));
	TestEqual(TEXT("slot 2 is Legs with its own colour"),
		SReal33DOutfitPanel::SlotText(Draft, OutfitSlot::Legs).ToString(),
		FString(TEXT("Legs  58")));
	TestEqual(TEXT("slot 3 is Feet with its own colour"),
		SReal33DOutfitPanel::SlotText(Draft, OutfitSlot::Feet).ToString(),
		FString(TEXT("Feet  76")));

	// The status may report the bound and what will not happen, and may never
	// report success: there is no success signal on this wire to report.
	const FString Status = SReal33DOutfitPanel::StatusText(Dialog, Draft).ToString();
	TestTrue(TEXT("the status names the colour bound"), Status.Contains(TEXT("132")));
	TestTrue(TEXT("the status says the server does not acknowledge"),
		Status.Contains(TEXT("does not acknowledge")));
	TestFalse(TEXT("the status never claims the outfit was applied"),
		Status.Contains(TEXT("applied")) || Status.Contains(TEXT("saved"))
			|| Status.Contains(TEXT("success")));

	// Stepping cannot leave the band, so Submit stays available throughout.
	for (int32 Step = 0; Step < 12; ++Step)
	{
		Draft.StepOutfit(1);
		TestTrue(TEXT("stepping stays inside the server's band"),
			Dialog.Contains(Draft.OutfitId()));
		TestTrue(TEXT("and submit stays available"),
			SReal33DOutfitPanel::CanSubmit(Dialog, Draft));
	}

	// A selection outside the band is refused by the gate even if a draft
	// somehow held one -- which is what a newer, narrower SV_CMD_OUTFIT does.
	FReal33DOutfitDialog Narrowed = Dialog;
	Narrowed.First = 130;
	Narrowed.Last = 130;
	OutfitDraft Wide;
	Wide.Open(128, 131, 128, Dialog.Current.Colors);
	TestFalse(TEXT("a narrowed band refuses a selection it no longer names"),
		SReal33DOutfitPanel::CanSubmit(Narrowed, Wide));
	TestTrue(TEXT("and the status says so"),
		SReal33DOutfitPanel::StatusText(Narrowed, Wide).ToString()
			.Contains(TEXT("outside")));

	// An empty band cannot happen against a healthy server, so it is reported
	// rather than repaired, and nothing may be sent under it.
	FReal33DOutfitDialog Inverted;
	Inverted.bOpen = true;
	Inverted.First = 139;
	Inverted.Last = 136;
	OutfitDraft NoBand;
	NoBand.Open(Inverted.First, Inverted.Last, 137, Dialog.Current.Colors);
	TestEqual(TEXT("an inverted band names no outfit"), Inverted.Count(), 0);
	TestFalse(TEXT("and cannot submit"),
		SReal33DOutfitPanel::CanSubmit(Inverted, NoBand));
	TestTrue(TEXT("the range reports it granted nothing"),
		SReal33DOutfitPanel::RangeText(Inverted).ToString().Contains(TEXT("no selectable")));
	TestTrue(TEXT("the status reports it granted nothing"),
		SReal33DOutfitPanel::StatusText(Inverted, NoBand).ToString()
			.Contains(TEXT("empty range")));
	TestTrue(TEXT("the inverted band is reported as sent, not reordered"),
		SReal33DOutfitPanel::RangeText(Inverted).ToString().Contains(TEXT("139..136")));

	// A disguise is explained rather than silently rewritten into an outfit.
	// CL_CMD_SET_OUTFIT is seven fixed bytes and cannot carry an object type.
	TestTrue(TEXT("an ordinary outfit has no disguise note"),
		SReal33DOutfitPanel::DisguiseNote(Dialog).IsEmpty());
	FReal33DOutfitDialog Disguised = Dialog;
	Disguised.Current.bDisguisedAsObject = true;
	Disguised.Current.ObjectType = 2969;
	Disguised.Current.OutfitId = 0;
	const FString Note = SReal33DOutfitPanel::DisguiseNote(Disguised).ToString();
	TestTrue(TEXT("a disguise names the object type the server sent"),
		Note.Contains(TEXT("2969")));
	TestTrue(TEXT("and says it cannot be submitted"),
		Note.Contains(TEXT("cannot be submitted")));

	// The chooser is registered like every other surface, so visibility keeps
	// one owner -- but it is not a control-strip panel and nothing local may
	// open it.
	FReal33DPanelRegistry Registry;
	const TSharedRef<SWidget> Window = SNew(SBox);
	TestTrue(TEXT("the chooser registers under its own id"),
		Registry.Register(Real33D::PanelIds::Outfit, Window));
	TestFalse(TEXT("and cannot be registered twice"),
		Registry.Register(Real33D::PanelIds::Outfit, Window));
	TestTrue(TEXT("the id is distinct from the other two modals"),
		Real33D::PanelIds::Outfit != Real33D::PanelIds::Trade
			&& Real33D::PanelIds::Outfit != Real33D::PanelIds::TextEditor);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DStackMoveTest,
	"REAL33D.Presentation.StackMoveControls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReal33DStackMoveTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using Real33D::ClampStackAmount;
	using Real33D::DecideStackMove;
	using Real33D::kMaxWireStackCount;
	using Real33D::StackMoveKind;

	// ------------------------------------------------------------ the gesture
	//
	// The same three rules tests/real33d_stack_move_tests.cpp fixes without an
	// engine, re-asserted here so a build that compiles the header into the
	// module cannot have changed them.
	TestTrue(TEXT("a plain drag moves the whole stack"),
		DecideStackMove(true, 47, false, false).Kind == StackMoveKind::Send
			&& DecideStackMove(true, 47, false, false).Count == 47);
	TestTrue(TEXT("shift+drag moves exactly one"),
		DecideStackMove(true, 47, true, false).Kind == StackMoveKind::Send
			&& DecideStackMove(true, 47, true, false).Count == 1);
	TestTrue(TEXT("ctrl+drag asks, opening on the whole stack"),
		DecideStackMove(true, 47, false, true).Kind == StackMoveKind::Ask
			&& DecideStackMove(true, 47, false, true).Count == 47);
	TestTrue(TEXT("ctrl wins when both are held, because a question can be cancelled"),
		DecideStackMove(true, 47, true, true).Kind == StackMoveKind::Ask);

	// The behaviour this client already had, preserved: a sword carries no
	// amount byte, so no modifier may produce a partial move of one.
	for (int32 Mask = 0; Mask < 4; ++Mask)
	{
		const Real33D::StackMoveDecision Plain =
			DecideStackMove(false, 9, (Mask & 1) != 0, (Mask & 2) != 0);
		TestTrue(TEXT("a non-stackable object always moves whole, whatever is held"),
			Plain.Kind == StackMoveKind::Send && Plain.Count == 1);
	}

	// The two bounds are the protocol's: CMoveObject refuses a count of zero
	// and reads exactly one byte.
	TestEqual(TEXT("the answer cannot fall below one"),
		static_cast<int32>(ClampStackAmount(0, 10)), 1);
	TestEqual(TEXT("nor rise above the stack"),
		static_cast<int32>(ClampStackAmount(11, 10)), 10);
	TestEqual(TEXT("nor above the one count byte on the wire"),
		static_cast<int32>(ClampStackAmount(999, 999)),
		static_cast<int32>(kMaxWireStackCount));
	TestEqual(TEXT("a stack of one admits exactly one answer"),
		static_cast<int32>(ClampStackAmount(7, 1)), 1);

	// ------------------------------------------------------- the window rules
	TestEqual(TEXT("the heading is the classic client's own title"),
		SReal33DAmountPanel::Title().ToString(), FString(TEXT("Move Objects")));
	TestEqual(TEXT("the answer is shown with the bound it sits in"),
		SReal33DAmountPanel::AmountText(3, 12).ToString(), FString(TEXT("3  of 12")));
	TestEqual(TEXT("and an out-of-range answer is shown clamped, not raw"),
		SReal33DAmountPanel::AmountText(99, 12).ToString(), FString(TEXT("12  of 12")));
	const FString Note = SReal33DAmountPanel::NoteText().ToString();
	TestTrue(TEXT("the note says nothing has been sent yet"),
		Note.Contains(TEXT("Nothing has been sent")));
	TestTrue(TEXT("the note says cancel sends nothing"),
		Note.Contains(TEXT("Cancel sends nothing")));
	TestFalse(TEXT("the note never claims the objects were moved"),
		Note.Contains(TEXT("moved")) || Note.Contains(TEXT("success")));

	// ----------------------------------------------------- the window's life
	int32 Chosen = 0;
	int32 ChosenCalls = 0;
	int32 AbandonCalls = 0;
	const TSharedRef<SReal33DAmountPanel> Window = SNew(SReal33DAmountPanel)
		.OnChosen(FReal33DOnAmountChosen::CreateLambda(
			[&Chosen, &ChosenCalls](uint32 Amount)
			{
				Chosen = static_cast<int32>(Amount);
				++ChosenCalls;
			}))
		.OnAbandoned(FReal33DOnAmountAbandoned::CreateLambda(
			[&AbandonCalls]() { ++AbandonCalls; }));

	// Built closed: a drag opens this window, never the client on its own.
	TestFalse(TEXT("the question is not on screen before a drag"), Window->IsShowing());
	TestTrue(TEXT("and takes no space"),
		Window->GetVisibility() == EVisibility::Collapsed);

	// Abandoning nothing reports nothing, so Escape is always safe to press.
	Window->Abandon();
	TestEqual(TEXT("abandoning a question that was never asked reports nothing"),
		AbandonCalls, 0);

	Window->Ask(12, FText::FromString(TEXT("Object 3031 from body slot 10 to container 0 slot 2.")));
	TestTrue(TEXT("asking puts the question on screen"), Window->IsShowing());
	TestEqual(TEXT("bounded by the stack the server reported"),
		static_cast<int32>(Window->GetMax()), 12);
	TestEqual(TEXT("and opening on it, so Ok alone equals a plain drag"),
		static_cast<int32>(Window->GetAmount()), 12);
	TestTrue(TEXT("the question does not swallow clicks on the panels behind it"),
		Window->GetVisibility() == EVisibility::SelfHitTestInvisible);

	// Cancel: the window closes, the caller is told, and NOTHING was chosen.
	Window->Abandon();
	TestFalse(TEXT("cancelling closes the question"), Window->IsShowing());
	TestTrue(TEXT("and gives its space back"),
		Window->GetVisibility() == EVisibility::Collapsed);
	TestEqual(TEXT("the caller is told exactly once"), AbandonCalls, 1);
	TestEqual(TEXT("and no amount was ever chosen, so nothing can have been sent"),
		ChosenCalls, 0);
	TestEqual(TEXT("no amount reached the caller"), Chosen, 0);

	// A stack the server described as empty is still one object, and the
	// question opens on an answer CMoveObject would accept rather than on zero.
	Window->Ask(0, FText::GetEmpty());
	TestEqual(TEXT("a zero stack still bounds the question at one"),
		static_cast<int32>(Window->GetMax()), 1);
	TestEqual(TEXT("and opens on one"), static_cast<int32>(Window->GetAmount()), 1);
	Window->Abandon();

	// Registered like every other surface, and distinct from all three of the
	// modals that existed before it.
	FReal33DPanelRegistry Registry;
	const TSharedRef<SWidget> Entry = SNew(SBox);
	TestTrue(TEXT("the question registers under its own id"),
		Registry.Register(Real33D::PanelIds::MoveAmount, Entry));
	TestFalse(TEXT("and cannot be registered twice"),
		Registry.Register(Real33D::PanelIds::MoveAmount, Entry));
	TestTrue(TEXT("the id is distinct from the three earlier modals"),
		Real33D::PanelIds::MoveAmount != Real33D::PanelIds::Outfit
			&& Real33D::PanelIds::MoveAmount != Real33D::PanelIds::Trade
			&& Real33D::PanelIds::MoveAmount != Real33D::PanelIds::TextEditor);
	return true;
}
#endif
