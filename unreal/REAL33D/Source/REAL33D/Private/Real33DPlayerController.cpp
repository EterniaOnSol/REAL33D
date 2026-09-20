#include "Real33DPlayerController.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "REAL33D.h"
#include "Real33DBridge.h"
#include "Real33DChatPanel.h"
#include "Real33DWorldActor.h"
#include "Widgets/Layout/SBox.h"

AReal33DPlayerController::AReal33DPlayerController()
{
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

	ChatPanel = SNew(SReal33DChatPanel)
		.Bridge(Bridge)
		.OnTypingChanged(FReal33DOnTypingChanged::CreateUObject(
			this, &AReal33DPlayerController::HandleTypingChanged));

	// Bottom left, where a chat console belongs and where it cannot be confused
	// with the counters overlay in the top left. Above the scene, below nothing.
	ChatRoot = SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(12.0f, 0.0f, 0.0f, 12.0f))
		[
			ChatPanel.ToSharedRef()
		];
	GetWorld()->GetGameViewport()->AddViewportWidgetContent(
		ChatRoot.ToSharedRef(), /*ZOrder=*/10);

	SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
}

void AReal33DPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	// Removed explicitly. A viewport widget outlives the actor that made it,
	// so a second session would otherwise open on top of the first one's panel
	// and the operator would be typing into a box wired to a dead bridge.
	if (ChatRoot.IsValid() && GetWorld() != nullptr
		&& GetWorld()->GetGameViewport() != nullptr)
	{
		GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(
			ChatRoot.ToSharedRef());
	}
	ChatRoot.Reset();
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
	InputComponent->BindKey(EKeys::W, IE_Pressed, this, &AReal33DPlayerController::WalkNorth);
	InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &AReal33DPlayerController::WalkNorth);
	InputComponent->BindKey(EKeys::D, IE_Pressed, this, &AReal33DPlayerController::WalkEast);
	InputComponent->BindKey(EKeys::Right, IE_Pressed, this, &AReal33DPlayerController::WalkEast);
	InputComponent->BindKey(EKeys::S, IE_Pressed, this, &AReal33DPlayerController::WalkSouth);
	InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &AReal33DPlayerController::WalkSouth);
	InputComponent->BindKey(EKeys::A, IE_Pressed, this, &AReal33DPlayerController::WalkWest);
	InputComponent->BindKey(EKeys::Left, IE_Pressed, this, &AReal33DPlayerController::WalkWest);

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
	if (!ChatPanel.IsValid())
	{
		return;
	}
	// EndTyping fires the change, which returns focus to the viewport. Calling
	// it when nothing was open is harmless: it is idempotent by design.
	ChatPanel->EndTyping();
}

void AReal33DPlayerController::HandleTypingChanged(bool bTyping)
{
	bTypingActive = bTyping;
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
	if (bTypingActive)
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

namespace
{
	/**
	 * Degrees of swing per unit of mouse movement.
	 *
	 * Chosen so a drag across a 1280-wide window is a little over a full turn,
	 * which is enough to get behind a creature without the view feeling like it
	 * is on ice. Not a measured value and nothing depends on it being exact.
	 */
	constexpr float kOrbitDegreesPerUnit = 0.35f;

	/** World units of camera distance per wheel notch. */
	constexpr float kZoomUnitsPerNotch = 120.0f;
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

void AReal33DPlayerController::BeginOrbit() { bOrbiting = true; }
void AReal33DPlayerController::EndOrbit()   { bOrbiting = false; }

void AReal33DPlayerController::OrbitYaw(float Value)
{
	if (!bOrbiting || Value == 0.0f)
	{
		return;
	}
	if (AReal33DWorld* World = GetWorldActor())
	{
		World->AddCameraOrbit(Value * kOrbitDegreesPerUnit, 0.0f);
	}
}

void AReal33DPlayerController::OrbitPitch(float Value)
{
	if (!bOrbiting || Value == 0.0f)
	{
		return;
	}
	if (AReal33DWorld* World = GetWorldActor())
	{
		// Negated so dragging down brings the camera towards eye level and
		// dragging up lifts it towards looking straight down. Reading a speech
		// tag above a creature is a shallow-angle job, and down is the
		// direction an operator reaches for when they want to see a face.
		World->AddCameraOrbit(0.0f, -Value * kOrbitDegreesPerUnit);
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
