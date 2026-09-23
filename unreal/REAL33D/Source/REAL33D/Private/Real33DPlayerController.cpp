#include "Real33DPlayerController.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "REAL33D.h"
#include "Real33DBridge.h"
#include "Real33DChatPanel.h"
#include "Real33DCreatureActor.h"
#include "Real33DAssetRegistry.h"
#include "Real33DHUD.h"
#include "Real33DTileActor.h"
#include "Real33DWorldActor.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"

AReal33DPlayerController::AReal33DPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	// The chat area has to be clickable, which means a cursor. The input mode
	// stays GameAndUI throughout: Slate gets first refusal on every key, and
	// anything it does not want reaches the bindings below.
	bShowMouseCursor = true;
}

void AReal33DPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (FParse::Param(FCommandLine::Get(), TEXT("real33d-gallery")))
	{
		return;
	}

	if (GEngine == nullptr || GetWorld() == nullptr
		|| GetWorld()->GetGameViewport() == nullptr)
	{
		UE_LOG(LogReal33D, Error,
			TEXT("no game viewport; the chat area cannot be shown"));
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UReal33DBridge* Bridge = GameInstance != nullptr
		? GameInstance->GetSubsystem<UReal33DBridge>() : nullptr;

	// The whole in-game UI, as one widget filling the viewport. Before
	// UNREAL-UI-FULL-PORT-001 the chat area and the vitals were two loose boxes
	// pinned to opposite corners; they are now panels inside the frame
	// gameinterface.otui describes, and the HUD owns both.
	HudRoot = SNew(SReal33DHUD)
		.Bridge(Bridge)
		.OnTypingChanged(FReal33DOnTypingChanged::CreateUObject(
			this, &AReal33DPlayerController::HandleTypingChanged));
	ChatPanel = HudRoot->GetChatPanel();

	GetWorld()->GetGameViewport()->AddViewportWidgetContent(
		HudRoot.ToSharedRef(), /*ZOrder=*/10);
	FString CatalogPath;
	bInspectorEnabled = FParse::Value(FCommandLine::Get(),
		TEXT("-real33d-experimental-catalog="), CatalogPath);
	if (bInspectorEnabled)
	{
		FString EvidenceDirectory;
		if (!FParse::Value(FCommandLine::Get(), TEXT("-real33d-evidence="), EvidenceDirectory))
		{
			EvidenceDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("V08QA"));
		}
		InspectorNotesPath = FPaths::Combine(EvidenceDirectory, TEXT("wall_inspector_notes.tsv"));
		InspectorRoot = SNew(SBox)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			.Padding(FMargin(0.0f, 12.0f, 12.0f, 0.0f))
			[
				SNew(SBorder).Padding(8.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(InspectorLabel, STextBlock)
						.Text(FText::FromString(TEXT("V08: clic izquierdo en un objeto para ver su ID")))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
					[
						SAssignNew(InspectorNote, SEditableTextBox)
						.HintText(FText::FromString(TEXT("Nota opcional sobre el objeto")))
						.MinDesiredWidth(300.0f)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton).Text(FText::FromString(TEXT("Bien")))
							.OnClicked_Lambda([this]() { SaveInspectorNote(TEXT("OK")); return FReply::Handled(); })
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f)
						[
							SNew(SButton).Text(FText::FromString(TEXT("Girar 90")))
							.OnClicked_Lambda([this]() { SaveInspectorNote(TEXT("ROTATE_90")); return FReply::Handled(); })
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton).Text(FText::FromString(TEXT("Guardar nota")))
							.OnClicked_Lambda([this]() { SaveInspectorNote(TEXT("NOTE")); return FReply::Handled(); })
						]
					]
				]
			];
		GetWorld()->GetGameViewport()->AddViewportWidgetContent(
			InspectorRoot.ToSharedRef(), /*ZOrder=*/11);
	}

	SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
}

