// Copyright 2017 Oh-Hyun Kwon. All Rights Reserved.
// Copyright 2018 David Kuhta. All Rights Reserved for additions.

#pragma once

#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"

#include "IGVEdgeMeshData.h"
#include "IGVEdgeSplineData.h"

#include "IGVEdge.generated.h"

USTRUCT()
struct IMSVGRAPHVIS_API FIGVEdge
{
	GENERATED_BODY()

public:
	class AIGVGraphActor* GraphActor;

	UPROPERTY(VisibleAnywhere, SaveGame, Category = ImmersiveGraphVisualization)
	int32 SourceIdx;

	UPROPERTY(VisibleAnywhere, SaveGame, Category = ImmersiveGraphVisualization)
	int32 TargetIdx;

	class AIGVNodeActor* SourceNode;
	class AIGVNodeActor* TargetNode;

	struct FIGVCluster* LowestCommonAncestor;
	TArray<struct FIGVCluster*> Clusters;
	int32 LowestCommonAncestorIdxInClusters;

	TArray<float> ClusterLevels;
	TArray<float> ClusterLevelsDefault;
	TArray<float> ClusterLevelsBeforeTransition;
	TArray<float> ClusterLevelsAfterTransition;
	TArray<FIGVEdgeSplineControlPointData> SplineControlPointData;
	FIGVEdgeMeshData MeshData;

	EIGVEdgeRenderGroup::Type RenderGroup;
	bool bInTransition;
	bool bUpdateMeshRequired;

public:
	FIGVEdge() = default;
	FIGVEdge(class AIGVGraphActor* const InGraphActor);

	FString ToString() const;

	void SetupClusters();

public:
	void UpdateDefaultClusterLevels();

	void UpdateSplineControlPoints();
	void UpdateDefaultSplineControlPoints();

protected:
	void UpdateSplineControlPointsImpl(TArray<float> const& InClusterLevels,
									   float const SourceLevelScale, float const TargetLevelScale);

public:
	float BundlingStrength() const;

	bool HasHighlightedNode() const;
	bool HasNeighborHighlightedNode() const;
	bool HasBothHighlightedNodes() const;

	bool IsDefaultRenderGroup() const;
	bool IsHighlightedRenderGroup() const;
	bool IsRemainedRenderGroup() const;
	bool IsHiddenRenderGroup() const;  //DPK
	void UpdateRenderGroup();

	void BeginTransition();
	void OnHighlightTransitionTimelineUpdate(ETimelineDirection::Type const Direction,
											 float const Alpha);
	void OnHighlightTransitionTimelineFinished(ETimelineDirection::Type const Direction);

	bool operator==(const FIGVEdge A) const;
	bool operator==(FIGVEdge A);
};
