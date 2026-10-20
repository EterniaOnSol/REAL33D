#include "Real33DBridge.h"
#include "REAL33D.h"
#include "Real33DChatViews.h"
#include "Real33DItemUsePolicy.h"
#include "Real33DMinimapPalette.h"

#include "HAL/PlatformProcess.h"
#include "HAL/RunnableThread.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"

// Protocol772Core. Included only here, in the worker. Nothing above this file
// may include a protocol header: that is the boundary.
THIRD_PARTY_INCLUDES_START
#include "fusion32/protocol772/channels.h"
#include "fusion32/protocol772/chat_log.h"
#include "fusion32/protocol772/editors.h"
#include "fusion32/protocol772/gamelogin.h"
#include "fusion32/protocol772/initial_world.h"
#include "fusion32/protocol772/login.h"
#include "fusion32/protocol772/movement.h"
#include "fusion32/protocol772/movement_ledger.h"
#include "fusion32/protocol772/outfit.h"
#include "fusion32/protocol772/talk_command.h"
#include "fusion32/protocol772/trade.h"
#include "fusion32/protocol772/talk_speaker.h"
#include "fusion32/protocol772/player_state.h"
#include "fusion32/protocol772/worldview.h"
#include "fusion32/protocol772/minimap.h"
THIRD_PARTY_INCLUDES_END

#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace p772 = fusion32::protocol772;

// Game-thread-only memory; no protocol/header leaks into Slate or Actors.
struct FReal33DMinimapStore
{
	p772::KnownMinimap Memory;
	FReal33DMinimapState State;
	FString Scope;
	FString Path;
	bool bDirty = false;
	double LastSave = 0.0;
};

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

// The outfit colour array must be exactly as wide as the one on the wire, and
// the client-side bound must be the server's own. Both are restated above this
// boundary for layers that cannot include ClientCore, and both are pinned here,
// in the one file that can see each pair at once. A narrower array would drop a
// colour; a looser bound would let this client send a selection CSetOutfit
// silently discards.
static_assert(FReal33DOutfit::ColorCount == int32(p772::kOutfitColorCount),
	"outfit colour count drift");
static_assert(Real33D::kOutfitSlotCount == int(p772::kOutfitColorCount),
	"outfit slot count drift");
static_assert(Real33D::kMaxOutfitColor == p772::kMaxOutfitColor,
	"outfit colour bound drift");

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
 * One creature's whole appearance, across the boundary.
 *
 * THE ONLY PLACE AN OutfitDescriptor IS CONVERTED, and the reason it exists is
 * that there used to be two and both were incomplete: each copied the id and
 * the disguise flag and dropped the four colours and the object type. One
 * function means a field added to `OutfitDescriptor` is either carried or
 * caught by the compiler, instead of being forgotten in whichever site the
 * next reader does not look at.
 *
 * The two branches are exclusive on the wire -- `cr.hh::TOutfit` is a union and
 * `SendOutfit` writes the id and then *either* an object type *or* the colours
 * -- so each is copied only under its own flag and the other is left at zero
 * rather than filled with something plausible.
 */
static FReal33DOutfit ToUnrealOutfit(const p772::OutfitDescriptor& From)
{
	FReal33DOutfit To;
	To.OutfitId = From.outfit_id;
	To.bDisguisedAsObject = From.disguised_as_object;
	if (From.disguised_as_object)
	{
		// OutfitID == 0, so the server sent an object type where the colours
		// would be. Dropping it left a disguised creature with no appearance
		// at all: id zero, no type, and four colours it never had.
		To.ObjectType = From.object_type;
	}
	else
	{
		for (int32 Index = 0; Index < FReal33DOutfit::ColorCount; ++Index)
		{
			To.Colors[Index] = From.colors[Index];
		}
	}
	return To;
}

/** Copies ClientCore's single combat record across the engine boundary. */
static FReal33DCombat ToUnrealCombat(const p772::CombatState& From)
{
	FReal33DCombat To;
	To.TargetCreatureId = From.target_creature_id;
	To.bFollowing = From.following;
	To.bTacticsSent = From.tactics_sent;
	switch (From.attack_mode)
	{
	case static_cast<uint8>(p772::AttackMode::Offensive):
		To.AttackMode = EReal33DAttackMode::Offensive; break;
	case static_cast<uint8>(p772::AttackMode::Defensive):
		To.AttackMode = EReal33DAttackMode::Defensive; break;
	default:
		To.AttackMode = EReal33DAttackMode::Balanced; break;
	}
	To.ChaseMode = From.chase_mode == static_cast<uint8>(p772::ChaseMode::Follow)
		? EReal33DChaseMode::Follow : EReal33DChaseMode::Stand;
	return To;
}

/**
 * The transcripts, wrapped so Real33DBridge.h needs nothing from fusion32/.
 *
 * ChatLog is documented as single-owner, single-thread. Its owner is the game
 * thread, because that is where DrainEvents runs and where the widget reads.
 * The worker never touches any of this.
 *
 * One log per surface, keyed by the view id, which is monotonic and never
 * reused. Keyed rather than parallel-indexed on purpose: an array indexed
 * alongside `Views` would have to be kept in step with every open and close by
 * hand, and the first missed close would show one conversation's lines under
 * another's name. A key cannot drift -- closing a view erases its log, and a
 * log with no view is unreachable.
 *
 * `Local` is the view id Local Chat holds, so the no-argument
 * GetChatTranscript keeps meaning exactly what it meant before channels
 * existed.
 */
struct FReal33DChatTranscript
{
	Real33D::ChatViewSet Views;
	std::map<std::uint32_t, p772::ChatLog> Logs;
	/** Bumped by any change to any transcript or to the set of surfaces. */
	uint64 Revision = 0;

	/** The channel list Fusion32 last sent, and whether it ever answered. */
	std::vector<FReal33DChannelEntry> ChannelList;
	bool bChannelListKnown = false;
	uint64 ChannelListRevision = 0;

	/**
	 * The one editor window Fusion32 has open, if any.
	 *
	 * One, not a stack: the server sends a fresh SV_CMD_EDIT_TEXT whenever
	 * another object is used and expects the previous window to be gone, so
	 * keeping several would leave a window on screen the server has already
	 * forgotten about.
	 */
	FReal33DEditor Editor;
	uint64 EditorRevision = 0;

	/**
	 * The one trade Fusion32 has open. One, because TPlayer holds a single
	 * TradeObject and TradePartner, so a second cannot exist.
	 */
	FReal33DTrade Trade;
	uint64 TradeRevision = 0;

	/**
	 * The one outfit chooser Fusion32 has offered, if any.
	 *
	 * One, and replaced wholesale by a newer SV_CMD_OUTFIT: the server treats
	 * it as a single window and a second request answers into the same one.
	 */
	FReal33DOutfitDialog OutfitDialog;
	uint64 OutfitDialogRevision = 0;

	FReal33DChatTranscript()
	{
		// Local Chat exists before the first connection, because a failure
		// during login has to have somewhere to be read.
		Logs.emplace(Views.LocalId(), p772::ChatLog());
	}

	/** The log for a view, created on first use. Never null. */
	p772::ChatLog& LogFor(std::uint32_t ViewId)
	{
		const auto Found = Logs.find(ViewId);
		if (Found != Logs.end())
		{
			return Found->second;
		}
		return Logs.emplace(ViewId, p772::ChatLog()).first->second;
	}

	const p772::ChatLog* FindLog(std::uint32_t ViewId) const
	{
		const auto Found = Logs.find(ViewId);
		return Found != Logs.end() ? &Found->second : nullptr;
	}

	/** Back to Local Chat alone, with nothing in it. Used when a session ends. */
	void Reset()
	{
		Views.Reset();
		Logs.clear();
		Logs.emplace(Views.LocalId(), p772::ChatLog());
		ChannelList.clear();
		// Not known again. The list was this player's, filtered by their own
		// rights, and a list from the previous session must not look current.
		bChannelListKnown = false;
		++ChannelListRevision;
		// A window belongs to the connection that opened it: the object id it
		// submits against means nothing to a new session.
		Editor = FReal33DEditor();
		++EditorRevision;
		// A trade belongs to the connection that opened it; the partner's
		// creature id means nothing in a new session.
		Trade = FReal33DTrade();
		++TradeRevision;
		// A chooser belongs to the connection that offered it. The band was
		// computed from that character's sex and premium right, so showing it
		// to a new session could offer outfits this character may not have.
		OutfitDialog = FReal33DOutfitDialog();
		++OutfitDialogRevision;
		++Revision;
	}
};

// The two chat-view enums are deliberately unrelated types -- one is Unreal's,
// one is Slate-free and testable with plain g++ -- so they are pinned here, in
// the one file that can see both.
static_assert(static_cast<uint8>(EReal33DChatViewKind::Local)
	== static_cast<uint8>(Real33D::ChatViewKind::Local), "chat view kind drift");
static_assert(static_cast<uint8>(EReal33DChatViewKind::Channel)
	== static_cast<uint8>(Real33D::ChatViewKind::Channel), "chat view kind drift");
static_assert(static_cast<uint8>(EReal33DChatViewKind::Private)
	== static_cast<uint8>(Real33D::ChatViewKind::Private), "chat view kind drift");

// This client refuses to join the one channel whose answer it cannot read. The
// number is Fusion32's own, from operate.hh::CHANNEL_RULEVIOLATIONS.
static_assert(UReal33DBridge::kRuleViolationsChannel
	== static_cast<int32>(p772::kChannelRuleViolations), "rule violations channel drift");

// The list types are enums.hh's own values. Pinned here, the one file that can
// see Unreal's mirror and ClientCore's definition at once, because submitting a
// guest list under the subowner byte would edit the wrong list entirely.
static_assert(static_cast<uint8>(EReal33DEditListType::Guest)
	== static_cast<uint8>(p772::EditListType::GuestList), "edit list type drift");
static_assert(static_cast<uint8>(EReal33DEditListType::Subowner)
	== static_cast<uint8>(p772::EditListType::SubownerList), "edit list type drift");
