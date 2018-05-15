// Copyright 2017 Oh-Hyun Kwon. All Rights Reserved.
// Copyright 2018 David Kuhta. All Rights Reserved for additions.

#include "IGVEdgeMeshData.h"

FIGVEdgeMeshData::FIGVEdgeMeshData()
{
	VertexBufferOffset[EIGVEdgeRenderGroup::Default] = 0;
	VertexBufferSize[EIGVEdgeRenderGroup::Default] = 0;
	VertexBufferOffset[EIGVEdgeRenderGroup::Highlighted] = 0;
	VertexBufferSize[EIGVEdgeRenderGroup::Highlighted] = 0;
	VertexBufferOffset[EIGVEdgeRenderGroup::Remained] = 0;
	VertexBufferSize[EIGVEdgeRenderGroup::Remained] = 0;
	VertexBufferOffset[EIGVEdgeRenderGroup::Hidden] = 0; //DPK
	VertexBufferSize[EIGVEdgeRenderGroup::Hidden] = 0; //DPK

	IndexBufferOffset[EIGVEdgeRenderGroup::Default] = 0;
	IndexBufferSize[EIGVEdgeRenderGroup::Default] = 0;
	IndexBufferOffset[EIGVEdgeRenderGroup::Highlighted] = 0;
	IndexBufferSize[EIGVEdgeRenderGroup::Highlighted] = 0;
	IndexBufferOffset[EIGVEdgeRenderGroup::Remained] = 0;
	IndexBufferSize[EIGVEdgeRenderGroup::Remained] = 0;
	IndexBufferOffset[EIGVEdgeRenderGroup::Hidden] = 0; //DPK
	IndexBufferSize[EIGVEdgeRenderGroup::Hidden] = 0; //DPK
}
