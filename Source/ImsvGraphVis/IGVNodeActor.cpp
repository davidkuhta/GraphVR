// Copyright 2017 Oh-Hyun Kwon. All Rights Reserved.

#include "IGVNodeActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "Runtime/Engine/Classes/Materials/Material.h"
#include "Runtime/Engine/Classes/Materials/MaterialInstanceDynamic.h"
#include <string>

#include "IGVGraphActor.h"
#include "IGVLog.h"

UMaterialInterface* GetNodeMaterial()
{
	static ConstructorHelpers::FObjectFinder<UMaterial> MaterialAsset(
		TEXT("/Game/Materials/M_Node.M_Node"));
	return MaterialAsset.Succeeded() ? MaterialAsset.Object->GetMaterial() : nullptr;
}

UMaterialInterface* GetTranslucentNodeMaterial()
{
	static ConstructorHelpers::FObjectFinder<UMaterial> MaterialAsset(
		TEXT("/Game/Materials/M_Node_Trans.M_Node_Trans"));
	return MaterialAsset.Succeeded() ? MaterialAsset.Object->GetMaterial() : nullptr;
}

AIGVNodeActor::AIGVNodeActor()
	: GraphActor(nullptr),
	  Label("Unknown"),
	  Pos2D(FVector2D::ZeroVector),
	  Pos3D(FVector::ZeroVector),
	  LevelScale(1.f),
	  LevelScaleBeforeTransition(1.f),
	  LevelScaleAfterTransition(1.f),
	  Color(FLinearColor::White),
	  BaseColor(FLinearColor::White),
	  DistanceToPickRay(FLT_MAX),
	  bIsHighlighted(false),
	  NumHighlightedNeighbors(0),
	  MeshMaterialInstance(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Node Mesh"));
	RootComponent = MeshComponent;

	TextRenderComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Node Text"));
	TextRenderComponent->AttachToComponent(RootComponent,
										   FAttachmentTransformRules::KeepRelativeTransform);
	TextRenderComponent->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	TextRenderComponent->VerticalAlignment = EVerticalTextAligment::EVRTA_TextTop;
	TextRenderComponent->SetRelativeLocation(FVector(20, 0, -12));
	TextRenderComponent->SetWorldSize(20);
	TextRenderComponent->SetVisibility(false);

	// Selected Node Image
	ImageComponent = CreateDefaultSubobject<UIGVNodeImageWidgetComponent>(TEXT("NodeImage"));
	ImageComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	ImageComponent->SetRelativeLocation(FVector(0, 0, 35));
	ImageComponent->SetVisibility(false);

	GetNodeMaterial();
	GetTranslucentNodeMaterial();

	DistanceToPickRays.Add(EControllerHand::Left, FLT_MAX);  //DPK Added
	DistanceToPickRays.Add(EControllerHand::Right, FLT_MAX); // DPK Added

	bIsHighlightedMap.Add(EControllerHand::Left, false);  //DPK Added
	bIsHighlightedMap.Add(EControllerHand::Right, false); // DPK Added
}

void AIGVNodeActor::Init(AIGVGraphActor* const InGraphActor)
{
	GraphActor = InGraphActor;

	MeshMaterialInstance = UMaterialInstanceDynamic::Create(GetNodeMaterial(), this);
	if (MeshMaterialInstance) MeshComponent->SetMaterial(0, MeshMaterialInstance);

	LevelScale = LevelScaleBeforeTransition = LevelScaleAfterTransition =
		GraphActor->DefaultLevelScale;
}

void AIGVNodeActor::BeginPlay()
{
	Super::BeginPlay();
}

void AIGVNodeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FString AIGVNodeActor::ToString() const
{
	TArray<FString> AncStrs;
	for (int32 const ClusterIdx : AncIdxs)
	{
		AncStrs.Add(FString::FromInt(ClusterIdx));
	}

	return FString::Printf(TEXT("Idx=%d Label=%s Pos2D=(%s) Pos3D=(%s) Ancestors=[%s]"), Idx,
						   *Label, *Pos2D.ToString(), *Pos3D.ToString(),
						   *FString::Join(AncStrs, TEXT(" ")));
}

void AIGVNodeActor::SetPos3D()
{
	Pos3D = GraphActor->Project(Pos2D);
	RootComponent->SetRelativeLocation(Pos3D * LevelScale * GraphActor->GetSphereRadius());
	UpdateRotation();
}

