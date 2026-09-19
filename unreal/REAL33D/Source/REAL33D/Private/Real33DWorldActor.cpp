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
		Creature->SetHealthPercent(Event.HealthPercent);
		Creature->Configure(Event.CreatureId, Event.bIsLocalPlayer, Event.CreatureName,
			Registry);
		UE_LOG(LogReal33D, Log, TEXT("creature %u \"%s\" appeared at %d,%d,%d health %u%%"),
			Event.CreatureId, *Event.CreatureName,
			Event.Position.X, Event.Position.Y, Event.Position.Z, Event.HealthPercent);
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

void AReal33DWorld::ApplyCameraTransform()
{
	if (Camera == nullptr)
	{
		return;
	}

	// The actor sits on the player, so orbiting is entirely a matter of where
	// the camera is placed relative to it. Put the camera one Distance back
	// along the direction it looks, and the point it looks at is the actor's
	// origin by construction — which is what keeps the player centred no matter
	// how far the view is swung around.
	const FRotator Look(CameraPitch, CameraYaw, 0.0f);
	Camera->SetRelativeLocation(-Look.Vector() * CameraDistance);
	Camera->SetRelativeRotation(Look);
}

void AReal33DWorld::AddCameraOrbit(float DeltaYawDegrees, float DeltaPitchDegrees)
{
	CameraYaw = FRotator::ClampAxis(CameraYaw + DeltaYawDegrees);

	// Clamped to stay above the ground looking down. The floor is drawn as a
	// plane, so a camera level with it or below sees a horizon of nothing, and
	// there is no recovery gesture that would be obvious to an operator who got
	// there by accident. The shallow end is left at -5 rather than 0 because
	// reading a name or a speech tag above a creature is exactly what the
	// shallow angles are for.
	CameraPitch = FMath::Clamp(CameraPitch + DeltaPitchDegrees,
		kCameraPitchMin, kCameraPitchMax);

	ApplyCameraTransform();
}

void AReal33DWorld::AddCameraDistance(float Delta)
{
	CameraDistance = FMath::Clamp(CameraDistance + Delta,
		kCameraDistanceMin, kCameraDistanceMax);
	ApplyCameraTransform();
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
	const AReal33DCreature* Local = GetLocalPlayer();

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
		TEXT("    \"walks_requested_from_unreal\": %d,\n")
		TEXT("    \"walks_accepted\": %d,\n")
		TEXT("    \"walks_rejected\": %d,\n")
		TEXT("    \"walks_unanswered\": %d,\n")
		TEXT("    \"external_relocations\": %d,\n")
		TEXT("    \"local_player_moves_total\": %d,\n")
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
		Stats.Frames, Stats.Commands, Stats.ResidualBytes, Stats.UnsupportedOpcodes,
		Stats.Anomalies, Stats.RequestedSteps, Stats.AcceptedSelfWalks,
		Stats.RejectedSteps, Stats.UnansweredSteps, Stats.ExternalRelocations,
		Stats.LocalPlayerMoves,
		Stats.Tiles,
		Stats.VisibleCreatures, Stats.bViewportSynchronised ? TEXT("true") : TEXT("false"),
		Tiles.Num(), Creatures.Num(), TilesSpawned, TilesRemoved, CreaturesAppeared,
		CreaturesVanished, CreatureMoves, DuplicateSpawnAttempts, OrphanEvents,
		SpeechOnCreature, SpeechInChatArea, ServerMessagesReceived,
		ClientNoticesRaised, TranscriptLines, SpeechSeconds,
		Stats.TalkMessages,
		LastDiagnostic.IsEmpty() ? TEXT("none") : *EscapeForJson(LastDiagnostic),
		*FString::Join(TranscriptJson, TEXT(",\n")),
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
