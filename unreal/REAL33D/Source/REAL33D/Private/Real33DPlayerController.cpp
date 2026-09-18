#include "Real33DPlayerController.h"

#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "REAL33D.h"
#include "Real33DBridge.h"
#include "Real33DWorldActor.h"

AReal33DPlayerController::AReal33DPlayerController()
{
	bShowMouseCursor = false;
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

	// Chat line control.
	//
	// Enter says, F2 yells, F3 whispers. The mode is chosen by the key rather
	// than a typed "#y " prefix, because "#" is unbound here and needs a
	// modifier on most layouts, which made yell unreachable entirely.
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this,
		&AReal33DPlayerController::ToggleChat);
	InputComponent->BindKey(EKeys::F2, IE_Pressed, this,
		&AReal33DPlayerController::CycleTalkMode);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this,
		&AReal33DPlayerController::CancelChat);
	InputComponent->BindKey(EKeys::BackSpace, IE_Pressed, this,
		&AReal33DPlayerController::Backspace);

	// Typing. Bound once per key with the character it produces, because
	// UInputComponent reports keys and not characters: there is no character
	// event to subscribe to from a plain PlayerController. Lower case and
	// digits are enough to type a test phrase, and this is a test harness for
	// the protocol path rather than a chat client.
	const FString Letters = TEXT("abcdefghijklmnopqrstuvwxyz");
	for (int32 Index = 0; Index < Letters.Len(); ++Index)
	{
		BindTypingKey(FKey(*FString::Chr(FChar::ToUpper(Letters[Index]))), Letters[Index]);
	}
	static const FKey Digits[] = { EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three,
		EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
	for (int32 Index = 0; Index < 10; ++Index)
	{
		BindTypingKey(Digits[Index], static_cast<TCHAR>(TEXT('0') + Index));
	}
	BindTypingKey(EKeys::SpaceBar, TEXT(' '));
}

void AReal33DPlayerController::BindTypingKey(const FKey& Key, TCHAR Glyph)
{
	// Bound manually because BindKey has no payload overload, and one handler
	// per letter would be twenty-six near-identical functions.
	FInputKeyBinding Binding(FInputChord(Key), IE_Pressed);
	Binding.KeyDelegate.GetDelegateForManualSet().BindLambda(
		[this, Glyph]() { TypeCharacter(Glyph); });
	InputComponent->KeyBindings.Emplace(MoveTemp(Binding));
}

void AReal33DPlayerController::TypeCharacter(TCHAR Glyph)
{
	if (!bComposing)
	{
		return;
	}
	// reference/game/src/receiving.cc::CTalk reads into char Text[256]; the
	// builder refuses anything longer, so stop here rather than let the player
	// type a line that will be rejected.
	if (Composing.Len() >= 255)
	{
		return;
	}
	Composing.AppendChar(Glyph);
}

const TCHAR* AReal33DPlayerController::TalkModePrefix() const
{
	switch (TalkMode)
	{
	case 1:  return TEXT("#w ");
	case 2:  return TEXT("#y ");
	default: return TEXT("");
	}
}

const TCHAR* AReal33DPlayerController::TalkModeLabel() const
{
	switch (TalkMode)
	{
	case 1:  return TEXT("whisper");
	case 2:  return TEXT("yell");
	default: return TEXT("say");
	}
}

void AReal33DPlayerController::CycleTalkMode()
{
	// Changing mode does not open or close a line: the mode is a property of the
	// player, not of the message being typed. Cycling mid-line is therefore
	// allowed and simply changes how the line will be sent.
	TalkMode = static_cast<uint8>((TalkMode + 1) % 3);
	UE_LOG(LogReal33D, Log, TEXT("talk mode is now %s"), TalkModeLabel());
}

void AReal33DPlayerController::ToggleChat() { OpenOrSend(); }

void AReal33DPlayerController::OpenOrSend()
{
	if (!bComposing)
	{
		bComposing = true;
		// Only what the player types is held here. The mode is applied on send,
		// not seeded into the buffer, so cycling the mode while a line is open
		// changes how it goes out instead of leaving a stale prefix behind.
		Composing.Empty();
		UE_LOG(LogReal33D, Log, TEXT("%s line opened; walk keys are inert"),
			TalkModeLabel());
		return;
	}

	// Closing. Send only if there is something to send: an empty line is what
	// CTalk refuses, so treat Enter on an empty line as simply closing it.
	const FString Text = Composing;
	bComposing = false;
	Composing.Empty();
	if (Text.IsEmpty())
	{
		UE_LOG(LogReal33D, Log, TEXT("chat line closed empty; nothing sent"));
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UReal33DBridge* Bridge = GameInstance != nullptr
		? GameInstance->GetSubsystem<UReal33DBridge>() : nullptr;
	if (Bridge == nullptr || !Bridge->IsRunning())
	{
		UE_LOG(LogReal33D, Warning, TEXT("not connected; \"%s\" not sent"), *Text);
		return;
	}

	// An intent, exactly like a walk. Fusion32 decides whether anyone hears it,
	// and the authoritative talk it broadcasts is what this client will draw.
	// The mode is applied here, at send, so it reflects whatever the mode is now
	// rather than what it was when the line was opened.
	const uint32 SayId = Bridge->RequestSay(FString(TalkModePrefix()) + Text);
	UE_LOG(LogReal33D, Log, TEXT("%s %u: \"%s\" handed to Fusion32"),
		TalkModeLabel(), SayId, *Text);
}

void AReal33DPlayerController::CancelChat()
{
	if (!bComposing)
	{
		return;
	}
	bComposing = false;
	Composing.Empty();
	UE_LOG(LogReal33D, Log, TEXT("chat line abandoned"));
}

void AReal33DPlayerController::Backspace()
{
	if (bComposing && Composing.Len() > 0)
	{
		Composing.LeftChopInline(1);
	}
}

void AReal33DPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (GEngine == nullptr)
	{
		return;
	}

	// The line has to be visible while it is typed, or the operator is typing
	// blind into a client that looks identical whether chat is open or not. The
	// prompt names the mode, because a mode that persists and is invisible is a
	// mode that sends the wrong thing: the player would have no way to know a
	// yell three messages ago is still in force.
	if (bComposing)
	{
		GEngine->AddOnScreenDebugMessage(200, 0.0f, FColor(120, 220, 255),
			FString::Printf(TEXT("%s> %s_"), TalkModeLabel(), *Composing));
	}
	else if (TalkMode != 0)
	{
		// Shown even with no line open, for the same reason.
		GEngine->AddOnScreenDebugMessage(201, 0.0f, FColor(200, 200, 120),
			FString::Printf(TEXT("talk mode: %s  (F2 to change)"), TalkModeLabel()));
	}
}

void AReal33DPlayerController::Request(uint8 Direction)
{
	// The one place the rule lives: while a chat line is open, walk keys are
	// letters. Typing "was" must not walk the player west, north and south.
	if (bComposing)
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

void AReal33DPlayerController::DumpEvidence()
{
	for (TActorIterator<AReal33DWorld> It(GetWorld()); It; ++It)
	{
		It->WriteEvidence(TEXT("Manual"));
		UE_LOG(LogReal33D, Log, TEXT("manual evidence snapshot requested"));
		return;
	}
}
