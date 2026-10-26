
/*
 * This file creates an amalgamation of all source files in order to keep this
 * library simple to compile, effectively, this is the only source file that
 * needs to compile.
 *
 * This is not the correct way to do this (the concept is called a Unity build,
 * or a jumbo build), but it does work.
 *
 * Source files were created with the ".impl" extension in order to prevent
 * new library users from being misled into compiling all source files
 * themselves (which could be done, tbf), but these are otherwise normal
 * C++ source files (.cpp).
 */

#include "./Context.impl"
#include "./DynamicPin.impl"
#include "./IContainedContext.impl"
#include "./INodeBehaviour.impl"
#include "./Link.impl"
#include "./NodeRef.impl"
#include "./PinConnectionHolder.impl"
#include "./PinRef.impl"
#include "./pin_base.impl"
#include "./uid_helpers.impl"
