// The chat-view model: identity, routing and removal, without Slate, without a
// bridge and without a server. What a live run then has to prove is that the
// operator can see and use the surfaces this decides on.

#include "Real33DChatViews.h"

#include <cassert>
#include <iostream>
#include <string>

namespace
{
using Real33D::ChatView;
using Real33D::ChatViewKind;
using Real33D::ChatViewSet;

void LocalChatAlwaysExists()
{
	ChatViewSet Views;
	assert(Views.size() == 1);
	assert(Views.views().front().kind == ChatViewKind::Local);
	assert(Views.LocalId() != 0);
	assert(Views.Selected() == Views.LocalId());
	// Local Chat carries no channel number and no character name.
	assert(Views.views().front().channel == 0);
	assert(Views.views().front().name.empty());

	// It cannot be closed: it is where every unaddressed line goes.
	assert(!Views.Close(Views.LocalId()));
	assert(Views.size() == 1);
	// Nor by an id nobody issued.
	assert(!Views.Close(0));
	assert(!Views.Close(9999));
	assert(Views.size() == 1);
}

void ChannelsOpenAndClose()
{
	ChatViewSet Views;
	const std::uint32_t Local = Views.LocalId();

	// Nothing is joined until the server says so.
	assert(Views.FindChannel(4) == 0);

	const std::uint32_t Game = Views.OpenChannel(4, "Game-Chat", false);
	assert(Game != 0 && Game != Local);
	assert(Views.size() == 2);
	assert(Views.FindChannel(4) == Game);
	assert(Views.Find(Game)->kind == ChatViewKind::Channel);
	assert(Views.Find(Game)->channel == 4);
	assert(Views.Find(Game)->name == "Game-Chat");
	assert(!Views.Find(Game)->own_channel);
	// Local Chat is preserved, and stays first.
	assert(Views.views().front().id == Local);

	// A second channel is a second view, in the order they were opened.
	const std::uint32_t Help = Views.OpenChannel(7, "Help", false);
	assert(Views.size() == 3);
	assert(Views.views()[1].id == Game && Views.views()[2].id == Help);
	assert(Views.FindChannel(7) == Help);
	// And is not confused with the first.
	assert(Game != Help);
	assert(Views.FindChannel(5) == 0);

	// Re-opening a channel already here updates it instead of adding a second
	// tab for the same number: the server resends the open on every join.
	const std::uint32_t Again = Views.OpenChannel(4, "Game-Chat renamed", true);
	assert(Again == Game);
	assert(Views.size() == 3);
	assert(Views.Find(Game)->name == "Game-Chat renamed");
	assert(Views.Find(Game)->own_channel);

	// Closing is by the number SV_CMD_CLOSE_CHANNEL carries.
	assert(Views.CloseChannel(4));
	assert(Views.size() == 2);
	assert(Views.FindChannel(4) == 0);
	assert(Views.Find(Game) == nullptr);
	// Closing one leaves the other alone.
	assert(Views.FindChannel(7) == Help);
	// Closing a channel that is not open is a refusal, not a crash.
	assert(!Views.CloseChannel(4));
	assert(!Views.CloseChannel(99));
	assert(Views.size() == 2);
}

void PrivateConversations()
{
	ChatViewSet Views;
	const std::uint32_t Rook = Views.OpenPrivate("Rook Sample");
	assert(Rook != 0);
	assert(Views.size() == 2);
	assert(Views.Find(Rook)->kind == ChatViewKind::Private);
	assert(Views.Find(Rook)->name == "Rook Sample");
	// A private conversation carries no channel number, because
	// SV_CMD_PRIVATE_CHANNEL carries none.
	assert(Views.Find(Rook)->channel == 0);

	// One character, one tab, whatever the case the name arrives in: Fusion32
	// addresses a character case-insensitively.
	assert(Views.OpenPrivate("Rook Sample") == Rook);
	assert(Views.OpenPrivate("rook sample") == Rook);
	assert(Views.OpenPrivate("ROOK SAMPLE") == Rook);
	assert(Views.size() == 2);
	// The spelling the server first gave is the one kept.
	assert(Views.Find(Rook)->name == "Rook Sample");
	// A different character is a different conversation.
	const std::uint32_t Other = Views.OpenPrivate("Rook Samples");
	assert(Other != Rook);
	assert(Views.size() == 3);
	assert(Views.FindPrivate("rook samples") == Other);
	assert(Views.FindPrivate("nobody") == 0);

	// A nameless private conversation could never be addressed, so none opens.
	assert(Views.OpenPrivate("") == 0);
	assert(Views.size() == 3);

	assert(Views.Close(Rook));
	assert(Views.FindPrivate("Rook Sample") == 0);
	assert(Views.size() == 2);

	// A channel number and a character name are different namespaces: channel 0
	// is the guild channel, not the private conversation opened first.
	ChatViewSet Mixed;
	const std::uint32_t Guild = Mixed.OpenChannel(0, "Some Guild", false);
	const std::uint32_t Named = Mixed.OpenPrivate("Some Guild");
	assert(Guild != Named);
	assert(Mixed.FindChannel(0) == Guild);
	assert(Mixed.FindPrivate("Some Guild") == Named);
}

void IdsAreNeverReused()
{
	// A tab's callback holds an id, not a position. If ids were reused, a click
	// left over from a closed conversation could address a new one.
	ChatViewSet Views;
	const std::uint32_t First = Views.OpenChannel(4, "Game-Chat", false);
	assert(Views.CloseChannel(4));
	const std::uint32_t Second = Views.OpenChannel(4, "Game-Chat", false);
	assert(Second != First);
	assert(Views.Find(First) == nullptr);

	// Including across a session, which is what Reset stands for.
	const std::uint32_t BeforeReset = Views.OpenChannel(7, "Help", false);
	Views.Reset();
	assert(Views.size() == 1);
	assert(Views.views().front().kind == ChatViewKind::Local);
	assert(Views.Find(BeforeReset) == nullptr);
	assert(Views.Find(Second) == nullptr);
	assert(Views.LocalId() != 0);
	assert(Views.Selected() == Views.LocalId());
	const std::uint32_t AfterReset = Views.OpenChannel(7, "Help", false);
	assert(AfterReset != BeforeReset);
}

void RoutingNeverLosesALine()
{
	ChatViewSet Views;
	const std::uint32_t Local = Views.LocalId();

	// A channel this client is not in: the line is still something the server
	// said, so it goes to Local Chat rather than nowhere. Fusion32 answers
	// CL_CMD_LEAVE_CHANNEL with nothing, so a line in flight arrives after the
	// tab has already gone.
	assert(Views.RouteChannel(4) == Local);

	const std::uint32_t Game = Views.OpenChannel(4, "Game-Chat", false);
	assert(Views.RouteChannel(4) == Game);
	// Another channel's traffic does not land in this one.
	assert(Views.RouteChannel(7) == Local);
	const std::uint32_t Help = Views.OpenChannel(7, "Help", false);
	assert(Views.RouteChannel(7) == Help);
	assert(Views.RouteChannel(4) == Game);

	assert(Views.CloseChannel(4));
	assert(Views.RouteChannel(4) == Local);

	// A private line opens the conversation it belongs to, because the server
	// announces an incoming private message with nothing but the message.
	assert(Views.FindPrivate("Rook Sample") == 0);
	const std::uint32_t Rook = Views.RoutePrivate("Rook Sample");
	assert(Rook != Local && Rook != 0);
	assert(Views.FindPrivate("Rook Sample") == Rook);
	// The second line from the same character joins the first.
	assert(Views.RoutePrivate("Rook Sample") == Rook);
	assert(Views.RoutePrivate("rook sample") == Rook);
	// An unnamed one has no conversation to belong to.
	assert(Views.RoutePrivate("") == Local);
}

void SelectionFollowsWhatIsThere()
{
	ChatViewSet Views;
	const std::uint32_t Local = Views.LocalId();
	const std::uint32_t Game = Views.OpenChannel(4, "Game-Chat", false);

	// Opening a channel does not steal the selection: the player chose where
	// they were typing.
	assert(Views.Selected() == Local);
	assert(Views.Select(Game));
	assert(Views.Selected() == Game);
	// An id nobody issued cannot be selected, and does not disturb the choice.
	assert(!Views.Select(0));
	assert(!Views.Select(4242));
	assert(Views.Selected() == Game);

	// Closing the selected view falls back to Local, never to a neighbour the
	// player did not choose: the next line typed must not go somewhere they are
	// not looking.
	const std::uint32_t Help = Views.OpenChannel(7, "Help", false);
	assert(Views.CloseChannel(4));
	assert(Views.Selected() == Local);
	// Closing one that is not selected leaves the selection where it was.
	assert(Views.Select(Help));
	const std::uint32_t Rook = Views.OpenPrivate("Rook Sample");
	assert(Views.Close(Rook));
	assert(Views.Selected() == Help);

	Views.Reset();
	assert(Views.Selected() == Views.LocalId());
}
}

int main()
{
	LocalChatAlwaysExists();
	ChannelsOpenAndClose();
	PrivateConversations();
	IdsAreNeverReused();
	RoutingNeverLosesALine();
	SelectionFollowsWhatIsThere();
	std::cout << "chat views PASS\n";
	return 0;
}
