#include "Real33DBridge.h"

#include "HAL/PlatformProcess.h"
#include "HAL/RunnableThread.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"

// Protocol772Core. Included only here, in the worker. Nothing above this file
// may include a protocol header: that is the boundary.
THIRD_PARTY_INCLUDES_START
#include "fusion32/protocol772/gamelogin.h"
#include "fusion32/protocol772/initial_world.h"
#include "fusion32/protocol772/login.h"
#include "fusion32/protocol772/movement.h"
#include "fusion32/protocol772/movement_ledger.h"
#include "fusion32/protocol772/player_state.h"
#include "fusion32/protocol772/worldview.h"
THIRD_PARTY_INCLUDES_END

#include <chrono>
#include <string>
#include <vector>

namespace p772 = fusion32::protocol772;

namespace
{
	Real33D::FMapPosition ToBridge(const p772::MapPosition& Position)
	{
		return Real33D::FMapPosition{ Position.x, Position.y, Position.z };
	}

	FString ReadTextFile(const FString& Path)
	{
		FString Contents;
		FFileHelper::LoadFileToString(Contents, *Path);
		return Contents.TrimStartAndEnd();
	}

	/** Parses KEY=VALUE lines. Values are used and never logged. */
	TMap<FString, FString> ReadEnvFile(const FString& Path)
	{
		TMap<FString, FString> Values;
		FString Contents;
		if (!FFileHelper::LoadFileToString(Contents, *Path))
		{
			return Values;
		}
		TArray<FString> Lines;
		Contents.ParseIntoArrayLines(Lines);
		for (const FString& Line : Lines)
		{
			FString Key, Value;
			if (Line.Split(TEXT("="), &Key, &Value))
			{
				Values.Add(Key.TrimStartAndEnd(), Value.TrimStartAndEnd());
			}
		}
		return Values;
	}
}

/**
 * The worker. Everything protocol-shaped happens here and nowhere else.
 */
class FReal33DWorker : public FRunnable
{
public:
	explicit FReal33DWorker(const FReal33DConnectionConfig& InConfig)
		: Config(InConfig)
	{
	}

	virtual bool Init() override { return true; }

	virtual uint32 Run() override
	{
		if (!LoadInputs())
		{
			return 0;
		}
		if (!LoginAndEnterGame())
		{
			return 0;
		}
		// Connect used a generous timeout, which is right for a handshake and
		// wrong for a loop that must notice a keypress. This is the interval at
		// which the worker gets control back to send what the player asked for.
		Session.SetReadTimeout(std::chrono::milliseconds(60));

		Publish(MakeSimple(EReal33DEventKind::Connected));

		// reference/game/src/connections.cc::TConnection::Process drops a client
		// whose last command is 90 rounds old, one round per second, so a purely
		// listening client is disconnected after ninety seconds.
		double LastKeepalive = FPlatformTime::Seconds();

		while (!bStopping)
		{
			// Before the reads, not only after them. A read blocks until the
			// read timeout, so an intent posted just after the previous drain
			// would otherwise wait a whole read cycle before it is even sent.
			DrainIntents();

			bool bReceivedAnything = false;
			for (int32 Reads = 0; Reads < 8 && !bStopping; ++Reads)
			{
				const auto Read = Session.ReadNextMessage();
				if (!Read.ok())
				{
					if (Read.transport.io_status == p772::IoStatus::TimedOut)
					{
						break;
					}
					Publish(MakeFailure(EReal33DEventKind::Disconnected,
						TEXT("the session ended")));
					return 0;
				}
				bReceivedAnything = true;
				if (!ProcessPayload(Read.message.message_bytes))
				{
					return 0;
				}
			}

			PublishWorldEvents();
			DrainIntents();

			const double Now = FPlatformTime::Seconds();
			if (Now - LastKeepalive >= 20.0)
			{
				LastKeepalive = Now;
				Session.SendCommand(p772::BuildPingCommand());
			}
			if (!bReceivedAnything)
			{
				FPlatformProcess::Sleep(0.01f);
			}
		}

		Session.SendCommand(p772::BuildLogoutCommand());
		Session.Disconnect();
		Publish(MakeSimple(EReal33DEventKind::Disconnected));
		return 0;
	}

