// Copyright 2017 Oh-Hyun Kwon. All Rights Reserved.
// Copyright 2018 David Kuhta. All Rights Reserved for additions.

#include "IGVPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Controller.h"
#include "Runtime/Engine/Classes/Camera/CameraComponent.h"

// VR
#include "IGVGraphActor.h"
#include "IGVLog.h"

AIGVPawn::AIGVPawn() : CursorDistanceScale(0.7)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// LeftGripState
	bLeftGripHeld = false;
	bRightGripHeld = false;

	BaseEyeHeight = 0.0f;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->AttachToComponent(RootComponent,
									   FAttachmentTransformRules::KeepRelativeTransform);

	// Disable positional tracking
	CameraComponent->bUsePawnControlRotation = false;
	CameraComponent->bLockToHmd = true;

	CursorDirectionIndicatorMeshComponent =
		CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CursorDirectionIndicator"));
	CursorDirectionIndicatorMeshComponent->AttachToComponent(
		CameraComponent, FAttachmentTransformRules::KeepRelativeTransform);
	CursorDirectionIndicatorMeshComponent->SetRelativeLocation(FVector(200, 0, 0));
	CursorDirectionIndicatorMeshComponent->SetVisibility(false);

	HelpTextRenderComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HelpText"));
	HelpTextRenderComponent->AttachToComponent(CameraComponent,
											   FAttachmentTransformRules::KeepRelativeTransform);
	HelpTextRenderComponent->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	HelpTextRenderComponent->VerticalAlignment = EVerticalTextAligment::EVRTA_TextCenter;
	HelpTextRenderComponent->SetRelativeLocation(FVector(500, 0, 0));
	HelpTextRenderComponent->SetRelativeRotation((-FVector::ForwardVector).Rotation());
	HelpTextRenderComponent->SetVisibility(false);

	LeftHandComponent = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftHand"));
	LeftHandComponent->Hand = EControllerHand::Left;
	LeftHandComponent->SetupAttachment(RootComponent);

	LCursorMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LCursor"));
	LCursorMeshComponent->AttachToComponent(LeftHandComponent,
		FAttachmentTransformRules::KeepRelativeTransform);
	LCursorMeshComponent->SetRelativeLocation(FVector(400, 0, 0));

	RightHandComponent = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightHand"));
	RightHandComponent->Hand = EControllerHand::Right;
	RightHandComponent->SetupAttachment(RootComponent);

	RCursorMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RCursor"));
	RCursorMeshComponent->AttachToComponent(RightHandComponent,
		FAttachmentTransformRules::KeepRelativeTransform);
	RCursorMeshComponent->SetRelativeLocation(FVector(400, 0, 0));

	// Selected Node Details
	NodeDetailComponent = CreateDefaultSubobject<UIGVNodeDetailWidgetComponent>(TEXT("NodeDetail"));
	NodeDetailComponent->AttachToComponent(LeftHandComponent, FAttachmentTransformRules::KeepRelativeTransform);
	NodeDetailComponent->SetVisibility(false);

	// Selected Graph Details
	GraphDetailsComponent = CreateDefaultSubobject<UIGVGraphDetailsWidgetComponent>(TEXT("GraphDetails"));
	GraphDetailsComponent->AttachToComponent(RightHandComponent, FAttachmentTransformRules::KeepRelativeTransform);
	GraphDetailsComponent->SetVisibility(false);

	ShowGraphDetails = false;
	ShowNodeDetails = false;

	PointingMap.Add(EControllerHand::Left, 0.f);
	PointingMap.Add(EControllerHand::Right, 0.f);
}

void AIGVPawn::BeginPlay()
{
	Super::BeginPlay();
}

void AIGVPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	DrawLaser(LeftHandComponent);
	DrawLaser(RightHandComponent);
	if (bRightGripHeld)
	{
		OnRightGripPressed();
	}
	if (bLeftGripHeld)
	{
		OnLeftGripPressed();
	}
	if (ShowGraphDetails)
	{
		UpdateGraphDetailsWidget();
	}
	//UpdateCursor();
}

void AIGVPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InputComponent->BindAction(TEXT("MotionController_Left_Trigger"), IE_Released, this,
		&AIGVPawn::OnLeftTriggerButtonReleased);
	InputComponent->BindAction(TEXT("MotionController_Right_Trigger"), IE_Released, this,
		&AIGVPawn::OnRightTriggerButtonReleased);
	InputComponent->BindAction(TEXT("MotionController_Left_Grip1"), IE_Pressed, this,
		&AIGVPawn::OnLeftGripPressed);
	InputComponent->BindAction(TEXT("MotionController_Left_Grip1"), IE_Released, this,
		&AIGVPawn::OnLeftGripReleased);
	InputComponent->BindAction(TEXT("MotionController_Right_Grip1"), IE_Pressed, this,
		&AIGVPawn::OnRightGripPressed);
	InputComponent->BindAction(TEXT("MotionController_Right_Grip1"), IE_Released, this,
		&AIGVPawn::OnRightGripReleased);
	InputComponent->BindAction(TEXT("MotionController_Right_Thumbstick"), IE_Pressed, this,
		&AIGVPawn::OnRightThumbstickPressed);
	InputComponent->BindAction(TEXT("MotionController_Right_Thumbstick"), IE_Released, this,
		&AIGVPawn::OnRightThumbstickReleased);
	InputComponent->BindAction(TEXT("OculusTouch_Right_FaceButton1"), IE_Pressed, this,
		&AIGVPawn::OnATouched);
	InputComponent->BindAction(TEXT("OculusTouch_Right_FaceButton1"), IE_Released, this,
		&AIGVPawn::OnATouchReleased);
	InputComponent->BindAction(TEXT("OculusTouch_Right_FaceButton2"), IE_Pressed, this,
		&AIGVPawn::OnBTouched);
	InputComponent->BindAction(TEXT("OculusTouch_Right_FaceButton2"), IE_Released, this,
		&AIGVPawn::OnBTouchReleased);
	InputComponent->BindAction(TEXT("MotionController_Left_FaceButton1"), IE_Pressed, this,
		&AIGVPawn::OnYTouched);
	InputComponent->BindAction(TEXT("MotionController_Left_FaceButton1"), IE_Released, this,
		&AIGVPawn::OnYTouchReleased);
	InputComponent->BindAction(TEXT("MotionController_Left_FaceButton1"), IE_DoubleClick, this,
		&AIGVPawn::OnYDoublePressed);
	InputComponent->BindAction(TEXT("MotionController_Left_Thumbstick"), IE_Pressed, this,
		&AIGVPawn::OnLeftThumbstickPressed);
	InputComponent->BindAction(TEXT("MotionController_Left_Thumbstick"), IE_Released, this,
		&AIGVPawn::OnLeftThumbstickReleased);
	InputComponent->BindAction(TEXT("MotionController_Left_Thumbstick"), IE_DoubleClick, this,
		&AIGVPawn::OnLeftThumbstickDoublePressed);
	InputComponent->BindAxis(TEXT("OculusTouch_Left_FaceButton2"), this, &AIGVPawn::OnLeftXCapTouch);
	InputComponent->BindAxis(TEXT("OculusTouch_Left_IndexPointing"), this, &AIGVPawn::OnLeftIndexPointing);
	InputComponent->BindAxis(TEXT("OculusTouch_Right_IndexPointing"), this, &AIGVPawn::OnRightIndexPointing);
}

void AIGVPawn::AddControllerYawInput(float Value)
{
	APlayerController* const PlayerController = Cast<APlayerController>(Controller);
	if (PlayerController)
	{
		PickRayRotation.Yaw += Value * PlayerController->InputYawScale;
	}
}

void AIGVPawn::AddControllerPitchInput(float Value)
{
	APlayerController* const PlayerController = Cast<APlayerController>(Controller);
	if (PlayerController)
	{
		PickRayRotation.Pitch += -Value * PlayerController->InputPitchScale;
	}
}

