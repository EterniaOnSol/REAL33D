#include "Real33DPlayerController.h"

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
}

void AReal33DPlayerController::Request(uint8 Direction)
{
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
