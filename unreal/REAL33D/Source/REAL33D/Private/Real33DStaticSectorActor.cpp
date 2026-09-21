#include "Real33DStaticSectorActor.h"

#include "Components/BillboardComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Real33DAssetRegistry.h"
#include "REAL33D.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

AReal33DStaticSector::AReal33DStaticSector()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

UTexture2D* AReal33DStaticSector::LoadClassicPreview(uint16 TypeId, const FString& Directory)
{
	if (TObjectPtr<UTexture2D>* Existing = PreviewTextures.Find(TypeId))
	{
		return Existing->Get();
	}
	const FString Path = FPaths::Combine(Directory, FString::Printf(TEXT("%u.png"), TypeId));
	UTexture2D* Texture = FPaths::FileExists(Path)
		? FImageUtils::ImportFileAsTexture2D(Path) : nullptr;
	PreviewTextures.Add(TypeId, Texture);
	return Texture;
}

void AReal33DStaticSector::Build(const TArray<FReal33DStaticItem>& Items,
	const UReal33DAssetRegistry* Registry,
	const FString& ReferencePreviewDirectory,
	const Real33D::FWorldOrigin& Origin)
{
	check(IsInGameThread());
	TMap<UStaticMesh*, UHierarchicalInstancedStaticMeshComponent*> Groups;
	TMap<uint32, FReal33DVisual> Visuals;
	for (const FReal33DStaticItem& Item : Items)
	{
		const bool bGround = Item.Stack == 0;
		FReal33DVisual Visual = bGround
			? Registry->ResolveGround(Item.TypeId)
			: Registry->ResolveThing(Item.TypeId, false);
		const FIntVector Key = Real33D::ToKey(Item.Position);
		FTileRefs& Refs = TileRefs.FindOrAdd(Key);
		FVector Location = Real33D::ToWorld(Origin, Item.Position);
		if (!bGround)
		{
			Location.Z += Real33D::UnitsPerSqm * 0.08 * FMath::Max(0, Item.Stack - 1);
		}

		if (Visual.bIsExperimental && Visual.Mesh != nullptr)
		{
			UHierarchicalInstancedStaticMeshComponent*& Group = Groups.FindOrAdd(Visual.Mesh);
			if (Group == nullptr)
			{
				Group = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
				Group->SetupAttachment(Root);
				Group->SetStaticMesh(Visual.Mesh);
				Group->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Group->RegisterComponent();
				MeshComponents.Add(Group);
			}
			const FTransform Transform(Visual.Rotation, Location + Visual.Offset, Visual.Scale);
			const int32 Index = Group->AddInstance(Transform, false);
			Refs.Meshes.Add(FInstanceRef{Group, Index, Transform});
			++V08Resolved;
			continue;
		}

		if (UTexture2D* Texture = LoadClassicPreview(Item.TypeId, ReferencePreviewDirectory))
		{
			UBillboardComponent* Sprite = NewObject<UBillboardComponent>(this);
			Sprite->SetupAttachment(Root);
			Sprite->SetSprite(Texture);
			Sprite->SetHiddenInGame(false);
			Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Sprite->SetRelativeLocation(Location + FVector(0.0, 0.0,
				bGround ? 2.0 : Real33D::UnitsPerSqm * 0.5));
			Sprite->SetRelativeScale3D(FVector(0.75));
			Sprite->ComponentTags.Add(FName(*FString::Printf(
				TEXT("CLASSIC_SPRITE_FALLBACK_%u"), Item.TypeId)));
			Sprite->RegisterComponent();
			SpriteComponents.Add(Sprite);
			Refs.Sprites.Add(Sprite);
			++ClassicFallback;
			if (Item.TypeId == 469)
			{
				UE_LOG(LogReal33D, Log, TEXT("wide world TypeId=469 CLASSIC_SPRITE_FALLBACK at %d,%d,%d"),
					Item.Position.X, Item.Position.Y, Item.Position.Z);
			}
			continue;
		}

		if (Visual.Mesh != nullptr)
		{
			UHierarchicalInstancedStaticMeshComponent*& Group = Groups.FindOrAdd(Visual.Mesh);
			if (Group == nullptr)
			{
				Group = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
				Group->SetupAttachment(Root);
				Group->SetStaticMesh(Visual.Mesh);
				Group->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Group->RegisterComponent();
				MeshComponents.Add(Group);
			}
			const FTransform Transform(Visual.Rotation, Location + Visual.Offset, Visual.Scale);
			const int32 Index = Group->AddInstance(Transform, false);
			Refs.Meshes.Add(FInstanceRef{Group, Index, Transform});
		}
		++MissingPhysicalAsset;
	}
}

bool AReal33DStaticSector::IsAuthoritative(
	const FIntVector& Key, const Real33D::FMapPosition& Anchor) const
{
	const int32 Offset = Anchor.Z - Key.Z;
	const int32 MinX = Anchor.X - 8 + Offset;
	const int32 MinY = Anchor.Y - 6 + Offset;
	return Key.X >= MinX && Key.X < MinX + 18
		&& Key.Y >= MinY && Key.Y < MinY + 14;
}