void AIGVPawn::DrawLaser(UMotionControllerComponent* HandComponent)
{
	APlayerController* const PlayerController = Cast<APlayerController>(Controller);

	//Hit contains information about what the raycast hit.
	FHitResult Hit;

	//PickRayOrigin = GetActorLocation();
	PickRayOrigin = HandComponent->GetComponentLocation(); //FVector LeftPos?
	//PickRayDirection = PickRayRotation.Vector().GetSafeNormal();
	PickRayRotation = HandComponent->GetComponentRotation();
	PickRayDirection = PickRayRotation.Vector().GetSafeNormal();
	PickRayInformation.Add(HandComponent->Hand, MakeTuple(PickRayOrigin, PickRayRotation));

	FVector CursorWorldPosition =
		PickRayOrigin + CursorDistanceScale * GraphActor->GetSphereRadius() * PickRayDirection;

	TArray<USceneComponent*> children = HandComponent->GetAttachChildren();
	auto FilteredArray = children.FilterByPredicate([](const USceneComponent* child) {
		return child->GetName().Find(TEXT("Cursor")) != 0;
	});

	//IGV_LOG_S(Warning, TEXT("Actor %s"), *FilteredArray[0]->GetName());
	//for (USceneComponent* child : children) {
	//	IGV_LOG_S(Warning, TEXT("Actor %s"), *child->GetName());
	//}

	auto CursorMeshComponent = FilteredArray[0];
	CursorMeshComponent->SetWorldLocation(CursorWorldPosition);
	CursorMeshComponent->SetWorldRotation(PickRayRotation);

	//Collision parameters. The following syntax means that we don't want the trace to be complex
	FCollisionQueryParams CollisionParameters;

	//Perform the line trace
	//The ECollisionChannel parameter is used in order to determine what we are looking for when performing the raycast
	ActorLineTraceSingle(Hit, PickRayOrigin, CursorWorldPosition, ECollisionChannel::ECC_WorldDynamic, CollisionParameters);

	//DrawDebugLine is used in order to see the raycast we performed
	//The boolean parameter used here means that we want the lines to be persistent so we can see the actual raycast
	//The last parameter is the width of the lines.
	if (PointingMap[HandComponent->Hand] == 1.0)
	{
		DrawDebugLine(GetWorld(), PickRayOrigin, CursorWorldPosition, FColor::Green, false, -1, 0, 1.f);
	}

	//if ((GetViewRotation().Vector() | PickRayDirection) < 0.76604444311 /* cos 40 deg */)
	/*{
		CursorDirectionIndicatorMeshComponent->SetVisibility(true);
		FVector const RelativeDirection = CameraComponent->GetComponentTransform()
			.InverseTransformVectorNoScale(PickRayDirection)
			.GetSafeNormal();
		FVector const TargetDirection =
			FVector(0, RelativeDirection.Y, RelativeDirection.Z).GetSafeNormal();
		CursorDirectionIndicatorMeshComponent->SetRelativeLocation(FVector(200, 0, 0) +
			TargetDirection * 85);
		CursorDirectionIndicatorMeshComponent->SetRelativeRotation(TargetDirection.Rotation());
	}
	else
	{
		CursorDirectionIndicatorMeshComponent->SetVisibility(false);
	}*/
}

void AIGVPawn::OnLeftXCapTouch(float Value)
{
	IGV_LOG(Log, TEXT("LeftXCapTouch. Value: %f"), Value);

	if (GraphActor && (Value == 1.0))
	{
		IGV_LOG(Log, TEXT("LeftXCapTouch Touched"));
		ShowNodeDetails = true;
		GraphActor->OnLeftXCapTouch();
	}
	else
	{
		ShowNodeDetails = false;
		NodeDetailComponent->SetVisibility(false);
	}
}

void AIGVPawn::OnLeftIndexPointing(float Value)
{
	IGV_LOG(Log, TEXT("LeftIndexPointing. Value: %f"), Value);

	PointingMap[EControllerHand::Left] = Value;

}

