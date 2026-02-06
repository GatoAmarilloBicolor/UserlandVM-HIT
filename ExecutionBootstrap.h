#ifndef EXECUTION_BOOTSTRAP_H
#define EXECUTION_BOOTSTRAP_H

#include "Loader.h"
#include "ExecutionEngine.h"

class ExecutionBootstrap {
public:
    ExecutionBootstrap();
    ~ExecutionBootstrap();
    
    status_t ExecuteProgram(const char *programPath, char **argv, char **env);
    
private:
    void *fExecutionEngine;
    void *fDynamicLinker;
    void *fGUIBackend;
    void *fSymbolResolver;
    void *fMemoryManager;
    void *fThreadManager;
    void *fProfiler;
};

#endif
