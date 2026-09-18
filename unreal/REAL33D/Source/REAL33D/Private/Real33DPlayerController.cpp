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
		&AReal33DPlayerController::ToggleYell);
	InputComponent->BindKey(EKeys::F3, IE_Pressed, this,
		&AReal33DPlayerController::ToggleWhisper);
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

void AReal33DPlayerController::ToggleChat()    { OpenOrSend(TEXT(""), TEXT("say")); }
void AReal33DPlayerController::ToggleYell()    { OpenOrSend(TEXT("#y "), TEXT("yell")); }
void AReal33DPlayerController::ToggleWhisper() { OpenOrSend(TEXT("#w "), TEXT("whisper")); }

void AReal33DPlayerController::OpenOrSend(const TCHAR* Prefix, const TCHAR* Label)
{
	if (!bComposing)
	{
		bComposing = true;
		// The prefix is seeded rather than typed, so the mode is chosen by the
		// key that opened the line. The bridge still parses the prefix, which
		// keeps one code path for both the key and the classic convention.
		Composing = Prefix;
		UE_LOG(LogReal33D, Log, TEXT("%s line opened; walk keys are inert"), Label);
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
	const uint32 SayId = Bridge->RequestSay(Text);
	UE_LOG(LogReal33D, Log, TEXT("say %u: \"%s\" handed to Fusion32"), SayId, *Text);
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

	// The line has to be visible while it is typed, or the operator is typing
	// blind into a client that looks identical whether chat is open or not.
	if (bComposing && GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(200, 0.0f, FColor(120, 220, 255),
			FString::Printf(TEXT("say> %s_"), *Composing));
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
