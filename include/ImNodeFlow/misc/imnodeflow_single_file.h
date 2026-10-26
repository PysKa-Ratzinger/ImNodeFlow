// ImNodeFlow: single-file wrapper include
//
// Include this file in *one* C++ file to create the implementation.

#ifndef IMNODEFLOW_SINGLE_FILE_H
#define IMNODEFLOW_SINGLE_FILE_H
#pragma once

// NOLINTBEGIN (bugprone-suspicious-include)
#include "../../../../src/Context.cpp"
#include "../../../../src/DynamicPin.cpp"
#include "../../../../src/IContainedContext.cpp"
#include "../../../../src/INodeBehaviour.cpp"
#include "../../../../src/Link.cpp"
#include "../../../../src/NodeRef.cpp"
#include "../../../../src/pin_base.cpp"
#include "../../../../src/PinConnectionHolder.cpp"
#include "../../../../src/PinRef.cpp"
#include "../../../../src/uid_helpers.cpp"
// NOLINTEND

#endif  // IMNODEFLOW_SINGLE_FILE_H
