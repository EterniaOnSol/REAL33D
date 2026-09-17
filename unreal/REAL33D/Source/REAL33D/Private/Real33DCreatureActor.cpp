#include "Real33DCreatureActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/**
	 * How fast the drawn position catches up with the logical one.
	 *
	 * A step in Fusion32 takes at least a few hundred milliseconds, so easing
	 * over roughly a sixth of a second stays well inside one step and cannot
	 * make the body lag a whole field behind. This is presentation only: the
	 * logical position has already changed by the time this runs.
	 */
	constexpr double InterpolationRate = 6.0;
}

AReal33DCreature::AReal33DCreature()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	NameTag = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameTag"));
	NameTag->SetupAttachment(Root);
	NameTag->SetRelativeLocation(FVector(0.0, 0.0, 110.0));
	NameTag->SetHorizontalAlignment(EHTA_Center);
	NameTag->SetWorldSize(18.0f);
}

void AReal33DCreature::Configure(uint32 InCreatureId, bool bInIsLocalPlayer,
	const FString& InName, const UReal33DAssetRegistry* Registry)
{
	CreatureId = InCreatureId;
	bIsLocalPlayer = bInIsLocalPlayer;

	// A creature the viewport revealed mid-move can arrive without a name. Show
	// the id rather than an empty tag, so the evidence run can still tell two
	// bodies apart.
	NameTag->SetText(FText::FromString(
		InName.IsEmpty() ? FString::Printf(TEXT("#%u"), InCreatureId) : InName));
	NameTag->SetTextRenderColor(bIsLocalPlayer ? FColor(80, 180, 255) : FColor(255, 210, 160));

	if (Registry != nullptr && Registry->IsReady())
	{
		const FReal33DVisual Visual = Registry->ResolveCreature(InCreatureId, bIsLocalPlayer);
		Body->SetStaticMesh(Visual.Mesh);
		Body->SetRelativeScale3D(Visual.Scale);
		Body->SetRelativeLocation(Visual.Offset);
		if (Visual.Material != nullptr)
		{
			UMaterialInstanceDynamic* Dynamic =
				UMaterialInstanceDynamic::Create(Visual.Material, Body);
			if (Dynamic != nullptr)
			{
				Dynamic->SetVectorParameterValue(TEXT("Color"), Visual.Tint);
				Body->SetMaterial(0, Dynamic);
			}
		}
	}
}

void AReal33DCreature::CommitPosition(const Real33D::FWorldOrigin& Origin,
	const Real33D::FMapPosition& Position, bool bSnap)
{
	LogicalPosition = Position;
	TargetLocation = Real33D::ToWorld(Origin, Position);

	// A floor change is never a walk across the scene, and neither is the first
	// placement or a re-anchor. Ease only what is genuinely one step.
	const bool bFar = !bPlaced
		|| FVector::DistSquared(DrawnLocation, TargetLocation)
			> FMath::Square(Real33D::UnitsPerSqm * 1.5);
	if (bSnap || bFar)
	{
		DrawnLocation = TargetLocation;
	}
	bPlaced = true;
	SetActorLocation(DrawnLocation);
}

void AReal33DCreature::SetFacing(uint8 Direction)
{
	SetActorRotation(Real33D::ToRotation(Direction));
}

void AReal33DCreature::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The tag must stay readable whichever way the body faces, so it is turned
	// back towards the camera rather than inheriting the creature's rotation.
	if (const APlayerController* Controller = GetWorld() != nullptr
			? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
		NameTag->SetWorldRotation(FRotator(0.0, ViewRotation.Yaw + 180.0, 0.0));
	}

	if (!bPlaced || DrawnLocation.Equals(TargetLocation, 0.05))
	{
		return;
	}
	DrawnLocation = FMath::VInterpTo(DrawnLocation, TargetLocation, DeltaSeconds,
		InterpolationRate);
	SetActorLocation(DrawnLocation);
}