void AIGVPawn::OnRightIndexPointing(float Value)
{
	IGV_LOG(Log, TEXT("RightIndexPointing. Value: %f"), Value);

	PointingMap[EControllerHand::Right] = Value;

}

void AIGVPawn::OnLeftTriggerButtonReleased()
{
	if (GraphActor)
	{
		GraphActor->OnLeftTriggerButtonReleased();
	}
}

void AIGVPawn::OnRightTriggerButtonReleased()
{
	if (GraphActor)
	{
		GraphActor->OnRightTriggerButtonReleased();
	}
}

void AIGVPawn::OnLeftGripPressed()
{
	auto Hand = EControllerHand::Left;
	auto PROrigin = PickRayInformation[Hand].Key;
	auto PRRotation = PickRayInformation[Hand].Value;
	auto PRDirection = PRRotation.Vector().GetSafeNormal();

	FRotator LeftRotation = LeftHandComponent->GetComponentRotation().GetNormalized();
	if (!bLeftGripHeld) {
		bLeftGripHeld = true;
		LastLeftRotation = LeftRotation;
	}
	//IGV_LOG_S(Log, TEXT("Left Grip Pressed")); //Change Warning to Log
	if (GraphActor)
	{
		if (LastLeftRotation != LeftRotation)
		{
			FRotator DeltaLeftRotation = LeftRotation - LastLeftRotation;
			//IGV_LOG_S(Log, TEXT("Left Grip Pressed")); //Change Warning to Log
			//IGV_LOG_S(Log, TEXT("LeftRotation is %s"), *LeftRotation.ToString());
			//FRotator NormalizedLeftRotation = LeftRotation.GetNormalized();
			//IGV_LOG_S(Log, TEXT("NormalizedLeftRotation is %s"), *NormalizedLeftRotation.ToString());
			//GraphActor->OnLeftGripPressed(NormalizedLeftRotation);
			//GraphActor->OnLeftGripPressed(DeltaLeftRotation);
			GraphActor->OnLeftGripPressed(PROrigin, PRRotation);
		}
		LastLeftRotation = LeftRotation;
	}
}

void AIGVPawn::OnLeftGripReleased()
{
	bLeftGripHeld = false;
}

void AIGVPawn::OnRightGripPressed()
{
	FRotator RightRotation = RightHandComponent->GetComponentRotation().GetNormalized();
	if (!bRightGripHeld) {
		bRightGripHeld = true;
		LastRightRotation = RightRotation;
	}
	//IGV_LOG_S(Log, TEXT("Left Grip Pressed")); //Change Warning to Log
	if (GraphActor)
	{
		if (LastRightRotation != RightRotation)
		{
			FRotator DeltaRightRotation = RightRotation - LastRightRotation;
			//IGV_LOG_S(Log, TEXT("Left Grip Pressed")); //Change Warning to Log
			//IGV_LOG_S(Log, TEXT("LeftRotation is %s"), *LeftRotation.ToString());
			//FRotator NormalizedLeftRotation = LeftRotation.GetNormalized();
			//IGV_LOG_S(Log, TEXT("NormalizedLeftRotation is %s"), *NormalizedLeftRotation.ToString());
			//GraphActor->OnLeftGripPressed(NormalizedLeftRotation);
			GraphActor->OnRightGripPressed(DeltaRightRotation);
		}
		LastRightRotation = RightRotation;
	}
}

void AIGVPawn::OnRightGripReleased()
{
	bRightGripHeld = false;
}

