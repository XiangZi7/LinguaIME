#ifdef LINGUA_BOOTSTRAP
#include "bootstrap_service.h"
#else
#include "text_service.h"
#include <delayimp.h>
#endif
#include <new>

#ifndef LINGUA_BOOTSTRAP
namespace {
SRWLOCK g_creation_lock = SRWLOCK_INIT;
std::atomic<bool> g_rust_loaded{false};
FARPROC WINAPI load_rust_bridge(unsigned notification, PDelayLoadInfo info) {
    if (notification!=dliNotePreLoadLibrary || strcmp(info->szDll,"ime_ffi.dll")!=0) return nullptr;
    wchar_t path[MAX_PATH]{};
    if (!GetModuleFileNameW(g_module,path,MAX_PATH)) return nullptr;
    auto* separator=wcsrchr(path,L'\\');
    if (!separator) return nullptr;
    wcscpy_s(separator+1,static_cast<size_t>(path+MAX_PATH-separator-1),L"ime_ffi.dll");
    const auto module=LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (module) g_rust_loaded=true;
    return reinterpret_cast<FARPROC>(module);
}
}
extern "C" const PfnDliHook __pfnDliNotifyHook2=load_rust_bridge;
#endif

HINSTANCE g_module=nullptr;
std::atomic<long> g_objects{0};

class ClassFactory final : public IClassFactory {
public:
    ClassFactory() { ++g_objects; }
    ~ClassFactory() { --g_objects; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** output) override {
        if (!output) return E_POINTER;
        *output=nullptr;
        if (id==IID_IUnknown || id==IID_IClassFactory) { *output=static_cast<IClassFactory*>(this); AddRef(); return S_OK; }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references_; }
    ULONG STDMETHODCALLTYPE Release() override { const auto n=--references_; if (!n) delete this; return n; }
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID id, void** output) override {
        if (outer) return CLASS_E_NOAGGREGATION;
        try {
#ifdef LINGUA_BOOTSTRAP
            auto* service=new(std::nothrow) BootstrapService();
#else
            auto* service=new(std::nothrow) TextService();
#endif
            if (!service) return E_OUTOFMEMORY;
            const auto result=service->QueryInterface(id,output);
            service->Release();
            return result;
        } catch (...) { return E_OUTOFMEMORY; }
    }
    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override { if (lock) ++g_objects; else --g_objects; return S_OK; }
private:
    std::atomic<ULONG> references_{1};
};

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason==DLL_PROCESS_ATTACH) { g_module=instance; DisableThreadLibraryCalls(instance); }
    return TRUE;
}
STDAPI DllCanUnloadNow() { return g_objects==0?S_OK:S_FALSE; }
STDAPI DllGetClassObject(REFCLSID clsid, REFIID id, void** output) {
    if (clsid!=kClsid) return CLASS_E_CLASSNOTAVAILABLE;
#ifndef LINGUA_BOOTSTRAP
    AcquireSRWLockShared(&g_creation_lock);
#endif
    auto* factory=new(std::nothrow) ClassFactory();
#ifndef LINGUA_BOOTSTRAP
    ReleaseSRWLockShared(&g_creation_lock);
#endif
    if (!factory) return E_OUTOFMEMORY;
    const auto result=factory->QueryInterface(id,output);
    factory->Release(); return result;
}

#ifndef LINGUA_BOOTSTRAP
extern "C" uint32_t WINAPI LinguaRuntimeAbiVersion() { return kRuntimeAbi; }
extern "C" HRESULT WINAPI LinguaRuntimePrepareUnload() {
    AcquireSRWLockExclusive(&g_creation_lock);
    HRESULT result = S_FALSE;
    if (g_objects == 0 && g_runtime_callback_depth == 0) {
        if (g_rust_loaded) {
            lingua_release_cache();
            __FUnloadDelayLoadedDLL2("ime_ffi.dll");
            g_rust_loaded=false;
        }
        UnregisterClassW(L"LinguaIME.Candidate.v1", g_module);
        result = S_OK;
    }
    ReleaseSRWLockExclusive(&g_creation_lock);
    return result;
}
#endif