static_assert(static_cast<uint8>(EReal33DEditListType::Door)
	== static_cast<uint8>(p772::EditListType::DoorList), "edit list type drift");

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
		enum class EKind : uint8 { Object, WithObject, OnCreature, Look };
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

	bool RequiresUseTarget(uint16 TypeId) const
	{
		FScopeLock Lock(&StatsMutex);
		return MultiUseTypes.count(TypeId) != 0;
	}

	struct FCombatIntent
	{
		enum class EKind : uint8 { Attack, Follow, Cancel, AttackMode, ChaseMode };
		uint32 ActionId = 0;
		EKind Kind = EKind::Attack;
		uint32 CreatureId = 0;
		EReal33DAttackMode AttackMode = EReal33DAttackMode::Balanced;
		EReal33DChaseMode ChaseMode = EReal33DChaseMode::Stand;
	};

	void PostCombat(const FCombatIntent& Intent)
	{
		CombatIntents.Enqueue(Intent);
	}

	/**
	 * Something the player asked of a channel. Semantic until DrainChannels
	 * encodes it, like every other intent here.
	 *
	 * The two talk kinds are in this queue rather than in `Says` because they
	 * carry a field a Say does not have, and because keeping them together
	 * means a join and the first line typed into the channel it opened cannot
	 * be reordered relative to one another.
	 */
	struct FChannelIntent
	{
		enum class EKind : uint8 { List, Join, Leave, OpenPrivate, ChannelTalk, PrivateTalk };
		uint32 ActionId = 0;
		EKind Kind = EKind::List;
		uint16 Channel = 0;
		/** The character, for OpenPrivate and PrivateTalk. */
		FString Name;
		FString Text;
	};

	void PostChannel(const FChannelIntent& Intent)
	{
		ChannelIntents.Enqueue(Intent);
	}

	/**
	 * An edited text or list the player submitted.
	 *
	 * `ListType` is Unknown for the text form, which is what distinguishes
	 * the two: CL_CMD_EDIT_TEXT carries an object id and a string, and
	 * CL_CMD_EDIT_LIST carries a type byte as well.
	 */
	struct FEditIntent
	{
		uint32 ActionId = 0;
		bool bIsText = true;
		uint32 Id = 0;
		EReal33DEditListType ListType = EReal33DEditListType::Unknown;
		FString Text;
	};

	void PostEdit(const FEditIntent& Intent)
	{
		EditIntents.Enqueue(Intent);
	}

	/** Something the player asked of a trade. Semantic until DrainTrades. */
	struct FTradeIntent
	{
		enum class EKind : uint8 { Offer, Inspect, Accept, Reject };
		uint32 ActionId = 0;
		EKind Kind = EKind::Offer;
		/** Offer only. */
		UReal33DBridge::FMoveSlot Object;
		uint16 TypeId = 0;
		uint8 StackIndex = 0;
		uint32 PartnerCreatureId = 0;
		/** Inspect only. */
		bool bPartnerOffer = false;
		uint8 Index = 0;
	};

	void PostTrade(const FTradeIntent& Intent)
	{
		TradeIntents.Enqueue(Intent);
	}

	/**
	 * Something the player asked of the outfit chooser.
	 *
	 * `Band` travels with a Set rather than being looked up when it is sent,
	 * because it is the band of the SV_CMD_OUTFIT the player was answering.
	 * Re-deriving it on the worker could validate a selection against a band
	 * from a chooser the player never saw -- a second SV_CMD_OUTFIT can arrive
	 * between the click and the drain -- which would let a selection through
	 * that the server then drops in silence.
	 */
	struct FOutfitIntent
	{
		enum class EKind : uint8 { Get, Set };
		uint32 ActionId = 0;
		EKind Kind = EKind::Get;
		/** Set only. */
		uint16 OutfitId = 0;
		std::uint8_t Colors[p772::kOutfitColorCount] = {};
		p772::OutfitSelectionRange Band;
	};

	void PostOutfit(const FOutfitIntent& Intent)
	{
		OutfitIntents.Enqueue(Intent);
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
		FString PaletteText;
		const FString PalettePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Minimap/appearance-palette.txt"));
		if (IFileManager::Get().FileSize(*PalettePath) <= 128 * 1024
			&& FFileHelper::LoadFileToString(PaletteText, *PalettePath))
			MinimapPalette.Load(TCHAR_TO_UTF8(*PaletteText));
		{
			FScopeLock Lock(&StatsMutex);
			MultiUseTypes = Real33D::ReadMultiUseTypeIds(TCHAR_TO_UTF8(*ObjectsText));
			UE_LOG(LogReal33D, Log, TEXT("Use target metadata: %u runtime MultiUse types"), static_cast<uint32>(MultiUseTypes.size()));
		}
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

			if (Decoded.update.kind == p772::ServerUpdateKind::CreatureAttribute
				&& Decoded.update.creature_attribute.attribute == p772::CreatureAttribute::Outfit)
			{
				// The authoritative appearance, for any creature including this
				// player. This is the only confirmation a submitted outfit ever
				// gets, and it is conditional: CSetOutfit broadcasts only when
				// the player is wearing their own outfit, so its absence proves
				// nothing and the chooser never waits for it.
				const auto& Source = Decoded.update.creature_attribute;
				FReal33DEvent Appearance;
				Appearance.Kind = EReal33DEventKind::CreatureAppearance;
				Appearance.CreatureId = Source.creature_id;
				Appearance.Outfit = ToUnrealOutfit(Source.outfit);
				Appearance.Direction = kNoDirection;
				Publish(MoveTemp(Appearance));

				FScopeLock Lock(&StatsMutex);
				++Stats.OutfitAppearanceUpdates;
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
				// Decided here, where the mode is still a number, so nothing
				// above the bridge has to recognise one. A private message is
				// the only talk addressed to this player alone, and it is what
				// decides the line belongs in a private conversation rather
				// than in Local Chat.
				Spoken.bPrivateMessage =
					Talk.mode == static_cast<std::uint8_t>(p772::TalkMode::PrivateMessage);
				const bool bWasChannel = Spoken.bHasChannel;
				const bool bWasPrivate = Spoken.bPrivateMessage;
				Publish(MoveTemp(Spoken));

				FScopeLock Lock(&StatsMutex);
				++Stats.TalkMessages;
				if (bWasChannel)
				{
					++Stats.ChannelTalkReceived;
				}
				if (bWasPrivate)
				{
					++Stats.PrivateTalkReceived;
				}
			}

			if (Decoded.update.kind == p772::ServerUpdateKind::Channel)
			{
				// The five channel commands, decoded by ClientCore and turned
				// into four events here. Nothing is stored on this thread:
				// WorldState deliberately keeps no channel membership --
				// Fusion32 owns the lists and resends them on request -- so
				// which tabs exist is presentation state and belongs to the
				// game thread, next to the transcripts.
				const p772::ChannelUpdate& Chan = Decoded.update.channel;
				FReal33DEvent Event;
				Event.Direction = kNoDirection;
				switch (Chan.kind)
				{
				case p772::ChannelUpdateKind::List:
					Event.Kind = EReal33DEventKind::ChannelList;
					Event.Channels.Reserve(static_cast<int32>(Chan.channels.size()));
					for (const p772::ChannelEntry& Each : Chan.channels)
					{
						FReal33DChannelEntry Row;
						// The offer row's id is 0xFFFF and is not a channel
						// number, so it is carried as the flag ClientCore
						// already resolved rather than as a number a caller
						// could mistake for one.
						Row.Id = Each.own_channel_offer
							? 0 : static_cast<int32>(Each.id);
						Row.Name = UTF8_TO_TCHAR(Each.name.c_str());
						Row.bOwnChannelOffer = Each.own_channel_offer;
						Event.Channels.Add(MoveTemp(Row));
					}
					break;

				case p772::ChannelUpdateKind::Opened:
				case p772::ChannelUpdateKind::OpenedOwn:
					Event.Kind = EReal33DEventKind::ChannelOpened;
					Event.bHasChannel = true;
					Event.Channel = static_cast<int32>(Chan.channel);
					Event.ChannelName = UTF8_TO_TCHAR(Chan.name.c_str());
					Event.bOwnChannel =
						Chan.kind == p772::ChannelUpdateKind::OpenedOwn;
					break;

				case p772::ChannelUpdateKind::PrivateOpened:
					Event.Kind = EReal33DEventKind::PrivateChannelOpened;
					Event.ChannelName = UTF8_TO_TCHAR(Chan.name.c_str());
					break;

				case p772::ChannelUpdateKind::Closed:
					Event.Kind = EReal33DEventKind::ChannelClosed;
					Event.bHasChannel = true;
					Event.Channel = static_cast<int32>(Chan.channel);
					break;
				}
				const EReal33DEventKind Published = Event.Kind;
				Publish(MoveTemp(Event));

				FScopeLock Lock(&StatsMutex);
				switch (Published)
				{
				case EReal33DEventKind::ChannelList:
					++Stats.ChannelListsReceived; break;
				case EReal33DEventKind::ChannelOpened:
					++Stats.ChannelsOpened; break;
				case EReal33DEventKind::ChannelClosed:
					++Stats.ChannelsClosed; break;
				case EReal33DEventKind::PrivateChannelOpened:
					++Stats.PrivateChannelsOpened; break;
				default:
					break;
				}
			}

			if (Decoded.update.kind == p772::ServerUpdateKind::Editor)
			{
				// Decoded by ClientCore, stored nowhere by WorldState --
				// Fusion32 owns the text and resends the whole body every
				// time the object is used -- so the open window is
				// presentation state and belongs to the game thread.
				const p772::EditorUpdate& Edit = Decoded.update.editor;
				FReal33DEvent Event;
				Event.Direction = kNoDirection;
				Event.Editor.bOpen = true;

				if (Edit.kind == p772::EditorUpdateKind::Text)
				{
					Event.Kind = EReal33DEventKind::EditTextOpened;
					Event.Editor.bIsText = true;
					Event.Editor.ObjectId = Edit.text.object_id;
					Event.Editor.TypeId = Edit.text.type_id;
					Event.Editor.MaxLength = Edit.text.max_length;
					Event.Editor.Text = UTF8_TO_TCHAR(Edit.text.text.c_str());
					Event.Editor.Editor = UTF8_TO_TCHAR(Edit.text.editor.c_str());
					UE_LOG(LogReal33D, Log,
						TEXT("edit text: object %u type %u, %d char(s), max %u, editor \"%s\""),
						Edit.text.object_id, Edit.text.type_id,
						Event.Editor.Text.Len(), Edit.text.max_length,
						*Event.Editor.Editor);
				}
				else
				{
					Event.Kind = EReal33DEventKind::EditListOpened;
					Event.Editor.bIsText = false;
					Event.Editor.ListId = Edit.list.id;
					// Unknown rather than a guess when the byte is not one of
					// the three. ClientCore already reports that; carrying it
					// as a fourth value keeps the window from submitting a
					// list under a type Fusion32 never named.
					Event.Editor.ListType = Edit.list.known_type
						? static_cast<EReal33DEditListType>(Edit.list.type)
						: EReal33DEditListType::Unknown;
					Event.Editor.Text = UTF8_TO_TCHAR(Edit.list.text.c_str());
					UE_LOG(LogReal33D, Log,
						TEXT("edit list: %s, id %u, %d char(s)"),
						Real33DEditListTypeLabel(Event.Editor.ListType),
						Edit.list.id, Event.Editor.Text.Len());
				}

				const bool bWasText = Event.Editor.bIsText;
				Publish(MoveTemp(Event));

				FScopeLock Lock(&StatsMutex);
				if (bWasText) { ++Stats.EditTextsOpened; }
				else          { ++Stats.EditListsOpened; }
			}

			if (Decoded.update.kind == p772::ServerUpdateKind::Trade)
			{
				// Decoded by ClientCore and stored nowhere by WorldState:
				// Fusion32 owns the trade and resends each offer whenever it
				// changes, so the open trade is presentation state and lives
				// on the game thread with the other surfaces.
				const p772::TradeUpdate& T = Decoded.update.trade;
				FReal33DEvent Event;
				Event.Direction = kNoDirection;
				switch (T.kind)
				{
				case p772::TradeUpdateKind::OwnOffer:
				case p772::TradeUpdateKind::PartnerOffer:
				{
					const bool bOwn = T.kind == p772::TradeUpdateKind::OwnOffer;
					Event.Kind = bOwn ? EReal33DEventKind::TradeOwnOffer
									  : EReal33DEventKind::TradePartnerOffer;
					Event.TradeOffer.bKnown = true;
					Event.TradeOffer.Name = UTF8_TO_TCHAR(T.name.c_str());
					Event.TradeOffer.Items.Reserve(static_cast<int32>(T.items.size()));
					for (const p772::ItemThing& Each : T.items)
					{
						Event.TradeOffer.Items.Add(ToUnrealItem(Each));
					}
					UE_LOG(LogReal33D, Log, TEXT("trade %s offer from \"%s\": %d object(s)"),
						bOwn ? TEXT("own") : TEXT("partner"),
						*Event.TradeOffer.Name, Event.TradeOffer.Items.Num());
					break;
				}
				case p772::TradeUpdateKind::Closed:
					Event.Kind = EReal33DEventKind::TradeClosed;
					// Named exactly, because the command cannot say which:
					// AcceptTrade closes both sides after moving the objects
					// and also closes both sides when a check fails.
					UE_LOG(LogReal33D, Log,
						TEXT("trade closed by Fusion32; the command does not say "
							 "whether it completed, only the inventory does"));
					break;
				}
				const EReal33DEventKind Published = Event.Kind;
				Publish(MoveTemp(Event));

				FScopeLock Lock(&StatsMutex);
				switch (Published)
				{
				case EReal33DEventKind::TradeOwnOffer:
					++Stats.TradeOwnOffersReceived; break;
				case EReal33DEventKind::TradePartnerOffer:
					++Stats.TradePartnerOffersReceived; break;
				case EReal33DEventKind::TradeClosed:
					++Stats.TradeClosesReceived; break;
				default: break;
				}
			}

			if (Decoded.update.kind == p772::ServerUpdateKind::OutfitDialog)
			{
				// Decoded by ClientCore and stored nowhere by WorldState, like
				// the trade: the chooser is a question the server asked, not a
				// fact about the world. It lives on the game thread with the
				// other surfaces.
				const p772::OutfitDialogUpdate& Dialog = Decoded.update.outfit_dialog;
				const p772::OutfitSelectionRange Range =
					p772::DialogSelectionRange(Dialog);

				FReal33DEvent Event;
				Event.Kind = EReal33DEventKind::OutfitDialogOpened;
				Event.Direction = kNoDirection;
				Event.OutfitDialog.bOpen = true;
				Event.OutfitDialog.Current = ToUnrealOutfit(Dialog.current);
				// The server's band, verbatim. Never recomputed here: the game
				// protocol does not carry this player's sex or premium right,
				// so the only honest source is the two words that arrived.
				Event.OutfitDialog.First = Range.first;
				Event.OutfitDialog.Last = Range.last;
				Publish(MoveTemp(Event));

				UE_LOG(LogReal33D, Log,
					TEXT("outfit chooser offered: current=%u%s band=%u..%u (%d outfit(s))"),
					Dialog.current.outfit_id,
					Dialog.current.disguised_as_object
						? TEXT(" [disguised as an object]") : TEXT(""),
					Range.first, Range.last, int32(Range.count()));
				if (Range.count() == 0)
				{
					// Cannot happen against a healthy server -- SendOutfit
					// computes both words from one expression -- so it is
					// reported rather than repaired.
					UE_LOG(LogReal33D, Warning,
						TEXT("outfit band %u..%u names no outfit; nothing is selectable"),
						Range.first, Range.last);
				}

				FScopeLock Lock(&StatsMutex);
				++Stats.OutfitDialogsReceived;
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
			if (Decoded.update.kind == p772::ServerUpdateKind::ClearTarget)
			{
				FScopeLock Lock(&StatsMutex);
				++Stats.TargetClearsReceived;
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
					Out.Outfit = ToUnrealOutfit(Known->second.outfit);
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
			case p772::WorldEventKind::CombatChanged:
				Out.Kind = EReal33DEventKind::CombatChanged;
				Out.Combat = ToUnrealCombat(Event.combat);
				break;
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
					// Carried as received. Only a cumulative object has one.
					Flat.bHasAmount = !Flat.bIsCreature && Thing.item.has_amount;
					Flat.Amount = Thing.item.amount;
					// Resolved here, where the object type table already lives,
					// so nothing above the bridge needs a type id to know how
					// to draw a field.
					Flat.bBlocking = !Flat.bIsCreature
						&& Types.Lookup(Thing.item.type_id).unpass;
					Flat.bGround = !Flat.bIsCreature
						&& Types.Lookup(Thing.item.type_id).priority == p772::ObjectPriority::Bank;
					Out.Things.Add(Flat);
				}
			}
			Publish(MoveTemp(Out));
		}
		if (State.map_initialized && State.viewport_synchronized())
		{
			FReal33DEvent Observed;
			Observed.Kind = EReal33DEventKind::MinimapObserved;
			Observed.Position = ToBridge(State.viewport_anchor);
			for (const auto& Cell : p772::ProjectMinimapFloor(State, Types, MinimapPalette.colors))
			{
				FReal33DMinimapCell Flat;
				Flat.Position = ToBridge(Cell.position);
				Flat.GroundType = Cell.terrain.ground_type;
				Flat.ColorType = Cell.terrain.color_type;
				Flat.bStaticObstacle = Cell.terrain.static_obstacle;
				Flat.bLive = true;
				Observed.MinimapCells.Add(Flat);
			}
			Publish(MoveTemp(Observed));
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
		DrainCombat();
		DrainChannels();
		DrainEdits();
		DrainTrades();
		DrainOutfits();

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
			case FUse::EKind::Look:
				Session.SendCommand(p772::BuildLookAtPointCommand(ToEndpoint(Use.Object)));
				break;
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
				if (Use.Kind == FUse::EKind::Look)
				{
					Stats.LooksRequested += 1;
				}
				else
				{
					Stats.UsesRequested += 1;
				}
			}
		}
	}

	static p772::AttackMode ToCoreAttackMode(EReal33DAttackMode Mode)
	{
		switch (Mode)
		{
		case EReal33DAttackMode::Offensive: return p772::AttackMode::Offensive;
		case EReal33DAttackMode::Defensive: return p772::AttackMode::Defensive;
		default: return p772::AttackMode::Balanced;
		}
	}

	static p772::ChaseMode ToCoreChaseMode(EReal33DChaseMode Mode)
	{
		return Mode == EReal33DChaseMode::Follow
			? p772::ChaseMode::Follow : p772::ChaseMode::Stand;
	}

	static p772::AttackMode CurrentAttackMode(const p772::WorldState& State)
	{
		switch (State.combat.attack_mode)
		{
		case static_cast<uint8>(p772::AttackMode::Offensive):
			return p772::AttackMode::Offensive;
		case static_cast<uint8>(p772::AttackMode::Defensive):
			return p772::AttackMode::Defensive;
		default:
			return p772::AttackMode::Balanced;
		}
	}

	static p772::ChaseMode CurrentChaseMode(const p772::WorldState& State)
	{
		return State.combat.chase_mode == static_cast<uint8>(p772::ChaseMode::Follow)
			? p772::ChaseMode::Follow : p772::ChaseMode::Stand;
	}

	static p772::SecureMode CurrentSecureMode(const p772::WorldState& State)
	{
		return State.combat.secure_mode == static_cast<uint8>(p772::SecureMode::Disabled)
			? p772::SecureMode::Disabled : p772::SecureMode::Enabled;
	}

	void DrainCombat()
	{
		FCombatIntent Intent;
		while (CombatIntents.Dequeue(Intent))
		{
			std::vector<std::uint8_t> Command;
			switch (Intent.Kind)
			{
			case FCombatIntent::EKind::Attack:
			{
				// Repeating the active action is the target toggle. The decision
				// reads WorldState here; no Slate row owns a selected id.
				const bool bToggleOff = State.combat.target_creature_id == Intent.CreatureId
					&& !State.combat.following;
				const uint32 Target = bToggleOff ? 0 : Intent.CreatureId;
				Command = p772::BuildAttackCommand(Target);
				if (!Session.SendCommand(Command).ok()) break;
				p772::NoteCombatRequest(&State, Target, false);
				FScopeLock Lock(&StatsMutex);
				++Stats.AttacksRequested;
				break;
			}
			case FCombatIntent::EKind::Follow:
			{
				const bool bToggleOff = State.combat.target_creature_id == Intent.CreatureId
					&& State.combat.following;
				const uint32 Target = bToggleOff ? 0 : Intent.CreatureId;
				Command = p772::BuildFollowCommand(Target);
				if (!Session.SendCommand(Command).ok()) break;
				p772::NoteCombatRequest(&State, Target, true);
				FScopeLock Lock(&StatsMutex);
				++Stats.FollowsRequested;
				break;
			}
			case FCombatIntent::EKind::Cancel:
				Command = p772::BuildCancelCommand();
				if (!Session.SendCommand(Command).ok()) break;
				p772::NoteCombatRequest(&State, 0, false);
				{
					FScopeLock Lock(&StatsMutex);
					++Stats.CombatCancelsRequested;
				}
				break;
			case FCombatIntent::EKind::AttackMode:
			{
				const p772::AttackMode Attack = ToCoreAttackMode(Intent.AttackMode);
				const p772::ChaseMode Chase = CurrentChaseMode(State);
				const p772::SecureMode Secure = CurrentSecureMode(State);
				Command = p772::BuildSetTacticsCommand(Attack, Chase, Secure);
				if (!Session.SendCommand(Command).ok()) break;
				p772::NoteTacticsRequest(&State, Attack, Chase, Secure);
				FScopeLock Lock(&StatsMutex);
				++Stats.TacticsRequested;
				break;
			}
			case FCombatIntent::EKind::ChaseMode:
			{
				const p772::AttackMode Attack = CurrentAttackMode(State);
				const p772::ChaseMode Chase = ToCoreChaseMode(Intent.ChaseMode);
				const p772::SecureMode Secure = CurrentSecureMode(State);
				Command = p772::BuildSetTacticsCommand(Attack, Chase, Secure);
				if (!Session.SendCommand(Command).ok()) break;
				p772::NoteTacticsRequest(&State, Attack, Chase, Secure);
				FScopeLock Lock(&StatsMutex);
				++Stats.TacticsRequested;
				break;
			}
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
				PublishTalkRefusal(Say.SayId, Built.error);
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

	/**
	 * A line ClientCore refused before it reached the wire.
	 *
	 * A refusal the player caused and can act on, so it is told to them rather
	 * than filed as a protocol diagnostic. Not a ServerMessage: Fusion32 never
	 * saw this command.
	 */
	void PublishTalkRefusal(uint32 ActionId, p772::TalkBuildError Error)
	{
		FReal33DEvent Refused;
		Refused.Kind = EReal33DEventKind::ClientNotice;
		Refused.Detail = FString::Printf(
			TEXT("Your message was not sent: %hs"),
			p772::TalkBuildErrorName(Error));
		Refused.Direction = kNoDirection;
		Publish(MoveTemp(Refused));

		UE_LOG(LogTemp, Warning, TEXT("REAL33D: talk %u refused before sending: %hs"),
			ActionId, p772::TalkBuildErrorName(Error));

		FScopeLock Lock(&StatsMutex);
		++Stats.SaysRefusedLocally;
	}

	void DrainChannels()
	{
		FChannelIntent Intent;
		while (ChannelIntents.Dequeue(Intent))
		{
			// Built by ClientCore in every case, including the two talk forms:
			// the channel number and the addressee are written by the same
			// builder that enforces what CTalk enforces, so a command cannot
			// leave here missing the field its mode requires.
			std::vector<std::uint8_t> Command;
			switch (Intent.Kind)
			{
			case FChannelIntent::EKind::List:
				Command = p772::BuildGetChannelsCommand();
				break;
			case FChannelIntent::EKind::Join:
				Command = p772::BuildJoinChannelCommand(Intent.Channel);
				break;
			case FChannelIntent::EKind::Leave:
				Command = p772::BuildLeaveChannelCommand(Intent.Channel);
				break;
			case FChannelIntent::EKind::OpenPrivate:
				Command = p772::BuildOpenPrivateChannelCommand(
					TCHAR_TO_UTF8(*Intent.Name));
				break;

			case FChannelIntent::EKind::ChannelTalk:
			case FChannelIntent::EKind::PrivateTalk:
			{
				const bool bPrivate = Intent.Kind == FChannelIntent::EKind::PrivateTalk;
				const auto Built = p772::BuildTalkCommand(
					static_cast<std::uint8_t>(bPrivate
						? p772::TalkMode::PrivateMessage
						: p772::TalkMode::ChannelCall),
					TCHAR_TO_UTF8(*Intent.Text),
					bPrivate ? std::string(TCHAR_TO_UTF8(*Intent.Name)) : std::string(),
					bPrivate ? std::uint16_t(0) : Intent.Channel);
				if (!Built.ok())
				{
					PublishTalkRefusal(Intent.ActionId, Built.error);
					continue;
				}
				Command = Built.payload;
				break;
			}
			}

			if (Command.empty() || !Session.SendCommand(Command).ok())
			{
				continue;
			}

			FScopeLock Lock(&StatsMutex);
			switch (Intent.Kind)
			{
			case FChannelIntent::EKind::List:
				++Stats.ChannelListsRequested; break;
			case FChannelIntent::EKind::Join:
				++Stats.ChannelJoinsRequested; break;
			case FChannelIntent::EKind::Leave:
				++Stats.ChannelLeavesRequested; break;
			case FChannelIntent::EKind::OpenPrivate:
				++Stats.PrivateChannelsRequested; break;
			case FChannelIntent::EKind::ChannelTalk:
				++Stats.ChannelSaysRequested; break;
			case FChannelIntent::EKind::PrivateTalk:
				++Stats.PrivateSaysRequested; break;
			}
		}
	}

	void DrainEdits()
	{
		FEditIntent Intent;
		while (EditIntents.Dequeue(Intent))
		{
			// Built by ClientCore in both shapes. Nothing is enforced here:
			// CEditText re-reads the object and re-checks WRITE and
			// WRITEONCE, and CEditList re-checks NAMEDOOR and TEXT for a
			// door, so refusing locally would be this client inventing a rule
			// Fusion32 applies itself.
			std::vector<std::uint8_t> Command;
			if (Intent.bIsText)
			{
				Command = p772::BuildEditTextCommand(
					Intent.Id, TCHAR_TO_UTF8(*Intent.Text));
			}
			else if (Intent.ListType != EReal33DEditListType::Unknown)
			{
				Command = p772::BuildEditListCommand(
					static_cast<p772::EditListType>(Intent.ListType),
					Intent.Id, TCHAR_TO_UTF8(*Intent.Text));
			}
			else
			{
				// A list whose type Fusion32 never named. CEditList logs
				// "Unbekannter Typ" and drops it, so sending one would be
				// traffic with no possible effect.
				UE_LOG(LogReal33D, Warning,
					TEXT("edit list %u not submitted: the server did not name its type"),
					Intent.Id);
				continue;
			}

			if (!Session.SendCommand(Command).ok())
			{
				// Told to the player, not dropped. `kGameClientFrameLimits`
				// caps a sent payload at 2048 bytes, so a text beyond about
				// 2041 characters cannot be encoded at all -- and the window
				// has already said the text was handed over. Staying silent
				// here would make that a false claim, which is the one thing
				// this milestone must not do. Observed live: four
				// submissions of 4515 to 7515 characters vanished with no
				// line in the log and no word to the player.
				//
				// Not a ServerMessage: Fusion32 never saw this command.
				FReal33DEvent Refused;
				Refused.Kind = EReal33DEventKind::ClientNotice;
				Refused.Detail = FString::Printf(
					TEXT("Your text was not sent: %d characters is more than "
						 "this client can transmit in one command."),
					Intent.Text.Len());
				Refused.Direction = kNoDirection;
				Publish(MoveTemp(Refused));

				UE_LOG(LogReal33D, Warning,
					TEXT("edit %u for id %u NOT sent: %d char(s) exceeds the frame limit"),
					Intent.ActionId, Intent.Id, Intent.Text.Len());
				continue;
			}

			// Named on the way out, because success is silent. Nothing else
			// in the log will say this went anywhere: operate.cc::EditText
			// writes the text and answers nothing, and only a refusal comes
			// back, as an ordinary server message.
			UE_LOG(LogReal33D, Log,
				TEXT("%s %u submitted to Fusion32 as %u, %d char(s); "
					 "acceptance is silent and only a refusal answers"),
				Intent.bIsText ? TEXT("edit text for object")
					: TEXT("edit list for id"),
				Intent.Id, Intent.ActionId, Intent.Text.Len());

			FScopeLock Lock(&StatsMutex);
			if (Intent.bIsText) { ++Stats.EditTextsSubmitted; }
			else                { ++Stats.EditListsSubmitted; }
		}
	}

	void DrainTrades()
	{
		FTradeIntent Intent;
		while (TradeIntents.Dequeue(Intent))
		{
			// Built by ClientCore in every case. Nothing is enforced here:
			// TCreature::Trade re-checks reach, distance, line of throw and
			// both sides' trade state itself and answers each refusal through
			// SendResult, so refusing locally would invent a rule Fusion32
			// already applies.
			std::vector<std::uint8_t> Command;
			switch (Intent.Kind)
			{
			case FTradeIntent::EKind::Offer:
				Command = p772::BuildTradeObjectCommand(ToEndpoint(Intent.Object),
					Intent.TypeId, Intent.StackIndex, Intent.PartnerCreatureId);
				break;
			case FTradeIntent::EKind::Inspect:
				Command = p772::BuildInspectTradeCommand(
					Intent.bPartnerOffer ? p772::TradeOfferSide::Partner
										 : p772::TradeOfferSide::Own,
					Intent.Index);
				break;
			case FTradeIntent::EKind::Accept:
				Command = p772::BuildAcceptTradeCommand();
				break;
			case FTradeIntent::EKind::Reject:
				Command = p772::BuildRejectTradeCommand();
				break;
			}

			if (Command.empty() || !Session.SendCommand(Command).ok())
			{
				UE_LOG(LogReal33D, Warning,
					TEXT("trade command %u was not sent"), Intent.ActionId);
				continue;
			}

			FScopeLock Lock(&StatsMutex);
			switch (Intent.Kind)
			{
			case FTradeIntent::EKind::Offer:   ++Stats.TradeOffersRequested; break;
			case FTradeIntent::EKind::Inspect: ++Stats.TradeInspectsRequested; break;
			case FTradeIntent::EKind::Accept:  ++Stats.TradeAcceptsRequested; break;
			case FTradeIntent::EKind::Reject:  ++Stats.TradeRejectsRequested; break;
			}
		}
	}

	void DrainOutfits()
	{
		FOutfitIntent Intent;
		while (OutfitIntents.Dequeue(Intent))
		{
			std::vector<std::uint8_t> Command;
			if (Intent.Kind == FOutfitIntent::EKind::Get)
			{
				Command = p772::BuildGetOutfitCommand();
			}
			else
			{
				// The one place this client refuses a command the server would
				// take, and the one place it is right to. CSetOutfit answers a
				// selection it drops with silence -- no error, no second
				// SV_CMD_OUTFIT, nothing -- so a refused command on the wire
				// would be indistinguishable here from an accepted one. The
				// tests are ClientCore's, in CSetOutfit's own order, against
				// the band the player was actually answering.
				const p772::OutfitColors Colors{ { Intent.Colors[0], Intent.Colors[1],
					Intent.Colors[2], Intent.Colors[3] } };
				const p772::OutfitSelectionError Refusal =
					p772::ValidateOutfitSelection(Intent.Band, Intent.OutfitId, Colors);
				if (Refusal != p772::OutfitSelectionError::None)
				{
					// The player caused it and can fix it, and the server never
					// saw it, so it is a notice and not a server message.
					FReal33DEvent Notice;
					Notice.Kind = EReal33DEventKind::ClientNotice;
					Notice.Direction = kNoDirection;
					Notice.Detail = FString::Printf(
						TEXT("Outfit %u was not sent: %s. Fusion32 allows %u..%u, "
							 "and each colour up to %u."),
						Intent.OutfitId,
						UTF8_TO_TCHAR(p772::OutfitSelectionErrorName(Refusal)),
						Intent.Band.first, Intent.Band.last,
						uint32(p772::kMaxOutfitColor));
					Publish(MoveTemp(Notice));

					UE_LOG(LogReal33D, Warning,
						TEXT("outfit selection %u refused locally: %s (band %u..%u)"),
						Intent.ActionId,
						UTF8_TO_TCHAR(p772::OutfitSelectionErrorName(Refusal)),
						Intent.Band.first, Intent.Band.last);

					FScopeLock Lock(&StatsMutex);
					++Stats.OutfitSelectionsRefused;
					continue;
				}
				Command = p772::BuildSetOutfitCommand(Intent.OutfitId, Colors);
			}

			if (Command.empty() || !Session.SendCommand(Command).ok())
			{
				UE_LOG(LogReal33D, Warning,
					TEXT("outfit command %u was not sent"), Intent.ActionId);
				continue;
			}

			if (Intent.Kind == FOutfitIntent::EKind::Set)
			{
				// Logged as sent, never as applied. Nothing will confirm it:
				// the CREATURE_OUTFIT_CHANGED broadcast is conditional on the
				// player already wearing their own outfit.
				UE_LOG(LogReal33D, Log,
					TEXT("outfit %u sent: id=%u colours=%u,%u,%u,%u (no reply is expected)"),
					Intent.ActionId, Intent.OutfitId, Intent.Colors[0],
					Intent.Colors[1], Intent.Colors[2], Intent.Colors[3]);
			}

			FScopeLock Lock(&StatsMutex);
			switch (Intent.Kind)
			{
			case FOutfitIntent::EKind::Get: ++Stats.OutfitDialogsRequested; break;
			case FOutfitIntent::EKind::Set: ++Stats.OutfitsSubmitted; break;
			}
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
	Real33D::MinimapPalette MinimapPalette;
	std::unordered_set<std::uint16_t> MultiUseTypes;
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
	TQueue<FCombatIntent, EQueueMode::Spsc> CombatIntents;
	TQueue<FChannelIntent, EQueueMode::Spsc> ChannelIntents;
	TQueue<FEditIntent, EQueueMode::Spsc> EditIntents;
	TQueue<FTradeIntent, EQueueMode::Spsc> TradeIntents;
	TQueue<FOutfitIntent, EQueueMode::Spsc> OutfitIntents;

	TQueue<FReal33DEvent, EQueueMode::Spsc> Events;
	TQueue<FIntent, EQueueMode::Spsc> Intents;

	mutable FCriticalSection StatsMutex;
	FReal33DStats Stats;
};

// ---------------------------------------------------------------- subsystem

const TCHAR* Real33DEditListTypeLabel(EReal33DEditListType Type)
{
	// The three names the 2D client gives these windows. Presentation only;
	// the byte each becomes is ClientCore's business.
	switch (Type)
	{
	case EReal33DEditListType::Guest:    return TEXT("Guest list");
	case EReal33DEditListType::Subowner: return TEXT("Subowner list");
	case EReal33DEditListType::Door:     return TEXT("Door list");
	default:                             return TEXT("Unnamed list");
	}
}

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
	delete Minimap;
	Minimap = nullptr;
}

void UReal33DBridge::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Transcript = new FReal33DChatTranscript();
	Minimap = new FReal33DMinimapStore();
}

void UReal33DBridge::Deinitialize()
{
	Disconnect();
	delete Transcript;
	Transcript = nullptr;
	delete Minimap;
	Minimap = nullptr;
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
	if (Minimap != nullptr)
	{
		SaveMinimap();
		const FString Identity = Config.Host + TEXT(":") + FString::FromInt(Config.LoginPort)
			+ TEXT(":772:") + Config.Account + TEXT(":") + Config.RuntimeDir;
		FTCHARToUTF8 Encoded(*Identity);
		uint8 Digest[FSHA1::DigestSize];
		FSHA1::HashBuffer(Encoded.Get(), Encoded.Length(), Digest);
		const FString Scope = BytesToHex(Digest, FSHA1::DigestSize);
		if (Scope != Minimap->Scope)
		{
			Minimap->Memory = p772::KnownMinimap{};
			Minimap->Scope = Scope;
			Minimap->Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Minimap"), Scope + TEXT(".r33map"));
			FString Text;
			if (IFileManager::Get().FileSize(*Minimap->Path) <= 32LL * 1024LL * 1024LL
				&& FFileHelper::LoadFileToString(Text, *Minimap->Path))
			{
				const bool bLoaded = Minimap->Memory.Load(TCHAR_TO_UTF8(*Text), TCHAR_TO_UTF8(*Scope));
				UE_LOG(LogReal33D, Log, TEXT("minimap cache load: %s cells=%llu"),
					bLoaded ? TEXT("accepted") : TEXT("rejected"),
					static_cast<unsigned long long>(Minimap->Memory.terrain().size()));
			}
		}
		Minimap->Memory.EndObservation();
		Minimap->State.bPlayerKnown = false;
		Minimap->State.KnownCells = static_cast<int32>(Minimap->Memory.terrain().size());
		Minimap->State.LiveCells = 0;
		++Minimap->State.Revision;
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
		TArray<FReal33DEvent> Remaining;
		DrainEvents(Remaining);
		delete Worker;
		Worker = nullptr;
	}
	if (Minimap != nullptr)
	{
		Minimap->Memory.EndObservation();
		Minimap->State.bPlayerKnown = false;
		Minimap->State.LiveCells = 0;
		++Minimap->State.Revision;
		SaveMinimap();
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
		//
		// Channels first: an open and the first line spoken in it can arrive in
		// the same drain, and the line has to find the tab already there.
		NoteChannel(Event);
		NoteEditor(Event);
		NoteTrade(Event);
		NoteOutfit(Event);
		NoteChat(Event);
		NoteMinimap(Event);
		OutEvents.Add(MoveTemp(Event));
	}
	if (Minimap != nullptr && FPlatformTime::Seconds() - Minimap->LastSave >= 30.0) SaveMinimap();
}

void UReal33DBridge::NoteMinimap(const FReal33DEvent& Event)
{
	if (Minimap == nullptr) return;
	if (Event.Kind == EReal33DEventKind::Disconnected || Event.Kind == EReal33DEventKind::Failed)
	{
		Minimap->Memory.EndObservation();
		Minimap->State.bPlayerKnown = false;
		Minimap->State.LiveCells = 0;
		++Minimap->State.Revision;
		SaveMinimap();
		return;
	}
	if (Event.Kind != EReal33DEventKind::MinimapObserved) return;
	std::vector<p772::MinimapCell> Cells;
	const auto Previous = Minimap->State;
	Cells.reserve(Event.MinimapCells.Num());
	for (const auto& Cell : Event.MinimapCells)
		Cells.push_back({{Cell.Position.X, Cell.Position.Y, Cell.Position.Z}, {Cell.GroundType, Cell.bStaticObstacle, Cell.ColorType}});
	Minimap->Memory.Observe(Cells);
	Minimap->State.Player = Event.Position;
	Minimap->State.bPlayerKnown = true;
	Minimap->State.KnownCells = static_cast<int32>(Minimap->Memory.terrain().size());
	Minimap->State.LiveCells = static_cast<int32>(Minimap->Memory.live_count());
	++Minimap->State.Revision;
	Minimap->bDirty = true;
	if (!Previous.bPlayerKnown || Previous.Player != Minimap->State.Player
		|| Previous.KnownCells != Minimap->State.KnownCells || Previous.LiveCells != Minimap->State.LiveCells)
	{
		UE_LOG(LogReal33D, Log, TEXT("minimap observation: player=%d,%d,%d known=%d live=%d viewport=18x14"),
			Event.Position.X, Event.Position.Y, Event.Position.Z,
			Minimap->State.KnownCells, Minimap->State.LiveCells);
	}
}

void UReal33DBridge::SaveMinimap()
{
	if (Minimap == nullptr || !Minimap->bDirty || Minimap->Path.IsEmpty()) return;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Minimap->Path), true);
	const FString Temp = Minimap->Path + TEXT(".tmp");
	const auto Text = Minimap->Memory.Save(TCHAR_TO_UTF8(*Minimap->Scope));
	if (FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(Text.c_str()), *Temp, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
		&& IFileManager::Get().Move(*Minimap->Path, *Temp, true, true))
	{
		Minimap->bDirty = false;
		Minimap->LastSave = FPlatformTime::Seconds();
		UE_LOG(LogReal33D, Log, TEXT("minimap cache saved: cells=%d"), Minimap->State.KnownCells);
	}
	else UE_LOG(LogReal33D, Warning, TEXT("minimap cache save failed"));
}

