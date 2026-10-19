#pragma once
#include "common.h"

inline constexpr uint32_t kRuntimeAbi = 1;
inline thread_local unsigned g_runtime_callback_depth = 0;

// A timer must never replace a module while one of its callbacks is on the stack.
struct RuntimeCall final {
    RuntimeCall() { ++g_runtime_callback_depth; }
    ~RuntimeCall() { --g_runtime_callback_depth; }
    RuntimeCall(const RuntimeCall&) = delete;
    RuntimeCall& operator=(const RuntimeCall&) = delete;
};

struct RuntimeState {
    uint32_t abi;
    BOOL idle;
    BOOL chinese;
};

MIDL_INTERFACE("393EF375-6F56-4CC7-A0DB-DFA518A9A18B")
ILinguaRuntimeControl : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetState(RuntimeState*) = 0;
    virtual HRESULT STDMETHODCALLTYPE RestoreMode(BOOL chinese) = 0;
};

MIDL_INTERFACE("46741D59-BB34-4E94-AED0-F68A0284479F")
ILinguaUpdateStatus : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetRuntimeVersion(BSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckForUpdate() = 0;
};
