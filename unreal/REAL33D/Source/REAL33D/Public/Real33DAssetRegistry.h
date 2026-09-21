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
 * Only TypeId 3501 currently has an approved mesh. All other identities
 * resolve to a placeholder, including the visually distinct TypeId 3508.
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

	/** Presentation scale; imported production meshes arrive in Unreal units. */
	UPROPERTY()
	FVector Scale = FVector::OneVector;

	/** Presentation rotation for a resolved mesh. */
	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	/** Offset from the field's floor plane to the mesh pivot. */
	UPROPERTY()
	FVector Offset = FVector::ZeroVector;

	UPROPERTY()
	FLinearColor Tint = FLinearColor::White;

	/** False once an approved asset from the visual pipeline backs this identity. */
	UPROPERTY()
	bool bIsPlaceholder = true;

	/** True only for the command-line-gated V08 QA catalog. Never approval. */
	UPROPERTY()
	bool bIsExperimental = false;

	/** QA visual source; logical TypeId stays with the WorldState thing. */
	UPROPERTY()
	uint16 VisualSourceTypeId = 0;
};

/** One row of the local-only V08 QA catalog. */
USTRUCT()
struct FReal33DExperimentalCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY()
	uint16 TypeId = 0;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Category;

	UPROPERTY()
	FString RefinementStatus;

	UPROPERTY()
	FString GeometryQuality;

	UPROPERTY()
	FString Generation;

	UPROPERTY()
	FString ImportStatus;

	UPROPERTY()
	FString MeshPath;

	UPROPERTY()
	FString Warnings;
};

UCLASS()
class REAL33D_API UReal33DAssetRegistry : public UObject
{
	GENERATED_BODY()

public:
	void Initialise();
	void EnableFrozenCatalogForWideWorld();

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

	bool IsExperimentalCatalogEnabled() const { return bExperimentalCatalogEnabled; }
	const TArray<FReal33DExperimentalCatalogEntry>& GetExperimentalCatalog() const
	{
		return ExperimentalCatalog;
	}

private:
	FReal33DVisual MakePlaceholder(EReal33DVisualKind Kind) const;
	void LoadExperimentalCatalog(bool bWideWorldRequired = false);
	bool TryResolveExperimental(uint16 TypeId, FReal33DVisual& OutVisual) const;

	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaneMesh = nullptr;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh = nullptr;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial = nullptr;

	/** Artist-approved mailbox for Fusion32 object TypeId 3501 only. */
	UPROPERTY()
	TObjectPtr<UStaticMesh> Mailbox3501Mesh = nullptr;

	/** One-shot, command-line-gated screenshot for the live acceptance run. */
	mutable bool bMailbox3501EvidenceRequested = false;

	UPROPERTY()
	TArray<FReal33DExperimentalCatalogEntry> ExperimentalCatalog;

	TMap<uint16, int32> ExperimentalCatalogById;
	bool bExperimentalCatalogEnabled = false;

	bool bReady = false;
};