void AIGVPawn::UpdateNodeDetailsWidget(class AIGVNodeActor* LeftPickedNode)
{
	auto NodeDetailUserWidget = (UIGVNodeDetailsUserWidget*)NodeDetailComponent->GetUserWidgetObject();
	class AIGVNodeActor* DisplayedNode = NodeDetailUserWidget->DisplayedNode;
	if (DisplayedNode != LeftPickedNode)
	{
		int32 NodeIndex = LeftPickedNode->Idx;
		int32 NodeEdgesNum = LeftPickedNode->Edges.Num();
		int32 NodeYear = LeftPickedNode->Year;
		FString NodeIndexString = FString::FromInt(NodeIndex);
		FString NodeEdgesNumString = FString::FromInt(NodeEdgesNum);
		FString NodeYearString = FString::FromInt(NodeYear);

		NodeDetailUserWidget->NodeId = NodeIndexString;
		NodeDetailUserWidget->PersonName = LeftPickedNode->Label;
		NodeDetailUserWidget->PersonBorn = NodeYearString;
		NodeDetailUserWidget->NodeEdges = NodeEdgesNumString;

		FSlateBrush ImageBrush = ((UIGVNodeImageUserWidget*)(LeftPickedNode->ImageComponent)->GetUserWidgetObject())->NodeBrush;
		NodeDetailUserWidget->NodeBrush = ImageBrush;

		NodeDetailComponent->SetVisibility(true);
	}
}

void AIGVPawn::OnYTouched()
{
	IGV_LOG(Log, TEXT("OnYTouched"));
	GraphActor->OnYReleased();
}

void AIGVPawn::OnYTouchReleased()
{
	IGV_LOG(Log, TEXT("OnYTouchReleased"));
}

void AIGVPawn::OnYDoublePressed()
{
	IGV_LOG(Log, TEXT("OnYDoublePressed"));
}


void AIGVPawn::OnLeftThumbstickDoublePressed()
{
	GraphActor->OnLeftThumbstickDoublePressed();
}


void AIGVPawn::OnLeftThumbstickPressed()
{
	GraphActor->OnLeftThumbstickPressed();
}

void AIGVPawn::OnLeftThumbstickReleased()
{

}

void AIGVPawn::OnATouched()
{

}

void AIGVPawn::OnATouchReleased()
{
	if (ShowGraphDetails)
	{
		GraphActor->ToggleFOV();
	}
}

void AIGVPawn::OnBTouched()
{

}

void AIGVPawn::OnBTouchReleased()
{
	if (ShowGraphDetails)
	{
		GraphActor->ToggleAspectRatio();
	}
}

void AIGVPawn::OnRightThumbstickPressed()
{
	IGV_LOG(Log, TEXT("LeftThumbstickPressed."));

	ShowGraphDetails = !ShowGraphDetails;

	if (ShowGraphDetails)
	{
		UpdateGraphDetailsWidget();
	}
	GraphDetailsComponent->SetVisibility(ShowGraphDetails);
}

void AIGVPawn::OnRightThumbstickReleased()
{

}

void AIGVPawn::SetupGraphDetailsWidget()
{
	auto GraphDetailsUserWidget = (UIGVGraphDetailsUserWidget*)GraphDetailsComponent->GetUserWidgetObject();

	int32 NodesNum = GraphActor->Nodes.Num();
	int32 EdgesNum = GraphActor->Edges.Num();
	FString NodesNumString = FString::FromInt(NodesNum);
	FString EdgesNumString = FString::FromInt(EdgesNum);

	GraphDetailsUserWidget->NumNodes = NodesNumString;
	GraphDetailsUserWidget->NumEdges = EdgesNumString;
}

void AIGVPawn::UpdateGraphDetailsWidget()
{
	auto GraphDetailsUserWidget = (UIGVGraphDetailsUserWidget*)GraphDetailsComponent->GetUserWidgetObject();

	FString AspectRatio = GraphActor->AspectRatioToString();
	int32 FOV = GraphActor->FieldOfView;
	FString FOVString = FString::FromInt(FOV);
	FOVString += FString("deg");

	auto HighlightedLevelScale = GraphActor->HighlightedLevelScale;
	int32 PickedCount = 0;
	for (AIGVNodeActor* const Node : GraphActor->Nodes)
	{
		if (Node->LevelScale == HighlightedLevelScale) PickedCount += 1;
	}

	FString PickedCountString = FString::FromInt(PickedCount);

	GraphDetailsUserWidget->NumPicked = PickedCountString;
	GraphDetailsUserWidget->FOV = FOVString;
	GraphDetailsUserWidget->AspectRatio = AspectRatio;
}