void AReal33DPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	// Removed explicitly. A viewport widget outlives the actor that made it,
	// so a second session would otherwise open on top of the first one's panel
	// and the operator would be typing into a box wired to a dead bridge.
	if (HudRoot.IsValid() && GetWorld() != nullptr
		&& GetWorld()->GetGameViewport() != nullptr)
	{
		GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(
			HudRoot.ToSharedRef());
	}
	if (InspectorRoot.IsValid() && GetWorld() != nullptr
		&& GetWorld()->GetGameViewport() != nullptr)
	{
		GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(InspectorRoot.ToSharedRef());
	}
	InspectorRoot.Reset();
	InspectorLabel.Reset();
	InspectorNote.Reset();
	HudRoot.Reset();
	LastVitalsText.Empty();
	ChatPanel.Reset();
	bTypingActive = false;

	Super::EndPlay(Reason);
}

void AReal33DPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent == nullptr)
	{
		return;
	}

	// enums.hh: DIRECTION_NORTH 0, EAST 1, SOUTH 2, WEST 3.
	InputComponent->BindKey(EKeys::W, IE_Pressed, this, &AReal33DPlayerController::WalkForward);
	InputComponent->BindKey(EKeys::D, IE_Pressed, this, &AReal33DPlayerController::WalkRight);
	InputComponent->BindKey(EKeys::S, IE_Pressed, this, &AReal33DPlayerController::WalkBackward);
	InputComponent->BindKey(EKeys::A, IE_Pressed, this, &AReal33DPlayerController::WalkLeft);
	InputComponent->BindKey(EKeys::W, IE_Released, this, &AReal33DPlayerController::ReleaseForward);
	InputComponent->BindKey(EKeys::D, IE_Released, this, &AReal33DPlayerController::ReleaseRight);
	InputComponent->BindKey(EKeys::S, IE_Released, this, &AReal33DPlayerController::ReleaseBackward);
	InputComponent->BindKey(EKeys::A, IE_Released, this, &AReal33DPlayerController::ReleaseLeft);
	InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &AReal33DPlayerController::WalkNorth);
	InputComponent->BindKey(EKeys::Right, IE_Pressed, this, &AReal33DPlayerController::WalkEast);
	InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &AReal33DPlayerController::WalkSouth);
	InputComponent->BindKey(EKeys::Left, IE_Pressed, this, &AReal33DPlayerController::WalkWest);

	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this,
		&AReal33DPlayerController::InspectUnderCursor);

	// Lets the operator take a labelled snapshot at any point of the run.
	InputComponent->BindKey(EKeys::F9, IE_Pressed, this,
		&AReal33DPlayerController::DumpEvidence);

	// Enter reaches here only when the chat box does not already have focus,
	// because Slate offers the key to the focused widget first and the box
	// commits on it. So one key both opens the line and sends it, without the
	// two paths having to know about each other.
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this,
		&AReal33DPlayerController::FocusChatInput);

	// Held right button plus mouse movement orbits; the wheel zooms.
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this,
		&AReal33DPlayerController::BeginOrbit);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this,
		&AReal33DPlayerController::EndOrbit);
	InputComponent->BindAxisKey(EKeys::MouseX, this,
		&AReal33DPlayerController::OrbitYaw);
	InputComponent->BindAxisKey(EKeys::MouseY, this,
		&AReal33DPlayerController::OrbitPitch);
	InputComponent->BindAxisKey(EKeys::MouseWheelAxis, this,
		&AReal33DPlayerController::ZoomCamera);

	// Escape has to be bound, not left to Slate.
	//
	// FSlateEditableTextLayout::HandleEscape only reports the key handled when
	// it had something to undo: a selection, a search, or text to revert. On an
	// empty box it returns Unhandled, so an operator who opens the line and
	// changes their mind would be left with the caret in the box, the gate shut
	// and no way to walk. It bubbles here instead and is closed explicitly.
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this,
		&AReal33DPlayerController::CloseChatInput);
}

void AReal33DPlayerController::FocusChatInput()
{
	if (!ChatPanel.IsValid())
	{
		return;
	}
	// The gate opens here as well as on the focus the panel is about to
	// receive, because both are explicit and BeginTyping is idempotent. What
	// must never happen is the caret arriving in the box with the gate shut.
	ChatPanel->BeginTyping();
	SetInputMode(FInputModeGameAndUI()
		.SetHideCursorDuringCapture(false)
		.SetWidgetToFocus(ChatPanel->GetInputWidget()));
}

