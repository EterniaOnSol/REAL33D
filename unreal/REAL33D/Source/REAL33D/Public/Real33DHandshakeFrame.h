#pragma once

#include <cstddef>
#include <cstdint>

// The two handshake commands, recognised by WHAT THEY ARE rather than by where
// they happen to sit in a payload.
//
// THIS EXISTS BECAUSE OF A REAL DEFECT, and the defect was the positional
// assumption. The payload walker used to accept SV_CMD_INIT_GAME only at
// offset 0 and SV_CMD_RIGHTS only at offset 8. On a cold login that holds --
// `crplayer.cc` builds a TPlayer and immediately calls `SendInitGame`,
// `SendRights`, `SendFullScreen` -- so it was never wrong in the ordinary case.
//
// It is wrong on a HANDOVER. When a character who is still online logs in
// again, `connections.cc` takes the other branch and calls
// `TPlayer::TakeOver`, which assigns the new connection FIRST and only then
// does its tidying: `Combat.StopAttack(0)` emits SV_CMD_CLEAR_TARGET (163, a
// zero-body command) onto that new connection BEFORE `SendInitGame` is
// reached. The first game payload is therefore
//
//     [CLEAR_TARGET][INIT_GAME ...][FULLSCREEN ...]
//
// with INIT_GAME at offset 1. Observed live, exactly:
//
//     SV_CMD_INIT_GAME is not decoded at offset 1 of 1921;
//     1920 bytes of the payload left unwalked;
//     head=A3 0A EA 03 00 00 32 00 00 64 58 7D
//
// `A3` is 163, `0A` is INIT_GAME, `EA 03 00 00` is creature 1002, `64` is
// FULLSCREEN. Missing the opcode cost the whole rest of the payload -- the
// entire initial world -- because an unsized command means the walker cannot
// locate the next one and stops, by design.
//
// Position is not a protocol fact. Nothing in `sending.cc` promises INIT_GAME
// begins a payload; it is simply what usually happens. Identity is a fact:
// 10 and 11 are the handshake's own opcodes and `connections.hh` gives no
// in-game command a value below 100 apart from the login errors (20..22) and
// ping (30), so neither can be confused with anything the in-game decoder
// handles.
//
// Deliberately free of Unreal types: tests/real33d_handshake_frame_tests.cpp
// compiles it with a plain C++ toolchain, like the other tests/real33d_*.cpp
// suites.
namespace Real33D
{

/** `connections.hh`: SV_CMD_INIT_GAME = 10, SV_CMD_RIGHTS = 11. */
inline constexpr std::uint8_t kInitGameOpcode = 10;
inline constexpr std::uint8_t kRightsOpcode = 11;

/**
 * Opcode, u32 creature id, u16 beat, u8 bug reports. Source:
 * `sending.cc::SendInitGame`, and `gamelogin.cpp` reads exactly these 8.
 */
inline constexpr std::size_t kInitGameLength = 8;

/** Opcode plus the 32 right bytes. Source: `gamelogin.cpp`'s `33`. */
inline constexpr std::size_t kRightsLength = 33;

enum class HandshakeCommand : std::uint8_t
{
	/** Not a handshake opcode. The in-game decoder owns it. */
	None,
	InitGame,
	Rights,
	/** A handshake opcode whose body has not all arrived yet. */
	Truncated,
};

struct HandshakeSpan
{
	HandshakeCommand Command = HandshakeCommand::None;
	/** Bytes to consume. Zero unless Command is InitGame or Rights. */
	std::size_t Length = 0;
};

/**
 * Classifies the command at a payload boundary.
 *
 * @param Opcode    the byte at the boundary.
 * @param Available how many bytes remain from the boundary to the end of the
 *                  payload, the opcode included.
 *
 * Only ever call this AT A COMMAND BOUNDARY — somewhere the walker has
 * established a command starts. Applied to an arbitrary offset it would read a
 * body byte as an opcode, which is the same class of mistake the positional
 * guards made, pointed the other way.
 */
inline HandshakeSpan HandshakeCommandAt(std::uint8_t Opcode, std::size_t Available)
{
	HandshakeSpan Span;
	if (Opcode == kInitGameOpcode)
	{
		if (Available < kInitGameLength)
		{
			Span.Command = HandshakeCommand::Truncated;
			return Span;
		}
		Span.Command = HandshakeCommand::InitGame;
		Span.Length = kInitGameLength;
		return Span;
	}
	if (Opcode == kRightsOpcode)
	{
		if (Available < kRightsLength)
		{
			Span.Command = HandshakeCommand::Truncated;
			return Span;
		}
		Span.Command = HandshakeCommand::Rights;
		Span.Length = kRightsLength;
		return Span;
	}
	return Span;
}

/** The creature id inside an INIT_GAME, little-endian at offset 1. */
inline std::uint32_t InitGameCreatureId(const std::uint8_t* At)
{
	return static_cast<std::uint32_t>(At[1])
		| (static_cast<std::uint32_t>(At[2]) << 8)
		| (static_cast<std::uint32_t>(At[3]) << 16)
		| (static_cast<std::uint32_t>(At[4]) << 24);
}

} // namespace Real33D
