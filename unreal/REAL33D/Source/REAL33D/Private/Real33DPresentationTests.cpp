#include "Real33DPresentationPolicy.h"
#include "Real33DChannelsPanel.h"
#include "Real33DChatPanel.h"
#include "Real33DChatViews.h"
#include "Real33DPanelRegistry.h"
#include "Real33DSkillsPanel.h"
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
#endif