void AReal33DPlayerController::CloseChatInput()
{
	if (HudRoot.IsValid() && HudRoot->IsTargeting())
	{
		HudRoot->CancelTargeting();
		return;
	}
	if (bTypingActive && ChatPanel.IsValid())
	{
		// EndTyping fires the change, which returns focus to the viewport.
		ChatPanel->EndTyping();
		return;
	}
	UGameInstance* GameInstance = GetGameInstance();
	UReal33DBridge* Bridge = GameInstance != nullptr
		? GameInstance->GetSubsystem<UReal33DBridge>() : nullptr;
	if (Bridge != nullptr && Bridge->IsRunning())
	{
		const uint32 ActionId = Bridge->RequestCancelCombat();
		UE_LOG(LogReal33D, Log, TEXT("combat input %u: general cancel"), ActionId);
	}
}

void AReal33DPlayerController::HandleTypingChanged(bool bTyping)
{
	bTypingActive = bTyping;
	if (bTyping)
	{
		for (bool& bHeld : bHeldMovement) bHeld = false;
	}
	if (!bTyping)
	{
		// Focus goes back to the viewport, so the next W is a step rather than
		// a character typed into a box nobody is looking at.
		SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
	}
}

void AReal33DPlayerController::Request(uint8 Direction)
{
	// The one place the rule lives: while the player is typing, walk keys are
	// letters. Typing "was" must not walk the player west, north and south.
	if (bTypingActive || (InspectorNote.IsValid() && InspectorNote->HasKeyboardFocus()))
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UReal33DBridge* Bridge = GameInstance != nullptr
		? GameInstance->GetSubsystem<UReal33DBridge>() : nullptr;
	if (Bridge == nullptr || !Bridge->IsRunning())
	{
		return;
	}
	// Every intent is logged with the key that produced it and the id that will
	// follow it through the rest of the chain. A walk request nobody can
	// account for is a defect, not noise; and a position change with no
	// request behind it is not this client's doing, which is exactly the
	// distinction a previous run failed to make.
	static const TCHAR* const Names[] = { TEXT("north"), TEXT("east"),
		TEXT("south"), TEXT("west") };
	const uint32 InputId = Bridge->RequestWalk(Direction);
	UE_LOG(LogReal33D, Log, TEXT("input %u: walk %s requested from Unreal"),
		InputId, Direction < 4 ? Names[Direction] : TEXT("?"));
}

void AReal33DPlayerController::WalkNorth() { Request(0); }
void AReal33DPlayerController::WalkEast()  { Request(1); }
void AReal33DPlayerController::WalkSouth() { Request(2); }
void AReal33DPlayerController::WalkWest()  { Request(3); }
void AReal33DPlayerController::WalkForward() { SetMovementHeld(0, true); }
void AReal33DPlayerController::WalkRight() { SetMovementHeld(1, true); }
void AReal33DPlayerController::WalkBackward() { SetMovementHeld(2, true); }
void AReal33DPlayerController::WalkLeft() { SetMovementHeld(3, true); }
void AReal33DPlayerController::ReleaseForward() { SetMovementHeld(0, false); }
void AReal33DPlayerController::ReleaseRight() { SetMovementHeld(1, false); }
void AReal33DPlayerController::ReleaseBackward() { SetMovementHeld(2, false); }
void AReal33DPlayerController::ReleaseLeft() { SetMovementHeld(3, false); }

void AReal33DPlayerController::SetMovementHeld(uint8 RelativeDirection, bool bHeld)
{
	if (RelativeDirection >= 4) return;
	const bool bWasHeld = bHeldMovement[RelativeDirection];
	bHeldMovement[RelativeDirection] = bHeld;
	if (bHeld)
	{
		ActiveHeldDirection = RelativeDirection;
		if (!bWasHeld)
		{
			RequestRelative(RelativeDirection);
			LastWalkIntentTime = FPlatformTime::Seconds();
		}
	}
	else if (ActiveHeldDirection == RelativeDirection)
	{
		for (uint8 Direction = 0; Direction < 4; ++Direction)
		{
			if (bHeldMovement[Direction]) ActiveHeldDirection = Direction;
		}
	}
}

