#include "Real33DAssetRegistry.h"

#include "REAL33D.h"
#include "Real33DCoords.h"
#include "HAL/PlatformFileManager.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "UObject/ConstructorHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

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
	Mailbox3501Mesh = LoadObject<UStaticMesh>(nullptr,
		TEXT("/Game/Visual/obj3501/decoration_bottom_overlay_obj3501_mailbox/StaticMeshes/decoration_bottom_overlay_obj3501_mailbox.decoration_bottom_overlay_obj3501_mailbox"));
	if (Mailbox3501Mesh == nullptr)
	{
		UE_LOG(LogReal33D, Error, TEXT("approved obj:3501 mailbox mesh is missing"));
	}
	else
	{
		UE_LOG(LogReal33D, Log, TEXT("loaded approved visual for obj:3501"));
	}
	LoadExperimentalCatalog();

	bReady = PlaneMesh != nullptr && CubeMesh != nullptr && CylinderMesh != nullptr;
	if (!bReady)
	{
		UE_LOG(LogReal33D, Error, TEXT("asset registry could not load the engine primitives"));
	}
}

void UReal33DAssetRegistry::LoadExperimentalCatalog()
{
	FString Path;
	const bool bExplicitPath = FParse::Value(FCommandLine::Get(),
		TEXT("-real33d-experimental-catalog="), Path);
	if (!bExplicitPath
		&& !FParse::Param(FCommandLine::Get(), TEXT("real33d-experimental-catalog"))
		&& !FParse::Param(FCommandLine::Get(), TEXT("real33d-gallery")))
	{
		return;
	}
	if (Path.IsEmpty())
	{
		Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("../../visual/qa/full_catalog_v08/experimental_catalog_runtime.json"));
	}
	Path = FPaths::ConvertRelativePathToFull(Path);
	FPaths::CollapseRelativeDirectories(Path);

	FString Contents;
	if (!FFileHelper::LoadFileToString(Contents, *Path))
	{
		UE_LOG(LogReal33D, Error, TEXT("experimental V08 catalog could not be read: %s"), *Path);
		return;
	}
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Contents);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()
		|| Root->GetStringField(TEXT("schema")) != TEXT("real33d.experimental-v08-runtime.v1"))
	{
		UE_LOG(LogReal33D, Error, TEXT("experimental V08 catalog schema is invalid: %s"), *Path);
		return;
	}
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (!Root->TryGetArrayField(TEXT("entries"), Entries) || Entries == nullptr)
	{
		UE_LOG(LogReal33D, Error, TEXT("experimental V08 catalog has no entries: %s"), *Path);
		return;
	}

	ExperimentalCatalog.Reset(Entries->Num());
	ExperimentalCatalogById.Reset();
	for (const TSharedPtr<FJsonValue>& Value : *Entries)
	{
		const TSharedPtr<FJsonObject> Object = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Object.IsValid())
		{
			continue;
		}
		const int64 RawId = Object->GetIntegerField(TEXT("item_id"));
		if (RawId < 0 || RawId > MAX_uint16 || ExperimentalCatalogById.Contains(static_cast<uint16>(RawId)))
		{
			UE_LOG(LogReal33D, Warning, TEXT("experimental V08 catalog skipped invalid/duplicate id %lld"), RawId);
			continue;
		}
		FReal33DExperimentalCatalogEntry Entry;
		Entry.TypeId = static_cast<uint16>(RawId);
		Object->TryGetStringField(TEXT("name"), Entry.Name);
		Object->TryGetStringField(TEXT("category"), Entry.Category);
		Object->TryGetStringField(TEXT("refinement_status"), Entry.RefinementStatus);
		Object->TryGetStringField(TEXT("geometry_quality"), Entry.GeometryQuality);
		Object->TryGetStringField(TEXT("generation"), Entry.Generation);
		Object->TryGetStringField(TEXT("import_status"), Entry.ImportStatus);
		Object->TryGetStringField(TEXT("mesh_path"), Entry.MeshPath);
		const TArray<TSharedPtr<FJsonValue>>* Warnings = nullptr;
		if (Object->TryGetArrayField(TEXT("warnings"), Warnings) && Warnings != nullptr)
		{
			TArray<FString> Text;
			for (const TSharedPtr<FJsonValue>& Warning : *Warnings)
			{
				FString Each;
				if (Warning.IsValid() && Warning->TryGetString(Each))
				{
					Text.Add(Each);
				}
			}
			Entry.Warnings = FString::Join(Text, TEXT(", "));
		}
		const int32 Index = ExperimentalCatalog.Add(MoveTemp(Entry));
		ExperimentalCatalogById.Add(ExperimentalCatalog[Index].TypeId, Index);
	}
	bExperimentalCatalogEnabled = ExperimentalCatalog.Num() > 0;
	UE_LOG(LogReal33D, Warning,
		TEXT("experimental V08 QA catalog ENABLED: %d entries; TEST_IMPORTED is not APPROVED/READY/INTEGRATED"),
		ExperimentalCatalog.Num());
}

