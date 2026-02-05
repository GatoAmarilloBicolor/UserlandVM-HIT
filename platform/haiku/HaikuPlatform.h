/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * HaikuPlatform.h - Central header for Haiku platform integration
 */

#ifndef HAIKU_PLATFORM_H
#define HAIKU_PLATFORM_H

// System components
#include "system/Haiku32SyscallDispatcher.h"
#include "system/HaikuOptimizer.h"

// Memory management
#include "memory/HaikuAddressSpace.h"

// Threading
#include "threading/HaikuThreading.h"

// GUI components
#include "gui/HaikuGUIBackend.h"
#include "gui/HaikuGUISyscalls.h"
#include "gui/HaikuGUIStub.h"
#include "gui/HaikuNativeGUIBackend.h"

// Compatibility layer
#include "compat/haiku/HaikuStubs.h"
#include "compat/haiku/HaikuCompat.h"

#endif // HAIKU_PLATFORM_H