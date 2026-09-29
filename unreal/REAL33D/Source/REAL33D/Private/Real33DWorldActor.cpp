#include "Real33DWorldActor.h"
#include "Real33DPresentationPolicy.h"
#include "Algo/Reverse.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformFileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "REAL33D.h"
#include "Real33DAssetRegistry.h"
#include "Real33DCreatureActor.h"
#include "Real33DTileActor.h"
#include "Real33DStaticSectorActor.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif

namespace
{
	/**
	 * Makes a string safe to put inside a JSON string literal.
	 *
	 * Chat text is typed by a player and arrives from other players, so it can
	 * contain a quote or a backslash. Writing it raw would produce an evidence
	 * file that does not parse, which is the one failure mode evidence must not
	 * have.
	 */
	FString EscapeForJson(const FString& Value)
	{
		FString Out;
		Out.Reserve(Value.Len() + 8);
		for (const TCHAR Glyph : Value)
		{
			switch (Glyph)
			{
			case TEXT('"'):  Out += TEXT("\\\""); break;
			case TEXT('\\'): Out += TEXT("\\\\"); break;
			case TEXT('\n'): Out += TEXT("\\n"); break;
			case TEXT('\r'): Out += TEXT("\\r"); break;
			case TEXT('\t'): Out += TEXT("\\t"); break;
			default:
				if (Glyph < 0x20)
				{
					Out += FString::Printf(TEXT("\\u%04x"), Glyph);
				}
				else
				{
					Out.AppendChar(Glyph);
				}
				break;
			}
		}
		return Out;
	}
}

AReal33DWorld::AReal33DWorld()
{
	PrimaryActorTick.bCanEverTick = true;
	// The scene must be built before the camera reads the local player's
	// position, otherwise the view lags one frame behind every step.
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	SetRootComponent(CameraRoot);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraRoot);
	// Looking down and to the north-west, far enough back that a viewport of
	// 18x14 fields is comfortably inside the frame.
	//
	// The three defaults below reproduce the fixed transform this camera carried
	// until the orbit was added: relative location (-620,-620,780) and rotation
	// (-42,45,0) are the same view expressed as yaw, pitch and distance, so an
	// operator who never touches the right mouse button sees what they saw
	// before.
	ApplyCameraTransform();

	// Turn the film tone curve off.
	//
	// Measured, not guessed: with the curve on, speech set to gold
	// (255,190,30) reached the screen as roughly (180,171,138), a near-grey
	// beige whose red and green are within nine of each other. Against the
	// green ground that reads as green, which is exactly what the operator
	// reported. The curve is doing what it is for, mapping HDR film-like into
	// display range, but this scene is flat-lit placeholder geometry with no
	// HDR range to preserve, so it only costs saturation.
	//
	// A readable colour matters more here than a filmic image, and the same
	// change stops the whole scene looking washed out.
	Camera->PostProcessSettings.bOverride_ToneCurveAmount = true;
	Camera->PostProcessSettings.ToneCurveAmount = 0.0f;
	Camera->PostProcessSettings.bOverride_ColorSaturation = true;
	Camera->PostProcessSettings.ColorSaturation = FVector4(1.15, 1.15, 1.15, 1.0);
}

void AReal33DWorld::BeginPlay()
{
	Super::BeginPlay();

	Registry = NewObject<UReal33DAssetRegistry>(this);
	Registry->Initialise();
	InitialiseWideWorld();

	// There is no pawn to possess, so the presenter is the view target itself.
	// Tick re-asserts this once, because the controller may not exist yet when
	// the game mode builds the scene.
	if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
	{
		Controller->SetViewTarget(this);
	}

	UGameInstance* GameInstance = GetGameInstance();
	UReal33DBridge* Bridge = GameInstance != nullptr
		? GameInstance->GetSubsystem<UReal33DBridge>() : nullptr;
	if (Bridge == nullptr)
	{
		UE_LOG(LogReal33D, Error, TEXT("no bridge subsystem; nothing will be presented"));
		return;
	}

	Config = UReal33DBridge::ConfigFromCommandLine();
	if (!Config.IsValid())
	{
		UE_LOG(LogReal33D, Error,
			TEXT("no runtime directory: pass -real33d-runtime=<dir> on the command line"));
		return;
	}

	UE_LOG(LogReal33D, Log, TEXT("connecting to %s:%d as account %s"),
		*Config.Host, Config.LoginPort, *Config.Account);
	Bridge->Connect(Config);
	FirstFrameTime = FPlatformTime::Seconds();

	// Overridable so a measured value can replace the unproven default without
	// a rebuild. See docs/UNREAL_CHAT.md for why it is unproven.
	float Seconds = 0.0f;
	if (FParse::Value(FCommandLine::Get(), TEXT("-real33d-speech-seconds="), Seconds)
		&& Seconds > 0.0f)
	{
		SpeechSeconds = Seconds;
	}
	UE_LOG(LogReal33D, Log,
		TEXT("speech display lifetime %.1fs (NOT a proven 7.72 value)"), SpeechSeconds);

	FString Directory;
	if (!FParse::Value(FCommandLine::Get(), TEXT("-real33d-evidence="), Directory)
		|| Directory.IsEmpty())
	{
		Directory = FPaths::ProjectSavedDir();
	}
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Directory);
	JournalPath = FPaths::Combine(Directory, TEXT("movement_journal.jsonl"));
	// Started fresh per session: a journal that mixed two runs would let a step
	// from one be read as the answer to a press from the other.
	FFileHelper::SaveStringToFile(FString(), *JournalPath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void AReal33DWorld::EndPlay(const EEndPlayReason::Type Reason)
{
	// The run must leave evidence even when it is closed from the window
	// button, which is the ordinary way a human ends the acceptance test.
	WriteEvidence(TEXT("EndPlay"));

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UReal33DBridge* Bridge = GameInstance->GetSubsystem<UReal33DBridge>())
		{
			Bridge->Disconnect();
		}
	}
	ClearWorld();
	Super::EndPlay(Reason);
}

AReal33DCreature* AReal33DWorld::GetLocalPlayer() const
{
	if (LocalCreatureId == 0)
	{
		return nullptr;
	}
	const TObjectPtr<AReal33DCreature>* Found = Creatures.Find(LocalCreatureId);
	return Found != nullptr ? Found->Get() : nullptr;
}

bool AReal33DWorld::IsKnownWalkTile(const FIntVector& Key) const
{
	const auto* Found = Tiles.Find(Key);
	const AReal33DTile* Tile = Found ? Found->Get() : nullptr;
	if (!Tile || Tile->GetThings().IsEmpty()) return false;
	bool bHasGround = false;
	for (const FReal33DThing& Thing : Tile->GetThings())
	{
		bHasGround |= Thing.bGround;
		if (Thing.bBlocking || (Thing.bIsCreature && Thing.CreatureId != LocalCreatureId)) return false;
	}
	return bHasGround; // A planning hint; only Fusion32 can accept the actual step.
}

bool AReal33DWorld::FindKnownWalkPath(const Real33D::FMapPosition& Target,
	TArray<FIntVector>& OutPath) const
{
	OutPath.Reset();
	const AReal33DCreature* Self = GetLocalPlayer();
	if (!Self || Target.Z != Self->GetLogicalPosition().Z) return false;
	const auto& Position = Self->GetLogicalPosition();
	const FIntVector Start(Position.X, Position.Y, Position.Z);
	const FIntVector Goal(Target.X, Target.Y, Target.Z);
	if (Goal == Start) return true;
	UGameInstance* Instance = GetGameInstance();
	const auto* Bridge = Instance ? Instance->GetSubsystem<UReal33DBridge>() : nullptr;
	if (!Bridge) return false;
	// Only current observed things override stale terrain estimates. WideWorld
	// Actors and minimap history never establish current mutable occupancy.
	TArray<Real33D::FMapPosition> Blockers;
	for (int32 X=Position.X-8; X<Position.X+10; ++X)
		for (int32 Y=Position.Y-6; Y<Position.Y+8; ++Y)
		{
			const FIntVector Key(X,Y,Position.Z);
			if (Key != Start && !IsKnownWalkTile(Key)) Blockers.Add({X,Y,Position.Z});
		}
	TArray<Real33D::FMapPosition> Path;
	if (!Bridge->FindMinimapPath(Position,Target,Blockers,Path)) return false;
	for (const auto& P : Path) OutPath.Emplace(P.X,P.Y,P.Z);
	return true;
}