bool UReal33DAssetRegistry::TryResolveExperimental(uint16 TypeId,
	FReal33DVisual& OutVisual) const
{
	if (!bExperimentalCatalogEnabled)
	{
		return false;
	}
	const int32* Index = ExperimentalCatalogById.Find(TypeId);
	if (Index == nullptr)
	{
		return false;
	}
	const FReal33DExperimentalCatalogEntry& Entry = ExperimentalCatalog[*Index];
	if (Entry.ImportStatus != TEXT("IMPORTED_OK") || Entry.MeshPath.IsEmpty())
	{
		return false;
	}
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Entry.MeshPath);
	if (Mesh == nullptr)
	{
		UE_LOG(LogReal33D, Warning, TEXT("experimental V08 mesh missing for obj:%u: %s"),
			TypeId, *Entry.MeshPath);
		return false;
	}
	OutVisual.Mesh = Mesh;
	OutVisual.Material = nullptr; // preserve the GLB's imported material slots
	OutVisual.Scale = FVector::OneVector;
	OutVisual.Offset = FVector::ZeroVector;
	OutVisual.Tint = FLinearColor::White;
	OutVisual.bIsPlaceholder = false;
	OutVisual.bIsExperimental = true;
	return true;
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

// Only obj:3501 has approved art. All other identities, including obj:3508,
// continue to resolve as placeholders until separately approved.
//
// The kind is deliberately not guessed from the type id. The authoritative
// classification lives in the master inventory, and inventing an id range here
// would be exactly the hardcoded thing-id-to-mesh mapping this class exists to
// prevent. Until the lookup exists, the caller's structural knowledge decides:
// a tile knows which of its things is the ground because the server's own stack
// order puts it first.

FReal33DVisual UReal33DAssetRegistry::ResolveThing(uint16 TypeId, bool bBlocking) const
{
	FReal33DVisual Experimental;
	if (TryResolveExperimental(TypeId, Experimental))
	{
		return Experimental;
	}
	if (TypeId == 3501 && Mailbox3501Mesh != nullptr)
	{
		FReal33DVisual Visual;
		Visual.Mesh = Mailbox3501Mesh;
		// The glTF import converts its 1-metre tile to 100 Unreal centimetres,
		// Y-up to Z-up, and leaves the source's floor-centred pivot at Z=0.
		Visual.Scale = FVector::OneVector;
		Visual.Offset = FVector::ZeroVector;
		Visual.bIsPlaceholder = false;
		UE_LOG(LogReal33D, Log,
			TEXT("resolved approved visual for obj:3501 (bIsPlaceholder=false)"));
		if (!bMailbox3501EvidenceRequested
			&& FParse::Param(FCommandLine::Get(), TEXT("real33d-evidence-obj3501")))
		{
			FString Directory;
			if (!FParse::Value(FCommandLine::Get(), TEXT("-real33d-evidence="), Directory)
				|| Directory.IsEmpty())
			{
				Directory = FPaths::ProjectSavedDir();
			}
			FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Directory);
			const FString Screenshot = FPaths::Combine(Directory,
				TEXT("obj3501_mailbox_live.png"));
			FScreenshotRequest::RequestScreenshot(Screenshot, false, false);
			bMailbox3501EvidenceRequested = true;
			UE_LOG(LogReal33D, Log, TEXT("requested obj:3501 evidence screenshot: %s"),
				*Screenshot);
		}
		return Visual;
	}
	// The only thing the placeholder is allowed to say about an object is
	// whether Fusion32 will let you walk onto its field, because that is the
	// only thing about it this client actually knows.
	return MakePlaceholder(bBlocking ? EReal33DVisualKind::Obstacle
	                                 : EReal33DVisualKind::Prop);
}

FReal33DVisual UReal33DAssetRegistry::ResolveGround(uint16 TypeId) const
{
	FReal33DVisual Experimental;
	if (TryResolveExperimental(TypeId, Experimental))
	{
		return Experimental;
	}
	return MakePlaceholder(EReal33DVisualKind::Ground);
}