FReal33DMinimapState UReal33DBridge::GetMinimapState() const
{
	check(IsInGameThread());
	return Minimap != nullptr ? Minimap->State : FReal33DMinimapState{};
}

void UReal33DBridge::GetMinimapCells(int32 Floor, int32 MinX, int32 MinY, int32 MaxX, int32 MaxY,
	TArray<FReal33DMinimapCell>& OutCells) const
{
	check(IsInGameThread());
	OutCells.Reset();
	if (!Minimap || Floor < 0 || Floor > 15) return;
	const auto& Terrain = Minimap->Memory.terrain();
	for (auto It = Terrain.lower_bound({MinX, 0, Floor}); It != Terrain.end(); ++It)
	{
		const auto& P = It->first;
		if (P.z != Floor || P.x > MaxX) break;
		if (P.y < MinY || P.y > MaxY) continue;
		FReal33DMinimapCell Cell;
		Cell.Position = {P.x, P.y, P.z};
		Cell.GroundType = It->second.ground_type;
		Cell.ColorType = It->second.color_type;
		Cell.bStaticObstacle = It->second.static_obstacle;
		Cell.bLive = Minimap->Memory.IsLive(P);
		OutCells.Add(Cell);
	}
}

bool UReal33DBridge::FindMinimapPath(Real33D::FMapPosition Start, Real33D::FMapPosition Goal,
	const TArray<Real33D::FMapPosition>& ObservedBlockers,
	TArray<Real33D::FMapPosition>& OutPath) const
{
	check(IsInGameThread());
	OutPath.Reset();
	if (!Minimap || !Minimap->State.bPlayerKnown || Start != Minimap->State.Player) return false;
	std::set<p772::MapPosition> Blockers;
	for (const auto& P : ObservedBlockers) Blockers.insert({P.X,P.Y,P.Z});
	std::vector<p772::MapPosition> Path;
	if (!Minimap->Memory.FindNavigationPath({Start.X,Start.Y,Start.Z},{Goal.X,Goal.Y,Goal.Z},Blockers,Path)) return false;
	for (const auto& P : Path) OutPath.Add({P.x,P.y,P.z});
	return true;
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
	{
		// Every utterance, including one that will also be drawn above its
		// speaker. A console that omitted what was said nearby would be a
		// partial record of a conversation, and the nearby case is exactly the
		// one an operator uses to check the distant case is not lying.
		//
		// Which surface it belongs on is `ChatViewSet`'s decision, and it is
		// made so that no line is ever dropped: a channel this client has left
		// and an unnamed private line both land in Local Chat.
		std::uint32_t ViewId = Transcript->Views.LocalId();
		if (Event.bHasChannel)
		{
			ViewId = Transcript->Views.RouteChannel(
				static_cast<std::uint16_t>(Event.Channel));
		}
		else if (Event.bPrivateMessage)
		{
			ViewId = Transcript->Views.RoutePrivate(TCHAR_TO_UTF8(*Event.Speaker));
		}
		Transcript->LogFor(ViewId).AddSpeech(
			TCHAR_TO_UTF8(*Event.Speaker),
			TCHAR_TO_UTF8(*Event.TalkMode),
			TCHAR_TO_UTF8(*Event.Detail));
		++Transcript->Revision;
		break;
	}

	case EReal33DEventKind::ServerMessage:
		// Local Chat, which is where it has always gone. SV_CMD_MESSAGE
		// carries a mode and a text and names no conversation, so putting one
		// in a channel tab would mean guessing from its English prose which
		// tab it was about -- and "Message sent to X." is exactly the line
		// that would tempt it.
		Transcript->LogFor(Transcript->Views.LocalId()).AddServerMessage(
			TCHAR_TO_UTF8(*Event.TalkMode),
			TCHAR_TO_UTF8(*Event.Detail));
		++Transcript->Revision;
		break;

	case EReal33DEventKind::ClientNotice:
		Transcript->LogFor(Transcript->Views.LocalId()).AddClientNotice(
			TCHAR_TO_UTF8(*Event.Detail));
		++Transcript->Revision;
		break;

	case EReal33DEventKind::Disconnected:
	case EReal33DEventKind::Failed:
		// A transcript from a previous connection must not appear to belong to
		// the new one, and neither must a tab: a channel is joined per
		// connection, so every one of them is gone. The sequence and the view
		// ids keep counting, so two arrivals are never confusable in evidence
		// even across the gap.
		Transcript->Reset();
		break;

	// Diagnostic is deliberately absent. It is developer prose and has no
	// player-facing surface; routing it here is the defect this milestone
	// exists to close.
	default:
		break;
	}
}