void AReal33DWorld::GetBattleList(TArray<FReal33DBattleEntry>& OutEntries) const
{
	OutEntries.Reset();

	const AReal33DCreature* Self = GetLocalPlayer();
	const Real33D::FMapPosition Here = Self != nullptr
		? Self->GetLogicalPosition() : Real33D::FMapPosition{};

	for (const TPair<uint32, TObjectPtr<AReal33DCreature>>& Pair : Creatures)
	{
		const AReal33DCreature* Creature = Pair.Value.Get();
		if (Creature == nullptr)
		{
			continue;
		}
		FReal33DBattleEntry Entry;
		Entry.CreatureId = Creature->GetCreatureId();
		Entry.Name = Creature->GetCreatureName();
		Entry.HealthPercent = Creature->GetHealthPercent();
		Entry.bIsLocalPlayer = Creature->IsLocalPlayer();
		Entry.bAttacked = Combat.TargetCreatureId == Entry.CreatureId
			&& !Combat.bFollowing;
		Entry.bFollowed = Combat.TargetCreatureId == Entry.CreatureId
			&& Combat.bFollowing;

		// Chebyshev, because a Tibia field is reached diagonally in one step:
		// the number of steps away is what "nearest" has always meant here, not
		// the straight-line distance. Only meaningful on our own floor, so a
		// creature on another one is pushed to the end rather than compared.
		const Real33D::FMapPosition& There = Creature->GetLogicalPosition();
		Entry.Distance = Self == nullptr || There.Z != Here.Z
			? MAX_int32
			: FMath::Max(FMath::Abs(There.X - Here.X), FMath::Abs(There.Y - Here.Y));
		OutEntries.Add(MoveTemp(Entry));
	}

	// Nearest first, and the local player never listed among their own targets.
	OutEntries.RemoveAll([](const FReal33DBattleEntry& Entry)
		{ return Entry.bIsLocalPlayer; });
	OutEntries.Sort([](const FReal33DBattleEntry& A, const FReal33DBattleEntry& B)
	{
		// Ties broken by id, not left to the map's iteration order: two
		// creatures the same distance away must not swap places every frame.
		return A.Distance != B.Distance
			? A.Distance < B.Distance : A.CreatureId < B.CreatureId;
	});
}

void AReal33DWorld::ClearWorld()
{
	for (TPair<FIntVector, TObjectPtr<AReal33DTile>>& Pair : Tiles)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}
	Tiles.Reset();

	for (TPair<FIntVector, TObjectPtr<AReal33DStaticSector>>& Pair : StaticSectors)
	{
		if (Pair.Value) Pair.Value->Destroy();
	}
	StaticSectors.Reset();
	bWideWorldAnchorSet = false;

	for (TPair<uint32, TObjectPtr<AReal33DCreature>>& Pair : Creatures)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}
	Creatures.Reset();
	LocalCreatureId = 0;
	PlayerVitals = FReal33DPlayerVitals{};
	// Cleared with the vitals and for the same reason: a reconnect must show
	// "--" until the new session's own SV_CMD_PLAYER_SKILLS and
	// SV_CMD_PLAYER_STATE arrive, not the previous character's numbers.
	PlayerSkills = FReal33DPlayerSkills{};
	PlayerConditions = FReal33DConditions{};
	// Equipment and containers go too. A reconnect re-receives all of it from
	// SendBodyInventory and whatever containers the character has open, so
	// keeping the old character's belongings would be showing items this
	// session was never told about.
	PlayerInventory = FReal33DInventory{};
	OpenContainers.Reset();
	Combat = FReal33DCombat{};
	bFloorVisibilityDirty = true;
	// Speech attached to a creature died with its actor above. The transcript
	// is the other half and the bridge clears it on the same disconnect, so a
	// reconnect inherits nothing the previous session was saying.
}

