#include "ImNodeFlow.h"

#include "./Context.impl"
#include "./DynamicPin.impl"
#include "./INodeBehaviour.impl"
#include "./Link.impl"
#include "./NodeRef.impl"
#include "./PinConnectionHolder.impl"
#include "./PinRef.impl"
#include "./uid_helpers.impl"

#include "ImNodeFlow/IContainedContext.h"

namespace ImFlow {

IContainedContext::~IContainedContext() = default;

} // namespace ImFlow
