/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#pragma once

#include "PlatformTypes.h"

class GuestContext;

// Interfaz para el manejador de llamadas al sistema.
// Abstrae las diferentes "personalidades" (Haiku32, Linux32, etc.)
class SyscallDispatcher {
public:
	virtual ~SyscallDispatcher() = default;

	// Inspecciona el contexto para determinar la syscall y los argumentos,
	// la ejecuta y modifica el contexto con el resultado.
	// Returns B_OK normally, or special codes for control flow (e.g., exit)
	virtual status_t Dispatch(GuestContext& context) { 
		DispatchLegacy(context);
		return B_OK;
	}

	// Legacy interface for backward compatibility
	virtual void DispatchLegacy(GuestContext& context) {}
};
