// Copyright 2017 Oh-Hyun Kwon. All Rights Reserved.

#include "IGVGraphActor.h"

#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SphereComponent.h"

#include "KWColorSpace.h"
#include "KWTask.h"

#include "IGVData.h"
#include "IGVEdgeMeshComponent.h"
#include "IGVFunctionLibrary.h"
#include "IGVLog.h"
#include "IGVNodeActor.h"
#include "IGVPawn.h"
#include "IGVPlayerController.h"
#include "IGVTreemapLayout.h"


UMaterialInterface* GetOutlineMaterial()
{
	static ConstructorHelpers::FObjectFinder<UMaterial> MaterialAsset(
		TEXT("/Game/Materials/PP_Outline_EdgeDetect.PP_Outline_EdgeDetect"));
	return MaterialAsset.Succeeded() ? MaterialAsset.Object->GetMaterial() : nullptr;
}

AIGVGraphActor::AIGVGraphActor()
	: Filename("lesmis.igv.json"),
	  Nodes(),
	  Edges(),
	  Clusters(),
	  PlanarExtent(1.f, 1.f),
	  FieldOfView(90.f),
	  AspectRatio(16.f / 9.f),
	  AspectRatioEnum(AREnum::HD),
	  ProjectionMode(EIGVProjection::Sphere_Stereographic_RadialWarping),
	  ClusterLevelScale(1.f),
	  ClusterLevelExponent(2.f),
	  ClusterLevelOffset(.1f),
	  TreemapNesting(.1f),
	  EdgeSplineResolution(24),
	  EdgeWidth(8.f),
	  EdgeNumSides(4),
	  EdgeBundlingStrength(.9f),
	  ColorHueMin(0.f),
	  ColorHueMax(210.f),
	  ColorHueOffset(0.f),
	  ColorChroma(.5f),
	  ColorLuminance(.5f),
	  PickRayDistSortedNodes(),
	  LastNearestNode(nullptr),
	  LastPickedNode(nullptr),
	  PickDistanceThreshold(30),
	  SelectAllDistanceThreshold(100),
	  DefaultLevelScale(1.f),
	  HighlightedLevelScale(.5f),
	  NeighborHighlightedLevelScale(.75f),
	  bUpdateDefaultEdgeMeshRequired(true),
	  LeftPickRayDistSortedNodes(),
	  RightPickRayDistSortedNodes()
      //PickRayDistSortedNodesHandMap()
{
	PrimaryActorTick.bCanEverTick = true;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	RootComponent = SphereComponent;

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcessComponent->AttachToComponent(RootComponent,
											FAttachmentTransformRules::KeepRelativeTransform);

	SkyLightComponent = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLightComponent->AttachToComponent(RootComponent,
										 FAttachmentTransformRules::KeepRelativeTransform);
	SkyLightComponent->SourceType = ESkyLightSourceType::SLS_SpecifiedCubemap;
	SkyLightComponent->SetIntensity(3.75);
	SkyLightComponent->SetCastShadows(false);

	DefaultEdgeGroupMeshComponent =
		CreateDefaultSubobject<UIGVEdgeMeshComponent>(TEXT("DefaultEdgeGroupMeshComponent"));
	DefaultEdgeGroupMeshComponent->Init(this, EIGVEdgeRenderGroup::Default);
	DefaultEdgeGroupMeshComponent->AttachToComponent(
		RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	HighlightedEdgeGroupMeshComponent =
		CreateDefaultSubobject<UIGVEdgeMeshComponent>(TEXT("HighlightedEdgeGroupMeshComponent"));
	HighlightedEdgeGroupMeshComponent->Init(this, EIGVEdgeRenderGroup::Highlighted);
	HighlightedEdgeGroupMeshComponent->AttachToComponent(
		RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	RemainedEdgeGroupMeshComponent =
		CreateDefaultSubobject<UIGVEdgeMeshComponent>(TEXT("RemainedEdgeGroupMeshComponent"));
	RemainedEdgeGroupMeshComponent->Init(this, EIGVEdgeRenderGroup::Remained);
	RemainedEdgeGroupMeshComponent->AttachToComponent(
		RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	SetSphereRadius(1000.0f);
	ResetAmbientOcclusion();

	GetOutlineMaterial();
	
	Http = &FHttpModule::Get();

	LastPickedNodeMap.Add(EControllerHand::Left, nullptr);
	LastPickedNodeMap.Add(EControllerHand::Right, nullptr);

	LastNearestNodeMap.Add(EControllerHand::Left, nullptr);
	LastNearestNodeMap.Add(EControllerHand::Right, nullptr);

	PickRayDistSortedNodesHandMap.Add(EControllerHand::Left, FPickRaySortedNodesArray());
	PickRayDistSortedNodesHandMap.Add(EControllerHand::Right, FPickRaySortedNodesArray());

	//IGV_LOG(Log, TEXT("PickRay Sorted Key Count: %d"), PickRayDistSortedNodesHandMap.Num());
	//TCircularQueue<class AIGVNodeActor*> NodeBridgeQueue = TCircularQueue<class AIGVNodeActor*>(3);
}

void AIGVGraphActor::BeginPlay()
{
	Super::BeginPlay();

	AIGVPlayerController* const PlayerController = UIGVFunctionLibrary::GetPlayerController(this);
	if (PlayerController)
	{
		PlayerController->GraphActor = this;
	}
	else
	{
		IGV_LOG_S(Warning, TEXT("Unable to find PlayerController"));
	}

	AIGVPawn* const Pawn = UIGVFunctionLibrary::GetPawn(this);
	if (Pawn)
	{
		Pawn->GraphActor = this;
	}
	else
	{
		IGV_LOG_S(Warning, TEXT("Unable to find Pawn"));
	}
	
	OutlineMaterialInstance = UMaterialInstanceDynamic::Create(GetOutlineMaterial(), this);

	HttpCallNode();
	HttpCallEdge();

	if (!Filename.IsEmpty())
	{
		//This function in IGVData calls GraphActor->SetupGraph() continuing below
		UIGVData::LoadFile(FPaths::Combine(UIGVData::DefaultDataDirPath(), Filename), this);
	}
}

void AIGVGraphActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateInteraction();
	UpdateEdgeMeshes();
}

void AIGVGraphActor::EmptyGraph()
{
	for (AIGVNodeActor* Node : Nodes)
	{
		Node->Destroy();
	}
	Nodes.Empty();
	Edges.Empty();
	Clusters.Empty();

	PickRayDistSortedNodes.Empty();
	LeftPickRayDistSortedNodes.Empty();
	RightPickRayDistSortedNodes.Empty();

	PickRayDistSortedNodesHandMap.Empty(); //DPK Added
	
	LastNearestNode = nullptr;
	LastPickedNode = nullptr;

	//LeftPickRayDistSortedNodes.Empty();
	//RightPickRayDistSortedNodes.Empty();
	//LastNearestNodeMap.Empty();
	//LastPickedNodeMap.Empty();
}

void AIGVGraphActor::ResetGraph()
{


	EmptyGraph();
	HttpCallNode();
	HttpCallEdge();
}

void AIGVGraphActor::RedrawGraph()
{
	Clusters.Empty();

	LastNearestNode = nullptr;
	LastPickedNode = nullptr;
	//LastPickedNodeMap.Empty();
	//LastNearestNodeMap.Empty();

	for (FIGVEdge& Edge : Edges) {
		Edge.Clusters.Empty();
		Edge.ClusterLevels.Empty();
		Edge.ClusterLevelsDefault.Empty();
		Edge.ClusterLevelsBeforeTransition.Empty();
		Edge.ClusterLevelsAfterTransition.Empty();
	}

	//SetupEdges();
	ConstructClusters(); // DPK
	SetupClusters();
	UpdateColors();
	UpdateTreemapLayout();
	SetupEdgeMeshes();
}

void AIGVGraphActor::SetupGraph()
{
	SetupNodes();
	SetupEdges();
	ConstructClusters(); // DPK
	SetupClusters();
	UpdateColors();
	UpdateTreemapLayout();
	SetupEdgeMeshes();

	AIGVPawn* const Pawn = UIGVFunctionLibrary::GetPawn(this);
	Pawn->SetupGraphDetailsWidget();
}

void AIGVGraphActor::SetupNodes()
{
	IGV_LOG(Log, TEXT("Setting up nodes"));

	PickRayDistSortedNodesHandMap.Empty();

	PickRayDistSortedNodesHandMap.Add(EControllerHand::Left, FPickRaySortedNodesArray());
	PickRayDistSortedNodesHandMap.Add(EControllerHand::Right, FPickRaySortedNodesArray());

	for (AIGVNodeActor* const Node : Nodes)
	{
		Node->SetText(Node->Label);
		Node->SetImage(Node->Label); // Setting Images
		PickRayDistSortedNodes.Add(Node);
		LeftPickRayDistSortedNodes.Add(Node);
		RightPickRayDistSortedNodes.Add(Node);
		PickRayDistSortedNodesHandMap[EControllerHand::Left].Nodes.Add(Node);
		PickRayDistSortedNodesHandMap[EControllerHand::Right].Nodes.Add(Node);

		IGV_LOG(Log, TEXT("Node: %s"), *Node->ToString());
	}

	//auto leftNodes = PickRayDistSortedNodesHandMap[EControllerHand::Left].Nodes.Num();
	//auto rightNodes = PickRayDistSortedNodesHandMap[EControllerHand::Right].Nodes.Num();
	//IGV_LOG(Log, TEXT("PickRay Dist - Left: %d, Right: %d"), leftNodes, rightNodes);
}

void AIGVGraphActor::SetupEdges()
{
	for (FIGVEdge& Edge : Edges)
	{
		Edge.SourceNode = Nodes[Edge.SourceIdx];
		Edge.TargetNode = Nodes[Edge.TargetIdx];

		Edge.SourceNode->Edges.Add(&Edge);
		Edge.TargetNode->Edges.Add(&Edge);

		Edge.SourceNode->Neighbors.Add(Edge.TargetNode);
		Edge.TargetNode->Neighbors.Add(Edge.SourceNode);

		IGV_LOG(Log, TEXT("Edge: %s"), *Edge.ToString());
	}
}

void AIGVGraphActor::SetupClusters()
{
	RootCluster = &Clusters.Last();
	check(RootCluster->ParentIdx == -1);

	for (FIGVCluster& Cluster : Clusters)
	{
		if (!Cluster.IsRoot())
		{
			Cluster.Parent = &Clusters[Cluster.ParentIdx];
			Cluster.Parent->Children.Add(&Cluster);
		}

		if (Cluster.IsLeaf())
		{
			Cluster.Node = Nodes[Cluster.NodeIdx];
		}
	}

	RootCluster->SetNumDescendantNodes();

	for (FIGVCluster& Cluster : Clusters)
	{
		IGV_LOG(Log, TEXT("Cluster: %s"), *Cluster.ToString());
	}

	//DPK WHY is this called
	for (FIGVEdge& Edge : Edges)
	{
		IGV_LOG(Log, TEXT("Clusters for Edge: %s"), *Edge.ToString());
		Edge.SetupClusters();
	}
}

void AIGVGraphActor::ConstructClusters()
{
	int type = UNWEIGHTED;
	int nb_pass = 0;
	long double precision = 0.000001L;
	int display_level = -2;

	TArray<TTuple<int, int>> links;
	vector<pair<int, int> > flat_links;
	vector<pair<int, int> > processed_links;

	for (FIGVEdge& Edge : Edges) {
		//IGV_LOG(Log, TEXT("Edge Too: %s"), *Edge.ToString());
		IGV_LOG(Log, TEXT("SourceIdx: %d"), Edge.SourceIdx);
		IGV_LOG(Log, TEXT("TargetIdx: %d"), Edge.TargetIdx);
		links.Emplace(Edge.SourceIdx, Edge.TargetIdx);
		flat_links.push_back(make_pair(Edge.SourceIdx, Edge.TargetIdx));
	}

	Graph g(flat_links, type);

	g.clean(type);

	stringstream data_stream;

	g.stream_binary(data_stream, type);

	srand(time(NULL) + _getpid());

	unsigned short nb_calls = 0;
	
	GraphB gb(data_stream, type);
	InitQuality(&gb, nb_calls);
	nb_calls++;

	Louvain c(-1, precision, q);

	bool improvement = true;

	long double quality = (c.qual)->quality();
	long double new_qual;

	int level = 0;
	int cumulative = 0;

	do {
		/*if (verbose) {
			cerr << "level " << level << ":\n";
			display_time("  start computation");
			cerr << "  network size: "
				<< (c.qual)->gb.nb_nodes << " nodes, "
				<< (c.qual)->gb.nb_links << " links, "
				<< (c.qual)->gb.total_weight << " total_weight" << endl;
		}*/

		improvement = c.one_level();
		/*cerr << "Improvement: " << std::boolalpha << improvement << endl;*/

		new_qual = (c.qual)->quality();

		/*if (++level == display_level)
			(c.qual)->gb.display();
		if (display_level == -1)*/
		processed_links = c.display_partitionKR(cumulative, improvement);

		
		///Testing
		const vector<pair<int, int> >::const_iterator link_begin(processed_links.begin());
		const vector<pair<int, int> >::const_iterator link_end(processed_links.end());
		vector<pair<int, int> >::const_iterator link;

		for (link = link_begin; link != link_end; ++link) {
			FIGVCluster Cluster = FIGVCluster::FIGVCluster(this);
			Cluster.Height = level;
			Cluster.Idx = (*link).first;
			Cluster.ParentIdx = (*link).second;
			if (level == 0) {
				Cluster.NodeIdx = (*link).first;
			}
			this->Clusters.Emplace(Cluster);
		}
		//Testing
		++level;

		cumulative += gb.nb_nodes;
		gb = c.partition2graph_binary();
		InitQuality(&gb, nb_calls);
		nb_calls++;

		c = Louvain(-1, precision, q);

		/*if (verbose)
			cerr << "  quality increased from " << quality << " to " << new_qual << endl;*/

		quality = new_qual;

		/*if (verbose)
			display_time("  end computation");*/

		//if (filename_part!=NULL && level==1) // do at least one more computation if partition is provided
		//improvement=true;
	} while (improvement);

	FIGVCluster RootCluster = FIGVCluster::FIGVCluster(this);
	RootCluster.Height = level;
	RootCluster.Idx = cumulative;// +1;
	this->Clusters.Emplace(RootCluster);
	IGV_LOG_S(Log, TEXT("Root Cluster Idx: %d"), RootCluster.Idx);

	//for (int i = 0; i < links.Num(); i++) {
	//	//TTuple<int, int> link = links[i];
	//	
	//	IGV_LOG(Log, TEXT("Link: %d"), links[i].Get<0>());
	//}

	delete q;

}

float AIGVGraphActor::GetSphereRadius() const
{
	return SphereComponent->GetUnscaledSphereRadius();
}

void AIGVGraphActor::SetSphereRadius(float Radius)
{
	SphereComponent->InitSphereRadius(Radius);
}

FVector AIGVGraphActor::Project(FVector2D const& P) const
{
	switch (ProjectionMode)
	{
		case EIGVProjection::Sphere_SphericalCoordinates:
			return UIGVProjection::ToSphere_SphericalCoordinates(P);
		case EIGVProjection::Sphere_Gnomonic:  //
			return UIGVProjection::ToSphere_Gnomonic(P);
		case EIGVProjection::Sphere_Gnomonic_RadialWarping:
			return UIGVProjection::ToSphere_Gnomonic_RadialWarping(P);
		case EIGVProjection::Sphere_Gnomonic_IndependentWarping:
			return UIGVProjection::ToSphere_Gnomonic_IndependentWarping(P);
		case EIGVProjection::Sphere_Stereographic:  //
			return UIGVProjection::ToSphere_Stereographic(P);
		case EIGVProjection::Sphere_Stereographic_RadialWarping:
			return UIGVProjection::ToSphere_Stereographic_RadialWarping(P);
		case EIGVProjection::Sphere_Stereographic_IndependentWarping:
			return UIGVProjection::ToSphere_Stereographic_IndependentWarping(P);
		default: checkNoEntry(); break;
	}
	return FVector::ZeroVector;
}

void AIGVGraphActor::UpdatePlanarExtent()
{
	PlanarExtent.X = FMath::DegreesToRadians(FieldOfView * 0.5);
	PlanarExtent.Y = PlanarExtent.X / AspectRatio;
}

void AIGVGraphActor::NormalizeNodePosition()
{
	UpdatePlanarExtent();

	FBox2D Bounds(ForceInitToZero);
	for (AIGVNodeActor* const Node : Nodes)
	{
		Bounds += Node->Pos2D;
	}

	FVector2D const BoundCenter = Bounds.GetCenter();
	FVector2D const BoundExtent = Bounds.GetExtent();

	for (AIGVNodeActor* const Node : Nodes)
	{
		FVector2D& P = Node->Pos2D;
		P -= BoundCenter;
		P /= BoundExtent;
		P *= PlanarExtent;
		Node->SetPos3D();
	}

	RootCluster->SetPosNonLeaf();

	bUpdateDefaultEdgeMeshRequired = true;
}

void AIGVGraphActor::UpdateTreemapLayout()
{
	UpdatePlanarExtent();

	FIGVTreemapLayout Layout(this);
	Layout.Compute();

	NormalizeNodePosition();
}

void AIGVGraphActor::SetupEdgeMeshes()
{
	FGraphEventArray Tasks;
	for (FIGVEdge& Edge : Edges)
	{
		Tasks.Add(FKWTask<>::ConstructAndDispatchWhenReady([&]() {
			// Edge.UpdateRenderGroup();
			Edge.UpdateSplineControlPoints();
		}));
	}
	FTaskGraphInterface::Get().WaitUntilTasksComplete(Tasks);

	DefaultEdgeGroupMeshComponent->Setup();
	bUpdateDefaultEdgeMeshRequired = false;

	HighlightedEdgeGroupMeshComponent->Setup();
	RemainedEdgeGroupMeshComponent->Setup();
}

void AIGVGraphActor::UpdateEdgeMeshes()
{
	EdgeUpdateTasks.Reset();

	if (bUpdateDefaultEdgeMeshRequired)
	{
		for (FIGVEdge& Edge : Edges)
		{
			EdgeUpdateTasks.Add(FKWTask<>::ConstructAndDispatchWhenReady(
				[&]() { Edge.UpdateDefaultSplineControlPoints(); }));
		}
		FTaskGraphInterface::Get().WaitUntilTasksComplete(EdgeUpdateTasks);
		DefaultEdgeGroupMeshComponent->Update();
		bUpdateDefaultEdgeMeshRequired = false;

		EdgeUpdateTasks.Reset();

		for (FIGVEdge& Edge : Edges)
		{
			EdgeUpdateTasks.Add(FKWTask<>::ConstructAndDispatchWhenReady([&]() {
				Edge.UpdateRenderGroup();
				Edge.UpdateSplineControlPoints();
				Edge.bUpdateMeshRequired = false;
			}));
		}
		FTaskGraphInterface::Get().WaitUntilTasksComplete(EdgeUpdateTasks);
		HighlightedEdgeGroupMeshComponent->Update();
		RemainedEdgeGroupMeshComponent->Update();
	}
	else
	{
		for (FIGVEdge& Edge : Edges)
		{
			if (Edge.bUpdateMeshRequired)
			{
				EdgeUpdateTasks.Add(FKWTask<>::ConstructAndDispatchWhenReady([&]() {
					Edge.UpdateRenderGroup();
					Edge.UpdateSplineControlPoints();
					Edge.bUpdateMeshRequired = false;
				}));
			}
		}
		FTaskGraphInterface::Get().WaitUntilTasksComplete(EdgeUpdateTasks);

		if (EdgeUpdateTasks.Num() > 0)
		{
			HighlightedEdgeGroupMeshComponent->Update();
			RemainedEdgeGroupMeshComponent->Update();
		}
	}
}

void AIGVGraphActor::UpdateColors()
{
	int32 const NumNodes = Nodes.Num();
	int32 Idx = 0;

	RootCluster->ForEachDescendantFirst([&](FIGVCluster& Cluster) {
		if (Cluster.IsLeaf())
		{
			float const Alpha = float(Idx) / NumNodes;
			float const Hue = FMath::Lerp(ColorHueMin, ColorHueMax, Alpha);
			Cluster.Node->UpdateColor(
				FLinearColor(UKWColorSpace::HCLtoRGB(Hue / 360.0, ColorChroma, ColorLuminance)));
			Idx++;
		}
	});

	bUpdateDefaultEdgeMeshRequired = true;
}

void AIGVGraphActor::ResetAmbientOcclusion()
{
	FPostProcessSettings& PostProcessSettings = PostProcessComponent->Settings;

	PostProcessSettings.bOverride_AmbientOcclusionIntensity = true;
	PostProcessSettings.AmbientOcclusionIntensity = 1.0;  // 0.5

	PostProcessSettings.bOverride_AmbientOcclusionRadius = true;
	PostProcessSettings.AmbientOcclusionRadius = 500.0;  // 40.0

	PostProcessSettings.bOverride_AmbientOcclusionRadiusInWS = true;
	PostProcessSettings.AmbientOcclusionRadiusInWS = true;  // false

	PostProcessSettings.bOverride_AmbientOcclusionDistance_DEPRECATED = true;
	PostProcessSettings.AmbientOcclusionDistance_DEPRECATED = 10000.0;  // 80.0

	PostProcessSettings.bOverride_AmbientOcclusionPower = true;
	PostProcessSettings.AmbientOcclusionPower = 2.5;  // 2.0

	PostProcessSettings.bOverride_AmbientOcclusionBias = true;
	PostProcessSettings.AmbientOcclusionBias = 0.5;  // 3.0

	PostProcessSettings.bOverride_AmbientOcclusionQuality = true;
	PostProcessSettings.AmbientOcclusionQuality = 100;  // 50.0

	PostProcessSettings.bOverride_AmbientOcclusionMipBlend = false;
	PostProcessSettings.AmbientOcclusionMipBlend = 0.6;  // 0.6

	PostProcessSettings.bOverride_AmbientOcclusionMipScale = false;
	PostProcessSettings.AmbientOcclusionMipScale = 1.0;  // 1.7
}

void AIGVGraphActor::UpdateInteraction()
{
	bool ShowImages;
	TArray<enum class EControllerHand> PickRayInformationKeys;
	PickRayDistSortedNodesHandMap.GenerateKeyArray(PickRayInformationKeys);
	
	IGV_LOG(Log, TEXT("PickRayInformationKeys: %d"), PickRayInformationKeys.Num());

	UpdateNodeDistanceToPickRay(); // Can pull this into for loop?

	for (auto HandKey : PickRayInformationKeys)
	{
		TArray<AIGVNodeActor*> PickRayDistSortedNodesHand;
		if (HandKey == EControllerHand::Left)
		{
			PickRayDistSortedNodesHand = LeftPickRayDistSortedNodes;
			ShowImages = ShowNodeImages;
		}
		else
		{
			 PickRayDistSortedNodesHand = RightPickRayDistSortedNodes;
			 ShowImages = true;
		}
		//TArray<AIGVNodeActor*> PickRayDistSortedNodesHand = PickRayDistSortedNodesHandMap[HandKey].Nodes;

		if (PickRayDistSortedNodesHand.Num() == 0) continue;

		IGV_LOG(Log, TEXT("PickRayDistSortedNodesHand: %d"), PickRayDistSortedNodesHand.Num());

		AIGVNodeActor* const NearestNode = PickRayDistSortedNodesHand[0];
		AIGVNodeActor* LastNearestNode = LastNearestNodeMap[HandKey];


		// update nearest node
		if (LastNearestNode != nullptr)
		{
			if (LastNearestNode != NearestNode)
			{
				LastNearestNodeMap[HandKey]->EndNearest(); // Does Nothing
				LastNearestNodeMap[HandKey] = NearestNode;
				LastNearestNodeMap[HandKey]->BeginNearest(); //Does Nothing
			}
		}
		else
		{
			LastNearestNodeMap[HandKey] = NearestNode;
			LastNearestNodeMap[HandKey]->BeginNearest(); // Does Nothing
		}

		AIGVNodeActor* LastPickedNode = LastPickedNodeMap[HandKey];
		
		// update picked node
		if (NearestNode->IsPicked(HandKey))  // nearest node actor is picked node actor
		{
			if (LastPickedNode != nullptr)  // has previous picked node actor
			{
				IGV_LOG(Log, TEXT("LastPickedNode: %s"), *LastPickedNode->ToString());
				if (LastPickedNode != NearestNode)  // different node picked
				{
					LastPickedNodeMap[HandKey]->EndPicked(HandKey);
					LastPickedNodeMap[HandKey] = NearestNode;
					LastPickedNodeMap[HandKey]->BeginPicked(HandKey, ShowImages);
				}
			}
			else  // new node actor picked
			{
				LastPickedNodeMap[HandKey] = NearestNode;
				LastPickedNodeMap[HandKey]->BeginPicked(HandKey, ShowImages);
			}
		}
		else
		{
			if (LastPickedNode != nullptr)
			{
				LastPickedNode->EndPicked(HandKey);
			}
			LastPickedNodeMap[HandKey] = nullptr;
		}
	}

	//if (PickRayDistSortedNodes.Num() == 0) return;

	//UpdateNodeDistanceToPickRay();
	//AIGVNodeActor* const NearestNode = PickRayDistSortedNodes[0];

	// update nearest node
	//if (LastNearestNode != nullptr)  // has previous nearest node actor
	//{
	//	if (LastNearestNode != NearestNode)  // different node actor is nearest
	//	{
	//		LastNearestNode->EndNearest(); // Does Nothing
	//		LastNearestNode = NearestNode;
	//		LastNearestNode->BeginNearest(); //Does Nothing
	//	}
	//}
	//else
	//{
	//	LastNearestNode = NearestNode;
	//	LastNearestNode->BeginNearest(); // Does Nothing
	//}

	// update picked node
	//if (NearestNode->IsPicked())  // nearest node actor is picked node actor
	//{
	//	if (LastPickedNode != nullptr)  // has previous picked node actor
	//	{
	//		if (LastPickedNode != NearestNode)  // different node picked
	//		{
	//			LastPickedNode->EndPicked();
	//			LastPickedNode = NearestNode;
	//			LastPickedNode->BeginPicked();
	//		}
	//	}
	//	else  // new node actor picked
	//	{
	//		LastPickedNode = NearestNode;
	//		LastPickedNode->BeginPicked();
	//	}
	//}
	//else
	//{
	//	if (LastPickedNode != nullptr)
	//	{
	//		LastPickedNode->EndPicked();
	//	}
	//	LastPickedNode = nullptr;
	//}
}

void AIGVGraphActor::UpdateNodeDistanceToPickRay()
{
	AIGVPawn* const Pawn = UIGVFunctionLibrary::GetPawn(this);
	if (Pawn == nullptr) return;
	TArray<enum class EControllerHand> PickRayInformationKeys;
	Pawn->PickRayInformation.GenerateKeyArray(PickRayInformationKeys);

	ShowNodeImages = !(Pawn->ShowNodeDetails);

	for (auto HandKey : PickRayInformationKeys)
	{
		auto PickRayInformation = Pawn->PickRayInformation[HandKey];
		FVector Origin = PickRayInformation.Key;
		FVector Direction = PickRayInformation.Value.Vector().GetSafeNormal();

		for (AIGVNodeActor* const Node : Nodes)
		{
			Node->DistanceToPickRays[HandKey] = FMath::PointDistToLine(Node->GetActorLocation(), Direction, Origin);
			Node->DistanceToPickRay = FMath::PointDistToLine(Node->GetActorLocation(), Direction, Origin);
		}

		if (HandKey == EControllerHand::Left)
		{
			LeftPickRayDistSortedNodes.Sort([&](AIGVNodeActor const& A, AIGVNodeActor const& B) {
				return A.DistanceToPickRays[HandKey] < B.DistanceToPickRays[HandKey];
			});
		}

		if (HandKey == EControllerHand::Right)
		{
			RightPickRayDistSortedNodes.Sort([&](AIGVNodeActor const& A, AIGVNodeActor const& B) {
				return A.DistanceToPickRays[HandKey] < B.DistanceToPickRays[HandKey];
			});
		}
		//PickRayDistSortedNodesHandMap[HandKey].Nodes.Sort([&](AIGVNodeActor const& A, AIGVNodeActor const& B) {
		//	return A.DistanceToPickRays[HandKey] < B.DistanceToPickRays[HandKey];
		//});

		//PickRayDistSortedNodesHandMap[HandKey].Nodes.Sort([&](AIGVNodeActor const& A, AIGVNodeActor const& B) {
		//	return A.DistanceToPickRays[HandKey] < B.DistanceToPickRays[HandKey];
		//});

		/*Node->DistanceToPickRay = FMath::PointDistToLine(
			Node->GetActorLocation(), Pawn->PickRayDirection, Pawn->PickRayOrigin);*/
	}
	
	/*LeftPickRayDistSortedNodes.Sort([](AIGVNodeActor const& A, AIGVNodeActor const& B) {
		return A.DistanceToPickRays[EControllerHand::Left] < B.DistanceToPickRays[EControllerHand::Left];
	});

	RightPickRayDistSortedNodes.Sort([](AIGVNodeActor const& A, AIGVNodeActor const& B) {
		return A.DistanceToPickRays[EControllerHand::Right] < B.DistanceToPickRays[EControllerHand::Right];
	});*/

	/*PickRayDistSortedNodes.Sort([](AIGVNodeActor const& A, AIGVNodeActor const& B) {
		return A.DistanceToPickRay < B.DistanceToPickRay;
	});*/
}

/*void AIGVGraphActor::OnLeftMouseButtonReleased()
{
	if (LastPickedNodeMap[EControllerHand::Right] != nullptr)
	{
		LastPickedNodeMap[EControllerHand::Right]->OnLeftMouseButtonReleased();
	}
}*/

void AIGVGraphActor::OnYReleased()
{
	auto Hand = EControllerHand::Left;
	AIGVNodeActor* LastLeftPickedNode = LastPickedNodeMap[Hand];
	if (LastLeftPickedNode != nullptr)
	{
		if (NodeBridgeQueue.IsEmpty())
		{
			NodeBridgeQueue.Enqueue(LastLeftPickedNode);
			LastLeftPickedNode->SetColor(FLinearColor::Red);
		}
		else if (NodeBridgeQueue.Count() == 1)
		{
			AIGVNodeActor* FirstNode;
			NodeBridgeQueue.Peek(FirstNode);
			if (LastLeftPickedNode != FirstNode)
			{
				NodeBridgeQueue.Enqueue(LastLeftPickedNode);
				LastLeftPickedNode->SetColor(FLinearColor::Blue);
			}
		}
		else if (NodeBridgeQueue.Count() == 2)
		{
			AIGVNodeActor* FirstNode;
			AIGVNodeActor* SecondNode;
			NodeBridgeQueue.Dequeue(FirstNode);
			NodeBridgeQueue.Dequeue(SecondNode);
			if (LastLeftPickedNode == SecondNode)
			{
				NodeBridgeQueue.Enqueue(FirstNode);
				FirstNode->SetColor(FLinearColor::Red);
				NodeBridgeQueue.Enqueue(SecondNode);
				SecondNode->SetColor(FLinearColor::Blue);
			}
			else if (LastLeftPickedNode == FirstNode)
			{
				NodeBridgeQueue.Enqueue(SecondNode);
				SecondNode->SetColor(FLinearColor::Red);
				NodeBridgeQueue.Enqueue(FirstNode);
				FirstNode->SetColor(FLinearColor::Blue);
			}
			else
			{
				FirstNode->ResetColor();
				NodeBridgeQueue.Enqueue(SecondNode);
				SecondNode->SetColor(FLinearColor::Red);
				NodeBridgeQueue.Enqueue(LastLeftPickedNode);
				LastLeftPickedNode->SetColor(FLinearColor::Blue);
			}
		}
	}
	else
	{
		ResetQueue();
	}
}

void AIGVGraphActor::ResetQueue()
{
	while (!NodeBridgeQueue.IsEmpty())
	{
		AIGVNodeActor* FirstNode;
		NodeBridgeQueue.Dequeue(FirstNode);
		FirstNode->ResetColor();
	}
}

void AIGVGraphActor::OnLeftThumbstickPressed()
{
	IGV_LOG(Log, TEXT("Left Thumbstick Pressed"));
	/*if (NodeBridgeQueue.Count() == 1)
	{
		AIGVNodeActor* FirstNode;
		NodeBridgeQueue.Dequeue(FirstNode);

		int32 FirstNodeIndex = Nodes.Find(FirstNode);
		IGV_LOG(Log, TEXT("First Node Index: %d"), FirstNodeIndex);

		for (const FIGVEdge &Edge : Edges) // originally for (auto const& Edge : Edges)
		{
			if ((Edge.SourceNode == FirstNode) || (Edge.TargetNode == FirstNode))
			{
				IGV_LOG(Log, TEXT("Removing Edge: %s"), *Edge.ToString());

				int32 EdgeIndex = Edges.Find(Edge);

				IGV_LOG(Log, TEXT("Removing Edges Index: %d"), EdgeIndex);

				Edge.SourceNode->Neighbors.Remove(Edge.TargetNode);
				Edge.TargetNode->Neighbors.Remove(Edge.SourceNode);

				IGV_LOG(Log, TEXT("Node Neighbors Removed"));

				Edge.SourceNode->Edges.Remove(&Edges[EdgeIndex]);
				Edge.TargetNode->Edges.Remove(&Edges[EdgeIndex]);

				IGV_LOG(Log, TEXT("Node Edges Removed"));

				Edges.RemoveAt(EdgeIndex);
			}
		}

		IGV_LOG(Log, TEXT("Before Nodes Count: %d"), Nodes.Num());
		Nodes.RemoveAt(FirstNodeIndex);
		IGV_LOG(Log, TEXT("First Node Removed"));
		IGV_LOG(Log, TEXT("After Nodes Count: %d"), Nodes.Num());

		FirstNode->Destroy();

		NodeBridgeQueue.Empty();

		SetupNodes();
		RedrawGraph();

	}*/
	if (NodeBridgeQueue.Count() == 2)
	{
		AIGVNodeActor* FirstNode;
		AIGVNodeActor* SecondNode;
		NodeBridgeQueue.Dequeue(FirstNode);
		NodeBridgeQueue.Dequeue(SecondNode);
		if (FirstNode != SecondNode)
		{
			for (const FIGVEdge &Edge : Edges) // originally for (auto const& Edge : Edges)
			{
				if ((Edge.SourceNode == FirstNode && Edge.TargetNode == SecondNode) ||
					(Edge.SourceNode == SecondNode && Edge.TargetNode == FirstNode))
				{
					IGV_LOG(Log, TEXT("Removing Edge: %s"), *Edge.ToString());

					int32 EdgeIndex = Edges.Find(Edge);

					IGV_LOG(Log, TEXT("Removing Edges Index: %d"), EdgeIndex);

					Edge.SourceNode->Neighbors.Remove(Edge.TargetNode);
					Edge.TargetNode->Neighbors.Remove(Edge.SourceNode);

					IGV_LOG(Log, TEXT("Node Neighbors Removed"));

					Edge.SourceNode->Edges.Remove(&Edges[EdgeIndex]);
					Edge.TargetNode->Edges.Remove(&Edges[EdgeIndex]);

					IGV_LOG(Log, TEXT("Node Edges Removed"));

					Edges.RemoveAt(EdgeIndex);

					NodeBridgeQueue.Empty();

					RedrawGraph();

					return;
				}
			}
			FIGVEdge Edge = FIGVEdge::FIGVEdge(this);
			const int32 SourceIndex = FirstNode->Idx;
			const int32 TargetIndex = SecondNode->Idx;

			Edge.SourceIdx = SourceIndex;
			Edge.TargetIdx = TargetIndex;

			Edges.Emplace(Edge);

			//FIGVEdge& Edgey = Edges[Edges.Num() - 1];
			FIGVEdge& Edgey = Edges.Last();

			Edgey.SourceNode = Nodes[Edgey.SourceIdx];
			Edgey.TargetNode = Nodes[Edgey.TargetIdx];

			Edgey.SourceNode->Edges.Add(&Edgey);
			Edgey.TargetNode->Edges.Add(&Edgey);

			Edgey.SourceNode->Neighbors.Add(Edgey.TargetNode);
			Edgey.TargetNode->Neighbors.Add(Edgey.SourceNode);

			IGV_LOG(Log, TEXT("Created Edge: %s"), *Edge.ToString());

			NodeBridgeQueue.Empty();

			RedrawGraph();
		}
	}
}

void AIGVGraphActor::OnLeftThumbstickDoublePressed()
{
	IGV_LOG(Log, TEXT("Left Thumbstick Double Pressed"));
	if (NodeBridgeQueue.Count() == 2)
	{
		AIGVNodeActor* FirstNode;
		AIGVNodeActor* SecondNode;
		NodeBridgeQueue.Dequeue(FirstNode);
		NodeBridgeQueue.Dequeue(SecondNode);
		if (FirstNode != SecondNode)
		{
			for (FIGVEdge const& Edge : Edges) //const FIGVEdge&
			{
				if ((Edge.SourceNode == FirstNode && Edge.TargetNode == SecondNode) ||
					(Edge.SourceNode == SecondNode && Edge.TargetNode == FirstNode))
				{
					IGV_LOG(Log, TEXT("Removing Edge: %s"), *Edge.ToString());

					FIGVEdge* Edgey = (FIGVEdge*) &Edge;

					Edge.SourceNode->Edges.Remove(Edgey);
					Edge.TargetNode->Edges.Remove(Edgey);

					Edge.SourceNode->Neighbors.Remove(Edge.TargetNode);
					Edge.TargetNode->Neighbors.Remove(Edge.SourceNode);

					Edges.Remove(Edge);

					NodeBridgeQueue.Empty();

					RedrawGraph();

					break;
				}
			}
		}
	}

}

void AIGVGraphActor::OnLeftXCapTouch()
{
	auto Hand = EControllerHand::Left;
	if (LastPickedNodeMap[Hand] != nullptr)
	{
		AIGVPawn* const Pawn = UIGVFunctionLibrary::GetPawn(this);
		Pawn->UpdateNodeDetailsWidget(LastPickedNodeMap[Hand]);
	}
}

void AIGVGraphActor::OnLeftTriggerButtonReleased()
{
	ResetQueue();
	auto Hand = EControllerHand::Left;
	if (LastPickedNodeMap[Hand] != nullptr)
	{
		LastPickedNodeMap[Hand]->OnLeftTriggerButtonReleased();
	}
	else if (!LastNearestNodeMap[Hand]->IsNearPicked())
	{
		for (AIGVNodeActor* Node : Nodes)
		{
			Node->EndPicked(EControllerHand::Right);
		}
		bUpdateDefaultEdgeMeshRequired = true;
	}
}

void AIGVGraphActor::OnRightTriggerButtonReleased()
{
	auto Hand = EControllerHand::Right;
	if (LastPickedNodeMap[Hand] != nullptr)
	{
		LastPickedNodeMap[Hand]->OnRightTriggerButtonReleased();
	}
	else if(!LastNearestNodeMap[Hand]->IsNearPicked())
	{
		for (AIGVNodeActor* Node : Nodes)
		{
			Node->BeginPicked(Hand);
		}
		bUpdateDefaultEdgeMeshRequired = true;
	}
}

void AIGVGraphActor::SetHalo(bool const bValue)
{
	PostProcessComponent->Settings.RemoveBlendable(OutlineMaterialInstance);

	if (bValue)
	{
		PostProcessComponent->Settings.AddBlendable(OutlineMaterialInstance, 1.0);
	}
}

void AIGVGraphActor::OnLeftGripPressed(FVector PickRayOrigin, FRotator PickRayRotation)
{
	//FRotator Test = FRotator(0.0, 45.0, 0.0);
	//SetActorRotation(Test);
	//SetActorRotation(FMath::Lerp(GetActorRotation(), Test, 0.05f));
	FRotator RootRotation = RootComponent->GetComponentRotation();
	FRotator NewRotation = RootRotation + PickRayRotation;
	//RootComponent->SetWorldRotation(NewRotation);
	//RootComponent->SetRelativeRotation(NewRotation);
	//RootComponent->SetComponentRotation(NewRotation);
	//SetActorRelativeRotation(NewRotation);
	//RootComponent->AddLocalRotation(PickRayRotation, false);
	auto Hand = EControllerHand::Left;
	if (LastPickedNodeMap[Hand] != nullptr)
	{
		LastPickedNodeMap[Hand]->OnLeftGripPressed(PickRayOrigin, PickRayRotation);
	}
}

void AIGVGraphActor::OnRightGripPressed(FRotator PickRayRotation)
{
	//FRotator Test = FRotator(0.0, 45.0, 0.0);
	//SetActorRotation(Test);
	//SetActorRotation(FMath::Lerp(GetActorRotation(), Test, 0.05f));
	FRotator RootRotation = RootComponent->GetComponentRotation();
	FRotator NewRotation = RootRotation + PickRayRotation;
	//RootComponent->SetWorldRotation(NewRotation);
	//RootComponent->SetRelativeRotation(NewRotation);
	//RootComponent->SetComponentRotation(NewRotation);
	SetActorRelativeRotation(NewRotation);
	//RootComponent->AddLocalRotation(PickRayRotation, false);
}

/* HTTP Call DPK */
void AIGVGraphActor::HttpCallNode()
{
	IGV_LOG(Log, TEXT("Calling Node"));
	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &AIGVGraphActor::OnResponseReceivedNode);
	//This is the url on which to process the request
	Request->SetURL("http://localhost:7474/db/data/transaction/commit");
	Request->SetVerb("POST");
	Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", TEXT("application/json"));
	FString Content = "{\"statements\":[{\"statement\":\"MATCH(n:Person) RETURN n\"}]})";
	FString SafeContent = *Content.ReplaceEscapedCharWithChar();
	Request->SetContentAsString(SafeContent);
	//Request->SetContentAsString("{\"statements\":[{\"statement\":\"CALL algo.louvain.stream('MATCH (m:Movie) RETURN id(m) as id', 'MATCH (m1:Movie)<--(:Person)-->(m2:Movie) WITH m1,m2,count(*) as common_actors WHERE common_actors > 2 RETURN id(m1) as source,id(m2) as target, common_actors as weight', { graph:'cypher' }) yield nodeId, community MATCH(movie:Movie) where id(movie) = nodeId RETURN  community, count(*) as communitySize, collect(nodeId) as movieIds, collect(movie.title) as movieTitles ORDER BY community desc\"}]}");
	Request->ProcessRequest();
}

/* Assigned function on successful http call */
void AIGVGraphActor::OnResponseReceivedNode(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	IGV_LOG(Log, TEXT("Node Response Received"));
	//Create a pointer to hold the json serialized data
	TSharedPtr<FJsonObject> JsonObject;

	//Create a reader pointer to read the json data
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	FString JsonString = Response->GetContentAsString();
	//IGV_LOG(Log, TEXT("JsonString: %s"), *JsonString);

	UWorld* const World = GetWorld();
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this->Instigator;

	//Deserialize the json data given Reader and the actual object to deserialize
	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		FAResponse NeoResponse;
		FJsonObjectConverter::JsonObjectStringToUStruct<FAResponse>(JsonString, &NeoResponse, 0, 0);
		IGV_LOG(Log, TEXT("Number in node array: %d"), NeoResponse.results.Num());
		for (int32 i = 0, nodes = NeoResponse.results[0].data.Num(); i < nodes; i++) {
			int32 Id = NeoResponse.results[0].data[i].meta[0].id;
			FString Name = NeoResponse.results[0].data[i].row[0].name;
			int32 Year = NeoResponse.results[0].data[i].row[0].born;
			IGV_LOG(Log, TEXT("Name: %s, NeoMap<%d, %d>"), *Name, Id, i);

			AIGVNodeActor* const NodeActor = World->SpawnActor<AIGVNodeActor>(
				this->NodeActorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			NodeActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
			NodeActor->Init(this);
			NodeActor->Idx = i;
			NeoMap.Add(Id, i);
			NodeActor->Label = Name;
			NodeActor->Year = Year;
			Nodes.Add(NodeActor);
		}
	}
}

void AIGVGraphActor::HttpCallEdge()
{
	IGV_LOG(Log, TEXT("Calling Edge"));
	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &AIGVGraphActor::OnResponseReceivedEdge);
	//This is the url on which to process the request
	Request->SetURL("http://localhost:7474/db/data/transaction/commit");
	Request->SetVerb("POST");
	Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", TEXT("application/json"));
	FString Content = "{\"statements\":[{\"statement\":\"MATCH (p1:Person)-[a1:ACTED_IN]->(m1:Movie)<-[a2:ACTED_IN]-(p2:Person) WHERE ID(p1) < ID(p2) RETURN p1, p2, m1\"}]})";
	FString SafeContent = *Content.ReplaceEscapedCharWithChar();
	Request->SetContentAsString(SafeContent);
	//Request->SetContentAsString("{\"statements\":[{\"statement\":\"CALL algo.louvain.stream('MATCH (m:Movie) RETURN id(m) as id', 'MATCH (m1:Movie)<--(:Person)-->(m2:Movie) WITH m1,m2,count(*) as common_actors WHERE common_actors > 2 RETURN id(m1) as source,id(m2) as target, common_actors as weight', { graph:'cypher' }) yield nodeId, community MATCH(movie:Movie) where id(movie) = nodeId RETURN  community, count(*) as communitySize, collect(nodeId) as movieIds, collect(movie.title) as movieTitles ORDER BY community desc\"}]}");
	Request->ProcessRequest();
}