	virtual void Stop() override { bStopping = true; }

	void PostIntent(uint32 InputId, uint8 Direction)
	{
		Intents.Enqueue(FIntent{ InputId, Direction });
	}

	bool Dequeue(FReal33DEvent& OutEvent) { return Events.Dequeue(OutEvent); }

	FReal33DStats Snapshot() const
	{
		FScopeLock Lock(&StatsMutex);
		return Stats;
	}

	bool IsStopping() const { return bStopping; }

private:
	// ---------------------------------------------------------------- setup

	bool LoadInputs()
	{
		const FString SecretsDir = FPaths::Combine(Config.RuntimeDir, TEXT("secrets"));
		const FString Modulus = ReadTextFile(
			FPaths::Combine(SecretsDir, TEXT("public-modulus.decimal")));
		const TMap<FString, FString> Credentials = ReadEnvFile(
			FPaths::Combine(SecretsDir, TEXT("credentials.env")));

		const FString IdKey = FString::Printf(TEXT("ACCOUNT_%s_ID"), *Config.Account);
		const FString PasswordKey = FString::Printf(TEXT("ACCOUNT_%s_PASSWORD"), *Config.Account);
		if (Modulus.IsEmpty() || !Credentials.Contains(IdKey))
		{
			Publish(MakeFailure(EReal33DEventKind::Failed,
				TEXT("the sanitized runtime secrets are not readable at the configured path")));
			return false;
		}
		AccountId = static_cast<uint32>(FCString::Strtoui64(*Credentials[IdKey], nullptr, 10));
		Password = TCHAR_TO_UTF8(*Credentials[PasswordKey]);

		if (p772::Rsa1024PublicKey::FromDecimalModulus(
				TCHAR_TO_UTF8(*Modulus), &RsaKey) != p772::CryptoError::None)
		{
			Publish(MakeFailure(EReal33DEventKind::Failed, TEXT("the runtime modulus was rejected")));
			return false;
		}

		FString ObjectsPath = Config.ObjectsSrvPath;
		if (ObjectsPath.IsEmpty())
		{
			ObjectsPath = FPaths::Combine(Config.RuntimeDir,
				TEXT("game"), TEXT("reference"), TEXT("dat"), TEXT("objects.srv"));
		}
		FString ObjectsText;
		if (!FFileHelper::LoadFileToString(ObjectsText, *ObjectsPath))
		{
			Publish(MakeFailure(EReal33DEventKind::Failed,
				FString::Printf(TEXT("objects.srv not readable at %s"), *ObjectsPath)));
			return false;
		}
		const auto Loaded = p772::LoadObjectTypeTableFromObjectsSrv(
			TCHAR_TO_UTF8(*ObjectsText));
		if (!Loaded.ok())
		{
			Publish(MakeFailure(EReal33DEventKind::Failed, TEXT("objects.srv did not parse")));
			return false;
		}
		Types = Loaded.table;
		return true;
	}

