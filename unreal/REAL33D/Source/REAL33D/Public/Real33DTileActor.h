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
	void SetFloorVisible(bool bVisible);

	const Real33D::FMapPosition& GetMapPosition() const { return MapPosition; }
	void SetMapPosition(const Real33D::FMapPosition& Position) { MapPosition = Position; }
	bool HasCoveringContent() const { return bHasCoveringContent; }

	/**
	 * The stack as WorldState last described it, in the server's own order.
	 *
	 * Kept so a click on this field can name what is on it. CL_CMD_USE_OBJECT
	 * and CL_CMD_USE_TWO_OBJECTS identify an object by its type and its index
	 * in the stack, and the server resolves that index against its own list --
	 * so it has to be the index the server would use, which is this one.
	 */
	const TArray<FReal33DThing>& GetThings() const { return Things; }

	/**
	 * The topmost object a use would act on, or false when there is none.
	 *
	 * Creatures are skipped: a creature is not an object and is named by id in
	 * CL_CMD_USE_ON_CREATURE instead. The stack index returned counts every
	 * entry, creatures included, because that is how the server indexes it.
	 */
	bool GetTopObject(uint16& OutTypeId, uint8& OutStackIndex) const;

private:
	void ClearComponents();

	UPROPERTY()
	TObjectPtr<USceneComponent> Root = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> StackComponents;

	/** What WorldState says is on this field. Presentation reads it; it owns nothing. */
	TArray<FReal33DThing> Things;

	Real33D::FMapPosition MapPosition;
	bool bHasCoveringContent = false;
	bool bFloorVisible = true;
};
