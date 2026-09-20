#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Real33DGalleryActor.generated.h"

class SReal33DGalleryPanel;
class UCameraComponent;
class UReal33DAssetRegistry;
class UStaticMeshComponent;

/** Command-line-gated 3D inspector for every V08 catalog row. */
UCLASS()
class REAL33D_API AReal33DGalleryActor : public AActor
{
	GENERATED_BODY()

public:
	AReal33DGalleryActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

public:
	UFUNCTION(Exec)
	void GoToItem(int32 TypeId);

private:
	void Select(uint16 TypeId);

	UPROPERTY()
	TObjectPtr<USceneComponent> Root = nullptr;
	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera = nullptr;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Preview = nullptr;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TileReference = nullptr;
	UPROPERTY()
	TObjectPtr<UReal33DAssetRegistry> Registry = nullptr;
	TSharedPtr<SReal33DGalleryPanel> Panel;
	TSharedPtr<SWidget> PanelRoot;
};