FReal33DVisual UReal33DAssetRegistry::ResolveCreature(uint32 CreatureId,
	bool bIsLocalPlayer) const
{
	(void)CreatureId;
	return MakePlaceholder(bIsLocalPlayer ? EReal33DVisualKind::LocalPlayer
	                                      : EReal33DVisualKind::Creature);
}

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DMailbox3501RegistryTest,
	"REAL33D.AssetRegistry.Mailbox3501",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReal33DMailbox3501RegistryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UReal33DAssetRegistry* Registry = NewObject<UReal33DAssetRegistry>();
	Registry->Initialise();
	TestTrue(TEXT("registry primitives loaded"), Registry->IsReady());

	const FReal33DVisual Approved = Registry->ResolveThing(3501, true);
	TestNotNull(TEXT("obj:3501 has a mesh"), Approved.Mesh.Get());
	TestFalse(TEXT("obj:3501 is production art"), Approved.bIsPlaceholder);
	TestEqual(TEXT("obj:3501 mesh path"), Approved.Mesh->GetPathName(),
		FString(TEXT("/Game/Visual/obj3501/decoration_bottom_overlay_obj3501_mailbox/StaticMeshes/decoration_bottom_overlay_obj3501_mailbox.decoration_bottom_overlay_obj3501_mailbox")));
	TestEqual(TEXT("obj:3501 runtime scale"), Approved.Scale, FVector::OneVector);
	TestEqual(TEXT("obj:3501 floor-centred offset"), Approved.Offset, FVector::ZeroVector);
	// The source carries 1,216 index triangles. Twenty-four have exactly zero
	// area, so Nanite retains the 1,192 triangles that can affect the image.
	// GetNumTriangles reports Nanite's generated fallback, not its main mesh.
	TestEqual(TEXT("obj:3501 non-degenerate Nanite triangles"),
		Approved.Mesh->GetNumNaniteTriangles(), uint32(1192));
	TestEqual(TEXT("obj:3501 material slots"), Approved.Mesh->GetStaticMaterials().Num(), 2);
	const FBox Bounds = Approved.Mesh->GetBoundingBox();
	TestTrue(TEXT("obj:3501 footprint and floor pivot"),
		Bounds.Min.Equals(FVector(-33.0, -32.5, 0.0), 0.05)
		&& Bounds.Max.Equals(FVector(33.0, 32.5, 136.5), 0.05));

	const FReal33DVisual Pending3508 = Registry->ResolveThing(3508, true);
	TestTrue(TEXT("obj:3508 stays a placeholder"), Pending3508.bIsPlaceholder);
	TestNotEqual(TEXT("obj:3508 does not reuse obj:3501"), Pending3508.Mesh.Get(),
		Approved.Mesh.Get());

	for (const uint16 TypeId : { uint16(0), uint16(3500), uint16(3502), uint16(65535) })
	{
		TestTrue(FString::Printf(TEXT("obj:%u stays a placeholder"), TypeId),
			Registry->ResolveThing(TypeId, false).bIsPlaceholder);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReal33DExperimentalV08RegistryTest,
	"REAL33D.AssetRegistry.ExperimentalV08",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReal33DExperimentalV08RegistryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FString CatalogPath;
	if (!FParse::Param(FCommandLine::Get(), TEXT("real33d-experimental-catalog"))
		&& !FParse::Value(FCommandLine::Get(), TEXT("-real33d-experimental-catalog="), CatalogPath))
	{
		AddInfo(TEXT("experimental catalog not enabled; gated test skipped"));
		return true;
	}
	UReal33DAssetRegistry* Registry = NewObject<UReal33DAssetRegistry>();
	Registry->Initialise();
	TestTrue(TEXT("experimental catalog enabled"), Registry->IsExperimentalCatalogEnabled());
	TestEqual(TEXT("all V08 records exposed"), Registry->GetExperimentalCatalog().Num(), 4913);
	const FReal33DVisual Sample = Registry->ResolveThing(602, false);
	TestTrue(TEXT("imported obj:602 marked experimental"), Sample.bIsExperimental);
	TestFalse(TEXT("imported obj:602 is not its placeholder"), Sample.bIsPlaceholder);
	TestNotNull(TEXT("imported obj:602 mesh loaded"), Sample.Mesh.Get());
	return true;
}

#endif
