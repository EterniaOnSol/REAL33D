#include "Real33DGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "Components/SkyAtmosphereComponent.h"
#include "EngineUtils.h"
#include "REAL33D.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Real33DGalleryActor.h"
#include "Real33DPlayerController.h"
#include "Real33DWorldActor.h"

namespace
{
	void SpawnLight(UWorld* World, const FRotator& Rotation, float Intensity,
		const FLinearColor& Colour, const TCHAR* Label, int32 ForwardPriority)
	{
		ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(
			ADirectionalLight::StaticClass(), FVector(0.0, 0.0, 2000.0), Rotation);
		if (Light == nullptr)
		{
			return;
		}
		Light->SetMobility(EComponentMobility::Movable);
#if WITH_EDITOR
		Light->SetActorLabel(Label);
#else
		(void)Label;
#endif
		if (UDirectionalLightComponent* Component =
				Cast<UDirectionalLightComponent>(Light->GetLightComponent()))
		{
			Component->SetIntensity(Intensity);
			Component->SetLightColor(Colour);
			// Says outright which of the two is the key light, instead of
			// leaving the renderer to warn that they are competing.
			Component->ForwardShadingPriority = ForwardPriority;
		}
	}
}

AReal33DGameMode::AReal33DGameMode()
{
	PlayerControllerClass = AReal33DPlayerController::StaticClass();
	// There is no pawn. The player's body is a creature actor owned by the
	// world presenter, and it is placed by WorldState, not by a movement
	// component. Giving the controller a pawn would put a second thing on
	// screen that Fusion32 knows nothing about.
	DefaultPawnClass = nullptr;
	bStartPlayersAsSpectators = false;
}

void AReal33DGameMode::StartPlay()
{
	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		const bool bGallery = FParse::Param(FCommandLine::Get(), TEXT("real33d-gallery"));
		bool bHasWorldActor = bGallery;
		if (bGallery)
		{
			World->SpawnActor<AReal33DGalleryActor>(AReal33DGalleryActor::StaticClass(),
				FVector::ZeroVector, FRotator::ZeroRotator);
		}
		for (TActorIterator<AReal33DWorld> It(World); It; ++It)
		{
			bHasWorldActor = true;
			break;
		}
		if (!bHasWorldActor)
		{
			World->SpawnActor<AReal33DWorld>(AReal33DWorld::StaticClass(),
				FVector::ZeroVector, FRotator::ZeroRotator);
		}

		// One directional light, not two.
		//
		// A sky light on an otherwise empty map has nothing to capture and
		// leaves the scene black, so a fill light was the obvious answer. But a
		// second directional light makes the renderer warn, every frame, that
		// two lights are competing for forward shading, and that warning is
		// drawn straight through the diagnostic overlay this run is read from.
		// Legible numbers are worth more here than a lit back face.
		SpawnLight(World, FRotator(-52.0, 35.0, 0.0), 3.4f,
			FLinearColor::White, TEXT("Key"), 1);

		// The fill comes from the sky instead. An atmosphere gives the sky
		// light something real to capture, so faces turned away from the sun
		// still read, and neither actor needs an asset from disk.
		World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(),
			FVector::ZeroVector, FRotator::ZeroRotator);
		if (ASkyLight* Sky = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(),
				FVector(0.0, 0.0, 2000.0), FRotator::ZeroRotator))
		{
			if (USkyLightComponent* Component = Sky->GetLightComponent())
			{
				Component->SetMobility(EComponentMobility::Movable);
				Component->SourceType = ESkyLightSourceType::SLS_CapturedScene;
				Component->bLowerHemisphereIsBlack = false;
				Component->SetIntensity(1.6f);
				Component->RecaptureSky();
			}
		}

		UE_LOG(LogReal33D, Log, TEXT("scene built from code"));
	}
	Super::StartPlay();
}