void UReal33DBridge::NoteChannel(const FReal33DEvent& Event)
{
	if (Transcript == nullptr)
	{
		return;
	}

	switch (Event.Kind)
	{
	case EReal33DEventKind::ChannelList:
		// Replaced wholesale, not merged. SendChannels sends the list this
		// player may see at this moment, and merging an older one into it would
		// keep offering a channel their rights no longer reach.
		Transcript->ChannelList.assign(Event.Channels.begin(), Event.Channels.end());
		Transcript->bChannelListKnown = true;
		++Transcript->ChannelListRevision;
		++Transcript->Revision;
		// Named on the way in, for the same reason the undecoded opcodes are
		// named on the way out: "the list arrived" is a number nobody can act
		// on, and an operator reading this later has to be able to see which
		// channels Fusion32 offered this character without a screenshot.
		for (const FReal33DChannelEntry& Row : Event.Channels)
		{
			UE_LOG(LogReal33D, Log, TEXT("channel list: %s"),
				Row.bOwnChannelOffer
					? *FString::Printf(TEXT("(own-channel offer) \"%s\""), *Row.Name)
					: *FString::Printf(TEXT("%d \"%s\""), Row.Id, *Row.Name));
		}
		UE_LOG(LogReal33D, Log, TEXT("channel list: %d row(s) from Fusion32"),
			Event.Channels.Num());
		break;

	case EReal33DEventKind::ChannelOpened:
	{
		// The server said this player is in it, so now there is a tab. A join
		// this client sent is not what creates one.
		const std::uint32_t ViewId = Transcript->Views.OpenChannel(
			static_cast<std::uint16_t>(Event.Channel),
			TCHAR_TO_UTF8(*Event.ChannelName),
			Event.bOwnChannel);
		// Touched so the surface exists with an empty transcript rather than
		// appearing only once something is said in it.
		Transcript->LogFor(ViewId);
		++Transcript->Revision;
		UE_LOG(LogReal33D, Log, TEXT("channel %d (%s) opened by Fusion32%s"),
			Event.Channel, *Event.ChannelName,
			Event.bOwnChannel ? TEXT(" as this player's own") : TEXT(""));
		break;
	}

	case EReal33DEventKind::ChannelClosed:
	{
		const std::uint32_t ViewId = Transcript->Views.FindChannel(
			static_cast<std::uint16_t>(Event.Channel));
		if (ViewId != 0)
		{
			Transcript->Views.Close(ViewId);
			Transcript->Logs.erase(ViewId);
			++Transcript->Revision;
		}
		UE_LOG(LogReal33D, Log, TEXT("channel %d closed by Fusion32"), Event.Channel);
		break;
	}

	case EReal33DEventKind::PrivateChannelOpened:
	{
		const std::uint32_t ViewId = Transcript->Views.OpenPrivate(
			TCHAR_TO_UTF8(*Event.ChannelName));
		if (ViewId != 0)
		{
			Transcript->LogFor(ViewId);
			++Transcript->Revision;
		}
		break;
	}

	default:
		break;
	}
}