	bool LoginAndEnterGame()
	{
		p772::LoginRequestOptions LoginOptions;
		LoginOptions.account_id = AccountId;
		LoginOptions.password = Password;
		const auto LoginRequest = p772::BuildLoginRequest(LoginOptions, RsaKey);
		if (!LoginRequest.ok())
		{
			Publish(MakeFailure(EReal33DEventKind::Failed, TEXT("login request could not be built")));
			return false;
		}

		p772::FramedConnection LoginConnection(
			{ p772::kLoginFrameLimits.max_receive_payload,
			  p772::kLoginFrameLimits.max_send_payload });
		if (!LoginConnection.Connect(TCHAR_TO_UTF8(*Config.Host),
				static_cast<uint16>(Config.LoginPort)).ok())
		{
			Publish(MakeFailure(EReal33DEventKind::Failed, TEXT("could not reach the login service")));
			return false;
		}
		const std::vector<std::uint8_t> Payload(
			LoginRequest.payload.begin(), LoginRequest.payload.end());
		if (!LoginConnection.SendFrame(Payload).ok())
		{
			Publish(MakeFailure(EReal33DEventKind::Failed, TEXT("login request could not be sent")));
			return false;
		}

		p772::LoginResponse Response;
		for (int32 Attempt = 0; Attempt < 50; ++Attempt)
		{
			const auto Read = LoginConnection.ReadOnce();
			if (!Read.ok() && Read.framing.frames.empty())
			{
				break;
			}
			if (Read.framing.frames.empty())
			{
				continue;
			}
			Response = p772::ParseLoginResponse(
				p772::DecryptXteaPayload(LoginRequest.xtea_key, Read.framing.frames.front()));
			break;
		}
		if (!Response.ok() || Response.characters.empty())
		{
			Publish(MakeFailure(EReal33DEventKind::Failed, TEXT("the character list was not returned")));
			return false;
		}
		LoginConnection.Disconnect();

		p772::GameLoginRequestOptions GameOptions;
		GameOptions.account_id = AccountId;
		GameOptions.character_name = Response.characters.front().name;
		GameOptions.password = Password;
		CharacterName = UTF8_TO_TCHAR(Response.characters.front().name.c_str());
		const auto GameRequest = p772::BuildGameLoginRequest(GameOptions, RsaKey);
		if (!GameRequest.ok())
		{
			Publish(MakeFailure(EReal33DEventKind::Failed, TEXT("game login could not be built")));
			return false;
		}
		if (!Session.Connect(TCHAR_TO_UTF8(*Config.Host),
				Response.characters.front().world_port).ok())
		{
			Publish(MakeFailure(EReal33DEventKind::Failed, TEXT("could not reach the game service")));
			return false;
		}
		if (!Session.SendLogin(GameRequest).ok())
		{
			Publish(MakeFailure(EReal33DEventKind::Failed, TEXT("game login could not be sent")));
			return false;
		}
		return true;
	}

	// ------------------------------------------------------------ the stream

	bool ProcessPayload(const std::vector<std::uint8_t>& Bytes)
	{
		std::size_t At = 0;
		int32 LocalCommands = 0;
		while (At < Bytes.size())
		{
			const std::uint8_t Opcode = Bytes[At];
			if (Opcode == p772::kGameInitOpcode && At == 0)
			{
				if (Bytes.size() < 8)
				{
					break;
				}
				State.local_creature_id = static_cast<std::uint32_t>(Bytes[1])
					| (static_cast<std::uint32_t>(Bytes[2]) << 8)
					| (static_cast<std::uint32_t>(Bytes[3]) << 16)
					| (static_cast<std::uint32_t>(Bytes[4]) << 24);
				At += 8;
				++LocalCommands;
				continue;
			}
			if (Opcode == p772::kGameRightsOpcode && At == 8 && Bytes.size() - At >= 33)
			{
				At += 33;
				++LocalCommands;
				continue;
			}

			const auto Decoded = p772::DecodeServerUpdate(
				Bytes, At, State.viewport_anchor, Types);
			if (!Decoded.ok())
			{
				Publish(MakeFailure(EReal33DEventKind::Failed,
					FString::Printf(TEXT("%hs could not be decoded: %hs"),
						p772::ServerCommandName(Opcode),
						p772::MapDecodeErrorName(Decoded.error))));
				return false;
			}
			if (Decoded.update.kind == p772::ServerUpdateKind::Unsupported)
			{
				// Stopping here is deliberate: an unsized command means the
				// rest of the payload cannot be located, and guessing where the
				// next one starts would be inventing semantics. The remainder
				// is counted as residual rather than skipped.
				//
				// Name it on the way out. "unsupported_opcodes: 2" is a number
				// nobody can act on; the command's own name says whether this
				// is the known chat gap or something new.
				Publish(MakeFailure(EReal33DEventKind::Diagnostic,
					FString::Printf(
						TEXT("%hs is not decoded; %d bytes of the payload left unwalked"),
						p772::ServerCommandName(Opcode),
						static_cast<int32>(Bytes.size() - At))));
				FScopeLock Lock(&StatsMutex);
				++Stats.UnsupportedOpcodes;
				break;
			}
			if (Decoded.update.kind == p772::ServerUpdateKind::Snapback)
			{
				// Fusion32 refused a step. The ledger says which one, so the
				// refusal can be joined to the key press that caused it.
				const auto Match = Ledger.NoteSnapback(FPlatformTime::Seconds());
				FReal33DEvent Refused;
				Refused.Kind = EReal33DEventKind::WalkRejected;
				Refused.RequestId = Match.request_id;
				Refused.InputId = TakeInputFor(Match.request_id);
				// A refusal moves nothing, so both positions are the one the
				// player is still standing on. Saying so beats leaving a zero.
				Refused.Position = ToBridge(State.viewport_anchor);
				Refused.PreviousPosition = Refused.Position;
				Refused.Direction = Match.matched
					? static_cast<uint8>(Match.direction) : kNoDirection;
				Publish(MoveTemp(Refused));

				FScopeLock Lock(&StatsMutex);
				Stats.RejectedSteps = static_cast<int32>(Ledger.counts().rejected);
			}
			const auto Applied = p772::ApplyServerUpdate(&State, Decoded.update, Types);
			if (!Applied.anomalies.empty())
			{
				FScopeLock Lock(&StatsMutex);
				Stats.Anomalies += static_cast<int32>(Applied.anomalies.size());
			}
			At += Decoded.update.bytes_consumed;
			++LocalCommands;
		}

		FScopeLock Lock(&StatsMutex);
		++Stats.Frames;
		Stats.Commands += LocalCommands;
		Stats.ResidualBytes += static_cast<int32>(Bytes.size() - At);
		Stats.Tiles = static_cast<int32>(State.tile_count());
		Stats.VisibleCreatures = static_cast<int32>(State.visible_creature_ids().size());
		Stats.bViewportSynchronised = State.viewport_synchronized();
		Stats.Anchor = ToBridge(State.viewport_anchor);
		return true;
	}

