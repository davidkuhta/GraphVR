// Copyright 2018 David Kuhta. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IGVGraphDetailsUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class IMSVGRAPHVIS_API UIGVGraphDetailsUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FString NumNodes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FString NumEdges;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FString NumPicked;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FString FOV;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FString AspectRatio;

	class AIGVNodeActor* DisplayedNode;
	
	
	
	
};