void AReal33DStaticSector::ApplyView(
	const Real33D::FMapPosition& Anchor, int32 VisualRadius)
{
	check(IsInGameThread());
	for (TPair<FIntVector, FTileRefs>& Pair : TileRefs)
	{
		const bool bSuppressed = ShouldSuppressTile(Pair.Key, Anchor, VisualRadius);
		for (FInstanceRef& Ref : Pair.Value.Meshes)
		{
			if (!Ref.Component || Ref.Index == INDEX_NONE)
			{
				continue;
			}
			FTransform Transform = Ref.VisibleTransform;
			if (bSuppressed)
			{
				Transform.SetScale3D(FVector::ZeroVector);
			}
			Ref.Component->UpdateInstanceTransform(
				Ref.Index, Transform, false, true, true);
		}
		for (TObjectPtr<UBillboardComponent>& Sprite : Pair.Value.Sprites)
		{
			if (Sprite)
			{
				Sprite->SetVisibility(!bSuppressed);
			}
		}
	}
}




bool AReal33DStaticSector::ShouldSuppressTile(const FIntVector& Key,
	const Real33D::FMapPosition& Anchor, int32 VisualRadius)
{
	const int32 Offset = Anchor.Z - Key.Z;
	const int32 MinX = Anchor.X - 8 + Offset;
	const int32 MinY = Anchor.Y - 6 + Offset;
	const bool bLive = Key.X >= MinX && Key.X < MinX + 18
		&& Key.Y >= MinY && Key.Y < MinY + 14;
	return bLive || FMath::Abs(Key.X - Anchor.X) > VisualRadius
		|| FMath::Abs(Key.Y - Anchor.Y) > VisualRadius;
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DWideWorldWindowTest,
	"REAL33D.WideWorld.AuthoritativeWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReal33DWideWorldWindowTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const Real33D::FMapPosition Anchor{32330, 32226, 7};
	TestTrue(TEXT("live minimum is suppressed"),
		AReal33DStaticSector::ShouldSuppressTile(FIntVector(32322, 32220, 7), Anchor, 64));
	TestTrue(TEXT("live maximum is suppressed"),
		AReal33DStaticSector::ShouldSuppressTile(FIntVector(32339, 32233, 7), Anchor, 64));
	TestFalse(TEXT("adjacent static tile remains"),
		AReal33DStaticSector::ShouldSuppressTile(FIntVector(32340, 32233, 7), Anchor, 64));
	TestTrue(TEXT("upper floor uses protocol XY offset"),
		AReal33DStaticSector::ShouldSuppressTile(FIntVector(32323, 32221, 6), Anchor, 64));
	TestTrue(TEXT("outside configured radius is suppressed"),
		AReal33DStaticSector::ShouldSuppressTile(FIntVector(32395, 32226, 7), Anchor, 64));
	TestFalse(TEXT("radius boundary remains visible"),
		AReal33DStaticSector::ShouldSuppressTile(FIntVector(32394, 32240, 7), Anchor, 64));
	const FIntVector TransitionTile(32340, 32226, 7);
	TestFalse(TEXT("static before entering live window"),
		AReal33DStaticSector::ShouldSuppressTile(TransitionTile, Anchor, 64));
	const Real33D::FMapPosition Stepped{32331, 32226, 7};
	TestTrue(TEXT("same static tile suppressed after live transition"),
		AReal33DStaticSector::ShouldSuppressTile(TransitionTile, Stepped, 64));
	const TSet<FIntVector> Before = AReal33DStaticSector::DesiredSectors(Anchor, 32);
	const Real33D::FMapPosition SectorStep{32362, 32226, 7};
	const TSet<FIntVector> After = AReal33DStaticSector::DesiredSectors(SectorStep, 32);
	bool bHasUnload = false;
	for (const FIntVector& Key : Before) bHasUnload |= !After.Contains(Key);
	bool bHasLoad = false;
	for (const FIntVector& Key : After) bHasLoad |= !Before.Contains(Key);
	TestTrue(TEXT("sector movement has unload candidates"), bHasUnload);
	TestTrue(TEXT("sector movement has load candidates"), bHasLoad);
	return true;
}
#endif






TSet<FIntVector> AReal33DStaticSector::DesiredSectors(
	const Real33D::FMapPosition& Anchor, int32 VisualRadius)
{
	const int32 MinSX = (Anchor.X - VisualRadius) / 32;
	const int32 MaxSX = (Anchor.X + VisualRadius) / 32;
	const int32 MinSY = (Anchor.Y - VisualRadius) / 32;
	const int32 MaxSY = (Anchor.Y + VisualRadius) / 32;
	TSet<FIntVector> Result;
	for (int32 Z = 0; Z <= 7; ++Z)
	{
		for (int32 SX = MinSX; SX <= MaxSX; ++SX)
		{
			for (int32 SY = MinSY; SY <= MaxSY; ++SY)
			{
				Result.Add(FIntVector(SX, SY, Z));
			}
		}
	}
	return Result;
}