void UReal33DBridge::NoteEditor(const FReal33DEvent& Event)
{
	if (Transcript == nullptr)
	{
		return;
	}
	if (Event.Kind != EReal33DEventKind::EditTextOpened
		&& Event.Kind != EReal33DEventKind::EditListOpened)
	{
		return;
	}
	// Replaced wholesale. A second use while the first window is open is the
	// ordinary case, and the server has already moved on from the first.
	Transcript->Editor = Event.Editor;
	++Transcript->EditorRevision;
}

void UReal33DBridge::NoteTrade(const FReal33DEvent& Event)
{
	if (Transcript == nullptr)
	{
		return;
	}
	switch (Event.Kind)
	{
	case EReal33DEventKind::TradeOwnOffer:
		// The first offer of either side is what opens the window. An accept
		// already sent belongs to the previous shape of the trade: Fusion32
		// resends an offer when it changes, and a changed offer is a thing
		// the player must look at again before accepting.
		Transcript->Trade.bOpen = true;
		Transcript->Trade.Own = Event.TradeOffer;
		Transcript->Trade.bAcceptSent = false;
		++Transcript->TradeRevision;
		break;

	case EReal33DEventKind::TradePartnerOffer:
		Transcript->Trade.bOpen = true;
		Transcript->Trade.Partner = Event.TradeOffer;
		Transcript->Trade.bAcceptSent = false;
		++Transcript->TradeRevision;
		break;

	case EReal33DEventKind::TradeClosed:
		// Cleared wholesale. The command says the trade is over and nothing
		// else; whether anything changed hands is the inventory's business,
		// and the inventory is mirrored separately from Fusion32's own
		// SV_CMD_SET_INVENTORY and container commands.
		Transcript->Trade = FReal33DTrade();
		++Transcript->TradeRevision;
		break;

	case EReal33DEventKind::Disconnected:
	case EReal33DEventKind::Failed:
		Transcript->Trade = FReal33DTrade();
		++Transcript->TradeRevision;
		break;

	default:
		break;
	}
}