void AReal33DWorld::HandleEvent(const FReal33DEvent& Event)
{
	using namespace Real33D;

	switch (Event.Kind)
	{
	case EReal33DEventKind::Connected:
		bConnected = true;
		DisconnectedAt = 0.0;
		UE_LOG(LogReal33D, Log, TEXT("connected: %s"), *Event.Detail);
		break;

	case EReal33DEventKind::Disconnected:
	case EReal33DEventKind::Failed:
		bConnected = false;
		UE_LOG(LogReal33D, Warning, TEXT("session ended: %s"), *Event.Detail);
		// Snapshot first: the acceptance criterion about ghosts is about what
		// the scene held at the moment the session ended, and about what it
		// holds afterwards. Clearing first would erase the first half.
		WriteEvidence(Event.Kind == EReal33DEventKind::Failed
			? TEXT("Failed") : TEXT("Disconnected"));
		// Every actor in the scene exists because WorldState said so. With no
		// session there is no WorldState, so nothing may survive: this is what
		// keeps a reconnect from finding ghosts of the previous one.
		ClearWorld();
		Origin = FWorldOrigin();
		WriteEvidence(TEXT("AfterCleanup"));
		DisconnectedAt = FPlatformTime::Seconds();
		break;

	// The outgoing chain. None of these touch an Actor: the walk that matters
	// arrives as an ordinary CreatureMoved for the local player, and these say
	// what caused it.
	case EReal33DEventKind::WalkSent:
		JournalMovement(Event);
		UE_LOG(LogReal33D, Log, TEXT("input %u: walk command %u sent to Fusion32"),
			Event.InputId, Event.RequestId);
		break;

	case EReal33DEventKind::WalkAccepted:
		JournalMovement(Event);
		UE_LOG(LogReal33D, Log,
			TEXT("input %u: Fusion32 accepted request %u, %d,%d,%d -> %d,%d,%d"),
			Event.InputId, Event.RequestId,
			Event.PreviousPosition.X, Event.PreviousPosition.Y, Event.PreviousPosition.Z,
			Event.Position.X, Event.Position.Y, Event.Position.Z);
		break;

	case EReal33DEventKind::WalkRejected:
		JournalMovement(Event);
		UE_LOG(LogReal33D, Warning,
			TEXT("input %u: Fusion32 refused request %u; position unchanged"),
			Event.InputId, Event.RequestId);
		break;

	case EReal33DEventKind::ExternalRelocation:
		JournalMovement(Event);
		UE_LOG(LogReal33D, Log,
			TEXT("Fusion32 relocated us with no request outstanding, %d,%d,%d -> %d,%d,%d"),
			Event.PreviousPosition.X, Event.PreviousPosition.Y, Event.PreviousPosition.Z,
			Event.Position.X, Event.Position.Y, Event.Position.Z);
		break;

	case EReal33DEventKind::WalkUnanswered:
		JournalMovement(Event);
		UE_LOG(LogReal33D, Warning, TEXT("a walk request expired unanswered"));
		break;

	case EReal33DEventKind::PlayerVitals:
		PlayerVitals = Event.Vitals;
		break;

	case EReal33DEventKind::PlayerSkills:
		PlayerSkills = Event.Skills;
		break;

	case EReal33DEventKind::PlayerConditions:
		PlayerConditions = Event.Conditions;
		break;

	case EReal33DEventKind::InventoryChanged:
		PlayerInventory = Event.Inventory;
		break;

	case EReal33DEventKind::ContainerChanged:
	{
		// The list holds only what is open, so a close removes the entry and
		// the panel for it goes with it. Kept in container-number order so the
		// windows do not reshuffle when an unrelated container changes.
		const int32 Existing = OpenContainers.IndexOfByPredicate(
			[&Event](const FReal33DContainer& Each)
			{ return Each.Number == Event.Container.Number; });
		if (!Event.Container.bOpen)
		{
			if (Existing != INDEX_NONE)
			{
				OpenContainers.RemoveAt(Existing);
			}
			break;
		}
		if (Existing != INDEX_NONE)
		{
			OpenContainers[Existing] = Event.Container;
			break;
		}
		OpenContainers.Add(Event.Container);
		OpenContainers.Sort([](const FReal33DContainer& A, const FReal33DContainer& B)
			{ return A.Number < B.Number; });
		break;
	}

	case EReal33DEventKind::CombatChanged:
		Combat = Event.Combat;
		RefreshCombatFeedback();
		UE_LOG(LogReal33D, Log,
			TEXT("combat state: target=%u action=%s tactics_sent=%s attack_mode=%d chase_mode=%d"),
			Combat.TargetCreatureId,
			Combat.TargetCreatureId == 0 ? TEXT("none")
				: (Combat.bFollowing ? TEXT("follow") : TEXT("attack")),
			Combat.bTacticsSent ? TEXT("true") : TEXT("false"),
			static_cast<int32>(Combat.AttackMode),
			static_cast<int32>(Combat.ChaseMode));
		break;

	case EReal33DEventKind::CreatureHealth:
		if (TObjectPtr<AReal33DCreature>* Hurt = Creatures.Find(Event.CreatureId))
		{
			if (Hurt->Get())
			{
				(*Hurt)->SetHealthPercent(Event.HealthPercent);
			}
		}
		break;

	case EReal33DEventKind::Talk:
		PresentSpeech(Event);
		if (Event.bHasChannel)
		{
			UE_LOG(LogReal33D, Log, TEXT("talk [%s] channel %d, %s: \"%s\""),
				*Event.TalkMode, Event.Channel,
				Event.Speaker.IsEmpty() ? TEXT("(anonymous)") : *Event.Speaker,
				*Event.Detail);
		}
		else if (Event.TalkLayout == EReal33DTalkLayout::Positional)
		{
			UE_LOG(LogReal33D, Log,
				TEXT("talk [%s] at %d,%d,%d, %s: \"%s\"  speaker %s creature %u"),
				*Event.TalkMode, Event.Position.X, Event.Position.Y, Event.Position.Z,
				Event.Speaker.IsEmpty() ? TEXT("(unnamed)") : *Event.Speaker,
				*Event.Detail, *Event.SpeakerResolution, Event.CreatureId);
		}
		else
		{
			UE_LOG(LogReal33D, Log, TEXT("talk [%s] %s: \"%s\""),
				*Event.TalkMode,
				Event.Speaker.IsEmpty() ? TEXT("(anonymous)") : *Event.Speaker,
				*Event.Detail);
		}
		break;

	case EReal33DEventKind::Diagnostic:
		// Prose from the client core about something it could not consume.
		// Kept so the evidence can say which command, not just how many. It
		// reaches no player-facing surface: the chat area never sees this.
		LastDiagnostic = Event.Detail;
		UE_LOG(LogReal33D, Warning, TEXT("client core: %s"), *Event.Detail);
		break;

	case EReal33DEventKind::ServerMessage:
		// Fusion32 talking to this player. Already filed in the transcript by
		// the bridge, so there is nothing to draw here; logged because a
		// refusal the player was shown should also be readable in evidence.
		++ServerMessagesReceived;
		UE_LOG(LogReal33D, Log, TEXT("server message [%s]: %s"),
			*Event.TalkMode, *Event.Detail);
		break;

	case EReal33DEventKind::ClientNotice:
		// This client refusing to send something, already in the transcript.
		// Counted apart from the server's own messages so evidence cannot
		// present a local refusal as something Fusion32 said.
		++ClientNoticesRaised;
		UE_LOG(LogReal33D, Warning, TEXT("client notice: %s"), *Event.Detail);
		break;

	case EReal33DEventKind::LocalPlayerIdentified:
		LocalCreatureId = Event.CreatureId;
		bFloorVisibilityDirty = true;
		UE_LOG(LogReal33D, Log, TEXT("local player is creature %u"), Event.CreatureId);
		break;

	case EReal33DEventKind::AnchorMoved:
		// The first anchor of the session fixes the origin, and nothing moves
		// it afterwards: re-centring would shift every actor in the scene on
		// each step. See Real33DCoords.h for why an origin exists at all.
		Origin.EnsureSet(Event.Position);
		WideWorldAnchor = Event.Position;
		bWideWorldAnchorSet = true;
		UpdateWideWorld();
		break;

	case EReal33DEventKind::TileUpserted:
	{
		Origin.EnsureSet(Event.Position);
		const FIntVector Key = ToKey(Event.Position);
		TObjectPtr<AReal33DTile>* Existing = Tiles.Find(Key);
		AReal33DTile* Tile = Existing != nullptr ? Existing->Get() : nullptr;
		if (Tile == nullptr)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Tile = GetWorld()->SpawnActor<AReal33DTile>(
				AReal33DTile::StaticClass(), ToWorld(Origin, Event.Position),
				FRotator::ZeroRotator, Params);
			if (Tile == nullptr)
			{
				++OrphanEvents;
				break;
			}
			Tile->SetMapPosition(Event.Position);
			Tiles.Add(Key, Tile);
			++TilesSpawned;
		}
		Tile->ApplyStack(Event.Things, Registry);
		bFloorVisibilityDirty = true;
		break;
	}

	case EReal33DEventKind::TileRemoved:
	{
		TObjectPtr<AReal33DTile> Removed;
		if (Tiles.RemoveAndCopyValue(ToKey(Event.Position), Removed))
		{
			if (Removed)
			{
				Removed->Destroy();
			}
			++TilesRemoved;
		}
		bFloorVisibilityDirty = true;
		break;
	}

	case EReal33DEventKind::CreatureAppeared:
	{
		Origin.EnsureSet(Event.Position);
		if (Creatures.Contains(Event.CreatureId))
		{
			// WorldView never announces a creature twice. If this ever fires,
			// the diff and the scene have diverged and the run is not clean.
			++DuplicateSpawnAttempts;
			UE_LOG(LogReal33D, Error, TEXT("creature %u announced twice"),
				Event.CreatureId);
			break;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AReal33DCreature* Creature = GetWorld()->SpawnActor<AReal33DCreature>(
			AReal33DCreature::StaticClass(), ToWorld(Origin, Event.Position),
			FRotator::ZeroRotator, Params);
		if (Creature == nullptr)
		{
			++OrphanEvents;
			break;
		}
		Creature->SetHealthPercent(Event.HealthPercent);
		Creature->Configure(Event.CreatureId, Event.bIsLocalPlayer, Event.CreatureName,
			Registry);
		Creature->ApplyOutfit(Event.OutfitId, Event.bDisguisedAsObject, Registry);
		Creature->SetCombatFeedback(
			Combat.TargetCreatureId == Event.CreatureId && !Combat.bFollowing,
			Combat.TargetCreatureId == Event.CreatureId && Combat.bFollowing);
		UE_LOG(LogReal33D, Log, TEXT("creature %u \"%s\" appeared at %d,%d,%d health %u%%"),
			Event.CreatureId, *Event.CreatureName,
			Event.Position.X, Event.Position.Y, Event.Position.Z, Event.HealthPercent);
		// An appearance is not a walk. Place it, do not slide it in.
		Creature->CommitPosition(Origin, Event.Position, /*bSnap=*/true);
		Creature->SetFacing(Event.Direction);
		Creatures.Add(Event.CreatureId, Creature);
		++CreaturesAppeared;
		bFloorVisibilityDirty = true;
		break;
	}

	case EReal33DEventKind::CreatureMoved:
	{
		TObjectPtr<AReal33DCreature>* Found = Creatures.Find(Event.CreatureId);
		if (Found == nullptr || !Found->Get())
		{
			// A move for a creature the scene never spawned. Count it rather
			// than inventing an actor: an invented actor would be a body at a
			// position no one confirmed.
			++OrphanEvents;
			UE_LOG(LogReal33D, Warning, TEXT("move for unknown creature %u"),
				Event.CreatureId);
			break;
		}
		(*Found)->SetFacing(Event.Direction);
		(*Found)->CommitPosition(Origin, Event.Position, /*bSnap=*/false);
		++CreatureMoves;
		bFloorVisibilityDirty = true;
		break;
	}

	case EReal33DEventKind::CreatureAppearance:
	{
		if (auto* Found = Creatures.Find(Event.CreatureId); Found && Found->Get())
			(*Found)->ApplyOutfit(Event.OutfitId, Event.bDisguisedAsObject, Registry);
		break;
	}

	case EReal33DEventKind::CreatureVanished:
	{
		TObjectPtr<AReal33DCreature> Removed;
		if (Creatures.RemoveAndCopyValue(Event.CreatureId, Removed))
		{
			if (Removed)
			{
				Removed->Destroy();
			}
			++CreaturesVanished;
		}
		bFloorVisibilityDirty = true;
		break;
	}
	}
}

