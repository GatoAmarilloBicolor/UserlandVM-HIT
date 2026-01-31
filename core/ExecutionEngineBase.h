/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#pragma once

#include <SupportDefs.h>

class GuestContextBase;

/**
 * Abstract base class for execution engines
 * Implementations: InterpreterX86_32, JITCompilerX86_32, InterpreterARM, etc.
 */
class ExecutionEngineBase {
public:
    virtual ~ExecutionEngineBase() = default;

    /**
     * Execute guest code until termination or error
     * @param context Guest execution context to run
     * @return B_OK on successful execution completion
     *         B_ERROR or specific error code on failure
     */
    virtual status_t Run(GuestContextBase& context) = 0;

    /**
     * Optional: Get information about the engine
     */
    virtual const char* GetEngineName() const {
        return "ExecutionEngine";
    }
};
