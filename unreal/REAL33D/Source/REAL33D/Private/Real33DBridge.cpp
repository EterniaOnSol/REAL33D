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
#include "fusion32/protocol772/chat_log.h"
#include "fusion32/protocol772/gamelogin.h"
#include "fusion32/protocol772/initial_world.h"
#include "fusion32/protocol772/login.h"
#include "fusion32/protocol772/movement.h"
#include "fusion32/protocol772/movement_ledger.h"
#include "fusion32/protocol772/talk_command.h"
#include "fusion32/protocol772/talk_speaker.h"
#include "fusion32/protocol772/player_state.h"
#include "fusion32/protocol772/worldview.h"
THIRD_PARTY_INCLUDES_END

#include <chrono>
#include <string>
#include <vector>

namespace p772 = fusion32::protocol772;

// FReal33DConditions mirrors Fusion32's condition bits so that Real33DBridge.h
// stays free of anything under fusion32/. A mirror that drifts would draw the
// wrong status icon and look like a server fault, so the two are pinned here --
// the one file that can see both definitions at once.
static_assert(static_cast<uint8>(FReal33DConditions::Poisoned)
	== static_cast<uint8>(p772::PlayerStateFlag::Poisoned), "condition bit drift");
static_assert(static_cast<uint8>(FReal33DConditions::Burning)
	== static_cast<uint8>(p772::PlayerStateFlag::Burning), "condition bit drift");
static_assert(static_cast<uint8>(FReal33DConditions::Electrified)
	== static_cast<uint8>(p772::PlayerStateFlag::Electrified), "condition bit drift");
static_assert(static_cast<uint8>(FReal33DConditions::Drunk)
	== static_cast<uint8>(p772::PlayerStateFlag::Drunk), "condition bit drift");
static_assert(static_cast<uint8>(FReal33DConditions::ManaShield)
	== static_cast<uint8>(p772::PlayerStateFlag::ManaShield), "condition bit drift");
static_assert(static_cast<uint8>(FReal33DConditions::Slowed)
	== static_cast<uint8>(p772::PlayerStateFlag::Slowed), "condition bit drift");
static_assert(static_cast<uint8>(FReal33DConditions::Hasted)
	== static_cast<uint8>(p772::PlayerStateFlag::Hasted), "condition bit drift");
static_assert(static_cast<uint8>(FReal33DConditions::LogoutBlocked)
	== static_cast<uint8>(p772::PlayerStateFlag::LogoutBlocked), "condition bit drift");

// The inventory array is indexed by the server's own slot numbers, so it has
// to be wide enough for the last one. Pinned here rather than assumed.
static_assert(FReal33DInventory::LastSlot == p772::kInventoryLastSlot,
	"inventory slot range drift");
static_assert(FReal33DInventory::SlotCount > p772::kInventoryLastSlot,
	"inventory array too small for INVENTORY_LAST");

/** One object, copied field by field across the boundary rather than memcpy'd. */
static FReal33DItem ToUnrealItem(const p772::ItemThing& From)
{
	FReal33DItem To;
	To.TypeId = From.type_id;
	To.bHasAmount = From.has_amount;
	To.Amount = From.amount;
	To.bHasLiquidColour = From.has_liquid_color;
	To.LiquidColour = From.liquid_color;
	return To;
}

/**
 * The transcript, wrapped so Real33DBridge.h needs nothing from fusion32/.
 *
 * ChatLog is documented as single-owner, single-thread. Its owner is the game
 * thread, because that is where DrainEvents runs and where the widget reads.
 * The worker never touches it.
 */
struct FReal33DChatTranscript
{
	p772::ChatLog Log;
	uint64 Revision = 0;
};

namespace
{
	Real33D::FMapPosition ToBridge(const p772::MapPosition& Position)
	{
		return Real33D::FMapPosition{ Position.x, Position.y, Position.z };
	}

