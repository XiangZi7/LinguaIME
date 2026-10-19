#include "runtime_contract.h"
#include <new>

HINSTANCE g_module = nullptr;
std::atomic<long> g_objects{0};
namespace {
bool g_idle = true;
bool g_fail_activation = false;
bool g_wrong_abi = false;

class MockRuntime final : public ITfTextInputProcessorEx, public ILinguaRuntimeControl {
public:
    MockRuntime() { ++g_objects; }
    ~MockRuntime() { --g_objects; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** output) override {
        if (!output) return E_POINTER;
        *output = nullptr;
        if (id == IID_IUnknown || id == IID_ITfTextInputProcessor || id == IID_ITfTextInputProcessorEx)
            *output = static_cast<ITfTextInputProcessorEx*>(this);
        else if (id == __uuidof(ILinguaRuntimeControl))
            *output = static_cast<ILinguaRuntimeControl*>(this);
        else return E_NOINTERFACE;
        AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references_; }
    ULONG STDMETHODCALLTYPE Release() override {
        const auto count = --references_;
        if (!count) delete this;
        return count;
    }
    HRESULT STDMETHODCALLTYPE Activate(ITfThreadMgr* manager, TfClientId client) override {
        return ActivateEx(manager, client, 0);
    }
    HRESULT STDMETHODCALLTYPE ActivateEx(ITfThreadMgr*, TfClientId, DWORD) override {
        if (g_fail_activation) return E_FAIL;
        if (active_) return E_UNEXPECTED;
        active_ = true;
        chinese_ = true;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Deactivate() override { active_ = false; return S_OK; }
    HRESULT STDMETHODCALLTYPE GetState(RuntimeState* state) override {
        if (!state) return E_POINTER;
        *state = {kRuntimeAbi, g_idle, chinese_};
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestoreMode(BOOL chinese) override {
        if (!active_) return E_UNEXPECTED;
        chinese_ = chinese != FALSE;
        return S_OK;
    }
private:
    ULONG references_ = 1;
    bool active_ = false;
    bool chinese_ = true;
};

class Factory final : public IClassFactory {
public:
    Factory() { ++g_objects; }
    ~Factory() { --g_objects; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** output) override {
        if (!output) return E_POINTER;
        *output = nullptr;
        if (id != IID_IUnknown && id != IID_IClassFactory) return E_NOINTERFACE;
        *output = static_cast<IClassFactory*>(this); AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references_; }
    ULONG STDMETHODCALLTYPE Release() override {
        const auto count = --references_;
        if (!count) delete this;
        return count;
    }
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID id, void** output) override {
        if (outer) return CLASS_E_NOAGGREGATION;
        auto* service = new (std::nothrow) MockRuntime();
        if (!service) return E_OUTOFMEMORY;
        const auto result = service->QueryInterface(id, output);
        service->Release(); return result;
    }
    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override {
        if (lock) ++g_objects; else --g_objects;
        return S_OK;
    }
private:
    ULONG references_ = 1;
};
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) g_module = module;
    return TRUE;
}
STDAPI DllCanUnloadNow() {
    return g_objects == 0 ? S_OK : S_FALSE;
}
STDAPI DllGetClassObject(REFCLSID clsid, REFIID id, void** output) {
    if (clsid != kClsid) return CLASS_E_CLASSNOTAVAILABLE;
    auto* factory = new (std::nothrow) Factory();
    if (!factory) return E_OUTOFMEMORY;
    const auto result = factory->QueryInterface(id, output);
    factory->Release(); return result;
}
extern "C" __declspec(dllexport) uint32_t WINAPI LinguaRuntimeAbiVersion() {
    return g_wrong_abi ? kRuntimeAbi + 1 : kRuntimeAbi;
}
extern "C" __declspec(dllexport) HRESULT WINAPI LinguaRuntimePrepareUnload() {
    return DllCanUnloadNow();
}
extern "C" __declspec(dllexport) void WINAPI SetMockIdle(BOOL idle) { g_idle = idle != FALSE; }
extern "C" __declspec(dllexport) void WINAPI SetMockActivationFailure(BOOL fail) {
    g_fail_activation = fail != FALSE;
}
extern "C" __declspec(dllexport) void WINAPI SetMockWrongAbi(BOOL wrong) { g_wrong_abi = wrong != FALSE; }
extern "C" __declspec(dllexport) unsigned WINAPI GetMockFlavor() { return MOCK_FLAVOR; }
