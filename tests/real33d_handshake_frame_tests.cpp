// The handshake-opcode rule, without Unreal and without a server.
//
// The case that matters is the one that was broken: a payload whose first
// command is SV_CMD_CLEAR_TARGET and whose SECOND command is SV_CMD_INIT_GAME,
// which is what Fusion32 sends when `TPlayer::TakeOver` hands an already-online
// character to a new connection. The bytes below are the real ones observed
// live, head=A3 0A EA 03 00 00 32 00 00 64 ...

#include "Real33DHandshakeFrame.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
using namespace Real33D;

/** SV_CMD_CLEAR_TARGET, `connections.hh` 163, zero body. */
constexpr std::uint8_t kClearTarget = 163;
/** SV_CMD_FULLSCREEN, 100. */
constexpr std::uint8_t kFullScreen = 100;

void AHandshakeOpcodeIsRecognisedWhereverItSits()
{
	// The whole point: offset plays no part. Only the opcode and how much is
	// left do.
	for (const std::size_t Available : {kInitGameLength, kInitGameLength + 1,
			std::size_t{1921}})
	{
		const HandshakeSpan Span = HandshakeCommandAt(kInitGameOpcode, Available);
		assert(Span.Command == HandshakeCommand::InitGame);
		assert(Span.Length == kInitGameLength);
	}
	for (const std::size_t Available : {kRightsLength, kRightsLength + 7,
			std::size_t{500}})
	{
		const HandshakeSpan Span = HandshakeCommandAt(kRightsOpcode, Available);
		assert(Span.Command == HandshakeCommand::Rights);
		assert(Span.Length == kRightsLength);
	}
}

void AnOrdinaryOpcodeIsLeftToTheInGameDecoder()
{
	for (const std::uint8_t Opcode : {kClearTarget, kFullScreen,
			std::uint8_t{101}, std::uint8_t{170}, std::uint8_t{190},
			std::uint8_t{0}, std::uint8_t{255}})
	{
		const HandshakeSpan Span = HandshakeCommandAt(Opcode, 1000);
		assert(Span.Command == HandshakeCommand::None);
		assert(Span.Length == 0);
	}
}

void ATruncatedHandshakeCommandIsNotConsumed()
{
	// Consuming a length that has not arrived would walk past the end of the
	// payload and resume on a byte that is not an opcode.
	for (std::size_t Available = 0; Available < kInitGameLength; ++Available)
	{
		const HandshakeSpan Span = HandshakeCommandAt(kInitGameOpcode, Available);
		assert(Span.Command == HandshakeCommand::Truncated);
		assert(Span.Length == 0);
	}
	for (std::size_t Available = 0; Available < kRightsLength; ++Available)
	{
		const HandshakeSpan Span = HandshakeCommandAt(kRightsOpcode, Available);
		assert(Span.Command == HandshakeCommand::Truncated);
		assert(Span.Length == 0);
	}
}

void TheCreatureIdIsReadLittleEndian()
{
	// EA 03 00 00 is 1002, the character this was observed with.
	const std::uint8_t InitGame[kInitGameLength] =
		{ kInitGameOpcode, 0xEA, 0x03, 0x00, 0x00, 0x32, 0x00, 0x00 };
	assert(InitGameCreatureId(InitGame) == 1002u);

	const std::uint8_t Wide[kInitGameLength] =
		{ kInitGameOpcode, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00 };
	assert(InitGameCreatureId(Wide) == 0xFFFFFFFFu);
}

// --------------------------------------------- the payload that was broken

/** Walks a payload the way the bridge does, returning where it stopped. */
std::size_t WalkPayload(const std::vector<std::uint8_t>& Bytes,
	std::uint32_t* OutCreatureId, int* OutCommands)
{
	std::size_t At = 0;
	while (At < Bytes.size())
	{
		const HandshakeSpan Span = HandshakeCommandAt(Bytes[At], Bytes.size() - At);
		if (Span.Command == HandshakeCommand::Truncated)
		{
			break;
		}
		if (Span.Command == HandshakeCommand::InitGame)
		{
			*OutCreatureId = InitGameCreatureId(Bytes.data() + At);
			At += Span.Length;
			++*OutCommands;
			continue;
		}
		if (Span.Command == HandshakeCommand::Rights)
		{
			At += Span.Length;
			++*OutCommands;
			continue;
		}
		// Stand-in for the in-game decoder: CLEAR_TARGET is one byte and
		// FULLSCREEN ends this fixture.
		if (Bytes[At] == kClearTarget)
		{
			At += 1;
			++*OutCommands;
			continue;
		}
		break;
	}
	return At;
}

void TheTakeOverPayloadIsWalkedPastInitGame()
{
	// [CLEAR_TARGET][INIT_GAME creature 1002][FULLSCREEN ...]
	std::vector<std::uint8_t> Bytes = {
		kClearTarget,
		kInitGameOpcode, 0xEA, 0x03, 0x00, 0x00, 0x32, 0x00, 0x00,
		kFullScreen, 0x58, 0x7D,
	};
	std::uint32_t Creature = 0;
	int Commands = 0;
	const std::size_t Stopped = WalkPayload(Bytes, &Creature, &Commands);

	// It reaches the FULLSCREEN, which is the byte the whole initial world
	// hangs off. Before the fix it stopped at offset 1 and never got here.
	assert(Stopped == 9);
	assert(Bytes[Stopped] == kFullScreen);
	assert(Creature == 1002u);
	assert(Commands == 2);
}

void TheColdLoginPayloadStillWalksTheSameWay()
{
	// [INIT_GAME][RIGHTS][FULLSCREEN ...] -- the ordinary case, which the
	// positional guards did handle and which must not regress.
	std::vector<std::uint8_t> Bytes;
	Bytes.push_back(kInitGameOpcode);
	for (const std::uint8_t B : {0xEA, 0x03, 0x00, 0x00, 0x32, 0x00, 0x00}) Bytes.push_back(B);
	Bytes.push_back(kRightsOpcode);
	for (int i = 0; i < 32; ++i) Bytes.push_back(static_cast<std::uint8_t>(i));
	Bytes.push_back(kFullScreen);

	std::uint32_t Creature = 0;
	int Commands = 0;
	const std::size_t Stopped = WalkPayload(Bytes, &Creature, &Commands);
	assert(Stopped == kInitGameLength + kRightsLength);
	assert(Bytes[Stopped] == kFullScreen);
	assert(Creature == 1002u);
	assert(Commands == 2);
}

void AnInitGameSplitAcrossPayloadsIsNotHalfConsumed()
{
	// Only part of the command has arrived. The walker must stop cleanly and
	// leave the bytes alone rather than consume eight it does not have.
	std::vector<std::uint8_t> Bytes = { kClearTarget, kInitGameOpcode, 0xEA, 0x03 };
	std::uint32_t Creature = 0;
	int Commands = 0;
	const std::size_t Stopped = WalkPayload(Bytes, &Creature, &Commands);
	assert(Stopped == 1);
	assert(Creature == 0u);
	assert(Commands == 1);
}

} // namespace

int main()
{
	AHandshakeOpcodeIsRecognisedWhereverItSits();
	AnOrdinaryOpcodeIsLeftToTheInGameDecoder();
	ATruncatedHandshakeCommandIsNotConsumed();
	TheCreatureIdIsReadLittleEndian();

	TheTakeOverPayloadIsWalkedPastInitGame();
	TheColdLoginPayloadStillWalksTheSameWay();
	AnInitGameSplitAcrossPayloadsIsNotHalfConsumed();

	std::cout << "real33d handshake frame tests: 7 cases passed\n";
	return 0;
}
