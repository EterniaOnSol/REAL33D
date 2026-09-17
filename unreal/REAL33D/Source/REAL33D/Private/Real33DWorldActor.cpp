#include "Real33DWorldActor.h"

#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
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
	Camera->SetRelativeLocation(FVector(-620.0, -620.0, 780.0));
	Camera->SetRelativeRotation(FRotator(-42.0, 45.0, 0.0));
}

void AReal33DWorld::BeginPlay()
{
	Super::BeginPlay();

	Registry = NewObject<UReal33DAssetRegistry>(this);
	Registry->Initialise();

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

	for (TPair<uint32, TObjectPtr<AReal33DCreature>>& Pair : Creatures)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}
	Creatures.Reset();
	LocalCreatureId = 0;
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

	case EReal33DEventKind::Diagnostic:
		// Prose from the client core about something it could not consume.
		// Kept so the evidence can say which command, not just how many.
		LastDiagnostic = Event.Detail;
		UE_LOG(LogReal33D, Warning, TEXT("client core: %s"), *Event.Detail);
		break;

	case EReal33DEventKind::LocalPlayerIdentified:
		LocalCreatureId = Event.CreatureId;
		UE_LOG(LogReal33D, Log, TEXT("local player is creature %u"), Event.CreatureId);
		break;

	case EReal33DEventKind::AnchorMoved:
		// The first anchor of the session fixes the origin, and nothing moves
		// it afterwards: re-centring would shift every actor in the scene on
		// each step. See Real33DCoords.h for why an origin exists at all.
		Origin.EnsureSet(Event.Position);
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
		Creature->Configure(Event.CreatureId, Event.bIsLocalPlayer, Event.CreatureName,
			Registry);
		// An appearance is not a walk. Place it, do not slide it in.
		Creature->CommitPosition(Origin, Event.Position, /*bSnap=*/true);
		Creature->SetFacing(Event.Direction);
		Creatures.Add(Event.CreatureId, Creature);
		++CreaturesAppeared;
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
		break;
	}
	}
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
	const FVector Desired = Local->GetActorLocation();
	SetActorLocation(FMath::VInterpTo(GetActorLocation(), Desired, DeltaSeconds, 8.0));
}

void AReal33DWorld::DrawOverlay()
{
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
		FString::Printf(TEXT("steps asked %d  refused %d   server moved us %d   viewport %s"),
			Stats.RequestedSteps, Stats.RejectedSteps, Stats.LocalPlayerMoves,
			Stats.bViewportSynchronised ? TEXT("in sync") : TEXT("waiting")));
	GEngine->AddOnScreenDebugMessage(5, 0.0f, Clean,
		FString::Printf(
			TEXT("residual %d  unsupported %d  anomalies %d  dup %d  orphan %d"),
			Stats.ResidualBytes, Stats.UnsupportedOpcodes, Stats.Anomalies,
			DuplicateSpawnAttempts, OrphanEvents));
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

	UpdateCamera(DeltaSeconds);
	DrawOverlay();
}

void AReal33DWorld::WriteEvidence(const FString& Reason)
{
	UGameInstance* GameInstance = GetGameInstance();
	UReal33DBridge* Bridge = GameInstance != nullptr
		? GameInstance->GetSubsystem<UReal33DBridge>() : nullptr;
	const FReal33DStats Stats = Bridge != nullptr ? Bridge->GetStats() : FReal33DStats();
	const AReal33DCreature* Local = GetLocalPlayer();

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
	const FString Path = FPaths::Combine(Directory,
		Reason == TEXT("EndPlay")
			? FString(TEXT("unreal_slice_evidence.json"))
			: FString::Printf(TEXT("unreal_slice_evidence_%s.json"), *Reason));

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
		TEXT("  \"clientcore\": {\n")
		TEXT("    \"frames\": %d,\n")
		TEXT("    \"commands\": %d,\n")
		TEXT("    \"residual_bytes\": %d,\n")
		TEXT("    \"unsupported_opcodes\": %d,\n")
		TEXT("    \"protocol_anomalies\": %d,\n")
		TEXT("    \"steps_requested\": %d,\n")
		TEXT("    \"steps_rejected\": %d,\n")
		TEXT("    \"local_player_moves\": %d,\n")
		TEXT("    \"worldstate_tiles\": %d,\n")
		TEXT("    \"worldstate_visible_creatures\": %d,\n")
		TEXT("    \"viewport_synchronised\": %s\n")
		TEXT("  },\n")
		TEXT("  \"presentation\": {\n")
		TEXT("    \"tile_actors\": %d,\n")
		TEXT("    \"creature_actors\": %d,\n")
		TEXT("    \"tiles_spawned\": %d,\n")
		TEXT("    \"tiles_removed\": %d,\n")
		TEXT("    \"creatures_appeared\": %d,\n")
		TEXT("    \"creatures_vanished\": %d,\n")
		TEXT("    \"creature_moves\": %d,\n")
		TEXT("    \"duplicate_spawn_attempts\": %d,\n")
		TEXT("    \"orphan_events\": %d\n")
		TEXT("  },\n")
		TEXT("  \"last_diagnostic\": \"%s\",\n")
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
		Stats.Frames, Stats.Commands, Stats.ResidualBytes, Stats.UnsupportedOpcodes,
		Stats.Anomalies, Stats.RequestedSteps, Stats.RejectedSteps, Stats.LocalPlayerMoves,
		Stats.Tiles,
		Stats.VisibleCreatures, Stats.bViewportSynchronised ? TEXT("true") : TEXT("false"),
		Tiles.Num(), Creatures.Num(), TilesSpawned, TilesRemoved, CreaturesAppeared,
		CreaturesVanished, CreatureMoves, DuplicateSpawnAttempts, OrphanEvents,
		LastDiagnostic.IsEmpty() ? TEXT("none") : *LastDiagnostic,
		*FString::Join(CreatureLines, TEXT(",\n")));

	if (FFileHelper::SaveStringToFile(Json, *Path))
	{
		UE_LOG(LogReal33D, Log, TEXT("evidence written to %s"), *Path);
	}
	else
	{
		UE_LOG(LogReal33D, Error, TEXT("could not write evidence to %s"), *Path);
	}
}
