#ifndef IM_NODE_FLOW_FWD
#define IM_NODE_FLOW_FWD
#pragma once

namespace ImFlow {

template <typename T, bool TIsConst = false>
class InPinRef;
template <typename T, bool TIsConst = false>
class OutPinRef;
template <bool TIsConst = false>
class PinRef;
class INodeBehaviour;
template <bool TIsConst = false>
class NodeRefImpl;
class ImNodeFlow;

using NodeRef = NodeRefImpl<false>;
using ConstNodeRef = NodeRefImpl<true>;

} // namespace ImFlow

#endif // IM_NODE_FLOW_FWD
