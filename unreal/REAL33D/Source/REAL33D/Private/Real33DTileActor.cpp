#include "Real33DTileActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Real33DCoords.h"

AReal33DTile::AReal33DTile()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void AReal33DTile::ClearComponents()
{
	for (TObjectPtr<UStaticMeshComponent>& Component : StackComponents)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}
	StackComponents.Reset();
}

void AReal33DTile::ApplyStack(const TArray<FReal33DThing>& Things,
	const UReal33DAssetRegistry* Registry)
{
	check(IsInGameThread());
	ClearComponents();
	// An empty/void tile above the player is not a roof.
	bHasCoveringContent = Things.ContainsByPredicate([](const FReal33DThing& Thing)
	{
		return !Thing.bIsCreature && Thing.TypeId != 0 && Thing.TypeId != 100;
	});
	if (Registry == nullptr || !Registry->IsReady())
	{
		return;
	}

	// Creatures are drawn by their own actors so they can move between fields
	// without rebuilding a tile. A tile draws only the items of its stack, in
	// the order WorldState holds them, which is the server's own stack order
	// from map.cc::PlaceObject.
	//
	// The first item of a field is its ground: GetObjectPriority gives BANK the
	// lowest priority value, so a ground always sorts to index zero.
	int32 Index = 0;
	for (const FReal33DThing& Thing : Things)
	{
		if (Thing.bIsCreature)
		{
			continue;
		}

		const bool bIsGround = Index == 0;
		FReal33DVisual Visual = bIsGround
			? Registry->ResolveGround(Thing.TypeId)
			: Registry->ResolveThing(Thing.TypeId, Thing.bBlocking);
		if (!bIsGround && !Thing.bBlocking)
		{
			// Stack walkable clutter so the layering reads. Blocking objects
			// keep their own height: raising them would make a wall float.
			Visual.Offset.Z += Real33D::UnitsPerSqm * 0.08 * static_cast<double>(Index - 1);
		}

		UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this);
		Component->SetupAttachment(Root);
		Component->RegisterComponent();
		Component->SetStaticMesh(Visual.Mesh);
		if (Visual.bIsExperimental)
		{
			// QA picking only: the inspector traces imported mesh geometry.
			Component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Component->SetCollisionResponseToAllChannels(ECR_Ignore);
			Component->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			Component->ComponentTags.Add(FName(*FString::Printf(TEXT("V08_%u"), Thing.TypeId)));
		}
		else
		{
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		Component->SetCastShadow(!bIsGround);
		Component->SetRelativeScale3D(Visual.Scale);
		Component->SetRelativeRotation(Visual.Rotation);
		Component->SetRelativeLocation(Visual.Offset);

		if (Visual.Material != nullptr)
		{
			UMaterialInstanceDynamic* Dynamic =
				UMaterialInstanceDynamic::Create(Visual.Material, Component);
			if (Dynamic != nullptr)
			{
				Dynamic->SetVectorParameterValue(TEXT("Color"), Visual.Tint);
				Component->SetMaterial(0, Dynamic);
			}
		}
		StackComponents.Add(Component);
		++Index;
	}
	SetFloorVisible(bFloorVisible);
}

void AReal33DTile::SetFloorVisible(bool bVisible)
{
	bFloorVisible = bVisible;
	SetActorHiddenInGame(!bVisible);
	SetActorEnableCollision(bVisible);
	for (const TObjectPtr<UStaticMeshComponent>& Component : StackComponents)
	{
		if (!Component) continue;
		Component->SetVisibility(bVisible);
		Component->SetCollisionEnabled(bVisible && !Component->ComponentTags.IsEmpty()
			? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}
