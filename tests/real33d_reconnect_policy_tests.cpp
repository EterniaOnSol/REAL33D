// The reconnect-ownership rule, without Unreal and without a server.
//
// The case that matters is the one that was broken: a client Fusion32 closed
// because the same character logged in elsewhere must not reconnect by itself,
// at any attempt count and after any delay. The case that must NOT regress is
// a lost link, which is still retried exactly as before.

#include "Real33DReconnectPolicy.h"

#include <cassert>
#include <iostream>

namespace
{
using namespace Real33D;

constexpr double kAfterDelay = kReconnectDelaySeconds + 0.5;

void ADeliberateCloseIsNeverReclaimed()
{
	// The displaced client. No attempt count and no amount of waiting turns a
	// close performed by the other end into something worth retrying: waiting
	// longer does not make taking someone else's character back correct.
	for (int Attempts = 0; Attempts <= kMaxReconnectAttempts + 2; ++Attempts)
	{
		for (const double Waited : {0.0, kAfterDelay, 60.0, 86400.0})
		{
			assert(!ShouldReconnect(SessionEnd::Closed, Attempts, Waited));
		}
	}
}

void ALostLinkIsStillRetried()
{
	// Unchanged behaviour, and the reason this is a rule about the KIND of
	// ending rather than reconnect being switched off.
	assert(ShouldReconnect(SessionEnd::Lost, 0, kAfterDelay));
	assert(ShouldReconnect(SessionEnd::Lost, 1, kAfterDelay));
	assert(ShouldReconnect(SessionEnd::Lost, kMaxReconnectAttempts - 1, kAfterDelay));
}

void TheAttemptCapIsUnchanged()
{
	assert(!ShouldReconnect(SessionEnd::Lost, kMaxReconnectAttempts, kAfterDelay));
	assert(!ShouldReconnect(SessionEnd::Lost, kMaxReconnectAttempts + 5, kAfterDelay));
}

void TheDelayIsUnchanged()
{
	assert(!ShouldReconnect(SessionEnd::Lost, 0, 0.0));
	assert(!ShouldReconnect(SessionEnd::Lost, 0, kReconnectDelaySeconds));
	assert(ShouldReconnect(SessionEnd::Lost, 0, kReconnectDelaySeconds + 0.001));
}

void ALiveSessionIsNeverReconnected()
{
	for (int Attempts = 0; Attempts <= kMaxReconnectAttempts; ++Attempts)
	{
		assert(!ShouldReconnect(SessionEnd::None, Attempts, kAfterDelay));
	}
}

void TheTransportStateMapsToTheRightEnding()
{
	// RemoteClosed is the only thing Fusion32 leaves behind when it displaces
	// a client, so it has to be the thing that means "closed".
	assert(SessionEndFromOrderlyClose(true) == SessionEnd::Closed);
	assert(SessionEndFromOrderlyClose(false) == SessionEnd::Lost);
	assert(!ShouldReconnect(SessionEndFromOrderlyClose(true), 0, kAfterDelay));
	assert(ShouldReconnect(SessionEndFromOrderlyClose(false), 0, kAfterDelay));
}

void TheObservedPingPongCannotHappen()
{
	// Replays the shape of the certified failure: each client is displaced,
	// and under the old rule each would retry three times. Count how many
	// reclaims the policy now permits across both clients. It must be zero.
	int Reclaims = 0;
	for (int Client = 0; Client < 2; ++Client)
	{
		for (int Attempt = 0; Attempt < kMaxReconnectAttempts; ++Attempt)
		{
			if (ShouldReconnect(SessionEnd::Closed, Attempt, kAfterDelay))
			{
				++Reclaims;
			}
		}
	}
	assert(Reclaims == 0);

	// And the same loop over a lost link still yields the retries that are
	// supposed to happen, so the test above is not passing vacuously.
	int Retries = 0;
	for (int Attempt = 0; Attempt < kMaxReconnectAttempts; ++Attempt)
	{
		if (ShouldReconnect(SessionEnd::Lost, Attempt, kAfterDelay))
		{
			++Retries;
		}
	}
	assert(Retries == kMaxReconnectAttempts);
}

} // namespace

int main()
{
	ADeliberateCloseIsNeverReclaimed();
	ALostLinkIsStillRetried();
	TheAttemptCapIsUnchanged();
	TheDelayIsUnchanged();
	ALiveSessionIsNeverReconnected();
	TheTransportStateMapsToTheRightEnding();
	TheObservedPingPongCannotHappen();

	std::cout << "real33d reconnect policy tests: 7 cases passed\n";
	return 0;
}