const FReal33DTrade& UReal33DBridge::GetTrade() const
{
	check(IsInGameThread());
	static const FReal33DTrade Closed;
	return Transcript != nullptr ? Transcript->Trade : Closed;
}

uint64 UReal33DBridge::GetTradeRevision() const
{
	return Transcript != nullptr ? Transcript->TradeRevision : 0;
}

uint32 UReal33DBridge::RequestTradeOffer(const FMoveSlot& Object, uint16 TypeId,
	uint8 StackIndex, uint32 PartnerCreatureId)
{
	if (Worker == nullptr || PartnerCreatureId == 0)
	{
		// CTradeObject reads a partner id and TCreature::Trade answers
		// PLAYERNOTONLINE for one that names nobody; zero never names anyone.
		return 0;
	}
	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FTradeIntent Intent;
	Intent.ActionId = ActionId;
	Intent.Kind = FReal33DWorker::FTradeIntent::EKind::Offer;
	Intent.Object = Object;
	Intent.TypeId = TypeId;
	Intent.StackIndex = StackIndex;
	Intent.PartnerCreatureId = PartnerCreatureId;
	Worker->PostTrade(Intent);
	return ActionId;
}

uint32 UReal33DBridge::RequestTradeInspect(bool bPartnerOffer, uint8 Index)
{
	if (Worker == nullptr)
	{
		return 0;
	}
	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FTradeIntent Intent;
	Intent.ActionId = ActionId;
	Intent.Kind = FReal33DWorker::FTradeIntent::EKind::Inspect;
	Intent.bPartnerOffer = bPartnerOffer;
	Intent.Index = Index;
	Worker->PostTrade(Intent);
	return ActionId;
}

