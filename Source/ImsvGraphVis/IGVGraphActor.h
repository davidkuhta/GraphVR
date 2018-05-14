// Copyright 2017 Oh-Hyun Kwon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "IGVCluster.h"
#include "IGVEdge.h"
#include "IGVProjection.h"

#include "Runtime/Online/HTTP/Public/Http.h" //DPK Added
#include "louvain/graph.h"
#include "louvain/graph_binary.h"
#include "louvain/louvain.h"
#include "louvain/modularity.h"
#include <io.h>
#include <process.h>
#include <tuple>
#include "Json.h"
#include "JsonUtilities.h"

#include "IGVGraphActor.generated.h"

USTRUCT()
struct FPickRaySortedNodesArray {
	GENERATED_USTRUCT_BODY()

	TArray<class AIGVNodeActor*> Nodes;
};

enum class AREnum : uint8
{
	Square,
	Traditional,
	Standard,
	Computer,
	HD,
	Cinema,
	Univisium,
	Widescreen
};

UCLASS()
class IMSVGRAPHVIS_API AIGVGraphActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = ImmersiveGraphVisualization)
		FString Filename;

	UPROPERTY(EditDefaultsOnly, Category = ImmersiveGraphVisualization)
		TSubclassOf<class AIGVNodeActor> NodeActorClass;

	UPROPERTY(VisibleAnywhere, Category = ImmersiveGraphVisualization)
		class USphereComponent* SphereComponent;

	UPROPERTY(VisibleAnywhere, Category = ImmersiveGraphVisualization)
		class UPostProcessComponent* PostProcessComponent;

	UPROPERTY(VisibleAnywhere, Category = ImmersiveGraphVisualization)
		class USkyLightComponent* SkyLightComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = ImmersiveGraphVisualization)
		class UIGVEdgeMeshComponent* DefaultEdgeGroupMeshComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = ImmersiveGraphVisualization)
		class UIGVEdgeMeshComponent* HighlightedEdgeGroupMeshComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = ImmersiveGraphVisualization)
		class UIGVEdgeMeshComponent* RemainedEdgeGroupMeshComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = ImmersiveGraphVisualization)
		class UIGVEdgeMeshComponent* HiddenEdgeGroupMeshComponent;

	UPROPERTY()
		class UMaterialInstanceDynamic* OutlineMaterialInstance;

public:
	TArray<class AIGVNodeActor*> Nodes;
	TArray<FIGVEdge> Edges;
	TArray<FIGVCluster> Clusters;
	FIGVCluster* RootCluster;
	TMap<int32, int32> NeoMap;

	FVector2D PlanarExtent;

	Quality *q;

	AREnum AspectRatioEnum;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization,
		meta = (UIMin = "1.0", UIMax = "360", ClampMin = "1", ClampMax = "360.0"))
		float FieldOfView;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization,
		meta = (UIMin = "0.01", UIMax = "100", ClampMin = "0.01", ClampMax = "100.0"))
		float AspectRatio;  // for Treemap layout

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		EIGVProjection ProjectionMode;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		float ClusterLevelScale;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		float ClusterLevelExponent;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		float ClusterLevelOffset;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		float TreemapNesting;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		int32 EdgeSplineResolution;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		float EdgeWidth;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization,
		meta = (ClampMin = "2", ClampMax = "32", UIMin = "2", UIMax = "32"))
		int32 EdgeNumSides;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		float EdgeBundlingStrength;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		float ColorHueMin;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		float ColorHueMax;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		float ColorHueOffset;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		float ColorChroma;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		float ColorLuminance;

	TArray<class AIGVNodeActor*> PickRayDistSortedNodes;
	class AIGVNodeActor* LastNearestNode;
	class AIGVNodeActor* LastPickedNode;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		float PickDistanceThreshold;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame,
		Category = ImmersiveGraphVisualization)
		float SelectAllDistanceThreshold;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame, Category = ImmersiveGraph)
		float DefaultLevelScale;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame, Category = ImmersiveGraph)
		float HighlightedLevelScale;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, SaveGame, Category = ImmersiveGraph)
		float NeighborHighlightedLevelScale;

	FGraphEventArray EdgeUpdateTasks;
	bool bUpdateDefaultEdgeMeshRequired;
	FString AspectRatioToString();

