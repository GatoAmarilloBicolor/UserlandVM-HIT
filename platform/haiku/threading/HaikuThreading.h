/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * Haiku OS Native Threading Implementation
 * Uses Haiku's native threading for optimal performance
 */

#ifndef _HAIKU_THREADING_H
#define _HAIKU_THREADING_H

// Haiku OS native includes
#include <kernel/OS.h>
#include <SupportDefs.h>
#include <Locker.h>

// Haiku native thread wrapper
class HaikuThread {
public:
    HaikuThread();
    virtual ~HaikuThread();

    // Thread lifecycle management
    status_t CreateThread(thread_func entryFunction, const char* name, int32 priority, void* data);
    status_t CreateDetachedThread(thread_func entryFunction, const char* name, int32 priority, void* data);
    status_t Start();
    status_t Stop();
    status_t Kill();
    status_t Resume();
    status_t Suspend();
    status_t Wait(uint32 flags = 0, bigtime_t timeout = B_INFINITE_TIMEOUT);
    
    // Thread state queries
    thread_id GetThreadID() const { return fThreadID; }
    status_t GetStatus() const { return fStatus; }
    bool IsRunning() const;
    bool IsPaused() const;
    
    // Thread properties
    void SetPriority(int32 priority);
    int32 GetPriority() const;
    void SetName(const char* name);
    const char* GetName() const;

private:
    thread_id fThreadID;
    status_t fStatus;
    char fName[B_OS_NAME_LENGTH];
    int32 fPriority;
    bool fIsDetached;
    
    // Internal thread data
    void* fThreadData;
    static status_t ThreadEntry(void* data);
    
    // Thread data cleanup
    void Cleanup();
};

// Haiku thread pool for efficient thread management
class HaikuThreadPool {
public:
    HaikuThreadPool(size_t maxThreads);
    virtual ~HaikuThreadPool();
    
    // Thread pool operations
    status_t Initialize();
    status_t Shutdown();
    status_t SubmitTask(thread_func task, void* data, thread_id* threadID = nullptr);
    status_t WaitForTask(thread_id threadID, status_t* result = nullptr);
    status_t WaitForAllTasks(uint32 timeout = B_INFINITE_TIMEOUT);
    
    // Pool statistics
    size_t GetActiveThreadCount() const;
    size_t GetAvailableThreadCount() const;
    size_t GetMaxThreads() const { return fMaxThreads; }
    
    // Pool configuration
    status_t SetThreadPriority(int32 priority);
    status_t SetThreadStackSize(size_t stackSize);
    status_t SetThreadPoolName(const char* name);

private:
    struct ThreadTask {
        thread_id id;
        thread_func function;
        void* data;
        status_t result;
        bool completed;
    };
    
    ThreadTask* fTasks;
    HaikuThread* fThreads;
    size_t fMaxThreads;
    size_t fActiveCount;
    size_t fCompletedCount;
    
    // Synchronization
    BLocker fLock;
    sem_id fTaskSem;
    sem_id fCompleteSem;
    
    // Thread pool worker
    static status_t WorkerThread(void* data);
    status_t StartWorkerThreads();
    
    // Task management
    ThreadTask* GetNextTask();
    void MarkTaskCompleted(ThreadTask* task, status_t result);
};

// Haiku TLS (Thread Local Storage) support
class HaikuTLS {
public:
    HaikuTLS();
    virtual ~HaikuTLS();
    
    // TLS key management
    status_t AllocateKey();
    status_t AllocateKey(const char* name);
    status_t FreeKey();
    
    // Thread-local data operations
    status_t SetValue(void* value);
    status_t GetValue(void** value);
    status_t SetValueForThread(thread_id threadID, void* value);
    status_t GetValueForThread(thread_id threadID, void** value);
    
    // Key information
    thread_key GetKey() const { return fTLSKey; }
    const char* GetKeyName() const { return fName; }
    bool IsValid() const { return fValid; }

private:
    thread_key fTLSKey;
    char fName[B_OS_NAME_LENGTH];
    bool fValid;
    
    static void TLSDestructor(void* value);
};

// Thread utilities
namespace HaikuThreadUtils {
    // High-level thread operations
    status_t CreateDetachedWorker(thread_func worker, void* data, const char* name = "worker");
    status_t CreateThreadWithTimeout(thread_func entry, void* data, bigtime_t timeout, thread_id* threadID = nullptr);
    status_t TerminateThread(thread_id threadID, status_t exitCode);
    
    // Thread information
    status_t GetThreadInfo(thread_id threadID, thread_info* info);
    status_t GetTeamInfo(team_info* info);
    status_t GetThreadList(thread_id* threadArray, int32 maxCount, int32* actualCount);
    
    // Thread monitoring
    bigtime_t GetThreadRunTime(thread_id threadID);
    size_t GetThreadCPUUsage(thread_id threadID);
    
    // Debugging utilities
    void DumpThreadInfo();
    void DumpThreadPoolInfo(const HaikuThreadPool* pool);
    void DumpAllThreads();
}

// Thread safety utilities
class HaikuMutex {
public:
    HaikuMutex();
    virtual ~HaikuMutex();
    
    status_t Lock();
    status_t TryLock(bigtime_t timeout = 0);
    status_t Unlock();
    bool IsLocked() const;

private:
    BLocker fLock;
};

class HaikuReadWriteLock {
public:
    HaikuReadWriteLock();
    virtual ~HaikuReadWriteLock();
    
    status_t LockRead();
    status_t TryLockRead(bigtime_t timeout = 0);
    status_t LockWrite();
    status_t TryLockWrite(bigtime_t timeout = 0);
    status_t Unlock();
    
    bool IsReadLocked() const;
    bool IsWriteLocked() const;

private:
    BLocker fReadLock;
    BLocker fWriteLock;
    int32 fReaderCount;
};

#endif // _HAIKU_THREADING_H