uint32 UReal33DBridge::RequestTradeAccept()
{
	if (Worker == nullptr)
	{
		return 0;
	}
	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FTradeIntent Intent;
	Intent.ActionId = ActionId;
	Intent.Kind = FReal33DWorker::FTradeIntent::EKind::Accept;
	Worker->PostTrade(Intent);

	// Recorded as "this client asked", never as "the trade is done".
	// AcceptTrade returns silently while the other side has not accepted.
	if (Transcript != nullptr && Transcript->Trade.bOpen)
	{
		Transcript->Trade.bAcceptSent = true;
		++Transcript->TradeRevision;
	}
	return ActionId;
}

uint32 UReal33DBridge::RequestTradeReject()
{
	if (Worker == nullptr)
	{
		return 0;
	}
	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FTradeIntent Intent;
	Intent.ActionId = ActionId;
	Intent.Kind = FReal33DWorker::FTradeIntent::EKind::Reject;
	Worker->PostTrade(Intent);

	// Applied here, because TPlayer::RejectTrade sends the PARTNER a close
	// and a "Trade cancelled." message and sends the rejecting player
	// nothing at all. This client's own command is the only signal it gets.
	if (Transcript != nullptr)
	{
		Transcript->Trade = FReal33DTrade();
		++Transcript->TradeRevision;
	}
	UE_LOG(LogReal33D, Log,
		TEXT("trade reject %u sent; Fusion32 answers the partner, not us"), ActionId);
	return ActionId;
}

void UReal33DBridge::NoteOutfit(const FReal33DEvent& Event)
{
	if (Transcript == nullptr)
	{
		return;
	}
	switch (Event.Kind)
	{
	case EReal33DEventKind::OutfitDialogOpened:
		// Replaced wholesale. A second SV_CMD_OUTFIT is the server answering
		// again, and the band it carries is the only one that may be obeyed
		// from now on -- merging it with the previous one could leave an
		// outfit selectable that the newer answer withdrew.
		Transcript->OutfitDialog = Event.OutfitDialog;
		++Transcript->OutfitDialogRevision;
		break;

	case EReal33DEventKind::Disconnected:
	case EReal33DEventKind::Failed:
		Transcript->OutfitDialog = FReal33DOutfitDialog();
		++Transcript->OutfitDialogRevision;
		break;

	default:
		break;
	}
}

const FReal33DOutfitDialog& UReal33DBridge::GetOutfitDialog() const
{
	check(IsInGameThread());
	static const FReal33DOutfitDialog Closed;
	return Transcript != nullptr ? Transcript->OutfitDialog : Closed;
}

uint64 UReal33DBridge::GetOutfitDialogRevision() const
{
	return Transcript != nullptr ? Transcript->OutfitDialogRevision : 0;
}

bool UReal33DBridge::CloseOutfitDialog()
{
	check(IsInGameThread());
	if (Transcript == nullptr || !Transcript->OutfitDialog.bOpen)
	{
		return false;
	}
	// Local only. 7.72 has no command that cancels a chooser and Fusion32 keeps
	// no record that one was offered, so there is nothing to tell the server.
	Transcript->OutfitDialog = FReal33DOutfitDialog();
	++Transcript->OutfitDialogRevision;
	return true;
}

uint32 UReal33DBridge::RequestOutfitDialog()
{
	if (Worker == nullptr)
	{
		return 0;
	}
	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FOutfitIntent Intent;
	Intent.ActionId = ActionId;
	Intent.Kind = FReal33DWorker::FOutfitIntent::EKind::Get;
	Worker->PostOutfit(Intent);
	// NOTHING IS OPENED HERE. The answer may be SV_CMD_OUTFIT or it may be a
	// refusal message for a gamemaster outfit, and only the former is a chooser.
	UE_LOG(LogReal33D, Log,
		TEXT("outfit chooser %u requested; Fusion32 answers with a chooser or a message"),
		ActionId);
	return ActionId;
}

uint32 UReal33DBridge::RequestSetOutfit(uint16 OutfitId,
	const uint8 (&Colors)[FReal33DOutfit::ColorCount])
{
	check(IsInGameThread());
	if (Worker == nullptr)
	{
		return 0;
	}
	// A selection only means something against the chooser it answers. With no
	// chooser open there is no band, and validating against a guessed one would
	// be the invention this whole feature is built to avoid.
	const FReal33DOutfitDialog& Dialog = GetOutfitDialog();
	if (!Dialog.bOpen)
	{
		UE_LOG(LogReal33D, Warning,
			TEXT("outfit %u not sent: Fusion32 has not offered a chooser"), OutfitId);
		return 0;
	}

	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FOutfitIntent Intent;
	Intent.ActionId = ActionId;
	Intent.Kind = FReal33DWorker::FOutfitIntent::EKind::Set;
	Intent.OutfitId = OutfitId;
	for (int32 Index = 0; Index < FReal33DOutfit::ColorCount; ++Index)
	{
		Intent.Colors[Index] = Colors[Index];
	}
	// The band travels with the intent: it is the one the player was answering,
	// not whichever one is current by the time the worker drains this.
	Intent.Band.first = Dialog.First;
	Intent.Band.last = Dialog.Last;
	Worker->PostOutfit(Intent);
	return ActionId;
}

const FReal33DEditor& UReal33DBridge::GetEditor() const
{
	check(IsInGameThread());
	// A function-local default rather than a member, so the "nothing is open"
	// answer cannot be written to by accident.
	static const FReal33DEditor Closed;
	return Transcript != nullptr ? Transcript->Editor : Closed;
}

uint64 UReal33DBridge::GetEditorRevision() const
{
	return Transcript != nullptr ? Transcript->EditorRevision : 0;
}

bool UReal33DBridge::CloseEditor()
{
	check(IsInGameThread());
	if (Transcript == nullptr || !Transcript->Editor.bOpen)
	{
		return false;
	}
	Transcript->Editor = FReal33DEditor();
	++Transcript->EditorRevision;
	return true;
}

uint32 UReal33DBridge::RequestEditText(uint32 ObjectId, const FString& Text)
{
	if (Worker == nullptr || ObjectId == 0)
	{
		// CEditText answers NOTACCESSIBLE for an object that does not exist,
		// and zero is never a real object id.
		return 0;
	}
	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FEditIntent Intent;
	Intent.ActionId = ActionId;
	Intent.bIsText = true;
	Intent.Id = ObjectId;
	Intent.Text = Text;
	Worker->PostEdit(Intent);
	return ActionId;
}

uint32 UReal33DBridge::RequestEditList(EReal33DEditListType Type, uint32 Id,
	const FString& Text)
{
	if (Worker == nullptr || Type == EReal33DEditListType::Unknown)
	{
		return 0;
	}
	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FEditIntent Intent;
	Intent.ActionId = ActionId;
	Intent.bIsText = false;
	Intent.Id = Id;
	Intent.ListType = Type;
	Intent.Text = Text;
	Worker->PostEdit(Intent);
	return ActionId;
}

void UReal33DBridge::GetChannelList(TArray<FReal33DChannelEntry>& OutChannels) const
{
	check(IsInGameThread());
	OutChannels.Reset();
	if (Transcript == nullptr)
	{
		return;
	}
	OutChannels.Reserve(static_cast<int32>(Transcript->ChannelList.size()));
	for (const FReal33DChannelEntry& Each : Transcript->ChannelList)
	{
		OutChannels.Add(Each);
	}
}

bool UReal33DBridge::IsChannelListKnown() const
{
	return Transcript != nullptr && Transcript->bChannelListKnown;
}

uint64 UReal33DBridge::GetChannelListRevision() const
{
	return Transcript != nullptr ? Transcript->ChannelListRevision : 0;
}

bool UReal33DBridge::IsChannelJoined(int32 Channel) const
{
	return Transcript != nullptr && Channel >= 0 && Channel <= 0xFFFF
		&& Transcript->Views.FindChannel(static_cast<std::uint16_t>(Channel)) != 0;
}

