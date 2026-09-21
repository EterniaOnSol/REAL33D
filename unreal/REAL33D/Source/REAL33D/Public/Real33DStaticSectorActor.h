#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Real33DCoords.h"
#include "Real33DStaticSectorActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UBillboardComponent;
class UReal33DAssetRegistry;

struct FReal33DStaticItem
{
	Real33D::FMapPosition Position;
	int32 Stack = 0;
	uint16 TypeId = 0;
	uint16 SourceTypeId = 0;
};

UCLASS()
class REAL33D_API AReal33DStaticSector : public AActor
{
	GENERATED_BODY()

public:
	AReal33DStaticSector();

	void Build(const TArray<FReal33DStaticItem>& Items,
		const UReal33DAssetRegistry* Registry,
		const FString& ReferencePreviewDirectory,
		const Real33D::FWorldOrigin& Origin);
	void ApplyView(const Real33D::FMapPosition& Anchor, int32 VisualRadius);
	static bool ShouldSuppressTile(const FIntVector& Key,
		const Real33D::FMapPosition& Anchor, int32 VisualRadius);
	static TSet<FIntVector> DesiredSectors(
		const Real33D::FMapPosition& Anchor, int32 VisualRadius);
	int32 GetV08Resolved() const { return V08Resolved; }
	int32 GetClassicFallback() const { return ClassicFallback; }
	int32 GetMissingPhysicalAsset() const { return MissingPhysicalAsset; }

private:
	struct FInstanceRef
	{
		TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Component = nullptr;
		int32 Index = INDEX_NONE;
		FTransform VisibleTransform;
	};
	struct FTileRefs
	{
		TArray<FInstanceRef> Meshes;
		TArray<TObjectPtr<UBillboardComponent>> Sprites;
	};
	bool IsAuthoritative(const FIntVector& Key, const Real33D::FMapPosition& Anchor) const;
	class UTexture2D* LoadClassicPreview(uint16 TypeId, const FString& Directory);

	UPROPERTY()
	TObjectPtr<USceneComponent> Root = nullptr;
	UPROPERTY()
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> MeshComponents;
	UPROPERTY()
	TArray<TObjectPtr<UBillboardComponent>> SpriteComponents;
	UPROPERTY()
	TMap<uint16, TObjectPtr<class UTexture2D>> PreviewTextures;

	TMap<FIntVector, FTileRefs> TileRefs;
	int32 V08Resolved = 0;
	int32 ClassicFallback = 0;
	int32 MissingPhysicalAsset = 0;
};