void AReal33DPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HudRoot.IsValid())
	{
		const AReal33DWorld* World = GetWorldActor();
		const FReal33DPlayerVitals Vitals = World != nullptr
			? World->GetPlayerVitals() : FReal33DPlayerVitals{};
		// One call, one owner: the HUD hands each panel the values it draws.
		HudRoot->Refresh(World);

		// The log records transitions, not frames. The values are printed
		// exactly as the server sent them, unclamped, so the evidence file
		// still shows a wire-level fault rather than a tidied-up version.
		const FString Text = Vitals.bKnown
			? FString::Printf(TEXT("HP %d/%d   Mana %d/%d   Level %d"),
				static_cast<int32>(Vitals.Hitpoints),
				static_cast<int32>(Vitals.MaxHitpoints),
				static_cast<int32>(Vitals.Mana),
				static_cast<int32>(Vitals.MaxMana),
				static_cast<int32>(Vitals.Level))
			: TEXT("HP --/--   Mana --/--   Level --");
		if (Text != LastVitalsText)
		{
			LastVitalsText = Text;
			UE_LOG(LogReal33D, Log, TEXT("HUD vitals: %s"), *Text);
		}
	}
	if (InspectorNote.IsValid() && InspectorNote->HasKeyboardFocus())
	{
		for (bool& bHeld : bHeldMovement) bHeld = false;
		return;
	}
	if (!bHeldMovement[ActiveHeldDirection] || bTypingActive) return;
	const double Now = FPlatformTime::Seconds();
	if (Now - LastWalkIntentTime >= HeldWalkIntervalSeconds)
	{
		RequestRelative(ActiveHeldDirection);
		LastWalkIntentTime = Now;
	}
}

void AReal33DPlayerController::RequestRelative(uint8 RelativeDirection)
{
	if (AReal33DWorld* World = GetWorldActor())
	{
		Request(World->CameraRelativeDirection(RelativeDirection));
	}
}

namespace
{
	/**
	 * Degrees of swing per unit of mouse movement.
	 *
	 * Chosen so a drag across a 1280-wide window is about 512 degrees,
	 * which is enough to get behind a creature without the view feeling like it
	 * is on ice. Not a measured value and nothing depends on it being exact.
	 */
	constexpr float kOrbitDegreesPerUnit = 0.40f;

	/** World units of camera distance per wheel notch. */
	constexpr float kZoomUnitsPerNotch = 120.0f;

	/** Pointer units before a right-button gesture stops being a click. */
	constexpr float kOrbitDragThreshold = 3.0f;
}

AReal33DWorld* AReal33DPlayerController::GetWorldActor()
{
	if (AReal33DWorld* Cached = WorldActor.Get())
	{
		return Cached;
	}
	for (TActorIterator<AReal33DWorld> It(GetWorld()); It; ++It)
	{
		WorldActor = *It;
		return *It;
	}
	return nullptr;
}

void AReal33DPlayerController::BeginOrbit()
{
	bOrbiting = true;
	bRightMouseDragged = false;
	PendingOrbitYaw = 0.0f;
	PendingOrbitPitch = 0.0f;
}

void AReal33DPlayerController::EndOrbit()
{
	const bool bWasDrag = bRightMouseDragged;
	bOrbiting = false;
	bRightMouseDragged = false;
	PendingOrbitYaw = 0.0f;
	PendingOrbitPitch = 0.0f;
	if (!bWasDrag)
	{
		InteractUnderCursor();
	}
}

void AReal33DPlayerController::OrbitYaw(float Value)
{
	if (!bOrbiting || Value == 0.0f)
	{
		return;
	}
	PendingOrbitYaw += Value;
	if (!bRightMouseDragged
		&& FMath::Abs(PendingOrbitYaw) + FMath::Abs(PendingOrbitPitch)
			>= kOrbitDragThreshold)
	{
		bRightMouseDragged = true;
	}
	if (bRightMouseDragged)
	{
		if (AReal33DWorld* World = GetWorldActor())
		{
			World->AddCameraOrbit(PendingOrbitYaw * kOrbitDegreesPerUnit,
				-PendingOrbitPitch * kOrbitDegreesPerUnit);
		}
		PendingOrbitYaw = 0.0f;
		PendingOrbitPitch = 0.0f;
	}
}