void UReal33DBridge::GetChatViews(TArray<FReal33DChatView>& OutViews) const
{
	check(IsInGameThread());
	OutViews.Reset();
	if (Transcript == nullptr)
	{
		return;
	}
	const auto& Views = Transcript->Views.views();
	OutViews.Reserve(static_cast<int32>(Views.size()));
	for (const Real33D::ChatView& View : Views)
	{
		FReal33DChatView Out;
		Out.ViewId = View.id;
		Out.Kind = static_cast<EReal33DChatViewKind>(View.kind);
		Out.Channel = static_cast<int32>(View.channel);
		Out.Name = UTF8_TO_TCHAR(View.name.c_str());
		Out.bOwnChannel = View.own_channel;
		OutViews.Add(MoveTemp(Out));
	}
}

uint32 UReal33DBridge::GetSelectedChatView() const
{
	return Transcript != nullptr ? Transcript->Views.Selected() : 0;
}

bool UReal33DBridge::SelectChatView(uint32 ViewId)
{
	check(IsInGameThread());
	return Transcript != nullptr && Transcript->Views.Select(ViewId);
}

bool UReal33DBridge::CloseChatView(uint32 ViewId)
{
	check(IsInGameThread());
	if (Transcript == nullptr)
	{
		return false;
	}
	const Real33D::ChatView* View = Transcript->Views.Find(ViewId);
	if (View == nullptr)
	{
		return false;
	}
	// A channel tab closing means leaving the channel, which is what the 2D's
	// close button does. The command goes out first: if the session is already
	// gone the tab still closes, because a tab for a channel this client can no
	// longer be in is worse than none.
	if (View->kind == Real33D::ChatViewKind::Channel)
	{
		RequestLeaveChannel(static_cast<int32>(View->channel));
		// RequestLeaveChannel closed it already, having found it the same way.
		return Transcript->Views.Find(ViewId) == nullptr;
	}
	if (!Transcript->Views.Close(ViewId))
	{
		return false;
	}
	Transcript->Logs.erase(ViewId);
	++Transcript->Revision;
	return true;
}

void UReal33DBridge::GetChatTranscript(TArray<FReal33DChatLine>& OutLines) const
{
	// Local Chat, which is what this has always returned.
	GetChatTranscript(Transcript != nullptr ? Transcript->Views.LocalId() : 0, OutLines);
}

void UReal33DBridge::GetChatTranscript(uint32 ViewId,
	TArray<FReal33DChatLine>& OutLines) const
{
	check(IsInGameThread());
	OutLines.Reset();
	if (Transcript == nullptr)
	{
		return;
	}
	const p772::ChatLog* Log = Transcript->FindLog(ViewId);
	if (Log == nullptr)
	{
		// A surface that is not open has no transcript. Empty rather than
		// Local Chat's: showing one conversation's lines under another's name
		// is the one mistake a multi-tab console must not make.
		return;
	}
	const auto& Entries = Log->entries();
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

uint32 UReal33DBridge::RequestChannelList()
{
	if (Worker == nullptr)
	{
		return 0;
	}
	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FChannelIntent Intent;
	Intent.ActionId = ActionId;
	Intent.Kind = FReal33DWorker::FChannelIntent::EKind::List;
	Worker->PostChannel(Intent);
	return ActionId;
}

uint32 UReal33DBridge::RequestJoinChannel(int32 Channel)
{
	if (Worker == nullptr)
	{
		return 0;
	}
	if (Channel < 0 || Channel > 0xFFFF)
	{
		// Not a word, so there is no command to build. CL_CMD_JOIN_CHANNEL
		// carries exactly one.
		return 0;
	}
	if (Channel == kRuleViolationsChannel)
	{
		// Refused by this client, not by the server, and said so plainly. The
		// answer to joining it is SV_CMD_OPEN_REQUEST_QUEUE, which this project
		// does not decode; an undecoded command stops the payload walk, so the
		// cost of asking is the rest of that packet.
		FReal33DEvent Notice;
		Notice.Kind = EReal33DEventKind::ClientNotice;
		Notice.Detail = TEXT("This client cannot open the Rule Violations channel.");
		Notice.Direction = kNoDirection;
		if (Transcript != nullptr)
		{
			Transcript->LogFor(Transcript->Views.LocalId())
				.AddClientNotice(TCHAR_TO_UTF8(*Notice.Detail));
			++Transcript->Revision;
		}
		UE_LOG(LogReal33D, Warning,
			TEXT("join refused locally: channel %d answers with an undecoded command"),
			Channel);
		return 0;
	}
	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FChannelIntent Intent;
	Intent.ActionId = ActionId;
	Intent.Kind = FReal33DWorker::FChannelIntent::EKind::Join;
	Intent.Channel = static_cast<uint16>(Channel);
	Worker->PostChannel(Intent);
	return ActionId;
}

uint32 UReal33DBridge::RequestLeaveChannel(int32 Channel)
{
	if (Worker == nullptr || Channel < 0 || Channel > 0xFFFF)
	{
		return 0;
	}
	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FChannelIntent Intent;
	Intent.ActionId = ActionId;
	Intent.Kind = FReal33DWorker::FChannelIntent::EKind::Leave;
	Intent.Channel = static_cast<uint16>(Channel);
	Worker->PostChannel(Intent);

	// Logged here rather than at the button, because there are two ways to
	// leave -- the Channels panel's Leave and the tab's close box -- and both
	// arrive here. Logging it at one of the callers made the other invisible:
	// a live run showed two joins and one leave, and the log could not say
	// whether the second leave had reached the wire or been dropped.
	UE_LOG(LogReal33D, Log, TEXT("leave of channel %d handed to Fusion32 as %u"),
		Channel, ActionId);

	// Applied here, because CLeaveChannel answers with nothing at all. This is
	// the one channel effect not waited for, and the reason is in the header.
	if (Transcript != nullptr)
	{
		const std::uint32_t ViewId = Transcript->Views.FindChannel(
			static_cast<std::uint16_t>(Channel));
		if (ViewId != 0)
		{
			Transcript->Views.Close(ViewId);
			Transcript->Logs.erase(ViewId);
			++Transcript->Revision;
		}
	}
	return ActionId;
}

uint32 UReal33DBridge::RequestOpenPrivateChannel(const FString& Name)
{
	if (Worker == nullptr || Name.IsEmpty())
	{
		// CPrivateChannel returns an empty name unsent.
		return 0;
	}
	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FChannelIntent Intent;
	Intent.ActionId = ActionId;
	Intent.Kind = FReal33DWorker::FChannelIntent::EKind::OpenPrivate;
	Intent.Name = Name;
	Worker->PostChannel(Intent);
	return ActionId;
}

uint32 UReal33DBridge::RequestChannelTalk(int32 Channel, const FString& Text)
{
	if (Worker == nullptr || Channel < 0 || Channel > 0xFFFF)
	{
		return 0;
	}
	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FChannelIntent Intent;
	Intent.ActionId = ActionId;
	Intent.Kind = FReal33DWorker::FChannelIntent::EKind::ChannelTalk;
	Intent.Channel = static_cast<uint16>(Channel);
	Intent.Text = Text;
	Worker->PostChannel(Intent);
	return ActionId;
}

uint32 UReal33DBridge::RequestPrivateTalk(const FString& Addressee, const FString& Text)
{
	if (Worker == nullptr)
	{
		return 0;
	}
	const uint32 ActionId = ++NextInputId;
	FReal33DWorker::FChannelIntent Intent;
	Intent.ActionId = ActionId;
	Intent.Kind = FReal33DWorker::FChannelIntent::EKind::PrivateTalk;
	Intent.Name = Addressee;
	Intent.Text = Text;
	Worker->PostChannel(Intent);
	return ActionId;
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

uint32 UReal33DBridge::RequestLookAtPoint(const FMoveSlot& Point)
{
	if (Worker == nullptr) return 0;
	FReal33DWorker::FUse Look;
	Look.UseId = ++NextInputId;
	Look.Kind = FReal33DWorker::FUse::EKind::Look;
	Look.Object = Point;
	Worker->PostUse(Look);
	return Look.UseId;
}

uint32 UReal33DBridge::RequestAttack(uint32 CreatureId)
{
	if (Worker == nullptr || CreatureId == 0)
	{
		return 0;
	}
	FReal33DWorker::FCombatIntent Intent;
	Intent.ActionId = ++NextInputId;
	Intent.Kind = FReal33DWorker::FCombatIntent::EKind::Attack;
	Intent.CreatureId = CreatureId;
	Worker->PostCombat(Intent);
	return Intent.ActionId;
}

uint32 UReal33DBridge::RequestFollow(uint32 CreatureId)
{
	if (Worker == nullptr || CreatureId == 0)
	{
		return 0;
	}
	FReal33DWorker::FCombatIntent Intent;
	Intent.ActionId = ++NextInputId;
	Intent.Kind = FReal33DWorker::FCombatIntent::EKind::Follow;
	Intent.CreatureId = CreatureId;
	Worker->PostCombat(Intent);
	return Intent.ActionId;
}

uint32 UReal33DBridge::RequestCancelCombat()
{
	if (Worker == nullptr)
	{
		return 0;
	}
	FReal33DWorker::FCombatIntent Intent;
	Intent.ActionId = ++NextInputId;
	Intent.Kind = FReal33DWorker::FCombatIntent::EKind::Cancel;
	Worker->PostCombat(Intent);
	return Intent.ActionId;
}

uint32 UReal33DBridge::RequestAttackMode(EReal33DAttackMode Mode)
{
	if (Worker == nullptr)
	{
		return 0;
	}
	FReal33DWorker::FCombatIntent Intent;
	Intent.ActionId = ++NextInputId;
	Intent.Kind = FReal33DWorker::FCombatIntent::EKind::AttackMode;
	Intent.AttackMode = Mode;
	Worker->PostCombat(Intent);
	return Intent.ActionId;
}

uint32 UReal33DBridge::RequestChaseMode(EReal33DChaseMode Mode)
{
	if (Worker == nullptr)
	{
		return 0;
	}
	FReal33DWorker::FCombatIntent Intent;
	Intent.ActionId = ++NextInputId;
	Intent.Kind = FReal33DWorker::FCombatIntent::EKind::ChaseMode;
	Intent.ChaseMode = Mode;
	Worker->PostCombat(Intent);
	return Intent.ActionId;
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

bool UReal33DBridge::RequiresUseTarget(uint16 TypeId) const
{
	return Worker != nullptr && Worker->RequiresUseTarget(TypeId);
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