void AReal33DWorld::RefreshCombatFeedback()
{
	check(IsInGameThread());
	for (TPair<uint32, TObjectPtr<AReal33DCreature>>& Pair : Creatures)
	{
		if (!Pair.Value)
		{
			continue;
		}
		Pair.Value->SetCombatFeedback(
			Combat.TargetCreatureId == Pair.Key && !Combat.bFollowing,
			Combat.TargetCreatureId == Pair.Key && Combat.bFollowing);
	}
}

void AReal33DWorld::UpdateFloorVisibility()
{
	if (!bFloorVisibilityDirty) return;
	const AReal33DCreature* Local = GetLocalPlayer();
	if (Local == nullptr) return;
	const Real33D::FMapPosition& Position = Local->GetLogicalPosition();
	const bool bCovered = Position.Z > 0
		&& Tiles.Contains(FIntVector(Position.X, Position.Y, Position.Z - 1))
		&& Tiles[FIntVector(Position.X, Position.Y, Position.Z - 1)]
		&& Tiles[FIntVector(Position.X, Position.Y, Position.Z - 1)]->HasCoveringContent();
	bHideUpperFloors = Real33D::Presentation::HideUpperFloors(Position.Z, bCovered);
	int32 HiddenTiles = 0;
	for (TPair<FIntVector, TObjectPtr<AReal33DTile>>& Pair : Tiles)
	{
		if (Pair.Value)
		{
			const bool bHide = bHideUpperFloors && Pair.Key.Z < Position.Z;
			Pair.Value->SetFloorVisible(!bHide);
			HiddenTiles += bHide ? 1 : 0;
		}
	}
	for (TPair<uint32, TObjectPtr<AReal33DCreature>>& Pair : Creatures)
	{
		if (Pair.Value)
		{
			Pair.Value->SetActorHiddenInGame(
				bHideUpperFloors && Pair.Value->GetLogicalPosition().Z < Position.Z);
		}
	}
	for (auto& Pair : StaticSectors)
		if (Pair.Value) Pair.Value->ApplyView(WideWorldAnchor, WideWorldVisualRadius, bHideUpperFloors);
	bFloorVisibilityDirty = false;
	UE_LOG(LogReal33D, Log, TEXT("floor visibility: player floor %d covered=%s hidden upper tiles=%d"),
		Position.Z, bCovered ? TEXT("true") : TEXT("false"), HiddenTiles);
}

uint8 AReal33DWorld::CameraRelativeDirection(uint8 RelativeDirection) const
{
	const FVector Forward = FRotator(0.0f, CameraYaw, 0.0f).Vector();
	const uint8 Cardinal = FMath::Abs(Forward.X) >= FMath::Abs(Forward.Y)
		? (Forward.X >= 0.0 ? 1 : 3)
		: (Forward.Y >= 0.0 ? 2 : 0);
	return static_cast<uint8>((Cardinal + RelativeDirection) % 4);
}

void AReal33DWorld::UpdateCamera(float DeltaSeconds)
{
	const AReal33DCreature* Local = GetLocalPlayer();
	if (Local == nullptr)
	{
		return;
	}
	// The camera follows where the body is drawn, not where it logically is, so
	// the view and the player move together instead of the map snapping ahead.
	const FVector Desired = Local->GetActorLocation() + FVector(0, 0, 55);
 const bool bSnap = CameraFloor != Local->GetLogicalPosition().Z
  || FVector::DistSquared(GetActorLocation(), Desired) > FMath::Square(500.0);
 CameraFloor = Local->GetLogicalPosition().Z;
 SetActorLocation(bSnap ? Desired : FMath::VInterpTo(GetActorLocation(), Desired, DeltaSeconds, 8.0));
 const float Smoothed = FMath::FInterpTo(DrawnCameraDistance, CameraDistance, DeltaSeconds, 10.0f);
 const FVector Eye = GetActorLocation() - FRotator(CameraPitch, CameraYaw, 0).Vector() * Smoothed;
 double SafeDistance = Smoothed;
 CameraCutaways = 0;
 for (auto& Pair : Tiles)
  if (Pair.Value) CameraCutaways += Pair.Value->ApplyCameraVisibility(GetActorLocation(), Eye, SafeDistance);
 for (auto& Pair : StaticSectors)
  if (Pair.Value) CameraCutaways += Pair.Value->ApplyCameraVisibility(GetActorLocation(), Eye, SafeDistance);
 bCameraObstructed = SafeDistance < Smoothed - 1.0;
 // Retract immediately for clearance; ease restoration and ordinary wheel zoom.
 DrawnCameraDistance = FMath::Min(Smoothed, static_cast<float>(SafeDistance));
 ApplyCameraTransform();
}

void AReal33DWorld::ApplyCameraTransform()
{
	if (Camera == nullptr)
	{
		return;
	}

	// The actor sits on the player, so orbiting is entirely a matter of where
	// the camera is placed relative to it. Put the camera one Distance back
	// along the direction it looks, and the point it looks at is the actor's
	// origin by construction, which is what keeps the player centred no matter
	// how far the view is swung around.
	const FRotator Look(CameraPitch, CameraYaw, 0.0f);
	Camera->SetRelativeLocation(-Look.Vector() * DrawnCameraDistance);
	Camera->SetRelativeRotation(Look);
}

void AReal33DWorld::AddCameraOrbit(float DeltaYawDegrees, float DeltaPitchDegrees)
{
	CameraYaw = FRotator::ClampAxis(CameraYaw + DeltaYawDegrees);

	// Keep a useful downward view; projected labels do not need a horizon angle.
	CameraPitch = FMath::Clamp(CameraPitch + DeltaPitchDegrees,
		kCameraPitchMin, kCameraPitchMax);

}

void AReal33DWorld::AddCameraDistance(float Delta)
{
	CameraDistance = FMath::Clamp(CameraDistance + Delta,
		kCameraDistanceMin, kCameraDistanceMax);
}

void AReal33DWorld::DrawOverlay()
{
	// Diagnostic counters belong in F9 evidence; the normal view stays clear.
	if (!FParse::Param(FCommandLine::Get(), TEXT("real33d-diagnostics"))) return;
	if (GEngine == nullptr)
	{
		return;
	}
	UGameInstance* GameInstance = GetGameInstance();
	UReal33DBridge* Bridge = GameInstance != nullptr
		? GameInstance->GetSubsystem<UReal33DBridge>() : nullptr;
	if (Bridge == nullptr)
	{
		return;
	}
	const FReal33DStats Stats = Bridge->GetStats();
	const AReal33DCreature* Local = GetLocalPlayer();

	const FColor Clean = (Stats.ResidualBytes == 0 && Stats.UnsupportedOpcodes == 0
		&& Stats.Anomalies == 0 && DuplicateSpawnAttempts == 0 && OrphanEvents == 0)
		? FColor::Green : FColor::Red;

	GEngine->AddOnScreenDebugMessage(1, 0.0f, bConnected ? FColor::Green : FColor::Red,
		FString::Printf(TEXT("REAL33D  %s   frames %d  commands %d"),
			bConnected ? TEXT("in world") : TEXT("offline"), Stats.Frames, Stats.Commands));
	GEngine->AddOnScreenDebugMessage(2, 0.0f, FColor::White,
		FString::Printf(TEXT("player %u at %d,%d,%d   anchor %d,%d,%d"),
			LocalCreatureId,
			Local != nullptr ? Local->GetLogicalPosition().X : 0,
			Local != nullptr ? Local->GetLogicalPosition().Y : 0,
			Local != nullptr ? Local->GetLogicalPosition().Z : 0,
			Stats.Anchor.X, Stats.Anchor.Y, Stats.Anchor.Z));
	GEngine->AddOnScreenDebugMessage(3, 0.0f, FColor::White,
		FString::Printf(TEXT("tiles %d actors  creatures %d actors   worldstate %d / %d"),
			Tiles.Num(), Creatures.Num(), Stats.Tiles, Stats.VisibleCreatures));
	GEngine->AddOnScreenDebugMessage(4, 0.0f, FColor::White,
		FString::Printf(
			TEXT("walks asked %d  accepted %d  refused %d  unanswered %d   pushed %d   viewport %s"),
			Stats.RequestedSteps, Stats.AcceptedSelfWalks, Stats.RejectedSteps,
			Stats.UnansweredSteps, Stats.ExternalRelocations,
			Stats.bViewportSynchronised ? TEXT("in sync") : TEXT("waiting")));
	// Speech is deliberately absent from this overlay. It used to be drawn here
	// with AddOnScreenDebugMessage, stacked under the counters, which is where
	// the operator could not read it: a diagnostics overlay is not where a
	// player reads chat. It goes to SReal33DChatPanel now, and these two
	// surfaces stay apart.
	GEngine->AddOnScreenDebugMessage(5, 0.0f, Clean,
		FString::Printf(
			TEXT("residual %d  unsupported %d  anomalies %d  dup %d  orphan %d"),
			Stats.ResidualBytes, Stats.UnsupportedOpcodes, Stats.Anomalies,
			DuplicateSpawnAttempts, OrphanEvents));
}


