#include "Real33DGalleryActor.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Real33DAssetRegistry.h"
#include "Real33DGalleryPanel.h"
#include "Widgets/Layout/SBox.h"

AReal33DGalleryActor::AReal33DGalleryActor()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Preview = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Preview"));
	Preview->SetupAttachment(Root);
	Preview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TileReference = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OneTileReference"));
	TileReference->SetupAttachment(Root);
	TileReference->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TileReference->SetRelativeLocation(FVector(0, 0, -1));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->SetRelativeLocation(FVector(350, -550, 300));
	Camera->SetRelativeRotation((FVector::ZeroVector - Camera->GetRelativeLocation()).Rotation());
}

void AReal33DGalleryActor::BeginPlay()
{
	Super::BeginPlay();
	Registry = NewObject<UReal33DAssetRegistry>(this);
	Registry->Initialise();
	if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
	{
		TileReference->SetStaticMesh(Plane); // exactly 100 x 100 UU: one REAL33D tile
	}
	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
	{
		Controller->SetViewTarget(this);
	}
	if (GetWorld()->GetGameViewport() != nullptr)
	{
		Panel = SNew(SReal33DGalleryPanel)
			.Catalog(&Registry->GetExperimentalCatalog())
			.OnSelection(FReal33DGallerySelection::CreateUObject(this, &AReal33DGalleryActor::Select));
		PanelRoot = SNew(SBox).WidthOverride(470).HAlign(HAlign_Left).VAlign(VAlign_Fill).Padding(12)[Panel.ToSharedRef()];
		GetWorld()->GetGameViewport()->AddViewportWidgetContent(PanelRoot.ToSharedRef(), 20);
	}
}

void AReal33DGalleryActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (PanelRoot.IsValid() && GetWorld() != nullptr && GetWorld()->GetGameViewport() != nullptr)
	{
		GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(PanelRoot.ToSharedRef());
	}
	PanelRoot.Reset();
	Panel.Reset();
	Super::EndPlay(Reason);
}

void AReal33DGalleryActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Preview->AddLocalRotation(FRotator(0, DeltaSeconds * 18.0f, 0));
}

void AReal33DGalleryActor::Select(uint16 TypeId)
{
	if (Registry == nullptr) return;
	const FReal33DVisual Visual = Registry->ResolveThing(TypeId, false);
	Preview->SetStaticMesh(Visual.Mesh);
	Preview->EmptyOverrideMaterials();
	Preview->SetRelativeLocation(Visual.Offset);
	Preview->SetRelativeScale3D(Visual.Scale);
	Preview->SetRelativeRotation(Visual.Rotation);
	if (Visual.Material != nullptr) Preview->SetMaterial(0, Visual.Material);
}

void AReal33DGalleryActor::GoToItem(int32 TypeId) { Select((uint16)TypeId); }

