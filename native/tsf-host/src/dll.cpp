#include "text_service.h"
#include <new>
#include <delayimp.h>

namespace {
FARPROC WINAPI load_rust_bridge(unsigned notification, PDelayLoadInfo info) {
    if (notification!=dliNotePreLoadLibrary || strcmp(info->szDll,"ime_ffi.dll")!=0) return nullptr;
    wchar_t path[MAX_PATH]{};
    if (!GetModuleFileNameW(g_module,path,MAX_PATH)) return nullptr;
    auto* separator=wcsrchr(path,L'\\');
    if (!separator) return nullptr;
    wcscpy_s(separator+1,static_cast<size_t>(path+MAX_PATH-separator-1),L"ime_ffi.dll");
    return reinterpret_cast<FARPROC>(LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS));
}
}
extern "C" const PfnDliHook __pfnDliNotifyHook2=load_rust_bridge;

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
            auto* service=new(std::nothrow) TextService();
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
    auto* factory=new(std::nothrow) ClassFactory();
    if (!factory) return E_OUTOFMEMORY;
    const auto result=factory->QueryInterface(id,output);
    factory->Release(); return result;
}
