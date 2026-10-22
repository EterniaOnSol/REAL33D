#pragma once

#include <cstdint>

// Whether a session that is no longer up should be reconnected by the client
// on its own.
//
// THIS EXISTS BECAUSE OF AN OWNERSHIP DEFECT, not a networking one. Fusion32
// hands a character to a second client by closing the first client's socket:
// `connections.cc` zeroes the old connection's CharacterID and calls
// `TConnection::Logout`, which -- because CharacterID is already zero -- does
// not even reach `StartLogout`. Nothing is sent. The displaced client simply
// sees its socket close.
//
// The client then retried, five seconds later, up to three times. That logged
// it straight back in, which displaced the client that had just taken the
// character, which retried in turn. Two clients produced six handovers and
// three reconnects each before the attempt cap stopped it, and the character
// belonged to whichever had retried most recently. Observed, before the fix:
//
//     R_A  connects 4  inits 4  reconnects 3
//     R_B  connects 4  inits 4  reconnects 3
//
// The rule below is the smallest thing that separates the two cases, and it
// uses the only evidence there is: whether the connection was CLOSED by one
// end or LOST. A close is deliberate and final -- a displacement, a server
// shutdown, or this client's own logout -- and reclaiming after one is how the
// ping-pong happened. A loss is a refused or unreachable connect or a
// transport error, says nothing about who owns the character, and is still
// worth retrying.
//
// Reconnect is NOT disabled. Every loss is still retried, with the attempt cap
// unchanged; only a deliberate close is left alone.
//
// Deliberately free of Unreal types: tests/real33d_reconnect_policy_tests.cpp
// compiles it with a plain C++ toolchain, like the other tests/real33d_*.cpp
// suites.
namespace Real33D
{

/** How a session stopped being usable. */
enum class SessionEnd : std::uint8_t
{
	/** Still up. Nothing to decide. */
	None,
	/**
	 * One end closed it on purpose. Fusion32 displacing this client, the
	 * server going down, or this client's own logout. On the wire this is the
	 * transport latching `ConnectionState::RemoteClosed`.
	 */
	Closed,
	/**
	 * The link failed: a connect that was refused or unreachable, or a
	 * transport error. Nothing here says anything about ownership.
	 */
	Lost,
};

/** The attempt cap, unchanged by this policy. */
inline constexpr int kMaxReconnectAttempts = 3;

/** Seconds a client waits before retrying a lost link. Unchanged. */
inline constexpr double kReconnectDelaySeconds = 5.0;

/**
 * Whether the client may reconnect on its own.
 *
 * @param End              how the session stopped.
 * @param AttemptsSoFar    retries already made since the last success.
 * @param SecondsSinceDrop how long the session has been down.
 *
 * A deliberate close is never retried, at any attempt count and after any
 * delay: waiting longer does not make reclaiming someone else's character
 * right, which is why this is a rule about the KIND of ending and not a
 * backoff.
 */
inline bool ShouldReconnect(SessionEnd End, int AttemptsSoFar, double SecondsSinceDrop)
{
	if (End != SessionEnd::Lost)
	{
		return false;
	}
	if (AttemptsSoFar >= kMaxReconnectAttempts)
	{
		return false;
	}
	return SecondsSinceDrop > kReconnectDelaySeconds;
}

/** The transport's latched state, as this policy reads it. */
inline SessionEnd SessionEndFromOrderlyClose(bool bOrderlyClose)
{
	return bOrderlyClose ? SessionEnd::Closed : SessionEnd::Lost;
}

} // namespace Real33D