	/**
	 * The one place a chosen mode becomes a protocol value.
	 *
	 * Nothing above the bridge may do this, which is why the enum above carries
	 * no numbers of its own and why this function is not exported.
	 */
	std::uint8_t ToWireTalkMode(EReal33DTalkMode Mode)
	{
		switch (Mode)
		{
		case EReal33DTalkMode::Whisper:
			return static_cast<std::uint8_t>(p772::TalkMode::Whisper);
		case EReal33DTalkMode::Yell:
			return static_cast<std::uint8_t>(p772::TalkMode::Yell);
		default:
			return static_cast<std::uint8_t>(p772::TalkMode::Say);
		}
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

	void PostSay(uint32 SayId, EReal33DTalkMode Mode, const FString& Text)
	{
		Says.Enqueue(FSay{ SayId, Mode, Text });
	}

	void PostMove(uint32 MoveId, const UReal33DBridge::FMoveSlot& From, uint16 TypeId,
		uint8 StackIndex, const UReal33DBridge::FMoveSlot& To, uint8 Count)
	{
		Moves.Enqueue(FMove{ MoveId, From, To, TypeId, StackIndex, Count });
	}

	/**
	 * A use the player asked for, in one of its three shapes.
	 *
	 * Declared here rather than beside the other intent structs because a
	 * parameter type has to be complete where the function is declared, not
	 * merely where its body is compiled.
	 */
	struct FUse
	{
		enum class EKind : uint8 { Object, WithObject, OnCreature };
		uint32 UseId = 0;
		EKind Kind = EKind::Object;
		UReal33DBridge::FMoveSlot Object;
		uint16 TypeId = 0;
		uint8 StackIndex = 0;
		/** Which open-container slot to show it in, when it is a container. */
		uint8 OpenAsContainer = 0;
		UReal33DBridge::FMoveSlot Target;
		uint16 TargetTypeId = 0;
		uint8 TargetStackIndex = 0;
		uint32 CreatureId = 0;
	};

	void PostUse(const FUse& Use)
	{
		Uses.Enqueue(Use);
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
			if (Decoded.update.kind == p772::ServerUpdateKind::Message)
			{
				// The server's own refusals arrive here: CTalk answers an
				// illegal yell with TALK_FAILURE_MESSAGE, and without surfacing
				// it the client looks broken when the server is simply saying
				// no. Whoever sent the command is the one who is told.
				//
				// Published as a ServerMessage rather than a Diagnostic. It was
				// a Diagnostic, wrapped in the prose "server message [Mode]:",
				// which put something a player must read on a developer surface
				// and made the two indistinguishable downstream. The text is
				// carried verbatim; the mode is named here, in the only file
				// allowed to know one.
				FReal33DEvent Said;
				Said.Kind = EReal33DEventKind::ServerMessage;
				Said.TalkMode = UTF8_TO_TCHAR(
					p772::MessageModeName(Decoded.update.message.mode));
				Said.Detail = UTF8_TO_TCHAR(Decoded.update.message.text.c_str());
				Said.Direction = kNoDirection;
				Publish(MoveTemp(Said));
			}

			if (Decoded.update.kind == p772::ServerUpdateKind::CreatureAttribute
				&& Decoded.update.creature_attribute.attribute
					== p772::CreatureAttribute::Health)
			{
				FReal33DEvent Health;
				Health.Kind = EReal33DEventKind::CreatureHealth;
				Health.CreatureId = Decoded.update.creature_attribute.creature_id;
				Health.HealthPercent = Decoded.update.creature_attribute.health_percent;
				Health.Direction = kNoDirection;
				Publish(MoveTemp(Health));

				FScopeLock Lock(&StatsMutex);
				++Stats.HealthUpdates;
			}

			if (Decoded.update.kind == p772::ServerUpdateKind::Talk)
			{
				const p772::TalkUpdate& Talk = Decoded.update.talk;
				FReal33DEvent Spoken;
				Spoken.Kind = EReal33DEventKind::Talk;
				Spoken.Speaker = UTF8_TO_TCHAR(Talk.speaker.c_str());
				Spoken.Detail = UTF8_TO_TCHAR(Talk.text.c_str());
				// Resolved to a name here, inside the only file allowed to know
				// the protocol, so nothing above this sees a mode number.
				Spoken.TalkMode = UTF8_TO_TCHAR(p772::TalkModeName(Talk.mode));
				Spoken.Direction = kNoDirection;
				switch (Talk.layout)
				{
				case p772::TalkLayout::Positional:
				{
					Spoken.TalkLayout = EReal33DTalkLayout::Positional;
					Spoken.Position = ToBridge(Talk.position);
					Spoken.PreviousPosition = Spoken.Position;
					// Resolved here, against the WorldState this thread owns,
					// so what crosses the boundary is a creature id the game
					// thread already keys its actors by. Unreal never learns
					// the rule, the name or the coordinates it was derived
					// from, and never has to search for a speaker itself.
					const auto Speaker = p772::ResolveTalkSpeaker(State, Talk);
					Spoken.CreatureId = Speaker.creature_id;
					Spoken.SpeakerResolution =
						UTF8_TO_TCHAR(p772::TalkSpeakerOutcomeName(Speaker.outcome));
					break;
				}
				case p772::TalkLayout::Channel:
					Spoken.TalkLayout = EReal33DTalkLayout::Channel;
					Spoken.bHasChannel = true;
					Spoken.Channel = static_cast<int32>(Talk.channel);
					break;
				default:
					Spoken.TalkLayout = EReal33DTalkLayout::Plain;
					break;
				}
				Publish(MoveTemp(Spoken));

				FScopeLock Lock(&StatsMutex);
				++Stats.TalkMessages;
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
			if (Decoded.update.kind == p772::ServerUpdateKind::PlayerData
				&& State.stats.known)
			{
				FReal33DEvent Vitals;
				Vitals.Kind = EReal33DEventKind::PlayerVitals;
				Vitals.Vitals.bKnown = true;
				Vitals.Vitals.Hitpoints = State.stats.hitpoints;
				Vitals.Vitals.MaxHitpoints = State.stats.max_hitpoints;
				Vitals.Vitals.Mana = State.stats.mana;
				Vitals.Vitals.MaxMana = State.stats.max_mana;
				Vitals.Vitals.Level = State.stats.level;
				// The rest of the same command. The character sheet reads these
				// and nothing else: a value it cannot show here is a value the
				// server did not send, which is what an empty row must mean.
				Vitals.Vitals.LevelPercent = State.stats.level_percent;
				Vitals.Vitals.Experience = State.stats.experience;
				Vitals.Vitals.Capacity = State.stats.capacity;
				Vitals.Vitals.MagicLevel = State.stats.magic_level;
				Vitals.Vitals.MagicLevelPercent = State.stats.magic_level_percent;
				Vitals.Vitals.SoulPoints = State.stats.soul_points;
				Publish(MoveTemp(Vitals));
			}
			if (Decoded.update.kind == p772::ServerUpdateKind::PlayerSkills
				&& State.skills.known)
			{
				// Copied out one field at a time rather than memcpy'd: the two
				// structs are deliberately unrelated types, so that ClientCore's
				// layout is free to change without silently reinterpreting
				// itself on this side of the boundary.
				const auto Copy = [](const p772::PlayerSkill& From)
				{
					FReal33DSkill To;
					To.Level = From.level;
					To.Percent = From.percent;
					return To;
				};
				FReal33DEvent Skills;
				Skills.Kind = EReal33DEventKind::PlayerSkills;
				Skills.Skills.bKnown = true;
				Skills.Skills.Fist = Copy(State.skills.fist);
				Skills.Skills.Club = Copy(State.skills.club);
				Skills.Skills.Sword = Copy(State.skills.sword);
				Skills.Skills.Axe = Copy(State.skills.axe);
				Skills.Skills.Distance = Copy(State.skills.distance);
				Skills.Skills.Shielding = Copy(State.skills.shielding);
				Skills.Skills.Fishing = Copy(State.skills.fishing);
				Publish(MoveTemp(Skills));
			}
			if (Decoded.update.kind == p772::ServerUpdateKind::PlayerState
				&& State.state.known)
			{
				FReal33DEvent Conditions;
				Conditions.Kind = EReal33DEventKind::PlayerConditions;
				Conditions.Conditions.bKnown = true;
				Conditions.Conditions.Flags = State.state.flags;
				Publish(MoveTemp(Conditions));
			}
			if (Decoded.update.kind == p772::ServerUpdateKind::Inventory)
			{
				// The whole set, not the one slot that changed. WorldState has
				// already applied it, and publishing the set means the HUD can
				// never show two slots from two different moments.
				FReal33DEvent Worn;
				Worn.Kind = EReal33DEventKind::InventoryChanged;
				Worn.Inventory.bKnown = true;
				for (int32 Slot = FReal33DInventory::FirstSlot;
					Slot <= FReal33DInventory::LastSlot; ++Slot)
				{
					const auto& Source = State.inventory[Slot];
					Worn.Inventory.bOccupied[Slot] = Source.occupied;
					Worn.Inventory.Items[Slot] = ToUnrealItem(Source.item);
				}
				Publish(MoveTemp(Worn));
			}
			if (Decoded.update.kind == p772::ServerUpdateKind::Container)
			{
				const std::uint8_t Number = Decoded.update.container.container;
				FReal33DEvent Changed;
				Changed.Kind = EReal33DEventKind::ContainerChanged;
				Changed.Container.Number = Number;
				if (Number < State.containers.size())
				{
					const auto& Source = State.containers[Number];
					Changed.Container.bOpen = Source.open;
					Changed.Container.TypeId = Source.type_id;
					Changed.Container.Name = UTF8_TO_TCHAR(Source.name.c_str());
					Changed.Container.Capacity = Source.capacity;
					Changed.Container.bHasParent = Source.has_parent;
					Changed.Container.Objects.Reserve(
						static_cast<int32>(Source.objects.size()));
					for (const auto& Object : Source.objects)
					{
						Changed.Container.Objects.Add(ToUnrealItem(Object));
					}
				}
				Publish(MoveTemp(Changed));
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
			// Health travels with the creature, so the name can be coloured the
			// moment the actor is spawned rather than only on the first change.
			if (Event.creature_id != 0)
			{
				const auto Known = State.known_creatures.find(Event.creature_id);
				if (Known != State.known_creatures.end())
				{
					Out.HealthPercent = Known->second.health_percent;
				}
			}
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

		DrainSays();
		DrainMoves();
		DrainUses();

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

	/** Turns a semantic slot into the wire's special-coordinate encoding. */
	static p772::MoveEndpoint ToEndpoint(const UReal33DBridge::FMoveSlot& Slot)
	{
		switch (Slot.Kind)
		{
		case UReal33DBridge::FMoveSlot::EKind::Container:
			return p772::MoveEndpoint::InContainer(Slot.Container, Slot.Slot);
		case UReal33DBridge::FMoveSlot::EKind::Map:
			return p772::MoveEndpoint::OnMap(p772::MapPosition{
				Slot.Position.X, Slot.Position.Y, Slot.Position.Z });
		case UReal33DBridge::FMoveSlot::EKind::Inventory:
		default:
			return p772::MoveEndpoint::InInventory(Slot.Slot);
		}
	}

	void DrainMoves()
	{
		FMove Move;
		while (Moves.Dequeue(Move))
		{
			// Sent and then forgotten. This client draws nothing: what the
			// object did is whatever SV_CMD_*_IN_CONTAINER and
			// SV_CMD_SET_INVENTORY say next, and a move Fusion32 refuses
			// produces neither, which is correct because nothing moved.
			Session.SendCommand(p772::BuildMoveObjectCommand(
				ToEndpoint(Move.From), Move.TypeId, Move.StackIndex,
				ToEndpoint(Move.To), Move.Count));
			{
				FScopeLock Lock(&StatsMutex);
				Stats.MovesRequested += 1;
			}
		}
	}

	void DrainUses()
	{
		FUse Use;
		while (Uses.Dequeue(Use))
		{
			switch (Use.Kind)
			{
			case FUse::EKind::WithObject:
				Session.SendCommand(p772::BuildUseTwoObjectsCommand(
					ToEndpoint(Use.Object), Use.TypeId, Use.StackIndex,
					ToEndpoint(Use.Target), Use.TargetTypeId, Use.TargetStackIndex));
				break;
			case FUse::EKind::OnCreature:
				Session.SendCommand(p772::BuildUseOnCreatureCommand(
					ToEndpoint(Use.Object), Use.TypeId, Use.StackIndex, Use.CreatureId));
				break;
			case FUse::EKind::Object:
			default:
				Session.SendCommand(p772::BuildUseObjectCommand(
					ToEndpoint(Use.Object), Use.TypeId, Use.StackIndex,
					Use.OpenAsContainer));
				break;
			}
			{
				FScopeLock Lock(&StatsMutex);
				Stats.UsesRequested += 1;
			}
		}
	}

	void DrainSays()
	{
		FSay Say;
		while (Says.Dequeue(Say))
		{
			// ClientCore builds the command and enforces exactly what CTalk
			// enforces, so a line the server would discard is refused here with
			// a reason the player can be told.
			//
			// The text is sent exactly as typed. It once carried a "#y " or
			// "#w " prefix that was parsed back out here, which meant the UI
			// encoded a wire convention and a player who genuinely began a line
			// with "#y " could not say so. The mode now arrives as itself.
			const auto Built = p772::BuildTalkCommand(
				ToWireTalkMode(Say.Mode), TCHAR_TO_UTF8(*Say.Text));
			if (!Built.ok())
			{
				// A refusal the player caused and can act on, so it is told to
				// them rather than filed as a protocol diagnostic. Not a
				// ServerMessage: Fusion32 never saw this command.
				FReal33DEvent Refused;
				Refused.Kind = EReal33DEventKind::ClientNotice;
				Refused.Detail = FString::Printf(
					TEXT("Your message was not sent: %hs"),
					p772::TalkBuildErrorName(Built.error));
				Refused.Direction = kNoDirection;
				Publish(MoveTemp(Refused));

				UE_LOG(LogTemp, Warning, TEXT("REAL33D: talk %u refused before sending: %hs"),
					Say.SayId, p772::TalkBuildErrorName(Built.error));

				FScopeLock Lock(&StatsMutex);
				++Stats.SaysRefusedLocally;
				continue;
			}
			if (!Session.SendCommand(Built.payload).ok())
			{
				continue;
			}
			FScopeLock Lock(&StatsMutex);
			++Stats.SaysRequested;
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

	struct FSay
	{
		uint32 SayId = 0;
		/** Semantic. Becomes a protocol value in DrainSays and nowhere else. */
		EReal33DTalkMode Mode = EReal33DTalkMode::Say;
		FString Text;
	};

	/** A move the player asked for. Semantic until DrainMoves encodes it. */
	struct FMove
	{
		uint32 MoveId = 0;
		UReal33DBridge::FMoveSlot From;
		UReal33DBridge::FMoveSlot To;
		uint16 TypeId = 0;
		uint8 StackIndex = 0;
		uint8 Count = 1;
	};

	TQueue<FSay, EQueueMode::Spsc> Says;
	TQueue<FMove, EQueueMode::Spsc> Moves;
	TQueue<FUse, EQueueMode::Spsc> Uses;

	TQueue<FReal33DEvent, EQueueMode::Spsc> Events;
	TQueue<FIntent, EQueueMode::Spsc> Intents;

	mutable FCriticalSection StatsMutex;
	FReal33DStats Stats;
};

// ---------------------------------------------------------------- subsystem

const TCHAR* Real33DTalkModeLabel(EReal33DTalkMode Mode)
{
	switch (Mode)
	{
	case EReal33DTalkMode::Whisper: return TEXT("Whisper");
	case EReal33DTalkMode::Yell:    return TEXT("Yell");
	default:                        return TEXT("Say");
	}
}

// Both of these are here, where FReal33DChatTranscript is a complete type.
UReal33DBridge::~UReal33DBridge()
{
	delete Transcript;
	Transcript = nullptr;
}

void UReal33DBridge::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Transcript = new FReal33DChatTranscript();
}

void UReal33DBridge::Deinitialize()
{
	Disconnect();
	delete Transcript;
	Transcript = nullptr;
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
		// Filed as it passes, so the transcript is complete whatever the
		// presenter does with the event afterwards, and so there is exactly one
		// place that decides what a player is allowed to read.
		NoteChat(Event);
		OutEvents.Add(MoveTemp(Event));
	}
}

void UReal33DBridge::NoteChat(const FReal33DEvent& Event)
{
	if (Transcript == nullptr)
	{
		return;
	}

	switch (Event.Kind)
	{
	case EReal33DEventKind::Talk:
		// Every utterance, including one that will also be drawn above its
		// speaker. A console that omitted what was said nearby would be a
		// partial record of a conversation, and the nearby case is exactly the
		// one an operator uses to check the distant case is not lying.
		Transcript->Log.AddSpeech(
			TCHAR_TO_UTF8(*Event.Speaker),
			TCHAR_TO_UTF8(*Event.TalkMode),
			TCHAR_TO_UTF8(*Event.Detail));
		++Transcript->Revision;
		break;

	case EReal33DEventKind::ServerMessage:
		Transcript->Log.AddServerMessage(
			TCHAR_TO_UTF8(*Event.TalkMode),
			TCHAR_TO_UTF8(*Event.Detail));
		++Transcript->Revision;
		break;

	case EReal33DEventKind::ClientNotice:
		Transcript->Log.AddClientNotice(TCHAR_TO_UTF8(*Event.Detail));
		++Transcript->Revision;
		break;

	case EReal33DEventKind::Disconnected:
	case EReal33DEventKind::Failed:
		// A transcript from a previous connection must not appear to belong to
		// the new one. The sequence keeps counting, so two arrivals are never
		// confusable in evidence even across the gap.
		Transcript->Log.Clear();
		++Transcript->Revision;
		break;

	// Diagnostic is deliberately absent. It is developer prose and has no
	// player-facing surface; routing it here is the defect this milestone
	// exists to close.
	default:
		break;
	}
}

void UReal33DBridge::GetChatTranscript(TArray<FReal33DChatLine>& OutLines) const
{
	check(IsInGameThread());
	OutLines.Reset();
	if (Transcript == nullptr)
	{
		return;
	}
	const auto& Entries = Transcript->Log.entries();
	OutLines.Reserve(static_cast<int32>(Entries.size()));
	for (const auto& Entry : Entries)
	{
		FReal33DChatLine Out;
		// Formatted by ClientCore, where the rule is tested, rather than by the
		// widget, where it would only be observable live.
		Out.Line = UTF8_TO_TCHAR(p772::ChatLog::Format(Entry).c_str());
		Out.bSystemLine = Entry.kind != p772::ChatLog::EntryKind::Speech;
		OutLines.Add(MoveTemp(Out));
	}
}

uint64 UReal33DBridge::GetChatRevision() const
{
	return Transcript != nullptr ? Transcript->Revision : 0;
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

UReal33DBridge::FMoveSlot UReal33DBridge::FMoveSlot::InInventory(uint8 InSlot)
{
	FMoveSlot Slot;
	Slot.Kind = EKind::Inventory;
	Slot.Slot = InSlot;
	return Slot;
}

UReal33DBridge::FMoveSlot UReal33DBridge::FMoveSlot::InContainer(
	uint8 InContainer, uint8 InSlot)
{
	FMoveSlot Slot;
	Slot.Kind = EKind::Container;
	Slot.Container = InContainer;
	Slot.Slot = InSlot;
	return Slot;
}

UReal33DBridge::FMoveSlot UReal33DBridge::FMoveSlot::OnMap(
	const Real33D::FMapPosition& InPosition)
{
	FMoveSlot Slot;
	Slot.Kind = EKind::Map;
	Slot.Position = InPosition;
	return Slot;
}

uint32 UReal33DBridge::RequestMoveObject(const FMoveSlot& From, uint16 TypeId,
	uint8 StackIndex, const FMoveSlot& To, uint8 Count)
{
	if (Worker == nullptr)
	{
		return 0;
	}
	// Numbered on the game thread at the moment the player asked, like a walk
	// or a say, so the request has an identity before anything else happens.
	const uint32 MoveId = ++NextInputId;
	Worker->PostMove(MoveId, From, TypeId, StackIndex, To, Count);
	return MoveId;
}

uint32 UReal33DBridge::RequestUseObject(const FMoveSlot& Object, uint16 TypeId,
	uint8 StackIndex, uint8 OpenAsContainer)
{
	if (Worker == nullptr)
	{
		return 0;
	}
	FReal33DWorker::FUse Use;
	Use.UseId = ++NextInputId;
	Use.Kind = FReal33DWorker::FUse::EKind::Object;
	Use.Object = Object;
	Use.TypeId = TypeId;
	Use.StackIndex = StackIndex;
	Use.OpenAsContainer = OpenAsContainer;
	Worker->PostUse(Use);
	return Use.UseId;
}

uint32 UReal33DBridge::RequestUseWithObject(const FMoveSlot& Object, uint16 TypeId,
	uint8 StackIndex, const FMoveSlot& Target, uint16 TargetTypeId,
	uint8 TargetStackIndex)
{
	if (Worker == nullptr)
	{
		return 0;
	}
	FReal33DWorker::FUse Use;
	Use.UseId = ++NextInputId;
	Use.Kind = FReal33DWorker::FUse::EKind::WithObject;
	Use.Object = Object;
	Use.TypeId = TypeId;
	Use.StackIndex = StackIndex;
	Use.Target = Target;
	Use.TargetTypeId = TargetTypeId;
	Use.TargetStackIndex = TargetStackIndex;
	Worker->PostUse(Use);
	return Use.UseId;
}

uint32 UReal33DBridge::RequestUseOnCreature(const FMoveSlot& Object, uint16 TypeId,
	uint8 StackIndex, uint32 CreatureId)
{
	if (Worker == nullptr)
	{
		return 0;
	}
	FReal33DWorker::FUse Use;
	Use.UseId = ++NextInputId;
	Use.Kind = FReal33DWorker::FUse::EKind::OnCreature;
	Use.Object = Object;
	Use.TypeId = TypeId;
	Use.StackIndex = StackIndex;
	Use.CreatureId = CreatureId;
	Worker->PostUse(Use);
	return Use.UseId;
}

uint32 UReal33DBridge::RequestTalk(EReal33DTalkMode Mode, const FString& Text)
{
	if (Worker == nullptr)
	{
		return 0;
	}
	// Numbered on the game thread like a walk, so a line can be followed from
	// the click that sent it to whatever Fusion32 does about it.
	const uint32 SayId = ++NextInputId;
	Worker->PostSay(SayId, Mode, Text);
	return SayId;
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
