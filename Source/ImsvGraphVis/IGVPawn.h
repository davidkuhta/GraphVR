// Copyright 2017 Oh-Hyun Kwon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "HeadMountedDisplayFunctionLibrary.h"
#include "MotionControllerComponent.h"
#include "InputCoreTypes.h" 
#include "Components/WidgetComponent.h" //DPK

#include "IGVNodeDetailWidgetComponent.h"
#include "IGVNodeDetailsUserWidget.h"
#include "IGVGraphDetailsWidgetComponent.h"
#include "IGVGraphDetailsUserWidget.h"
#include "IGVNodeActor.h"

#include "IGVPawn.generated.h"

UCLASS()
class IMSVGRAPHVIS_API AIGVPawn : public APawn
{
	GENERATED_BODY()

public:
	class AIGVGraphActor* GraphActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	class UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	class UStaticMeshComponent* LCursorMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	class UStaticMeshComponent* RCursorMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	class UStaticMeshComponent* CursorDirectionIndicatorMeshComponent;

	UPROPERTY(BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	float CursorDistanceScale;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	class UTextRenderComponent* HelpTextRenderComponent;

	UPROPERTY(BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FRotator PickRayRotation;

	UPROPERTY(BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FVector PickRayOrigin;

	UPROPERTY(BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FVector PickRayDirection;
	
	/* Motion Controllers */
	UPROPERTY(EditDefaultsOnly, Category = ImmersiveGraphVisualization)
	class UMotionControllerComponent* LeftHandComponent;

	UPROPERTY(EditDefaultsOnly, Category = ImmersiveGraphVisualization)
	class UMotionControllerComponent* RightHandComponent;

	FRotator LastLeftRotation;
	FRotator LastRightRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	class UIGVNodeDetailWidgetComponent* NodeDetailComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	class UIGVGraphDetailsWidgetComponent* GraphDetailsComponent;

	bool ShowGraphDetails;
	bool ShowNodeDetails;
	void SetupGraphDetailsWidget();
	void UpdateGraphDetailsWidget();

public:
	AIGVPawn();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void AddControllerYawInput(float Value) override;
	virtual void AddControllerPitchInput(float Value) override;

	//void UpdateCursor();
	void DrawLaser(class UMotionControllerComponent* HandComponent);

	void OnLeftTriggerButtonReleased();
	void OnRightTriggerButtonReleased();

	void OnLeftGripPressed();
	void OnLeftGripReleased();
	void OnRightGripPressed();
	void OnRightGripReleased();
	void OnRightThumbstickPressed();
	void OnRightThumbstickReleased();
	void OnLeftThumbstickPressed();
	void OnLeftThumbstickReleased();
	void OnLeftThumbstickDoublePressed();

	void OnLeftXCapTouch(float Value);
	void OnLeftIndexPointing(float Value);
	void OnRightIndexPointing(float Value);

	TMap<enum class EControllerHand, float> PointingMap;
	TMap<enum class EControllerHand, float> PickedMap;

	void UpdateNodeDetailsWidget(class AIGVNodeActor* LeftPickedNode);

	void OnATouched();
	void OnATouchReleased();
	void OnBTouched();
	void OnBTouchReleased();
	void OnYTouched();
	void OnYTouchReleased();
	void OnYDoublePressed();

	bool bLeftGripHeld;
	bool bRightGripHeld;

	TMap<enum class EControllerHand, TPair<FVector, FRotator> > PickRayInformation;
};