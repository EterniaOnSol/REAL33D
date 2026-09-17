#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Real33DAssetRegistry.h"
#include "Real33DBridge.h"
#include "Real33DTileActor.generated.h"

class UStaticMeshComponent;

/**
 * One visible field. Owns the components that draw its stack.
 *
 * Holds no authority: it is told what WorldState says the field contains and
 * rebuilds its components to match. It never decides what is on a field.
 */
UCLASS()
class REAL33D_API AReal33DTile : public AActor
{
	GENERATED_BODY()

public:
	AReal33DTile();

	/** Rebuilds the stack. Game thread only. */
	void ApplyStack(const TArray<FReal33DThing>& Things, const UReal33DAssetRegistry* Registry);

	const Real33D::FMapPosition& GetMapPosition() const { return MapPosition; }
	void SetMapPosition(const Real33D::FMapPosition& Position) { MapPosition = Position; }

private:
	void ClearComponents();

	UPROPERTY()
	TObjectPtr<USceneComponent> Root = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> StackComponents;

	Real33D::FMapPosition MapPosition;
};