void AIGVNodeActor::UpdateColor(FLinearColor const& C)
{
	BaseColor = C;
	SetColor(BaseColor);
	//MeshMaterialInstance->SetVectorParameterValue(TEXT("Base Color"), Color);
}

void AIGVNodeActor::SetColor(FLinearColor const& C)
{
	Color = C;
	ShowColor(Color);
}

void AIGVNodeActor::ResetColor()
{
	SetColor(BaseColor);
}

void AIGVNodeActor::ShowColor(FLinearColor const& C)
{
	if (MeshMaterialInstance != nullptr)
		MeshMaterialInstance->SetVectorParameterValue(TEXT("Base Color"), C);
}

void AIGVNodeActor::SetHalo(bool const bValue)
{
	MeshComponent->SetRenderCustomDepth(bValue);
}

void AIGVNodeActor::SetText(FString const& Value)
{
	TextRenderComponent->SetText(FText::FromString(Value));
}

void AIGVNodeActor::SetImage(FString const& Name)
{
	auto NodeImageUserWidget = (UIGVNodeImageUserWidget*)ImageComponent->GetUserWidgetObject();
	//std::string InputName = std::string(TCHAR_TO_UTF8(*Name));
	//std::transform(InputName.begin(), InputName.end(), InputName.begin(), [](char ch) {
	//	return ch == ' ' ? '_' : ch;
	//});
	//FString ProcessedName(InputName.c_str());

	// Formulate Image Path
	auto ProcessedName = Name.Replace(TEXT(" "), TEXT("_")).Replace(TEXT("'"), TEXT("_")).Replace(TEXT("."), TEXT("_"));
	FString ImagePath = TEXT("/Game/ActorImages/");
	ImagePath += ProcessedName;
	ImagePath += ".";
	ImagePath += ProcessedName;

	//NodeImageUserWidget->SetRenderScale(FVector(2.0f, 2.0f, 1.0f));
	//NodeImageUserWidget->NodeBrush.ImageSize = FVector2D(300, 400);
	FSlateBrush Brush = NodeImageUserWidget->NodeBrush;
	IGV_LOG(Log, TEXT("OldResourceName: %s"), *(NodeImageUserWidget->NodeBrush.GetResourceName().ToString()));
	//FString ImagePath = FPaths::ProjectContentDir() / TEXT("ActorImages/Keanu_Reeves.Keanu_Reeves");
	IGV_LOG(Log, TEXT("ImagePath: %s"), *ImagePath);
	//auto ImageTexture = LoadObject<UTexture2D>(NULL, TEXT("/Game/ActorImages/Keanu_Reeves.Keanu_Reeves"), NULL, LOAD_None, NULL);
	auto ImageTexture = LoadObject<UTexture2D>(NULL, *ImagePath, NULL, LOAD_None, NULL);
	if (!ImageTexture == NULL) {
		(NodeImageUserWidget->NodeBrush).SetResourceObject(ImageTexture);
	}
	IGV_LOG(Log, TEXT("NewResourceName: %s"), *(NodeImageUserWidget->NodeBrush.GetResourceName().ToString()));
	//NodeImageUserWidget->SetDesiredSizeInViewport(FVector2D(300.0f, 400.0f));
	ImageComponent->SetDrawSize(FVector2D(60.f, 80.f));
	ImageComponent->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
}

void AIGVNodeActor::UpdateRotation()
{
	RootComponent->SetRelativeRotation((FVector::ZeroVector - Pos3D).Rotation());
}

bool AIGVNodeActor::IsPicked() const
{
	return DistanceToPickRay < GraphActor->PickDistanceThreshold;
}

bool AIGVNodeActor::IsNearPicked() const
{
	return DistanceToPickRay < GraphActor->SelectAllDistanceThreshold;
}

bool AIGVNodeActor::IsPicked(enum class EControllerHand Hand) const
{
	return DistanceToPickRays[Hand] < GraphActor->PickDistanceThreshold;
}

void AIGVNodeActor::BeginNearest()
{
}

void AIGVNodeActor::EndNearest()
{
}

/*void AIGVNodeActor::BeginPicked()
{
	TextRenderComponent->SetVisibility(true);
}*/

void AIGVNodeActor::BeginPicked(enum class EControllerHand Hand, bool ShowImage)
{
	TextRenderComponent->SetVisibility(true);
	if (ShowImage)
	{
		ImageComponent->SetVisibility(true);
	}
	//IGV_LOG(Log, TEXT("Begin Picked Called Hand"));
	//IGV_LOG(Log, TEXT("Begin Picked Called Hand for: %s"), ToString());
	if (Hand == EControllerHand::Right)
	{
		ShowColor(FLinearColor::White);
	}

	/*if (Hand == EControllerHand::Left)
	{
		MeshMaterialInstance = UMaterialInstanceDynamic::Create(GetTranslucentNodeMaterial(), this);
		if (MeshMaterialInstance) MeshComponent->SetMaterial(0, MeshMaterialInstance);
	}*/
}

