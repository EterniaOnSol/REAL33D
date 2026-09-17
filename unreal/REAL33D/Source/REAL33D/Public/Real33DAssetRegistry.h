#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/Object.h"
#include "Real33DAssetRegistry.generated.h"

/**
 * The indirection between a Fusion32 identity and the Unreal asset that draws
 * it.
 *
 * Three identities stay apart, as visual/README.md requires:
 *   1. logical identity   the Fusion32 type id or creature id
 *   2. visual identity    the resolution decided here
 *   3. physical asset     whatever mesh and material come back
 *
 * Nothing above this class maps a thing id to a mesh. Actors ask the registry
 * and draw what it returns, so when the visual pipeline approves real art the
 * swap happens here alone: no change to Protocol772Core, no change to the
 * actors, no change to gameplay, because there is none.
 *
 * Until an approved asset exists for an identity, the registry answers with a
 * placeholder and says so. That is a resolution, not a failure.
 */

UENUM()
enum class EReal33DVisualKind : uint8
{
	Ground,
	Obstacle,
	Prop,
	Creature,
	LocalPlayer
};

USTRUCT()
struct FReal33DVisual
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> Material = nullptr;

	/** Scale applied to the engine primitive so it occupies one field. */
	UPROPERTY()
	FVector Scale = FVector::OneVector;

	/** Offset from the field's floor plane to the mesh pivot. */
	UPROPERTY()
	FVector Offset = FVector::ZeroVector;

	UPROPERTY()
	FLinearColor Tint = FLinearColor::White;

	/** False once an approved asset from the visual pipeline backs this identity. */
	UPROPERTY()
	bool bIsPlaceholder = true;
};

UCLASS()
class REAL33D_API UReal33DAssetRegistry : public UObject
{
	GENERATED_BODY()

public:
	void Initialise();

	/**
	 * Resolves a map object sitting above the ground.
	 * @param bBlocking the server's own UNPASS answer for this type.
	 */
	FReal33DVisual ResolveThing(uint16 TypeId, bool bBlocking) const;

	/** Resolves the object that forms a field's ground. */
	FReal33DVisual ResolveGround(uint16 TypeId) const;

	/** Resolves a creature, which may be this client's own player. */
	FReal33DVisual ResolveCreature(uint32 CreatureId, bool bIsLocalPlayer) const;

	bool IsReady() const { return bReady; }

private:
	FReal33DVisual MakePlaceholder(EReal33DVisualKind Kind) const;

	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaneMesh = nullptr;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh = nullptr;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial = nullptr;

	bool bReady = false;
};
