#include "Real33DCreatureActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** Constant visual travel avoids a deceleration and restart at each tile. */
	constexpr double WalkVisualSpeed = 220.0; // Unreal units per second
	constexpr double WalkBobHeight = 3.0;

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
	// Query-only: clicking a creature must be able to name the same id the
	// battle list names, but presentation collision must never move or block it.
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

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

	TargetTag = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TargetTag"));
	TargetTag->SetupAttachment(Root);
	TargetTag->SetRelativeLocation(FVector(0.0, 0.0, 86.0));
	TargetTag->SetHorizontalAlignment(EHTA_Center);
	TargetTag->SetWorldSize(14.0f);
	TargetTag->SetVisibility(false);

	// Text is drawn with the component's default material on purpose, and the
	// lighting is what was changed instead.
	//
	// Three engine text materials exist and none is both solid and colour-exact.
	// DefaultTextMaterialOpaque is solid but lit, so the colour is modulated by
	// the scene. UnlitText is colour-exact but ignores the font alpha, turning
	// every glyph into a filled quad. AntiAliasedTextMaterialTranslucent keeps
	// the glyphs and the colour but is alpha-blended, and reads as faint.
	//
	// Since the material must stay lit to stay solid, the fix is to stop the
	// lighting from tinting it: AReal33DGameMode's key light is white and the
	// sky light's blue ambient is gone, so a lit white surface reproduces its
	// own colour closely. Measured before that change, #ffff00 arrived at about
	// (180,171,138); the remaining error is brightness rather than hue.
	//
	// A genuinely correct fix is an authored unlit, alpha-masked text material,
	// which is content work for the visual pipeline. Recorded in
	// docs/UNREAL_CHAT.md.
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

void AReal33DCreature::SetHealthPercent(uint8 Percent)
{
	check(IsInGameThread());
	HealthPercent = Percent;

	// Bands specified by the operator, not derived from Fusion32: the server
	// sends a percentage and says nothing about colour. Recorded as
	// NOT_PROVEN in docs/UNREAL_CHAT.md rather than presented as 7.72 parity.
	//
	// Black last, because a creature at zero is about to stop existing and the
	// name should read as such rather than quietly turning invisible.
	// Four bands, as the operator specified: green, yellow, red, black.
	//
	// The first attempt put yellow at 60 and above, which sent a player sitting
	// at 58% straight past yellow into red and looked like a defect in the
	// health pipeline. It was not: the server was reporting 58% correctly. The
	// bands were simply wrong, so the halfway point now sits inside yellow
	// where a half-health creature belongs.
	FColor Colour;
	if (Percent >= 95)
	{
		Colour = FColor(0, 220, 0);        // effectively full
	}
	else if (Percent >= 40)
	{
		Colour = FColor(235, 235, 0);      // hurt
	}
	else if (Percent > 0)
	{
		Colour = FColor(230, 0, 0);        // badly hurt
	}
	else
	{
		Colour = FColor(10, 10, 10);       // dead, or health never reported
	}
	NameTag->SetTextRenderColor(Colour);
}

void AReal33DCreature::SetCombatFeedback(bool bAttacked, bool bFollowed)
{
	check(IsInGameThread());
	if (!bAttacked && !bFollowed)
	{
		TargetTag->SetText(FText::GetEmpty());
		TargetTag->SetVisibility(false);
		return;
	}
	TargetTag->SetText(FText::FromString(bFollowed ? TEXT("FOLLOW") : TEXT("ATTACK")));
	TargetTag->SetTextRenderColor(bFollowed
		? FColor(40, 190, 255) : FColor(255, 45, 45));
	TargetTag->SetVisibility(true);
}

void AReal33DCreature::Configure(uint32 InCreatureId, bool bInIsLocalPlayer,
	const FString& InName, const UReal33DAssetRegistry* Registry)
{
	CreatureId = InCreatureId;
	bIsLocalPlayer = bInIsLocalPlayer;

	// A creature the viewport revealed mid-move can arrive without a name. Show
	// the id rather than an empty tag, so the evidence run can still tell two
	// bodies apart.
	CreatureName = InName.IsEmpty()
		? FString::Printf(TEXT("#%u"), InCreatureId) : InName;
	NameTag->SetText(FText::FromString(CreatureName));
	SetHealthPercent(HealthPercent);

	if (Registry != nullptr && Registry->IsReady())
	{
		const FReal33DVisual Visual = Registry->ResolveCreature(InCreatureId, bIsLocalPlayer);
		Body->SetStaticMesh(Visual.Mesh);
		Body->SetRelativeScale3D(Visual.Scale);
		BodyBaseOffset = Visual.Offset;
		Body->SetRelativeLocation(BodyBaseOffset);
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
		TargetTag->SetWorldRotation(Facing);

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
		Body->SetRelativeLocation(BodyBaseOffset);
		return;
	}
	const FVector Previous = DrawnLocation;
	DrawnLocation = FMath::VInterpConstantTo(DrawnLocation, TargetLocation,
		DeltaSeconds, WalkVisualSpeed);
	SetActorLocation(DrawnLocation);
	WalkPhase += FVector::Dist2D(Previous, DrawnLocation)
		/ Real33D::UnitsPerSqm * 2.0 * PI;
	Body->SetRelativeLocation(BodyBaseOffset + FVector(0.0, 0.0,
		WalkBobHeight * FMath::Sin(WalkPhase)));
}