void AReal33DPlayerController::OrbitPitch(float Value)
{
	if (!bOrbiting || Value == 0.0f)
	{
		return;
	}
	PendingOrbitPitch += Value;
	if (!bRightMouseDragged
		&& FMath::Abs(PendingOrbitYaw) + FMath::Abs(PendingOrbitPitch)
			>= kOrbitDragThreshold)
	{
		bRightMouseDragged = true;
	}
	if (bRightMouseDragged)
	{
		// Negated so dragging down brings the camera towards eye level and
		// dragging up lifts it towards looking straight down. Reading a speech
		// tag above a creature is a shallow-angle job, and down is the
		// direction an operator reaches for when they want to see a face.
		if (AReal33DWorld* World = GetWorldActor())
		{
			World->AddCameraOrbit(PendingOrbitYaw * kOrbitDegreesPerUnit,
				-PendingOrbitPitch * kOrbitDegreesPerUnit);
		}
		PendingOrbitYaw = 0.0f;
		PendingOrbitPitch = 0.0f;
	}
}

void AReal33DPlayerController::ZoomCamera(float Value)
{
	if (Value == 0.0f)
	{
		return;
	}
	if (AReal33DWorld* World = GetWorldActor())
	{
		// Wheel forward is positive and means closer, so the sign flips.
		World->AddCameraDistance(-Value * kZoomUnitsPerNotch);
	}
}

void AReal33DPlayerController::DumpEvidence()
{
	for (TActorIterator<AReal33DWorld> It(GetWorld()); It; ++It)
	{
		It->WriteEvidence(TEXT("Manual"));
		UE_LOG(LogReal33D, Log, TEXT("manual evidence snapshot requested"));
		return;
	}
}

void AReal33DPlayerController::InspectUnderCursor()
{
	FHitResult Hit;
	if (!GetHitResultUnderCursorByChannel(
		UEngineTypes::ConvertToTraceType(ECC_Visibility), true, Hit)) return;

	if (const AReal33DCreature* Creature = Cast<AReal33DCreature>(Hit.GetActor()))
	{
		const uint32 CreatureId = Creature->GetCreatureId();
		if (HudRoot.IsValid()) HudRoot->CompleteUseOnCreature(CreatureId);
		return;
	}

	const AReal33DTile* Tile = Cast<AReal33DTile>(Hit.GetActor());
	if (Tile == nullptr || Hit.GetComponent() == nullptr) return;
	uint16 TopTypeId = 0;
	uint8 TopStackIndex = 0;
	if (Tile->GetTopObject(TopTypeId, TopStackIndex)
		&& HudRoot.IsValid()
		&& HudRoot->CompleteUseOnField(Tile->GetMapPosition(), TopTypeId, TopStackIndex))
	{
		return;
	}

	if (!bInspectorEnabled || !InspectorLabel.IsValid()) return;

	uint16 TypeId = 0;
	bool bFoundTag = false;
	for (const FName& Tag : Hit.GetComponent()->ComponentTags)
	{
		const FString Text = Tag.ToString();
		if (Text.StartsWith(TEXT("V08_")))
		{
			TypeId = static_cast<uint16>(FCString::Atoi(*Text.RightChop(4)));
			bFoundTag = true;
			break;
		}
	}
	if (!bFoundTag) return;

	const AReal33DWorld* World = GetWorldActor();
	const UReal33DAssetRegistry* Registry = World ? World->GetAssetRegistry() : nullptr;
	if (Registry == nullptr) return;
	const FReal33DExperimentalCatalogEntry* Entry = Registry->GetExperimentalCatalog().FindByPredicate(
		[TypeId](const FReal33DExperimentalCatalogEntry& Candidate)
		{ return Candidate.TypeId == TypeId; });
	if (Entry == nullptr) return;

	const Real33D::FMapPosition& Position = Tile->GetMapPosition();
	InspectorTypeId = TypeId;
	InspectorName = Entry->Name;
	InspectorStatus = Entry->RefinementStatus;
	InspectorPosition = FString::Printf(TEXT("%d,%d,%d"), Position.X, Position.Y, Position.Z);
	bInspectorSelection = true;
	const FReal33DVisual Display = Registry->ResolveThing(TypeId, false);
	const FString SourceLabel = Display.VisualSourceTypeId != 0
		&& Display.VisualSourceTypeId != TypeId
		? FString::Printf(TEXT(" | mesh %05u"), Display.VisualSourceTypeId) : FString();
	InspectorLabel->SetText(FText::FromString(FString::Printf(
		TEXT("ID %05u | %s | %s | tile %s%s"), TypeId,
		*InspectorName, *InspectorStatus, *InspectorPosition, *SourceLabel)));
	if (InspectorNote.IsValid()) InspectorNote->SetText(FText::GetEmpty());
	UE_LOG(LogReal33D, Log, TEXT("V08 inspector: TypeId=%u name=%s status=%s tile=%s"),
		TypeId, *InspectorName, *InspectorStatus, *InspectorPosition);
}