void AReal33DWorld::InitialiseWideWorld()
{
	WideWorldCacheDirectory = FPaths::Combine(
		FPaths::ProjectSavedDir(), TEXT("WideWorldCache"));
	FParse::Value(FCommandLine::Get(),
		TEXT("-real33d-wide-world-cache="), WideWorldCacheDirectory);
	WideWorldCacheDirectory = FPaths::ConvertRelativePathToFull(WideWorldCacheDirectory);
	FPaths::CollapseRelativeDirectories(WideWorldCacheDirectory);

	WideWorldPreviewDirectory = FPaths::Combine(
		FPaths::ProjectDir(), TEXT("../../visual/reference_pack/previews/item"));
	FParse::Value(FCommandLine::Get(),
		TEXT("-real33d-wide-world-previews="), WideWorldPreviewDirectory);
	WideWorldPreviewDirectory = FPaths::ConvertRelativePathToFull(WideWorldPreviewDirectory);
	FPaths::CollapseRelativeDirectories(WideWorldPreviewDirectory);

	int32 Radius = WideWorldVisualRadius;
	if (FParse::Value(FCommandLine::Get(), TEXT("-real33d-visual-radius="), Radius))
	{
		WideWorldVisualRadius = FMath::Clamp(Radius, 32, 512);
	}
	bWideWorldEnabled = !FParse::Param(FCommandLine::Get(), TEXT("real33d-no-wide-world"))
		&& FPaths::DirectoryExists(WideWorldCacheDirectory);
	if (bWideWorldEnabled && Registry != nullptr)
	{
		Registry->EnableFrozenCatalogForWideWorld();
	}
	UE_LOG(LogReal33D, Log,
		TEXT("wide world %s: radius=%d sector=32 cache=%s fallback=%s"),
		bWideWorldEnabled ? TEXT("enabled") : TEXT("disabled"),
		WideWorldVisualRadius, *WideWorldCacheDirectory, *WideWorldPreviewDirectory);
}

void AReal33DWorld::RequestStaticSector(const FIntVector& SectorKey)
{
	check(IsInGameThread());
	if (PendingStaticSectors.Contains(SectorKey))
	{
		return;
	}
	PendingStaticSectors.Add(SectorKey);
	const FString Path = FPaths::Combine(WideWorldCacheDirectory,
		FString::Printf(TEXT("%d-%d-%d.wws"), SectorKey.X, SectorKey.Y, SectorKey.Z));
	const TWeakObjectPtr<AReal33DWorld> WeakThis(this);
	Async(EAsyncExecution::ThreadPool, [WeakThis, SectorKey, Path]()
	{
		TArray<FReal33DStaticItem> Items;
		FString Error;
		FString Source;
		if (!FFileHelper::LoadFileToString(Source, *Path))
		{
			Error = TEXT("sector cache file absent");
		}
		else
		{
			TArray<FString> Lines;
			Source.ParseIntoArrayLines(Lines, true);
			for (const FString& Line : Lines)
			{
				TArray<FString> Fields;
				Line.ParseIntoArrayWS(Fields);
				if (Fields.Num() != 6)
				{
					Error = FString::Printf(TEXT("invalid cache row: %s"), *Line);
					Items.Reset();
					break;
				}
				FReal33DStaticItem Item;
				Item.Position.X = FCString::Atoi(*Fields[0]);
				Item.Position.Y = FCString::Atoi(*Fields[1]);
				Item.Position.Z = FCString::Atoi(*Fields[2]);
				Item.Stack = FCString::Atoi(*Fields[3]);
				Item.TypeId = static_cast<uint16>(FCString::Atoi(*Fields[4]));
				Item.SourceTypeId = static_cast<uint16>(FCString::Atoi(*Fields[5]));
				Items.Add(Item);
			}
		}
		AsyncTask(ENamedThreads::GameThread,
			[WeakThis, SectorKey, Items = MoveTemp(Items), Error = MoveTemp(Error)]() mutable
		{
			if (AReal33DWorld* World = WeakThis.Get())
			{
				World->InstallStaticSector(SectorKey, Items, Error);
			}
		});
	});
}

void AReal33DWorld::InstallStaticSector(const FIntVector& SectorKey,
	const TArray<FReal33DStaticItem>& Items, const FString& Error)
{
	check(IsInGameThread());
	PendingStaticSectors.Remove(SectorKey);
	StaticSectorCache.Add(SectorKey, Items);
	if (!Error.IsEmpty())
	{
		if (Error != TEXT("sector cache file absent"))
		{
			UE_LOG(LogReal33D, Error, TEXT("wide world %s: %s"),
				*SectorKey.ToString(), *Error);
		}
		return;
	}
	const bool bStillDesired = bWideWorldAnchorSet
		&& AReal33DStaticSector::DesiredSectors(
			WideWorldAnchor, WideWorldVisualRadius).Contains(SectorKey);
	if (!bWideWorldEnabled || !bStillDesired || Items.IsEmpty()
		|| StaticSectors.Contains(SectorKey))
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AReal33DStaticSector* Sector = GetWorld()->SpawnActor<AReal33DStaticSector>(
		AReal33DStaticSector::StaticClass(), FVector::ZeroVector,
		FRotator::ZeroRotator, Params);
	if (Sector == nullptr)
	{
		return;
	}
	Sector->Build(Items, Registry, WideWorldPreviewDirectory, Origin);
	Sector->ApplyView(WideWorldAnchor, WideWorldVisualRadius, bHideUpperFloors);
	StaticSectors.Add(SectorKey, Sector);
	++WideWorldLoads;
	WideWorldV08Resolved += Sector->GetV08Resolved();
	WideWorldClassicFallback += Sector->GetClassicFallback();
	WideWorldMissingPhysical += Sector->GetMissingPhysicalAsset();
	UE_LOG(LogReal33D, Log,
		TEXT("wide world loaded sector %s: objects=%d V08_RESOLVED=%d CLASSIC_SPRITE_FALLBACK=%d MISSING_PHYSICAL_ASSET=%d"),
		*SectorKey.ToString(), Items.Num(), Sector->GetV08Resolved(),
		Sector->GetClassicFallback(), Sector->GetMissingPhysicalAsset());
}