	/** Turns WorldState changes into events. The diff lives in ClientCore. */
	void PublishWorldEvents()
	{
		for (const p772::WorldEvent& Event : View.Diff(State))
		{
			FReal33DEvent Out;
			Out.Position = ToBridge(Event.position);
			Out.PreviousPosition = ToBridge(Event.previous_position);
			Out.CreatureId = Event.creature_id;
			Out.CreatureName = UTF8_TO_TCHAR(Event.creature_name.c_str());
			Out.Direction = Event.direction;
			Out.bIsLocalPlayer = Event.is_local_player;
			switch (Event.kind)
			{
			case p772::WorldEventKind::LocalPlayerIdentified:
				Out.Kind = EReal33DEventKind::LocalPlayerIdentified; break;
			case p772::WorldEventKind::AnchorMoved:
				Out.Kind = EReal33DEventKind::AnchorMoved; break;
			case p772::WorldEventKind::TileUpserted:
				Out.Kind = EReal33DEventKind::TileUpserted; break;
			case p772::WorldEventKind::TileRemoved:
				Out.Kind = EReal33DEventKind::TileRemoved; break;
			case p772::WorldEventKind::CreatureAppeared:
				Out.Kind = EReal33DEventKind::CreatureAppeared; break;
			case p772::WorldEventKind::CreatureMoved:
				Out.Kind = EReal33DEventKind::CreatureMoved;
				if (Event.is_local_player)
				{
					ClassifyLocalMove(Event.previous_position, Event.position);
				}
				break;
			case p772::WorldEventKind::CreatureVanished:
				Out.Kind = EReal33DEventKind::CreatureVanished; break;
			}
			if (Event.kind == p772::WorldEventKind::TileUpserted)
			{
				Out.Things.Reserve(static_cast<int32>(Event.things.size()));
				for (const p772::MapThing& Thing : Event.things)
				{
					FReal33DThing Flat;
					Flat.bIsCreature = Thing.kind == p772::MapThingKind::Creature;
					Flat.TypeId = Thing.item.type_id;
					Flat.CreatureId = Thing.creature.creature_id;
					// Resolved here, where the object type table already lives,
					// so nothing above the bridge needs a type id to know how
					// to draw a field.
					Flat.bBlocking = !Flat.bIsCreature
						&& Types.Lookup(Thing.item.type_id).unpass;
					Out.Things.Add(Flat);
				}
			}
			Publish(MoveTemp(Out));
		}
	}

