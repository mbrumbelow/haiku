/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */

#include "SpecialNode.h"

#include "Volume.h"


SpecialNode::SpecialNode(Volume *volume)
	:
	Node(volume, NODE_TYPE_SPECIAL)
{
}


SpecialNode::~SpecialNode()
{
}


status_t
SpecialNode::PublishVNode(fs_vnode *subVnode)
{
	status_t status = publish_vnode(GetVolume()->FSVolume(), GetID(),
		subVnode->private_node, subVnode->ops, GetMode(), 0);
	if (status != B_OK)
		return status;

	fIsKnownToVFS = true;
	return B_OK;
}


status_t
SpecialNode::SetSize(off_t newSize)
{
	return B_UNSUPPORTED;
}


off_t
SpecialNode::GetSize() const
{
	return 0;
}
