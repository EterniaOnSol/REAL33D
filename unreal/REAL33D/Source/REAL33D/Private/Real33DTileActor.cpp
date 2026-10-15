#include "Real33DTileActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Real33DCoords.h"
#include "Real33DPresentationPolicy.h"

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
	for (auto& Light : LampLights) if (Light) Light->DestroyComponent();
	LampLights.Reset();
}

bool AReal33DTile::GetTopObject(uint16& OutTypeId, uint8& OutStackIndex) const
{
	// Backwards: the server's list runs bottom to top, so the last entry that
	// is not a creature is the object a click lands on.
	for (int32 Index = Things.Num() - 1; Index >= 0; --Index)
	{
		const FReal33DThing& Thing = Things[Index];
		if (Thing.bIsCreature || Thing.TypeId == 0)
		{
			continue;
		}
		OutTypeId = Thing.TypeId;
		OutStackIndex = static_cast<uint8>(FMath::Min(Index, 255));
		return true;
	}
	return false;
}

void AReal33DTile::ApplyStack(const TArray<FReal33DThing>& InThings,
	const UReal33DAssetRegistry* Registry)
{
	check(IsInGameThread());
	// Remembered so a click on this field can name what is on it.
	Things = InThings;
	ClearComponents();
	// An empty/void tile above the player is not a roof.
	bHasCoveringContent = InThings.ContainsByPredicate([](const FReal33DThing& Thing)
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
	for (const FReal33DThing& Thing : InThings)
	{
		if (Thing.bIsCreature)
		{
			continue;
		}

		const bool bIsGround = Index == 0;
		FReal33DVisual Visual = bIsGround
			? Registry->ResolveGround(Thing.TypeId)
			: Registry->ResolveThing(Thing.TypeId, Thing.bBlocking);
		const bool bFlatSurface = Visual.Mesh && Real33D::Presentation::IsFlatSurface(
			Visual.Mesh->GetBoundingBox().TransformBy(FTransform(Visual.Rotation, Visual.Offset, Visual.Scale)));
		if (bFlatSurface)
		{
			Visual.Offset.Z += Real33D::Presentation::SurfaceLayerLift(Index);
		}
		else if (!bIsGround && !Thing.bBlocking)
		{
			// Stack walkable clutter so the layering reads. Blocking objects
			// keep their own height: raising them would make a wall float.
			Visual.Offset.Z += Real33D::UnitsPerSqm * 0.08 * static_cast<double>(Index - 1);
		}

		UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this);
		Component->SetupAttachment(Root);
		Component->RegisterComponent();
		Component->SetStaticMesh(Visual.Mesh);
		// Query-only: a right click must be able to name the field's top object.
		// This never participates in physics or blocks movement; Fusion32 alone
		// decides whether the field can be entered and whether the object can be
		// used. The hit names the tile, whose remembered WorldState stack supplies
		// the real type id and stack index.
		Component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Component->SetCollisionResponseToAllChannels(ECR_Ignore);
		Component->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		if (Visual.bIsExperimental)
		{
			// The tag is V08 inspector metadata only; ordinary object use does not
			// read it and remains available without the experimental catalog.
			Component->ComponentTags.Add(FName(*FString::Printf(TEXT("V08_%u"), Thing.TypeId)));
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
		// Only a received lit-lamp state creates light. Use never toggles it locally.
		// Static scenery has no live light state and cannot create these components.
		if (Thing.TypeId == 2109 && Visual.bIsExperimental && Visual.Mesh)
		{
			auto* Light = NewObject<UPointLightComponent>(this);
			Light->SetupAttachment(Root);
			const FBox LampBounds = Visual.Mesh->GetBoundingBox().TransformBy(
				FTransform(Visual.Rotation, Visual.Offset, Visual.Scale));
			Light->SetRelativeLocation(FVector(LampBounds.GetCenter().X,
				LampBounds.GetCenter().Y, LampBounds.Max.Z - 12.0));
			Light->SetLightColor(FLinearColor(1.0f, 0.65f, 0.22f));
			Light->SetIntensity(2500.0f);
			Light->SetAttenuationRadius(400.0f);
			Light->SetCastShadows(false);
			Light->RegisterComponent();
			LampLights.Add(Light);
		}
		++Index;
	}
	SetFloorVisible(bFloorVisible);
}

void AReal33DTile::SetFloorVisible(bool bVisible)
{
	bFloorVisible = bVisible;
	SetActorHiddenInGame(!bVisible);
	SetActorEnableCollision(bVisible);
	for (auto& Light : LampLights) if (Light) Light->SetVisibility(bVisible);
	for (const TObjectPtr<UStaticMeshComponent>& Component : StackComponents)
	{
		if (!Component) continue;
		Component->SetVisibility(bVisible);
		Component->SetCollisionEnabled(bVisible
			? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}

void AReal33DTile::MeasureCameraObstruction(const FVector& Focus, const FVector& Eye, double& SafeDistance) const
{
 if (!bFloorVisible) return;
 // Camera proximity never changes visibility or interaction collision.
 for (const auto& Component : StackComponents)
 {
  if (!Component) continue;
  Real33D::Presentation::LimitCamera(Focus, Eye, Component->Bounds.GetBox(), SafeDistance);
 }
}
