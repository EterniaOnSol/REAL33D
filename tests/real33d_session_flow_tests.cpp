// The launch -> login -> character list -> game -> logout -> login flow,
// without Unreal, without a server and without a human at the GUI.
//
// The cases that matter are the ones the client could not do at all before
// UNREAL-LOGIN-CHARACTER-SELECT-UI-001: that a session which has ended leaves
// the player somewhere they can act, that entering the game is reachable from
// exactly one phase so a second INIT_GAME cannot be asked for, and that none of
// it disturbs the reconnect-ownership rule a displaced client depends on.

#include "Real33DSessionFlow.h"

#include <cassert>
#include <cstring>
#include <iostream>

namespace
{
using namespace Real33D;

constexpr bool kOrderly = true;
constexpr bool kLost = false;
constexpr bool kHaveList = true;
constexpr bool kNoList = false;

/** Every phase, so a loop cannot quietly miss one that is added later. */
constexpr SessionPhase kAllPhases[] = {
	SessionPhase::Offline,
	SessionPhase::LoggingIn,
	SessionPhase::CharacterSelect,
	SessionPhase::EnteringGame,
	SessionPhase::InGame,
	SessionPhase::Reconnecting,
};

void TheFullFlowIsReachableInOrder()
{
	// Launch. Nothing is proven about anybody, so the account screen is the
	// surface and the only thing that may be started is a login.
	SessionPhase Phase = SessionPhase::Offline;
	assert(ShowsLoginSurface(Phase));
	assert(MayBeginAccountLogin(Phase));
	assert(!MayEnterGame(Phase));
	assert(!MayLogout(Phase));

	// The login request is in flight. Still the login surface -- the player is
	// watching it -- but nothing further may be asked for.
	Phase = SessionPhase::LoggingIn;
	assert(ShowsLoginSurface(Phase));
	assert(!MayBeginAccountLogin(Phase));
	assert(!MayEnterGame(Phase));

	// The login service answered. This is the one phase a character may be
	// entered from, and the account screen must not be re-submitted over it.
	Phase = SessionPhase::CharacterSelect;
	assert(ShowsLoginSurface(Phase));
	assert(MayEnterGame(Phase));
	assert(!MayBeginAccountLogin(Phase));

	// The game worker is running. The login surface is gone the moment a
	// character is committed to, not when the world finishes arriving.
	Phase = SessionPhase::EnteringGame;
	assert(!ShowsLoginSurface(Phase));
	assert(!MayEnterGame(Phase));
	assert(MayLogout(Phase));

	// INIT_GAME arrived.
	Phase = SessionPhase::InGame;
	assert(!ShowsLoginSurface(Phase));
	assert(!MayEnterGame(Phase));
	assert(MayLogout(Phase));
}

void EnteringTheGameHasExactlyOneDoor()
{
	// The duplicate-INIT_GAME guard, stated as the property it exists for: of
	// the six phases exactly one admits a game worker. A second worker started
	// while the first is alive is what produces two INIT_GAME commands for one
	// player-visible session, because Fusion32 displaces the first connection
	// rather than refusing the second.
	int Doors = 0;
	for (const SessionPhase Phase : kAllPhases)
	{
		if (MayEnterGame(Phase))
		{
			++Doors;
			assert(Phase == SessionPhase::CharacterSelect);
		}
	}
	assert(Doors == 1);

	// And the same for starting an account login, for the same reason one port
	// down: two login connections for one account is two answers to one
	// question.
	int LoginDoors = 0;
	for (const SessionPhase Phase : kAllPhases)
	{
		if (MayBeginAccountLogin(Phase))
		{
			++LoginDoors;
			assert(Phase == SessionPhase::Offline);
		}
	}
	assert(LoginDoors == 1);
}

void ALogoutReturnsToTheCharacterList()
{
	// This client's own logout is an orderly close. It must never be retried --
	// reclaiming a character the player just put down is the same defect as
	// reclaiming one somebody else took -- and the player must land on the
	// list, not in an empty scene.
	for (int Attempts = 0; Attempts <= kMaxReconnectAttempts + 2; ++Attempts)
	{
		const SessionEndOutcome Outcome =
			ResolveSessionEnd(kOrderly, Attempts, kHaveList);
		assert(!Outcome.bRetry);
		assert(Outcome.Phase == SessionPhase::CharacterSelect);
		assert(ShowsLoginSurface(Outcome.Phase));
		assert(MayEnterGame(Outcome.Phase));
	}
}

void ADeathReturnsToTheCharacterList()
{
	// A death ends as an orderly close: `TCreature`'s destructor calls
	// `Logout(30, true)` and Fusion32 closes the socket. It is therefore the
	// same ending as a logout and must reach the same place -- which is the
	// whole reason this is a rule about the KIND of ending rather than about
	// who caused it. Certified live under UNREAL-PLAYER-DEATH-RESPAWN-001.
	const SessionEndOutcome Death = ResolveSessionEnd(kOrderly, 0, kHaveList);
	const SessionEndOutcome Logout = ResolveSessionEnd(kOrderly, 0, kHaveList);
	assert(Death.Phase == Logout.Phase);
	assert(Death.bRetry == Logout.bRetry);
	assert(!Death.bRetry);
	assert(Death.Phase == SessionPhase::CharacterSelect);
}

void ADisplacementStillDoesNotReclaim()
{
	// The regression this must not cause. Fusion32 hands a character to a
	// second client by closing the first one's socket silently, and the
	// displaced client used to log straight back in and take it away again.
	// Returning to the character list is a surface change; it must not become
	// an automatic re-entry.
	for (int Attempts = 0; Attempts <= kMaxReconnectAttempts + 2; ++Attempts)
	{
		const SessionEndOutcome Outcome =
			ResolveSessionEnd(kOrderly, Attempts, kHaveList);
		assert(!Outcome.bRetry);
		// The list is offered, which is a thing the PLAYER acts on. Nothing
		// here enters a game by itself.
		assert(Outcome.Phase == SessionPhase::CharacterSelect);
	}

	// Stated against the policy header directly as well, so the two cannot
	// drift: no attempt count and no delay makes a close retryable.
	for (int Attempts = 0; Attempts <= kMaxReconnectAttempts + 2; ++Attempts)
	{
		for (const double Waited : { 0.0, kReconnectDelaySeconds + 1.0, 86400.0 })
		{
			assert(!ShouldReconnect(SessionEnd::Closed, Attempts, Waited));
		}
	}
}

void ALostLinkIsStillRetriedAndNotOfferedToThePlayer()
{
	// The reconnect path, unchanged. While the client is coming back by itself
	// the player is not asked anything: putting the character list up during
	// the retry would invite a second login while the first is still in
	// flight, which is exactly how two workers -- and two INIT_GAMEs -- happen.
	for (int Attempts = 0; Attempts < kMaxReconnectAttempts; ++Attempts)
	{
		const SessionEndOutcome Outcome =
			ResolveSessionEnd(kLost, Attempts, kHaveList);
		assert(Outcome.bRetry);
		assert(Outcome.Phase == SessionPhase::Reconnecting);
		assert(!ShowsLoginSurface(Outcome.Phase));
		assert(!MayEnterGame(Outcome.Phase));
	}
}

void AnExhaustedRetryLandsOnTheList()
{
	// What used to be the end of the road. After the attempt cap the old
	// client simply stopped, with an empty scene and no way back; now the
	// player gets the list.
	for (int Attempts = kMaxReconnectAttempts; Attempts <= kMaxReconnectAttempts + 5;
		++Attempts)
	{
		const SessionEndOutcome Outcome =
			ResolveSessionEnd(kLost, Attempts, kHaveList);
		assert(!Outcome.bRetry);
		assert(Outcome.Phase == SessionPhase::CharacterSelect);
	}
}

void WithoutAListTheAccountScreenIsTheFallback()
{
	// A list from an account that was never authenticated this run would be a
	// guess, so there is nothing to offer and the player is asked again.
	for (const bool bOrderly : { true, false })
	{
		const SessionEndOutcome Outcome = ResolveSessionEnd(
			bOrderly, kMaxReconnectAttempts, kNoList);
		assert(!Outcome.bRetry);
		assert(Outcome.Phase == SessionPhase::Offline);
		assert(MayBeginAccountLogin(Outcome.Phase));
	}

	// A lost link with attempts left still retries, list or no list: whether
	// the player has somewhere to go is not what decides whether the link is
	// worth another try.
	const SessionEndOutcome Retrying = ResolveSessionEnd(kLost, 0, kNoList);
	assert(Retrying.bRetry);
	assert(Retrying.Phase == SessionPhase::Reconnecting);
}

void TheWorldIsTornDownForEveryEnding()
{
	// Including the retried one. The retry re-receives the whole world in a
	// fresh SV_CMD_FULLSCREEN, so nothing may survive the gap; an Actor kept
	// across it would be a ghost with no WorldState behind it.
	for (const bool bOrderly : { true, false })
	{
		for (int Attempts = 0; Attempts <= kMaxReconnectAttempts + 1; ++Attempts)
		{
			for (const bool bList : { true, false })
			{
				assert(RequiresWorldTeardown(
					ResolveSessionEnd(bOrderly, Attempts, bList)));
			}
		}
	}
}

void TheLoginSurfaceAndTheHudAreNeverBothShown()
{
	// One rule answers both, so there is no state in which the character list
	// is typed into over a live game, or in which a dead session leaves the
	// HUD up with nothing behind it. Three phases each way, and the split is
	// exactly "has a game connection been committed to".
	int Login = 0;
	int Game = 0;
	for (const SessionPhase Phase : kAllPhases)
	{
		if (ShowsLoginSurface(Phase)) { ++Login; } else { ++Game; }
	}
	assert(Login == 3);
	assert(Game == 3);

	// The login surface is exactly the phases with no game worker running.
	assert(ShowsLoginSurface(SessionPhase::Offline));
	assert(ShowsLoginSurface(SessionPhase::LoggingIn));
	assert(ShowsLoginSurface(SessionPhase::CharacterSelect));
	assert(!ShowsLoginSurface(SessionPhase::EnteringGame));
	assert(!ShowsLoginSurface(SessionPhase::InGame));
	assert(!ShowsLoginSurface(SessionPhase::Reconnecting));
}

void RepeatedCharacterSwitchingIsOrdinary()
{
	// Switching character is a logout and a fresh entry, not a special case.
	// Ten round trips, each of which must pass through the one door and come
	// back to the list, and none of which may retry anything.
	SessionPhase Phase = SessionPhase::CharacterSelect;
	for (int Switch = 0; Switch < 10; ++Switch)
	{
		assert(MayEnterGame(Phase));
		Phase = SessionPhase::EnteringGame;
		assert(!MayEnterGame(Phase));
		Phase = SessionPhase::InGame;
		assert(MayLogout(Phase));

		const SessionEndOutcome Outcome = ResolveSessionEnd(kOrderly, 0, kHaveList);
		assert(!Outcome.bRetry);
		Phase = Outcome.Phase;
		assert(Phase == SessionPhase::CharacterSelect);
	}
}

void EveryPhaseHasADistinctName()
{
	// The names reach logs and evidence, so two phases sharing one would make
	// a run unreadable after the fact.
	for (const SessionPhase A : kAllPhases)
	{
		assert(std::strcmp(SessionPhaseName(A), "unknown") != 0);
		for (const SessionPhase B : kAllPhases)
		{
			if (A == B) { continue; }
			assert(std::strcmp(SessionPhaseName(A), SessionPhaseName(B)) != 0);
		}
	}
}

} // namespace

int main()
{
	TheFullFlowIsReachableInOrder();
	EnteringTheGameHasExactlyOneDoor();
	ALogoutReturnsToTheCharacterList();
	ADeathReturnsToTheCharacterList();
	ADisplacementStillDoesNotReclaim();
	ALostLinkIsStillRetriedAndNotOfferedToThePlayer();
	AnExhaustedRetryLandsOnTheList();
	WithoutAListTheAccountScreenIsTheFallback();
	TheWorldIsTornDownForEveryEnding();
	TheLoginSurfaceAndTheHudAreNeverBothShown();
	RepeatedCharacterSwitchingIsOrdinary();
	EveryPhaseHasADistinctName();

	std::cout << "real33d session flow tests: 12 cases passed\n";
	return 0;
}
