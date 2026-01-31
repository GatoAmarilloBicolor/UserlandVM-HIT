/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#pragma once

#include <SupportDefs.h>

class GuestContext;

// Interfaz para el motor de ejecución (intérprete, JIT, etc.)
class ExecutionEngine {
public:
	virtual ~ExecutionEngine() = default;

	virtual status_t Run(GuestContext& context) = 0;
};