void AReal33DWorld::UpdateWideWorld()
{
	check(IsInGameThread());
	if (!bWideWorldEnabled || !bWideWorldAnchorSet || !Origin.bSet)
	{
		return;
	}
	const TSet<FIntVector> Desired = AReal33DStaticSector::DesiredSectors(
		WideWorldAnchor, WideWorldVisualRadius);

	TArray<FIntVector> Remove;
	for (const TPair<FIntVector, TObjectPtr<AReal33DStaticSector>>& Pair : StaticSectors)
	{
		if (!Desired.Contains(Pair.Key))
		{
			Remove.Add(Pair.Key);
		}
	}
	for (const FIntVector& Key : Remove)
	{
		TObjectPtr<AReal33DStaticSector> Sector;
		if (StaticSectors.RemoveAndCopyValue(Key, Sector) && Sector)
		{
			Sector->Destroy();
			++WideWorldUnloads;
		}
	}

	for (const FIntVector& Key : Desired)
	{
		if (TObjectPtr<AReal33DStaticSector>* Existing = StaticSectors.Find(Key))
		{
			if (Existing->Get())
			{
				(*Existing)->ApplyView(WideWorldAnchor, WideWorldVisualRadius, bHideUpperFloors);
			}
			continue;
		}
		if (const TArray<FReal33DStaticItem>* Cached = StaticSectorCache.Find(Key))
		{
			if (!Cached->IsEmpty())
			{
				++WideWorldCacheHits;
				InstallStaticSector(Key, *Cached, FString());
			}
			continue;
		}
		RequestStaticSector(Key);
	}
}


void AReal33DWorld::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UGameInstance* GameInstance = GetGameInstance();
	UReal33DBridge* Bridge = GameInstance != nullptr
		? GameInstance->GetSubsystem<UReal33DBridge>() : nullptr;
	if (Bridge != nullptr)
	{
		TArray<FReal33DEvent> Events;
		Bridge->DrainEvents(Events);
		for (const FReal33DEvent& Event : Events)
		{
			HandleEvent(Event);
		}
		// Opt-in observational QA capture. No commands, movement or local success.
		if (MinimapQAObservedAt == 0.0 && Bridge->GetMinimapState().bPlayerKnown)
			MinimapQAObservedAt = FPlatformTime::Seconds();
		if (!bMinimapQACaptured && Bridge->GetMinimapState().bPlayerKnown
			&& MinimapQAObservedAt > 0.0 && FPlatformTime::Seconds() - MinimapQAObservedAt > 1.0
			&& FParse::Param(FCommandLine::Get(), TEXT("real33d-minimap-qa")))
		{
			bMinimapQACaptured = true;
			WriteEvidence(TEXT("MinimapInitial"));
		}
		if (!bMinimapQASettledCaptured && Bridge->GetMinimapState().bPlayerKnown
			&& MinimapQAObservedAt > 0.0 && FPlatformTime::Seconds() - MinimapQAObservedAt > 30.0
#if WITH_EDITOR
			&& FAssetCompilingManager::Get().GetNumRemainingAssets() == 0
#endif
			&& FParse::Param(FCommandLine::Get(), TEXT("real33d-minimap-qa")))
		{
			bMinimapQASettledCaptured = true;
			WriteEvidence(TEXT("MinimapSettled"));
		}
	}

	// A dropped session is retried rather than left dead. The scene was already
	// torn down when the drop arrived, so a retry starts from nothing and
	// rebuilds from whatever WorldState the server sends next: there is no
	// state carried across the gap that could survive as a ghost.
	if (Bridge != nullptr && !bConnected && DisconnectedAt > 0.0
		&& ReconnectAttempts < 3
		&& FPlatformTime::Seconds() - DisconnectedAt > 5.0)
	{
		++ReconnectAttempts;
		DisconnectedAt = FPlatformTime::Seconds();
		UE_LOG(LogReal33D, Log, TEXT("reconnecting, attempt %d"), ReconnectAttempts);
		Bridge->Disconnect();
		Bridge->Connect(Config);
	}

	if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (Controller->GetViewTarget() != this)
		{
			Controller->SetViewTarget(this);
		}
	}

	UpdateFloorVisibility();
	UpdateCamera(DeltaSeconds);
	DrawOverlay();
}

void AReal33DWorld::PresentSpeech(const FReal33DEvent& Event)
{
	check(IsInGameThread());

	// A resolved speaker means exactly one visible creature matched both the
	// position and, where present, the name. Anything else goes to the fallback
	// rather than to a guess: speech above the wrong creature is a worse
	// failure than speech that is merely not in the world.
	if (Event.CreatureId != 0)
	{
		if (TObjectPtr<AReal33DCreature>* Found = Creatures.Find(Event.CreatureId))
		{
			if (Found->Get())
			{
				(*Found)->ShowSpeech(Event.Detail, SpeechSeconds);
				++SpeechOnCreature;
				return;
			}
		}
	}

	// Still readable, just not in the world. Nothing is drawn here: the bridge
	// filed this line in the transcript as it was drained, and the chat area
	// reads that. Counting it is all this function has left to do, and the
	// count is what proves the distant case took this path rather than being
	// quietly attributed to a creature.
	++SpeechInChatArea;
}

void AReal33DWorld::JournalMovement(const FReal33DEvent& Event)
{
	if (JournalPath.IsEmpty())
	{
		return;
	}

	const TCHAR* Phase = TEXT("unknown");
	switch (Event.Kind)
	{
	case EReal33DEventKind::WalkSent:          Phase = TEXT("sent"); break;
	case EReal33DEventKind::WalkAccepted:      Phase = TEXT("accepted"); break;
	case EReal33DEventKind::WalkRejected:      Phase = TEXT("rejected"); break;
	case EReal33DEventKind::ExternalRelocation: Phase = TEXT("external_relocation"); break;
	case EReal33DEventKind::WalkUnanswered:    Phase = TEXT("unanswered"); break;
	default: return;
	}

	static const TCHAR* const Names[] = { TEXT("north"), TEXT("east"),
		TEXT("south"), TEXT("west") };
	const TCHAR* DirectionText =
		Event.Direction < 4 ? Names[Event.Direction] : TEXT("none");

	// One JSON object per line, appended as it happens, so the order in the
	// file is the order it occurred and a truncated run still says what it got
	// to. Positions are the server's, never the Actor's transform.
	const FString Line = FString::Printf(
		TEXT("{\"t\":%.3f,\"phase\":\"%s\",\"input_id\":%u,\"request_id\":%u,")
		TEXT("\"direction\":\"%s\",\"from\":{\"x\":%d,\"y\":%d,\"z\":%d},")
		TEXT("\"to\":{\"x\":%d,\"y\":%d,\"z\":%d}}\n"),
		FirstFrameTime > 0.0 ? FPlatformTime::Seconds() - FirstFrameTime : 0.0,
		Phase, Event.InputId, Event.RequestId, DirectionText,
		Event.PreviousPosition.X, Event.PreviousPosition.Y, Event.PreviousPosition.Z,
		Event.Position.X, Event.Position.Y, Event.Position.Z);

	FFileHelper::SaveStringToFile(Line, *JournalPath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(), FILEWRITE_Append);
}