/*void AIGVNodeActor::EndPicked()
{
	if (!bIsHighlighted) TextRenderComponent->SetVisibility(false);
}*/

void AIGVNodeActor::EndPicked(enum class EControllerHand Hand)
{
	TArray<bool> HighlightValues;
	bIsHighlightedMap.GenerateValueArray(HighlightValues);

	if (!HighlightValues.Contains(true)) {
		TextRenderComponent->SetVisibility(false);
		ImageComponent->SetVisibility(false);
	}
	//IGV_LOG(Log, TEXT("End Picked Called Hand for: %s"), ToString());
	if (Hand == EControllerHand::Right)
	{
		SetColor(Color);
	}

	/*if (Hand == EControllerHand::Left)
	{
		MeshMaterialInstance = UMaterialInstanceDynamic::Create(GetNodeMaterial(), this);
		if (MeshMaterialInstance)
		{
			MeshComponent->SetMaterial(0, MeshMaterialInstance);
			SetColor(Color);
		}
	}*/
}

void AIGVNodeActor::BeginHighlighted(enum class EControllerHand Hand)
{
	bIsHighlightedMap[Hand] = true;
	TextRenderComponent->SetVisibility(true);
	if (Hand == EControllerHand::Right)
	{
		bIsHighlighted = true;
		SetHalo(true);

		LevelScaleAfterTransition = GraphActor->HighlightedLevelScale;
		BeginTransition();

		for (AIGVNodeActor* const Neighbor : Neighbors)
		{
			Neighbor->BeginNeighborHighlighted();
		}
	}

	if (Hand == EControllerHand::Left)
	{
		MeshMaterialInstance = UMaterialInstanceDynamic::Create(GetTranslucentNodeMaterial(), this);
		if (MeshMaterialInstance) MeshComponent->SetMaterial(0, MeshMaterialInstance);
		for (FIGVEdge* const Edge : Edges)
		{
			Edge->bUpdateMeshRequired = true;
		}
	}
}

void AIGVNodeActor::BeginHighlighted()
{
	bIsHighlighted = true;
	SetHalo(true);
	TextRenderComponent->SetVisibility(true);

	LevelScaleAfterTransition = GraphActor->HighlightedLevelScale;
	BeginTransition();

	for (AIGVNodeActor* const Neighbor : Neighbors)
	{
		Neighbor->BeginNeighborHighlighted();
	}
}

void AIGVNodeActor::EndHighlighted(enum class EControllerHand Hand)
{
	bIsHighlightedMap[Hand] = false;
	TextRenderComponent->SetVisibility(false);

	if (Hand == EControllerHand::Right)
	{
		bIsHighlighted = false;

		if (HasHighlightedNeighbor())
		{
			LevelScaleAfterTransition = GraphActor->NeighborHighlightedLevelScale;
			SetHalo(true);
		}
		else
		{
			LevelScaleAfterTransition = GraphActor->DefaultLevelScale;
			SetHalo(false);
		}

		BeginTransition();

		for (AIGVNodeActor* const Neighbor : Neighbors)
		{
			Neighbor->EndNeighborHighlighted();
		}
	}

	if (Hand == EControllerHand::Left)
	{
		MeshMaterialInstance = UMaterialInstanceDynamic::Create(GetNodeMaterial(), this);
		if (MeshMaterialInstance)
		{
			MeshComponent->SetMaterial(0, MeshMaterialInstance);
			SetColor(Color);
		}
		for (FIGVEdge* const Edge : Edges)
		{
			Edge->bUpdateMeshRequired = true;
		}
	}
}

void AIGVNodeActor::EndHighlighted()
{
	bIsHighlighted = false;
	TextRenderComponent->SetVisibility(false);

	if (HasHighlightedNeighbor())
	{
		LevelScaleAfterTransition = GraphActor->NeighborHighlightedLevelScale;
		SetHalo(true);
	}
	else
	{
		LevelScaleAfterTransition = GraphActor->DefaultLevelScale;
		SetHalo(false);
	}

	BeginTransition();

	for (AIGVNodeActor* const Neighbor : Neighbors)
	{
		Neighbor->EndNeighborHighlighted();
	}
}