	/**
	 * Decides whose move this was and says so, rather than assuming it was ours.
	 *
	 * A creature walking into this player displaces them
	 * (reference/game/src/cract.cc TCreature::Move), and on the wire that is an
	 * ordinary move naming our own creature. Only the ledger can tell it apart
	 * from the answer to a walk we asked for, and it does so by requiring the
	 * landing field to be the one the outstanding request named.
	 */
	void ClassifyLocalMove(const p772::MapPosition& From, const p772::MapPosition& To)
	{
		const auto Outcome = Ledger.NoteLocalMove(From, To, FPlatformTime::Seconds());
		if (Outcome.cause == p772::LocalMoveCause::NoMovement)
		{
			return;
		}

		FReal33DEvent Out;
		Out.Position = ToBridge(To);
		Out.PreviousPosition = ToBridge(From);
		Out.bIsLocalPlayer = true;
		if (Outcome.cause == p772::LocalMoveCause::SelfWalkAccepted)
		{
			Out.Kind = EReal33DEventKind::WalkAccepted;
			Out.RequestId = Outcome.request_id;
			Out.InputId = TakeInputFor(Outcome.request_id);
			Out.Direction = static_cast<uint8>(Outcome.direction);
		}
		else
		{
			Out.Kind = EReal33DEventKind::ExternalRelocation;
			// Nobody asked for this, so it has no requested direction.
			Out.Direction = kNoDirection;
		}
		Publish(MoveTemp(Out));

		FScopeLock Lock(&StatsMutex);
		Stats.AcceptedSelfWalks = static_cast<int32>(Ledger.counts().accepted);
		Stats.ExternalRelocations = static_cast<int32>(Ledger.counts().external);
		++Stats.LocalPlayerMoves;
	}

	void DrainIntents()
	{
		FIntent Intent;
		while (Intents.Dequeue(Intent))
		{
			p772::CardinalDirection Cardinal = p772::CardinalDirection::North;
			switch (Intent.Direction)
			{
			case 1: Cardinal = p772::CardinalDirection::East; break;
			case 2: Cardinal = p772::CardinalDirection::South; break;
			case 3: Cardinal = p772::CardinalDirection::West; break;
			default: break;
			}
			// An intent, not a movement. Fusion32 decides, so all that happens
			// here is that the question is asked and recorded. The answer is
			// matched to it by the ledger when it arrives.
			if (!Session.SendCommand(p772::BuildWalkCommand(Cardinal)).ok())
			{
				continue;
			}
			const std::uint32_t RequestId =
				Ledger.NoteRequestSent(Cardinal, FPlatformTime::Seconds());

			FReal33DEvent Sent;
			Sent.Kind = EReal33DEventKind::WalkSent;
			Sent.InputId = Intent.InputId;
			Sent.RequestId = RequestId;
			Sent.Direction = Intent.Direction;
			// Where the player stood when the question was asked, so the
			// journal shows what the request was relative to rather than zeros.
			Sent.PreviousPosition = ToBridge(State.viewport_anchor);
			Sent.Position = Sent.PreviousPosition;
			Publish(MoveTemp(Sent));

			InputForRequest.Add(RequestId, Intent.InputId);

			FScopeLock Lock(&StatsMutex);
			Stats.RequestedSteps = static_cast<int32>(Ledger.counts().requested);
		}

		// A reply that never comes must not be allowed to claim a later move.
		// Three seconds is far longer than a local round trip; the server's own
		// step timing is well inside it.
		const double Now = FPlatformTime::Seconds();
		if (Ledger.ExpireBefore(Now - 3.0) > 0)
		{
			FReal33DEvent Expired;
			Expired.Kind = EReal33DEventKind::WalkUnanswered;
			Expired.Direction = kNoDirection;
			Publish(MoveTemp(Expired));
			FScopeLock Lock(&StatsMutex);
			Stats.UnansweredSteps = static_cast<int32>(Ledger.counts().unanswered);
		}
	}

	/** Looks up and forgets the key press a request came from. */
	uint32 TakeInputFor(std::uint32_t RequestId)
	{
		uint32 InputId = 0;
		if (const uint32* Found = InputForRequest.Find(RequestId))
		{
			InputId = *Found;
			InputForRequest.Remove(RequestId);
		}
		return InputId;
	}

	// ----------------------------------------------------------- publishing

	FReal33DEvent MakeSimple(EReal33DEventKind Kind) const
	{
		FReal33DEvent Event;
		Event.Kind = Kind;
		Event.CreatureName = CharacterName;
		Event.Detail = CharacterName;
		return Event;
	}