void AReal33DWorld::WriteEvidence(const FString& Reason)
{
	UGameInstance* GameInstance = GetGameInstance();
	UReal33DBridge* Bridge = GameInstance != nullptr
		? GameInstance->GetSubsystem<UReal33DBridge>() : nullptr;
	const FReal33DStats Stats = Bridge != nullptr ? Bridge->GetStats() : FReal33DStats();
	const FReal33DMinimapState Map = Bridge != nullptr ? Bridge->GetMinimapState() : FReal33DMinimapState{};
	const AReal33DCreature* Local = GetLocalPlayer();
	const bool bTargetActorVisible = Combat.TargetCreatureId != 0
		&& Creatures.Contains(Combat.TargetCreatureId);

	// What the chat area is holding at this instant. Recorded because "the
	// operator could read it" is the claim this milestone has to support, and a
	// transcript that is empty when a distant yell was counted would contradict
	// it in a way no screenshot could hide.
	TArray<FReal33DChatLine> Transcript;
	if (Bridge != nullptr)
	{
		Bridge->GetChatTranscript(Transcript);
	}
	const int32 TranscriptLines = Transcript.Num();

	TArray<FString> TranscriptJson;
	TranscriptJson.Reserve(TranscriptLines);
	for (const FReal33DChatLine& Each : Transcript)
	{
		TranscriptJson.Add(FString::Printf(
			TEXT("    { \"system_line\": %s, \"line\": \"%s\" }"),
			Each.bSystemLine ? TEXT("true") : TEXT("false"),
			*EscapeForJson(Each.Line)));
	}

	FString Directory;
	if (!FParse::Value(FCommandLine::Get(), TEXT("-real33d-evidence="), Directory)
		|| Directory.IsEmpty())
	{
		Directory = FPaths::ProjectSavedDir();
	}
	IPlatformFile& Platform = FPlatformFileManager::Get().GetPlatformFile();
	Platform.CreateDirectoryTree(*Directory);

	// The closing snapshot keeps the plain name; anything written mid-session
	// keeps its own, so a disconnect in the middle of a run is not overwritten
	// by the snapshot taken when the window closes.
	FString SnapshotTag = Reason;
	if (Reason == TEXT("Manual") && FParse::Param(FCommandLine::Get(), TEXT("real33d-minimap-qa")))
	{
		const FDateTime CapturedAt = FDateTime::UtcNow();
		SnapshotTag = FString::Printf(TEXT("Manual_%s_%03d"),
			*CapturedAt.ToString(TEXT("%Y%m%dT%H%M%S")), CapturedAt.GetMillisecond());
	}
	const FString Path = FPaths::Combine(Directory,
		Reason == TEXT("EndPlay")
			? FString(TEXT("unreal_slice_evidence.json"))
			: FString::Printf(TEXT("unreal_slice_evidence_%s.json"), *SnapshotTag));

	// What the player is wearing and what they have open, exactly as WorldState
	// holds it. The panels draw these and nothing else, so a reviewer can put a
	// slot in this file beside the server's own save file and see whether the
	// two agree. A screenshot cannot carry that: a type id drawn as a 32-pixel
	// picture is not readable, and the claim is about the id, not the picture.
	const FReal33DInventory& Worn = GetInventory();
	TArray<FString> InventoryLines;
	for (int32 Slot = FReal33DInventory::FirstSlot;
		Slot <= FReal33DInventory::LastSlot; ++Slot)
	{
		if (!Worn.bOccupied[Slot])
		{
			continue;
		}
		const FReal33DItem& Item = Worn.Items[Slot];
		InventoryLines.Add(FString::Printf(
			TEXT("    { \"slot\": %d, \"type_id\": %u, \"amount\": %s, \"liquid\": %s }"),
			Slot, Item.TypeId,
			Item.bHasAmount ? *FString::FromInt(Item.Amount) : TEXT("null"),
			Item.bHasLiquidColour ? *FString::FromInt(Item.LiquidColour) : TEXT("null")));
	}

	TArray<FString> ContainerLines;
	for (const FReal33DContainer& Open : GetContainers())
	{
		TArray<FString> ObjectLines;
		ObjectLines.Reserve(Open.Objects.Num());
		for (int32 Index = 0; Index < Open.Objects.Num(); ++Index)
		{
			const FReal33DItem& Item = Open.Objects[Index];
			ObjectLines.Add(FString::Printf(
				TEXT("        { \"index\": %d, \"type_id\": %u, \"amount\": %s,")
				TEXT(" \"liquid\": %s }"),
				Index, Item.TypeId,
				Item.bHasAmount ? *FString::FromInt(Item.Amount) : TEXT("null"),
				Item.bHasLiquidColour ? *FString::FromInt(Item.LiquidColour) : TEXT("null")));
		}
		ContainerLines.Add(FString::Printf(
			TEXT("    {\n")
			TEXT("      \"number\": %u,\n")
			TEXT("      \"type_id\": %u,\n")
			TEXT("      \"name\": \"%s\",\n")
			TEXT("      \"capacity\": %u,\n")
			TEXT("      \"has_parent\": %s,\n")
			TEXT("      \"object_count\": %d,\n")
			TEXT("      \"objects\": [\n%s\n      ]\n")
			TEXT("    }"),
			Open.Number, Open.TypeId, *EscapeForJson(Open.Name), Open.Capacity,
			Open.bHasParent ? TEXT("true") : TEXT("false"),
			Open.Objects.Num(), *FString::Join(ObjectLines, TEXT(",\n"))));
	}

	TArray<FString> CreatureLines;
	for (const TPair<uint32, TObjectPtr<AReal33DCreature>>& Pair : Creatures)
	{
		if (!Pair.Value)
		{
			continue;
		}
		const Real33D::FMapPosition& Position = Pair.Value->GetLogicalPosition();
		CreatureLines.Add(FString::Printf(
			TEXT("    { \"id\": %u, \"local\": %s, \"x\": %d, \"y\": %d, \"z\": %d }"),
			Pair.Key, Pair.Value->IsLocalPlayer() ? TEXT("true") : TEXT("false"),
			Position.X, Position.Y, Position.Z));
	}

	const FString Json = FString::Printf(
		TEXT("{\n")
		TEXT("  \"milestone\": \"UNREAL-SLICE-001\",\n")
		TEXT("  \"written_at\": \"%s\",\n")
		TEXT("  \"reason\": \"%s\",\n")
		TEXT("  \"seconds_in_session\": %.1f,\n")
		TEXT("  \"connected_at_close\": %s,\n")
		TEXT("  \"local_creature_id\": %u,\n")
		TEXT("  \"local_position\": { \"x\": %d, \"y\": %d, \"z\": %d },\n")
		TEXT("  \"origin\": { \"set\": %s, \"x\": %d, \"y\": %d, \"z\": %d },\n")
		TEXT("  \"anchor\": { \"x\": %d, \"y\": %d, \"z\": %d },\n")
		TEXT("  \"minimap\": { \"player_known\": %s, \"x\": %d, \"y\": %d, \"z\": %d, \"known_cells\": %d, \"live_cells\": %d },\n")
		TEXT("  \"clientcore\": {\n")
		TEXT("    \"frames\": %d,\n")
		TEXT("    \"commands\": %d,\n")
		TEXT("    \"residual_bytes\": %d,\n")
		TEXT("    \"unsupported_opcodes\": %d,\n")
		TEXT("    \"protocol_anomalies\": %d,\n")
		TEXT("    \"walks_requested_from_unreal\": %d,\n")
		TEXT("    \"walks_accepted\": %d,\n")
		TEXT("    \"walks_rejected\": %d,\n")
		TEXT("    \"walks_unanswered\": %d,\n")
		TEXT("    \"external_relocations\": %d,\n")
		TEXT("    \"local_player_moves_total\": %d,\n")
		TEXT("    \"says_requested\": %d,\n")
		TEXT("    \"moves_requested\": %d,\n")
		TEXT("    \"uses_requested\": %d,\n")
		TEXT("    \"looks_requested\": %d,\n")
		TEXT("    \"attacks_requested\": %d,\n")
		TEXT("    \"follows_requested\": %d,\n")
		TEXT("    \"combat_cancels_requested\": %d,\n")
		TEXT("    \"tactics_requested\": %d,\n")
		TEXT("    \"target_clears_received\": %d,\n")
		TEXT("    \"worldstate_tiles\": %d,\n")
		TEXT("    \"worldstate_visible_creatures\": %d,\n")
		TEXT("    \"viewport_synchronised\": %s\n")
		TEXT("  },\n")
		TEXT("  \"camera\": { \"pitch\": %.2f, \"yaw\": %.2f, \"requested_distance\": %.2f, \"actual_distance\": %.2f, \"obstructed\": %s, \"cutaway_meshes\": %d, \"hide_upper_floors\": %s },\n")
		TEXT("  \"wideworld\": { \"loads\": %d, \"unloads\": %d, \"sectors\": %d },\n")
		TEXT("  \"presentation\": {\n")
		TEXT("    \"tile_actors\": %d,\n")
		TEXT("    \"creature_actors\": %d,\n")
		TEXT("    \"tiles_spawned\": %d,\n")
		TEXT("    \"tiles_removed\": %d,\n")
		TEXT("    \"creatures_appeared\": %d,\n")
		TEXT("    \"creatures_vanished\": %d,\n")
		TEXT("    \"creature_moves\": %d,\n")
		TEXT("    \"duplicate_spawn_attempts\": %d,\n")
		TEXT("    \"orphan_events\": %d,\n")
		TEXT("    \"speech_shown_on_creature\": %d,\n")
		TEXT("    \"speech_shown_in_chat_area_only\": %d,\n")
		TEXT("    \"server_messages_received\": %d,\n")
		TEXT("    \"client_notices_raised\": %d,\n")
		TEXT("    \"chat_transcript_lines\": %d,\n")
		TEXT("    \"speech_seconds_NOT_PROVEN\": %.1f\n")
		TEXT("  },\n")
		TEXT("  \"talk_messages_decoded\": %d,\n")
		TEXT("  \"last_diagnostic\": \"%s\",\n")
		TEXT("  \"inventory_known\": %s,\n")
		TEXT("  \"inventory\": [\n%s\n  ],\n")
		TEXT("  \"open_containers\": %d,\n")
		TEXT("  \"containers\": [\n%s\n  ],\n")
		TEXT("  \"combat\": { \"target_creature_id\": %u, \"following\": %s,")
		TEXT(" \"tactics_sent\": %s, \"attack_mode\": %d, \"chase_mode\": %d,")
		TEXT(" \"target_actor_visible\": %s },\n")
		TEXT("  \"chat_transcript\": [\n%s\n  ],\n")
		TEXT("  \"creatures\": [\n%s\n  ]\n")
		TEXT("}\n"),
		*FDateTime::UtcNow().ToIso8601(), *Reason,
		FirstFrameTime > 0.0 ? FPlatformTime::Seconds() - FirstFrameTime : 0.0,
		bConnected ? TEXT("true") : TEXT("false"),
		LocalCreatureId,
		Local != nullptr ? Local->GetLogicalPosition().X : 0,
		Local != nullptr ? Local->GetLogicalPosition().Y : 0,
		Local != nullptr ? Local->GetLogicalPosition().Z : 0,
		Origin.bSet ? TEXT("true") : TEXT("false"),
		Origin.Position.X, Origin.Position.Y, Origin.Position.Z,
		Stats.Anchor.X, Stats.Anchor.Y, Stats.Anchor.Z,
		Map.bPlayerKnown ? TEXT("true") : TEXT("false"), Map.Player.X, Map.Player.Y, Map.Player.Z, Map.KnownCells, Map.LiveCells,
		Stats.Frames, Stats.Commands, Stats.ResidualBytes, Stats.UnsupportedOpcodes,
		Stats.Anomalies, Stats.RequestedSteps, Stats.AcceptedSelfWalks,
		Stats.RejectedSteps, Stats.UnansweredSteps, Stats.ExternalRelocations,
		Stats.LocalPlayerMoves,
		Stats.SaysRequested, Stats.MovesRequested, Stats.UsesRequested,
		Stats.LooksRequested,
		Stats.AttacksRequested, Stats.FollowsRequested,
		Stats.CombatCancelsRequested, Stats.TacticsRequested,
		Stats.TargetClearsReceived,
		Stats.Tiles,
		Stats.VisibleCreatures, Stats.bViewportSynchronised ? TEXT("true") : TEXT("false"),
		CameraPitch, CameraYaw, CameraDistance, DrawnCameraDistance, bCameraObstructed ? TEXT("true") : TEXT("false"), CameraCutaways, bHideUpperFloors ? TEXT("true") : TEXT("false"),
		WideWorldLoads, WideWorldUnloads, StaticSectors.Num(),
		Tiles.Num(), Creatures.Num(), TilesSpawned, TilesRemoved, CreaturesAppeared,
		CreaturesVanished, CreatureMoves, DuplicateSpawnAttempts, OrphanEvents,
		SpeechOnCreature, SpeechInChatArea, ServerMessagesReceived,
		ClientNoticesRaised, TranscriptLines, SpeechSeconds,
		Stats.TalkMessages,
		LastDiagnostic.IsEmpty() ? TEXT("none") : *EscapeForJson(LastDiagnostic),
		Worn.bKnown ? TEXT("true") : TEXT("false"),
		*FString::Join(InventoryLines, TEXT(",\n")),
		GetContainers().Num(),
		*FString::Join(ContainerLines, TEXT(",\n")),
		Combat.TargetCreatureId, Combat.bFollowing ? TEXT("true") : TEXT("false"),
		Combat.bTacticsSent ? TEXT("true") : TEXT("false"),
		static_cast<int32>(Combat.AttackMode), static_cast<int32>(Combat.ChaseMode),
		bTargetActorVisible ? TEXT("true") : TEXT("false"),
		*FString::Join(TranscriptJson, TEXT(",\n")),
		*FString::Join(CreatureLines, TEXT(",\n")));

	if (FFileHelper::SaveStringToFile(Json, *Path))
	{
		UE_LOG(LogReal33D, Log, TEXT("evidence written to %s"), *Path);
		// QA-only geometry inventory from current live presentation. It reads no
		// server database or unseen map; these are the same components being drawn.
		if (Local && FParse::Param(FCommandLine::Get(), TEXT("real33d-presentation-qa")))
		{
			TArray<FString> GeometryLines;
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			for (const auto& Pair : Tiles)
			{
				if (!Pair.Value) continue;
				TArray<UStaticMeshComponent*> Components;
				Pair.Value->GetComponents(Components);
				for (const UStaticMeshComponent* Component : Components)
				{
					if (!Component || !Component->GetStaticMesh()) continue;
					FString Identity;
					for (const FName& Tag : Component->ComponentTags)
						if (Tag.ToString().StartsWith(TEXT("V08_"))) Identity = Tag.ToString().Mid(4);
					if (Identity.IsEmpty()) continue;
					const FVector Size = Component->Bounds.GetBox().GetSize();
					const FRotator Rotation = Component->GetComponentRotation();
					FVector2D Pixel = FVector2D::ZeroVector;
					const bool bProjected = PC && PC->ProjectWorldLocationToScreen(Component->Bounds.Origin, Pixel);
					GeometryLines.Add(FString::Printf(TEXT("{\"type_id\":%s,\"tile\":[%d,%d,%d],\"mesh\":\"%s\",\"visible\":%s,\"rotation\":[%.2f,%.2f,%.2f],\"world_bounds_size\":[%.2f,%.2f,%.2f],\"projected\":%s,\"screen_center\":[%.2f,%.2f]}"),
						*Identity, Pair.Key.X, Pair.Key.Y, Pair.Key.Z,
						*EscapeForJson(Component->GetStaticMesh()->GetPathName()),
						Component->IsVisible() && !Pair.Value->IsHidden() ? TEXT("true") : TEXT("false"),
						Rotation.Pitch, Rotation.Yaw, Rotation.Roll, Size.X, Size.Y, Size.Z,
						bProjected ? TEXT("true") : TEXT("false"), Pixel.X, Pixel.Y));
				}
			}
			FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"source\":\"current_live_render_components\",\"objects\":[\n%s\n]}"),
				*FString::Join(GeometryLines, TEXT(",\n"))), *FPaths::ChangeExtension(Path, TEXT("geometry.json")));
		}
		if (Reason != TEXT("EndPlay") && FParse::Param(FCommandLine::Get(), TEXT("real33d-minimap-qa")))
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ChangeExtension(Path, TEXT("png")), true, false);
		}
	}
	else
	{
		UE_LOG(LogReal33D, Error, TEXT("could not write evidence to %s"), *Path);
	}
}

void AReal33DWorld::GetPresentationCreatures(TArray<AReal33DCreature*>& Out) const
{
 Out.Reset();
 const AReal33DCreature* Local = GetLocalPlayer();
 if (!Local) return;
 for (const auto& Pair : Creatures)
  if (Pair.Value && !Pair.Value->IsHidden() && Pair.Value->GetLogicalPosition().Z == Local->GetLogicalPosition().Z)
   Out.Add(Pair.Value);
 Out.Sort([this](const AReal33DCreature& A, const AReal33DCreature& B)
 {
  const int32 APriority = A.IsLocalPlayer() ? 0 : A.GetCreatureId() == Combat.TargetCreatureId ? 1 : 2;
  const int32 BPriority = B.IsLocalPlayer() ? 0 : B.GetCreatureId() == Combat.TargetCreatureId ? 1 : 2;
  return APriority != BPriority ? APriority < BPriority : A.GetCreatureId() < B.GetCreatureId();
 });
}
