// Copyright 2018 David Kuhta. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IGVNodeDetailsUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class IMSVGRAPHVIS_API UIGVNodeDetailsUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FString NodeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FString PersonName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FString PersonBorn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FString NodeEdges;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FSlateBrush NodeBrush;

	class AIGVNodeActor* DisplayedNode;
};
