#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Real33D
{
// The chat surfaces this client has open, and the rule that decides which one
// an arriving line belongs to.
//
// Plain std, no Slate and no protocol, for the same reason ClientCore's ChatLog
// is not in a widget: the rules worth getting right here are identity, routing
// and removal, and all three are testable without a renderer or a server.
// Whether the operator can *see* the resulting tab is a separate question that
// only a live run answers.
//
// WHAT THIS IS NOT. It is not a list of channels that exist. Fusion32 owns
// that list and sends it as SV_CMD_CHANNELS; a view appears here only after the
// server has confirmed an open (SV_CMD_OPEN_CHANNEL, SV_CMD_OPEN_OWN_CHANNEL,
// SV_CMD_PRIVATE_CHANNEL) or after a line has actually arrived from a named
// character. Nothing here invents a conversation.
//
// Not thread-safe by design. One owner, on one thread: the game thread, which
// is where events are drained and where the widget reads.

enum class ChatViewKind : std::uint8_t
{
	// Everything not addressed to a channel: ordinary speech, the server's own
	// messages, and this client's notices. Always present, never removable.
	Local,
	// A numbered 7.72 channel Fusion32 confirmed this player is in.
	Channel,
	// A private conversation with one named character. Carries no channel
	// number at all, because SV_CMD_PRIVATE_CHANNEL carries none.
	Private,
};

struct ChatView
{
	// Monotonic and never reused, including across a reconnect. A widget holds
	// one of these rather than an index, so a callback left over from a tab
	// that has since closed cannot address whatever conversation now occupies
	// its position.
	std::uint32_t id = 0;
	ChatViewKind kind = ChatViewKind::Local;
	// Meaningful for Channel only.
	std::uint16_t channel = 0;
	// The server's own channel name, or the character a private conversation is
	// with, in the spelling Fusion32 used. Empty for Local, whose label is this
	// client's own word for it and belongs to presentation.
	std::string name;
	// True when the channel arrived as SV_CMD_OPEN_OWN_CHANNEL. Recorded
	// because the two commands are the same shape and only the opcode says
	// whose channel it is; nothing here acts on it.
	bool own_channel = false;
};

class ChatViewSet
{
public:
	ChatViewSet() { Push(ChatViewKind::Local, 0, std::string(), false); }

	const std::vector<ChatView>& views() const { return Views; }
	std::size_t size() const { return Views.size(); }

	/** Local Chat's id. It is the first view and is always present. */
	std::uint32_t LocalId() const { return Views.front().id; }

	const ChatView* Find(std::uint32_t id) const
	{
		for (const ChatView& View : Views)
		{
			if (View.id == id) return &View;
		}
		return nullptr;
	}

	/** The open channel's view id, or 0 when this client is not in it. */
	std::uint32_t FindChannel(std::uint16_t channel) const
	{
		for (const ChatView& View : Views)
		{
			if (View.kind == ChatViewKind::Channel && View.channel == channel)
			{
				return View.id;
			}
		}
		return 0;
	}

	/**
	 * The open private conversation's view id, or 0.
	 *
	 * Matched without regard to case. Fusion32 addresses a character
	 * case-insensitively -- operate.cc::GetCharacterID -- so "rook sample" and
	 * "Rook Sample" are one character, and giving them two tabs would split one
	 * conversation across two surfaces.
	 */
	std::uint32_t FindPrivate(const std::string& name) const
	{
		for (const ChatView& View : Views)
		{
			if (View.kind == ChatViewKind::Private && SameName(View.name, name))
			{
				return View.id;
			}
		}
		return 0;
	}

	/**
	 * Records a channel Fusion32 confirmed. Returns its view id.
	 *
	 * Re-opening one that is already here updates the name and the ownership
	 * flag instead of adding a second tab for the same number: the server
	 * resends the open whenever the player joins again, and two tabs for one
	 * channel would split its transcript.
	 */
	std::uint32_t OpenChannel(std::uint16_t channel, const std::string& name, bool own)
	{
		if (const std::uint32_t Existing = FindChannel(channel))
		{
			ChatView& View = At(Existing);
			View.name = name;
			View.own_channel = own;
			return Existing;
		}
		return Push(ChatViewKind::Channel, channel, name, own);
	}

	/** Records a private conversation. Returns its view id, existing or new. */
	std::uint32_t OpenPrivate(const std::string& name)
	{
		if (name.empty())
		{
			// CPrivateChannel returns an empty name unsent, and a tab with no
			// character behind it could never be addressed.
			return 0;
		}
		if (const std::uint32_t Existing = FindPrivate(name))
		{
			return Existing;
		}
		return Push(ChatViewKind::Private, 0, name, false);
	}

	/**
	 * Closes a view. Returns false for an unknown id and for Local Chat.
	 *
	 * Local Chat is refused rather than removed: it is where every unaddressed
	 * line goes, and a client with nowhere to put ordinary speech would discard
	 * what the server said.
	 */
	bool Close(std::uint32_t id)
	{
		if (Views.empty() || id == LocalId())
		{
			return false;
		}
		for (std::size_t Index = 0; Index < Views.size(); ++Index)
		{
			if (Views[Index].id != id) continue;
			Views.erase(Views.begin() + static_cast<std::ptrdiff_t>(Index));
			if (SelectedId == id)
			{
				// Back to Local rather than to a neighbour. The player did not
				// choose the neighbour, and landing on another conversation
				// would send their next line somewhere they did not look at.
				SelectedId = LocalId();
			}
			return true;
		}
		return false;
	}

	/** Closes a channel by number, which is what SV_CMD_CLOSE_CHANNEL carries. */
	bool CloseChannel(std::uint16_t channel)
	{
		const std::uint32_t Id = FindChannel(channel);
		return Id != 0 && Close(Id);
	}

	/**
	 * Where a channel line belongs.
	 *
	 * Falls back to Local Chat for a channel this client is not in. Fusion32
	 * can legitimately deliver one: a message is broadcast to every subscriber
	 * at the moment it is spoken, and CL_CMD_LEAVE_CHANNEL is answered by
	 * nothing, so a line already in flight arrives after the tab has gone.
	 * Dropping it would lose something the server actually said.
	 */
	std::uint32_t RouteChannel(std::uint16_t channel) const
	{
		const std::uint32_t Id = FindChannel(channel);
		return Id != 0 ? Id : LocalId();
	}

	/**
	 * Where a private line belongs, opening a conversation if there is none.
	 *
	 *     PRIVATE_TAB_ON_ARRIVAL = REAL33D_UI_BEHAVIOUR
	 *
	 * Fusion32 does not announce an incoming private message with
	 * SV_CMD_PRIVATE_CHANNEL: operate.cc sends the receiver a plain SV_CMD_TALK
	 * in TALK_PRIVATE_MESSAGE mode and tells the sender "Message sent to X."
	 * through SV_CMD_MESSAGE. So the receiving client has to decide where to
	 * put it. Opening the conversation is still driven entirely by a line the
	 * server really delivered, and the name is the one the server put in it.
	 *
	 * An unnamed private line has no conversation to belong to and goes to
	 * Local Chat rather than to a nameless tab.
	 */
	std::uint32_t RoutePrivate(const std::string& name)
	{
		if (name.empty())
		{
			return LocalId();
		}
		return OpenPrivate(name);
	}

	/** The view the player is looking at, and the one a typed line is sent to. */
	std::uint32_t Selected() const { return SelectedId; }

	bool Select(std::uint32_t id)
	{
		if (Find(id) == nullptr)
		{
			return false;
		}
		SelectedId = id;
		return true;
	}

	/**
	 * Back to Local Chat alone. Used when a session ends.
	 *
	 * The id counter keeps counting, so a view from the previous connection and
	 * one from the next are never confusable in evidence.
	 */
	void Reset()
	{
		Views.clear();
		Push(ChatViewKind::Local, 0, std::string(), false);
	}

private:
	static bool SameName(const std::string& left, const std::string& right)
	{
		if (left.size() != right.size()) return false;
		for (std::size_t Index = 0; Index < left.size(); ++Index)
		{
			if (Lower(left[Index]) != Lower(right[Index])) return false;
		}
		return true;
	}

	// Spelled out rather than std::tolower, which is locale-dependent and takes
	// an int. A character name is ASCII.
	static char Lower(char value)
	{
		return (value >= 'A' && value <= 'Z')
			? static_cast<char>(value - 'A' + 'a') : value;
	}

	ChatView& At(std::uint32_t id)
	{
		for (ChatView& View : Views)
		{
			if (View.id == id) return View;
		}
		// Only ever called with an id Find has just returned.
		return Views.front();
	}

	std::uint32_t Push(ChatViewKind kind, std::uint16_t channel,
		const std::string& name, bool own)
	{
		ChatView View;
		View.id = ++NextId;
		View.kind = kind;
		View.channel = channel;
		View.name = name;
		View.own_channel = own;
		Views.push_back(View);
		if (kind == ChatViewKind::Local)
		{
			SelectedId = View.id;
		}
		return View.id;
	}

	std::vector<ChatView> Views;
	std::uint32_t NextId = 0;
	std::uint32_t SelectedId = 0;
};
}