/* Assigned function on successful http call */
void AIGVGraphActor::OnResponseReceivedEdge(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	IGV_LOG(Log, TEXT("Edge Response Received"));
	//Create a pointer to hold the json serialized data
	TSharedPtr<FJsonObject> JsonObject;

	//Create a reader pointer to read the json data
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	FString JsonString = Response->GetContentAsString();
	//IGV_LOG(Log, TEXT("JsonString: %s"), *JsonString);

	UWorld* const World = GetWorld();
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this->Instigator;

	//Deserialize the json data given Reader and the actual object to deserialize
	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		FAResponse NeoResponse;
		FJsonObjectConverter::JsonObjectStringToUStruct<FAResponse>(JsonString, &NeoResponse, 0, 0);
		
		for (int i = 0, nodes = NeoResponse.results[0].data.Num(); i < nodes; i++) {
			int32 sourceIdx = NeoMap[NeoResponse.results[0].data[i].meta[0].id];
			int32 targetIdx = NeoMap[NeoResponse.results[0].data[i].meta[1].id];
			IGV_LOG(Log, TEXT("Source: %d, Target: %d"), sourceIdx, targetIdx);

			FIGVEdge Edge = FIGVEdge::FIGVEdge(this);
			Edge.SourceIdx = sourceIdx;
			Edge.TargetIdx = targetIdx;
			Edges.Emplace(Edge);
		}
	}
	SetupGraph();
}

