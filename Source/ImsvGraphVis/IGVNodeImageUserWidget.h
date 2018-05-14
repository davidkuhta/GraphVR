// Copyright 2017 Oh-Hyun Kwon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "IGVNodeImageUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class IMSVGRAPHVIS_API UIGVNodeImageUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	UImage *NodeImage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ImmersiveGraphVisualization)
	FSlateBrush NodeBrush;
	
	
};
