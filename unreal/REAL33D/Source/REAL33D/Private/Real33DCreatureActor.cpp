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

	/**
	 * Speech height in world units at the reference camera distance.
	 *
	 * Text drawn at a fixed world size shrinks as the camera pulls back, which
	 * is how a message ends up unreadable exactly when a lot is going on. The
	 * size is rescaled each frame so what the operator sees stays the same
	 * regardless of distance.
	 */
	constexpr float kSpeechBaseWorldSize = 44.0f;
	constexpr double kSpeechReferenceDistance = 1100.0;
	constexpr float kSpeechMinWorldSize = 28.0f;
	constexpr float kSpeechMaxWorldSize = 150.0f;
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

	// Above the name, so a creature reads bottom-up as body, name, speech.
	SpeechTag = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SpeechTag"));
	SpeechTag->SetupAttachment(Root);
	SpeechTag->SetRelativeLocation(FVector(0.0, 0.0, 140.0));
	SpeechTag->SetHorizontalAlignment(EHTA_Center);
	SpeechTag->SetWorldSize(kSpeechBaseWorldSize);
	SpeechTag->SetTextRenderColor(FColor(255, 255, 0));
	SpeechTag->SetVisibility(false);

	// The default text material is deliberately left in place.
	//
	// It is lit, so the colour set above is modulated by scene lighting and the
	// blue ambient from the sky light: pure yellow reaches the screen closer to
	// a warm off-white than to #ffff00. That was measured, not assumed, at
	// roughly (180,171,138) before the tone curve was disabled.
	//
	// The obvious fix is worse. /Engine/EngineMaterials/UnlitText gives the
	// exact colour but ignores the font texture's alpha, so every character
	// renders as a filled quad and nothing is readable at all. Readable text in
	// an approximate colour beats an exact colour nobody can read.
	//
	// Getting both needs an authored unlit, alpha-masked text material, which
	// is content work and belongs with the visual pipeline rather than here.
	// Recorded in docs/UNREAL_CHAT.md as a known cosmetic limitation.
}

void AReal33DCreature::ShowSpeech(const FString& Text, float Seconds)
{
	check(IsInGameThread());
	// Newest message replaces the previous one. See docs/UNREAL_CHAT.md: the
	// classic client's behaviour for messages arriving in quick succession is
	// not established from source, so this takes the smallest safe option
	// rather than inventing a stacking rule and calling it parity.
	SpeechTag->SetText(FText::FromString(Text));
	SpeechTag->SetVisibility(true);
	SpeechExpiresAt = FPlatformTime::Seconds() + static_cast<double>(Seconds);
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
		const FRotator Facing(0.0, ViewRotation.Yaw + 180.0, 0.0);
		NameTag->SetWorldRotation(Facing);
		SpeechTag->SetWorldRotation(Facing);

		// Keep the apparent size constant, so speech reads the same however far
		// the camera happens to be.
		if (SpeechExpiresAt > 0.0)
		{
			const double Distance = FVector::Dist(ViewLocation, SpeechTag->GetComponentLocation());
			const float Scaled = static_cast<float>(
				kSpeechBaseWorldSize * (Distance / kSpeechReferenceDistance));
			SpeechTag->SetWorldSize(
				FMath::Clamp(Scaled, kSpeechMinWorldSize, kSpeechMaxWorldSize));
		}
	}

	if (SpeechExpiresAt > 0.0 && FPlatformTime::Seconds() >= SpeechExpiresAt)
	{
		SpeechTag->SetVisibility(false);
		SpeechTag->SetText(FText::GetEmpty());
		SpeechExpiresAt = 0.0;
	}

	if (!bPlaced || DrawnLocation.Equals(TargetLocation, 0.05))
	{
		return;
	}
	DrawnLocation = FMath::VInterpTo(DrawnLocation, TargetLocation, DeltaSeconds,
		InterpolationRate);
	SetActorLocation(DrawnLocation);
}