void AIGVNodeActor::BeginNeighborHighlighted()
{
	bool const TransionRequired = !(bIsHighlightedMap[EControllerHand::Right] || HasHighlightedNeighbor());

	NumHighlightedNeighbors++;

	if (TransionRequired)
	{
		LevelScaleAfterTransition = GraphActor->NeighborHighlightedLevelScale;
		BeginTransition();
		SetHalo(true);
	}
}

void AIGVNodeActor::EndNeighborHighlighted()
{
	NumHighlightedNeighbors--;
	check(NumHighlightedNeighbors >= 0);

	bool const TransionRequired = !(bIsHighlightedMap[EControllerHand::Right] || HasHighlightedNeighbor());

	if (TransionRequired)
	{
		LevelScaleAfterTransition = GraphActor->DefaultLevelScale;
		BeginTransition();
		SetHalo(false);
	}
}

bool AIGVNodeActor::HasHighlightedNeighbor() const
{
	return NumHighlightedNeighbors > 0;
}

void AIGVNodeActor::BeginTransition()
{
	LevelScaleBeforeTransition = LevelScale;

	for (FIGVEdge* const Edge : Edges)
	{
		Edge->BeginTransition();
	}

	PlayFromStartHighlightTransitionTimeline();
}

/*void AIGVNodeActor::OnLeftMouseButtonReleased()
{
	check(IsPicked(EControllerHand::Right));

	if (bIsHighlighted)
	{
		EndHighlighted();
	}
	else
	{
		BeginHighlighted();
	}
}*/

void AIGVNodeActor::OnRightTriggerButtonReleased()
{
	auto Hand = EControllerHand::Right;
	check(IsPicked(Hand));

	if (bIsHighlightedMap[Hand])
	{
		EndHighlighted(Hand);
	}
	else
	{
		BeginHighlighted(Hand);
	}
}

void AIGVNodeActor::OnLeftTriggerButtonReleased()
{
	auto Hand = EControllerHand::Left;
	check(IsPicked(Hand));

	if (bIsHighlightedMap[Hand])
	{
		EndHighlighted(Hand);
	}
	else
	{
		BeginHighlighted(Hand);
	}
}

void AIGVNodeActor::OnLeftGripPressed(FVector PickRayOrigin, FRotator PickRayRotation)
{
	//FRotator Test = FRotator(0.0, 45.0, 0.0);
	//SetActorRotation(Test);
	//SetActorRotation(FMath::Lerp(GetActorRotation(), Test, 0.05f));
	//FRotator RootRotation = RootComponent->GetComponentRotation();
	//FRotator NewRotation = RootRotation + PickRayRotation;
	//RootComponent->SetWorldRotation(NewRotation);
	//RootComponent->SetRelativeRotation(NewRotation);
	//RootComponent->SetComponentRotation(NewRotation);
	//SetActorRelativeRotation(NewRotation);
	//SetWorldRotation(PickRayRotation);
	//SetRelativeRotation(PickRayRotation);
	//SetComponentRotation(PickRayRotation);
	//SetActorRelativeRotation(PickRayRotation);
	//AddActorWorldRotation(PickRayRotation);
	//RootComponent->AddLocalRotation(PickRayRotation, false);
	FVector PickRayDirection = PickRayRotation.Vector().GetSafeNormal();
	//FVector CursorWorldPosition = PickRayOrigin + CursorDistanceScale * GraphActor->GetSphereRadius() * PRDirection;
	FVector NewWorldPosition = PickRayOrigin + 0.9 * LevelScale * GraphActor->GetSphereRadius() * PickRayDirection;

	RootComponent->SetWorldRotation(NewWorldPosition.Rotation());
	RootComponent->SetWorldLocation(NewWorldPosition);
	AddActorLocalRotation(FRotator(0, 180.0, 0));

}

void AIGVNodeActor::OnHighlightTransitionTimelineUpdate(ETimelineDirection::Type const Direction,
														float const Alpha)
{
	LevelScale = FMath::Lerp(LevelScaleBeforeTransition, LevelScaleAfterTransition, Alpha);
	SetPos3D();

	for (FIGVEdge* const Edge : Edges)
	{
		Edge->OnHighlightTransitionTimelineUpdate(Direction, Alpha);
	}
}

void AIGVNodeActor::OnHighlightTransitionTimelineFinished(ETimelineDirection::Type const Direction)
{
	for (FIGVEdge* const Edge : Edges)
	{
		Edge->OnHighlightTransitionTimelineFinished(Direction);
	}
}
