#include "Real33DAssetRegistry.h"

#include "REAL33D.h"
#include "Real33DCoords.h"
#include "UObject/ConstructorHelpers.h"

void UReal33DAssetRegistry::Initialise()
{
	// Engine primitives only. This slice proves architecture and
	// synchronisation, so nothing here is meant to look like Tibia.
	PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	CylinderMesh = LoadObject<UStaticMesh>(nullptr,
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	BaseMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	bReady = PlaneMesh != nullptr && CubeMesh != nullptr && CylinderMesh != nullptr;
	if (!bReady)
	{
		UE_LOG(LogReal33D, Error, TEXT("asset registry could not load the engine primitives"));
	}
}

FReal33DVisual UReal33DAssetRegistry::MakePlaceholder(EReal33DVisualKind Kind) const
{
	using namespace Real33D;
	FReal33DVisual Visual;
	Visual.Material = BaseMaterial;
	Visual.bIsPlaceholder = true;

	// The engine plane and cube are 100 uu across, which is already one field.
	switch (Kind)
	{
	case EReal33DVisualKind::Ground:
		Visual.Mesh = PlaneMesh;
		Visual.Scale = FVector(1.0, 1.0, 1.0);
		Visual.Offset = FVector::ZeroVector;
		Visual.Tint = FLinearColor(0.16f, 0.20f, 0.14f);
		break;

	case EReal33DVisualKind::Obstacle:
		Visual.Mesh = CubeMesh;
		Visual.Scale = FVector(0.98, 0.98, 0.9);
		Visual.Offset = FVector(0.0, 0.0, UnitsPerSqm * 0.45);
		Visual.Tint = FLinearColor(0.28f, 0.26f, 0.24f);
		break;

	case EReal33DVisualKind::Prop:
		// Deliberately flat and small. A walkable object drawn as a block reads
		// as a wall, and an operator walking straight through it is right to
		// call that wrong.
		Visual.Mesh = CubeMesh;
		Visual.Scale = FVector(0.5, 0.5, 0.06);
		Visual.Offset = FVector(0.0, 0.0, UnitsPerSqm * 0.03);
		Visual.Tint = FLinearColor(0.42f, 0.36f, 0.22f);
		break;

	case EReal33DVisualKind::Creature:
		Visual.Mesh = CylinderMesh;
		Visual.Scale = FVector(0.55, 0.55, 0.9);
		Visual.Offset = FVector(0.0, 0.0, UnitsPerSqm * 0.45);
		Visual.Tint = FLinearColor(0.75f, 0.25f, 0.20f);
		break;

	case EReal33DVisualKind::LocalPlayer:
		Visual.Mesh = CylinderMesh;
		Visual.Scale = FVector(0.55, 0.55, 0.9);
		Visual.Offset = FVector(0.0, 0.0, UnitsPerSqm * 0.45);
		Visual.Tint = FLinearColor(0.20f, 0.65f, 0.95f);
		break;
	}
	return Visual;
}

// No approved asset exists for any identity yet, so every resolution below is a
// placeholder. When the visual pipeline approves art, the lookup goes here,
// keyed by the same tracker id the master inventory uses, and nothing outside
// this file changes.
//
// The kind is deliberately not guessed from the type id. The authoritative
// classification lives in the master inventory, and inventing an id range here
// would be exactly the hardcoded thing-id-to-mesh mapping this class exists to
// prevent. Until the lookup exists, the caller's structural knowledge decides:
// a tile knows which of its things is the ground because the server's own stack
// order puts it first.

FReal33DVisual UReal33DAssetRegistry::ResolveThing(uint16 TypeId, bool bBlocking) const
{
	(void)TypeId;
	// The only thing the placeholder is allowed to say about an object is
	// whether Fusion32 will let you walk onto its field, because that is the
	// only thing about it this client actually knows.
	return MakePlaceholder(bBlocking ? EReal33DVisualKind::Obstacle
	                                 : EReal33DVisualKind::Prop);
}

FReal33DVisual UReal33DAssetRegistry::ResolveGround(uint16 TypeId) const
{
	(void)TypeId;
	return MakePlaceholder(EReal33DVisualKind::Ground);
}

FReal33DVisual UReal33DAssetRegistry::ResolveCreature(uint32 CreatureId,
	bool bIsLocalPlayer) const
{
	(void)CreatureId;
	return MakePlaceholder(bIsLocalPlayer ? EReal33DVisualKind::LocalPlayer
	                                      : EReal33DVisualKind::Creature);
}