	FReal33DEvent MakeFailure(EReal33DEventKind Kind, const FString& Detail) const
	{
		FReal33DEvent Event;
		Event.Kind = Kind;
		Event.Detail = Detail;
		return Event;
	}

	void Publish(FReal33DEvent&& Event) { Events.Enqueue(MoveTemp(Event)); }

	FReal33DConnectionConfig Config;
	FThreadSafeBool bStopping{ false };

	uint32 AccountId = 0;
	std::string Password;
	FString CharacterName;
	p772::Rsa1024PublicKey RsaKey;
	p772::ObjectTypeTable Types;
	p772::GameLoginSession Session;
	p772::WorldState State;
	p772::WorldView View;

	p772::MovementLedger Ledger;
	/** Joins a wire request back to the key press it came from. Worker only. */
	TMap<uint32, uint32> InputForRequest;

	struct FIntent
	{
		uint32 InputId = 0;
		uint8 Direction = 0;
	};

	TQueue<FReal33DEvent, EQueueMode::Spsc> Events;
	TQueue<FIntent, EQueueMode::Spsc> Intents;

	mutable FCriticalSection StatsMutex;
	FReal33DStats Stats;
};

// ---------------------------------------------------------------- subsystem

void UReal33DBridge::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UReal33DBridge::Deinitialize()
{
	Disconnect();
	Super::Deinitialize();
}

bool UReal33DBridge::Connect(const FReal33DConnectionConfig& Config)
{
	if (Worker != nullptr)
	{
		return false;
	}
	if (!Config.IsValid())
	{
		UE_LOG(LogTemp, Error,
			TEXT("REAL33D: no runtime directory configured. Pass -real33d-runtime=<path>."));
		return false;
	}
	Worker = new FReal33DWorker(Config);
	Thread = FRunnableThread::Create(Worker, TEXT("REAL33D.Protocol772"), 0,
		TPri_AboveNormal);
	if (Thread == nullptr)
	{
		delete Worker;
		Worker = nullptr;
		return false;
	}
	return true;
}

void UReal33DBridge::Disconnect()
{
	if (Thread != nullptr)
	{
		Thread->Kill(true);
		delete Thread;
		Thread = nullptr;
	}
	if (Worker != nullptr)
	{
		delete Worker;
		Worker = nullptr;
	}
}

bool UReal33DBridge::IsRunning() const
{
	return Worker != nullptr && !Worker->IsStopping();
}

void UReal33DBridge::DrainEvents(TArray<FReal33DEvent>& OutEvents)
{
	check(IsInGameThread());
	if (Worker == nullptr)
	{
		return;
	}
	FReal33DEvent Event;
	while (Worker->Dequeue(Event))
	{
		OutEvents.Add(MoveTemp(Event));
	}
}

uint32 UReal33DBridge::RequestWalk(uint8 Direction)
{
	if (Worker == nullptr)
	{
		return 0;
	}
	// Numbered on the game thread, at the moment the key was pressed, so the
	// identity of a press exists before anything else in the chain happens.
	const uint32 InputId = ++NextInputId;
	Worker->PostIntent(InputId, Direction);
	return InputId;
}

FReal33DStats UReal33DBridge::GetStats() const
{
	return Worker != nullptr ? Worker->Snapshot() : FReal33DStats{};
}

FReal33DConnectionConfig UReal33DBridge::ConfigFromCommandLine()
{
	FReal33DConnectionConfig Config;
	FString Value;
	if (FParse::Value(FCommandLine::Get(), TEXT("-real33d-runtime="), Value))
	{
		Config.RuntimeDir = Value;
	}
	if (FParse::Value(FCommandLine::Get(), TEXT("-real33d-account="), Value))
	{
		Config.Account = Value;
	}
	if (FParse::Value(FCommandLine::Get(), TEXT("-real33d-host="), Value))
	{
		Config.Host = Value;
	}
	if (FParse::Value(FCommandLine::Get(), TEXT("-real33d-objects="), Value))
	{
		Config.ObjectsSrvPath = Value;
	}
	int32 Port = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("-real33d-loginport="), Port))
	{
		Config.LoginPort = Port;
	}
	return Config;
}