public:
	AIGVGraphActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void EmptyGraph();

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void SetupGraph();

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		float GetSphereRadius() const;

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void SetSphereRadius(float Radius);

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		FVector Project(FVector2D const& P) const;

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void UpdatePlanarExtent();

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void NormalizeNodePosition();

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void UpdateTreemapLayout();

	/*UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void OnLeftMouseButtonReleased();*/

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void OnRightTriggerButtonReleased(); //DPK

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void OnLeftTriggerButtonReleased(); // DPK

	// Added for rotation of node sphere
	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void OnLeftGripPressed(FVector PickRayOrigin, FRotator PickRayRotation); //DPK
	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void OnRightGripPressed(FRotator PickRayRotation); //DPK

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void OnLeftXCapTouch(); //DPK

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void OnYReleased(); //DPK

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void OnLeftThumbstickPressed(); //DPK

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void OnLeftThumbstickDoublePressed(); //DPK

	UFUNCTION(BlueprintCallable, Category = ImmersiveGraphVisualization)
		void SetHalo(bool const bValue);

	/* DPK added */
	FHttpModule* Http;
	bool ShowNodeImages;

	/* The actual HTTP call */
	UFUNCTION()
	void HttpCallNode();
	void HttpCallEdge();

	/* Assign this function to call when the GET request processes succesfully */
	void OnResponseReceivedNode(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnResponseReceivedEdge(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	/*DPK Splitting up the Left and Right controller pick rays*/
	TMap<FString, class AIGVNodeActor*> LastPickedNodes;
	TMap<enum class EControllerHand, struct FPickRaySortedNodesArray> PickRayDistSortedNodesHandMap;
	TArray<class AIGVNodeActor*> LeftPickRayDistSortedNodes;
	TArray<class AIGVNodeActor*> RightPickRayDistSortedNodes;
	//TMap<enum class EControllerHand, TArray<class AIGVNodeActor*> > PickRayDistSortedNodesHandMap;
	TMap<enum class EControllerHand, class AIGVNodeActor*> LastNearestNodeMap;
	TMap<enum class EControllerHand, class AIGVNodeActor*> LastPickedNodeMap;
	//TCircularQueue<class AIGVNodeActor*>(3) NodeBridgeQueue;
	TCircularQueue<class AIGVNodeActor*> NodeBridgeQueue = TCircularQueue<class AIGVNodeActor*>(3);
	void ResetQueue();

	void ToggleFOV();
	void ToggleAspectRatio();

protected:
	void SetupNodes();
	void SetupEdges();
	void SetupClusters();
	void ConstructClusters();

	void SetupEdgeMeshes();
	void UpdateEdgeMeshes();

	void UpdateColors();

	void ResetAmbientOcclusion();

	void UpdateInteraction();
	void UpdateNodeDistanceToPickRay();

	void InitQuality(GraphB *gb, unsigned short nbc);

	void ResetGraph();
	void RedrawGraph();
};

USTRUCT()
struct FAMeta
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int32 id;

	UPROPERTY()
	FString type;

	UPROPERTY()
	bool deleted;
};


USTRUCT()
struct FARow
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int32 born;

	UPROPERTY()
	FString name;
};

USTRUCT()
struct FAData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FARow> row;

	UPROPERTY()
	TArray<FAMeta> meta;
};

USTRUCT()
struct FAResult
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FAData> data;

	UPROPERTY()
	TArray<FString> columns;
};


USTRUCT()
struct FAResponse
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FAResult> results;

	UPROPERTY()
	TArray<FString> error;
};