void AIGVGraphActor::InitQuality(GraphB *gb, unsigned short nbc) {

	if (nbc > 0)
		delete q;

	q = new Modularity(*gb);
}

void AIGVGraphActor::ToggleFOV()
{
	if (FieldOfView >= 360)
	{
		FieldOfView = 60;
	}
	else if (FieldOfView >= 180)
	{
		FieldOfView += 45;
	}
	else
	{
		FieldOfView += 10;
	}
	UpdateTreemapLayout();
}

void AIGVGraphActor::ToggleAspectRatio()
{
	switch (AspectRatioEnum)
	{
		case AREnum::Square :
			AspectRatioEnum = AREnum::Traditional;
			AspectRatio = 4.f / 3.f;
			break;
		case AREnum::Traditional :
			AspectRatioEnum = AREnum::Standard;
			AspectRatio = 11.f / 8.f;
			break;
		case AREnum::Standard :
			AspectRatioEnum = AREnum::Computer;
			AspectRatio = 16.f / 10.f;
			break;
		case AREnum::Computer :
			AspectRatioEnum = AREnum::HD;
			AspectRatio = 16.f / 9.f;
			break;
		case AREnum::HD :
			AspectRatioEnum = AREnum::Cinema;
			AspectRatio = 1.85f / 1.f;
			break;
		case AREnum::Cinema :
			AspectRatioEnum = AREnum::Univisium;
			AspectRatio = 18.f / 9.f;
			break;
		case AREnum::Univisium :
			AspectRatioEnum = AREnum::Widescreen;
			AspectRatio = 21.f / 9.f;
			break;
		case AREnum::Widescreen :
			AspectRatioEnum = AREnum::Square;
			AspectRatio = 1.f;
			break;
	}

	UpdateTreemapLayout();
}

FString AIGVGraphActor::AspectRatioToString()
{
	switch (AspectRatioEnum)
	{
	case AREnum::Square:
		return FString("1:1");
	case AREnum::Traditional:
		return FString("4:3");
	case AREnum::Standard:
		return FString("11:8");
	case AREnum::Computer:
		return FString("16:10");
	case AREnum::HD:
		return FString("16:9");
	case AREnum::Cinema:
		return FString("1.85:1");
	case AREnum::Univisium:
		return FString("18:9");
	case AREnum::Widescreen:
		return FString("21:9");
	default:
		return FString("Default");
	}
}