void AReal33DPlayerController::InteractUnderCursor()
{
	FHitResult Hit;
	if (!GetHitResultUnderCursorByChannel(
		UEngineTypes::ConvertToTraceType(ECC_Visibility), true, Hit)) return;

	if (const AReal33DCreature* Creature = Cast<AReal33DCreature>(Hit.GetActor()))
	{
		const uint32 CreatureId = Creature->GetCreatureId();
		if (HudRoot.IsValid() && HudRoot->CompleteUseOnCreature(CreatureId))
		{
			return;
		}
		UGameInstance* GameInstance = GetGameInstance();
		UReal33DBridge* Bridge = GameInstance != nullptr
			? GameInstance->GetSubsystem<UReal33DBridge>() : nullptr;
		if (Bridge == nullptr || !Bridge->IsRunning() || Creature->IsLocalPlayer())
		{
			return;
		}
		const uint32 ActionId = Bridge->RequestAttack(CreatureId);
		UE_LOG(LogReal33D, Log,
			TEXT("combat input %u from world right-click: attack creature %u"),
			ActionId, CreatureId);
		return;
	}

	const AReal33DTile* Tile = Cast<AReal33DTile>(Hit.GetActor());
	if (Tile == nullptr || !HudRoot.IsValid()) return;
	uint16 TypeId = 0;
	uint8 StackIndex = 0;
	if (Tile->GetTopObject(TypeId, StackIndex))
	{
		HudRoot->UseWorldObject(Tile->GetMapPosition(), TypeId, StackIndex);
	}
}

void AReal33DPlayerController::SaveInspectorNote(const FString& Verdict)
{
	if (!bInspectorEnabled || !bInspectorSelection || !InspectorLabel.IsValid())
	{
		if (InspectorLabel.IsValid()) InspectorLabel->SetText(
			FText::FromString(TEXT("Selecciona un objeto V08 antes de anotar.")));
		return;
	}
	FString Note = InspectorNote.IsValid() ? InspectorNote->GetText().ToString() : FString();
	Note.ReplaceInline(TEXT("\t"), TEXT(" "));
	Note.ReplaceInline(TEXT("\r"), TEXT(" "));
	Note.ReplaceInline(TEXT("\n"), TEXT(" "));
	const FString Directory = FPaths::GetPath(InspectorNotesPath);
	IFileManager::Get().MakeDirectory(*Directory, true);
	const bool bExists = IFileManager::Get().FileExists(*InspectorNotesPath);
	FString Line;
	if (!bExists) Line = TEXT("utc\ttype_id\tname\trefinement_status\tmap_position\tverdict\tnote\n");
	Line += FString::Printf(TEXT("%s\t%u\t%s\t%s\t%s\t%s\t%s\n"),
		*FDateTime::UtcNow().ToIso8601(), InspectorTypeId, *InspectorName,
		*InspectorStatus, *InspectorPosition, *Verdict, *Note);
	const bool bSaved = FFileHelper::SaveStringToFile(Line, *InspectorNotesPath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
	InspectorLabel->SetText(FText::FromString(FString::Printf(
		TEXT("ID %05u | %s | %s"), InspectorTypeId,
		bSaved ? TEXT("nota guardada") : TEXT("ERROR al guardar"), *InspectorPosition)));
	if (bSaved)
	{
		UE_LOG(LogReal33D, Log, TEXT("V08 inspector note: TypeId=%u verdict=%s path=%s"),
			InspectorTypeId, *Verdict, *InspectorNotesPath);
	}
	else
	{
		UE_LOG(LogReal33D, Error, TEXT("V08 inspector note could not be saved: %s"),
			*InspectorNotesPath);
	}
}
