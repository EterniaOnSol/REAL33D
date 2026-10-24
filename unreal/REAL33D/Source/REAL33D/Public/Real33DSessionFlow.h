#pragma once

#include "Real33DReconnectPolicy.h"

#include <cstdint>

// Where the client is between launching and playing, and what each ending of a
// session means for where it goes next.
//
// THIS EXISTS BECAUSE THE CLIENT USED TO HAVE NO SUCH PLACE. Before
// UNREAL-LOGIN-CHARACTER-SELECT-UI-001 there was exactly one path: the world
// actor's BeginPlay called Connect, the worker read an account out of a file,
// took `characters.front()` without asking, and when the session ended the
// worker returned and the process was finished. A death, a logout or a
// displacement left a running application with an empty scene and no way back
// in except closing it and launching again.
//
// The states below are the smallest set that distinguishes the moments that
// actually differ, and the distinctions are not cosmetic:
//
//   * `LoggingIn` is not `EnteringGame`. They are two different services on two
//     different ports -- the login service on 7171 answers with a character
//     list and closes, the game service on 7172 answers with INIT_GAME -- and a
//     failure of one says nothing about the other. Collapsing them is what made
//     "the character list was not returned" the client's only word for every
//     wrong password, unreachable host and missing character alike.
//   * `CharacterSelect` is not `Offline`. In the first the account is
//     authenticated and the server's own character list is in hand; in the
//     second nothing has been proven about anybody. Entering the game is
//     allowed from exactly one of them.
//   * `Reconnecting` is not `CharacterSelect`. A LOST link is retried by
//     `Real33DReconnectPolicy.h` and the player is not asked anything; putting
//     the character list back on screen during that retry would invite a second
//     login while the first is still coming back.
//
// Free of Unreal types on purpose, like `Real33DReconnectPolicy.h` and
// `Real33DWalkDirection.h`: tests/real33d_session_flow_tests.cpp compiles it
// with a plain C++ toolchain, so the flow is provable without a renderer, a
// server or a human at the GUI.
namespace Real33D
{

/** Where the client is between launching and playing. */
enum class SessionPhase : std::uint8_t
{
	/**
	 * Nothing is running and nothing has been proven. The account screen is
	 * asking for an account number and a password.
	 */
	Offline,
	/**
	 * The account-login worker is talking to the login service. No game
	 * connection exists and none may be started.
	 */
	LoggingIn,
	/**
	 * The login service answered with this account's characters. The list is
	 * the server's and the player is choosing from it.
	 */
	CharacterSelect,
	/**
	 * A game worker is running and INIT_GAME has not arrived. The character is
	 * chosen and the credentials are in flight; nothing is on screen yet.
	 */
	EnteringGame,
	/** Fusion32 sent INIT_GAME and identified the local player. */
	InGame,
	/**
	 * The link was LOST and the client is retrying it on its own, under
	 * `Real33DReconnectPolicy.h`. The player is not asked anything here.
	 */
	Reconnecting,
};

/** The phase's name, for logs and evidence. Never shown as a player message. */
inline const char* SessionPhaseName(SessionPhase Phase)
{
	switch (Phase)
	{
	case SessionPhase::Offline:         return "offline";
	case SessionPhase::LoggingIn:       return "logging-in";
	case SessionPhase::CharacterSelect: return "character-select";
	case SessionPhase::EnteringGame:    return "entering-game";
	case SessionPhase::InGame:          return "in-game";
	case SessionPhase::Reconnecting:    return "reconnecting";
	}
	return "unknown";
}

/**
 * Whether an account login may be started now.
 *
 * Offline only. Asking again while the first request is in flight would put two
 * login connections on 7171 for one account, and asking while a game session is
 * up would authenticate an account whose character is already playing.
 */
inline bool MayBeginAccountLogin(SessionPhase Phase)
{
	return Phase == SessionPhase::Offline;
}

/**
 * Whether a character may be entered now.
 *
 * CHARACTER SELECT ONLY, AND THIS IS THE DUPLICATE-INIT_GAME GUARD. A second
 * game worker started while the first is alive would produce a second
 * connection for the same character, which Fusion32 answers by displacing the
 * first -- `connections.cc` zeroes the old CharacterID and calls
 * `TConnection::Logout` -- and the client would see two INIT_GAME commands for
 * one player-visible session. One worker at a time, started from one phase.
 */
inline bool MayEnterGame(SessionPhase Phase)
{
	return Phase == SessionPhase::CharacterSelect;
}

/** Whether a game session exists that logging out would end. */
inline bool MayLogout(SessionPhase Phase)
{
	return Phase == SessionPhase::EnteringGame
		|| Phase == SessionPhase::InGame
		|| Phase == SessionPhase::Reconnecting;
}

/**
 * Whether the login/character-select surface is the thing on screen.
 *
 * The in-game HUD is shown for exactly the complement of this, so one rule
 * answers both and the two cannot be visible at once. `Reconnecting` counts as
 * in-game: the scene is empty but the client is coming back by itself, and the
 * player has nothing to decide.
 */
inline bool ShowsLoginSurface(SessionPhase Phase)
{
	return Phase == SessionPhase::Offline
		|| Phase == SessionPhase::LoggingIn
		|| Phase == SessionPhase::CharacterSelect;
}

/** What a session that has just ended means for where the client goes next. */
struct SessionEndOutcome
{
	SessionPhase Phase = SessionPhase::Offline;
	/**
	 * The client will reconnect by itself, without asking. True only for a
	 * LOST link with attempts left; `Real33DReconnectPolicy.h` still owns WHEN,
	 * and this only says WHETHER.
	 */
	bool bRetry = false;
};

/**
 * Where a session that has just ended leaves the client.
 *
 * @param bOrderlyClose    one end closed it on purpose: a logout, a death's
 *                         orderly close, a displacement, or a server shutdown.
 * @param AttemptsSoFar    retries already made since the last success.
 * @param bHaveCharacters  the login service's character list is still in hand.
 *
 * A deliberate close never retries, at any attempt count -- that is
 * `ShouldReconnect`'s rule and it is not restated here, it is called. What this
 * adds is the other half, which did not exist before: once the client will not
 * reconnect by itself, it goes back to the surface the player can act on
 * instead of sitting in an empty scene. With a character list in hand that is
 * the list; without one it is the account screen, because offering a stale list
 * from an account that was never authenticated this run would be offering a
 * guess.
 */
inline SessionEndOutcome ResolveSessionEnd(bool bOrderlyClose, int AttemptsSoFar,
	bool bHaveCharacters)
{
	const SessionEnd End = SessionEndFromOrderlyClose(bOrderlyClose);

	// Attempts left is the whole of what distinguishes "coming back" from
	// "over". The delay is deliberately not consulted: a client that has only
	// just dropped is still going to retry, and showing the character list for
	// those five seconds would be offering a choice that is about to be taken
	// away again.
	if (End == SessionEnd::Lost && AttemptsSoFar < kMaxReconnectAttempts)
	{
		return SessionEndOutcome{ SessionPhase::Reconnecting, true };
	}

	return SessionEndOutcome{
		bHaveCharacters ? SessionPhase::CharacterSelect : SessionPhase::Offline,
		false };
}

/**
 * Whether the world must be torn down for this ending.
 *
 * Always, and that is the point of stating it as a rule rather than leaving it
 * to each caller. Every Actor in the scene exists because WorldState said so,
 * and a session that has ended has no WorldState -- including one that is about
 * to be retried, because the retry re-receives the whole world in a fresh
 * SV_CMD_FULLSCREEN. Anything kept across the gap would be a ghost with nothing
 * authoritative behind it.
 */
inline bool RequiresWorldTeardown(const SessionEndOutcome& /*Outcome*/)
{
	return true;
}

} // namespace Real33D
