#ifndef AUTODELETTER_OS_H
#define AUTODELETTER_OS_H

// Haiku AutoDeleter stub for UserlandVM-HIT
// Minimal implementation for compilation compatibility

template<typename T>
class AutoDeleterOS {
public:
    AutoDeleterOS(T* ptr) : fPtr(ptr) {}
    ~AutoDeleterOS() { delete fPtr; }
private:
    T* fPtr;
};

#define AutoDeleterOS(type, name) AutoDeleterOS<type> name

#endif /* AUTODELETTER_OS_H */