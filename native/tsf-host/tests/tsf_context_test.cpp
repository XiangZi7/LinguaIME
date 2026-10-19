#include "text_service.h"
#include <iostream>
#include <stdexcept>

HINSTANCE g_module=GetModuleHandleW(nullptr);
std::atomic<long> g_objects{0};

namespace {
void require(HRESULT result, const char* step) {
    if (FAILED(result)) {
        std::cerr << step << " failed: 0x" << std::hex << static_cast<unsigned long>(result) << '\n';
        throw std::runtime_error(step);
    }
}
class CaptureSession final : public ITfEditSession {
public:
    ComPtr<ITfContext> context;
    std::wstring text;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** output) override {
        if (!output) return E_POINTER;
        *output=nullptr;
        if (id==IID_IUnknown || id==IID_ITfEditSession) { *output=this; AddRef(); return S_OK; }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
    ULONG STDMETHODCALLTYPE Release() override { auto n=--refs_; if (!n) delete this; return n; }
    HRESULT STDMETHODCALLTYPE DoEditSession(TfEditCookie cookie) override {
        ComPtr<ITfRange> start,end;
        HRESULT result=context->GetStart(cookie,&start);
        if (FAILED(result)) return result;
        result=context->GetEnd(cookie,&end);
        if (FAILED(result)) return result;
        result=start->ShiftEndToRange(cookie,end.Get(),TF_ANCHOR_END);
        if (FAILED(result)) return result;
        wchar_t buffer[256]{}; ULONG count=0;
        result=start->GetText(cookie,0,buffer,255,&count);
        text.assign(buffer,count);
        return result;
    }
private:
    ULONG refs_=1;
};
std::wstring read(ITfContext* context, TfClientId id) {
    auto* capture=new CaptureSession();
    capture->context=context;
    HRESULT inner=E_FAIL;
    require(context->RequestEditSession(id,capture,TF_ES_SYNC|TF_ES_READ,&inner),"read session");
    require(inner,"read text");
    const auto result=capture->text;
    capture->Release();
    return result;
}
}

int main() {
    try {
        require(CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED),"COM");
        const auto module=LoadLibraryExW(L"lingua_tsf.dll",nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!module) throw std::runtime_error("LoadLibrary");
        const auto get_factory=reinterpret_cast<HRESULT(WINAPI*)(REFCLSID,REFIID,void**)>(GetProcAddress(module,"DllGetClassObject"));
        if (!get_factory) throw std::runtime_error("DllGetClassObject");
        ComPtr<IClassFactory> factory;
        require(get_factory(kClsid,IID_PPV_ARGS(&factory)),"factory");
        ComPtr<ITfTextInputProcessorEx> exported_service;
        require(factory->CreateInstance(nullptr,IID_PPV_ARGS(&exported_service)),"service factory");
        exported_service.Reset();
        ComPtr<ITfThreadMgr> manager;
        require(CoCreateInstance(CLSID_TF_ThreadMgr,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&manager)),"thread manager");
        TfClientId id=TF_CLIENTID_NULL;
        require(manager->Activate(&id),"manager activation");
        ComPtr<TextService> service;
        service.Attach(new TextService());
        require(service->initialize_for_test(id),"test engine initialization");
        ComPtr<ITfDocumentMgr> document;
        require(manager->CreateDocumentMgr(&document),"document");
        ComPtr<ITfContext> context; TfEditCookie owner_cookie=0;
        require(document->CreateContext(id,0,nullptr,&context,&owner_cookie),"context");
        require(document->Push(context.Get()),"push");
        require(manager->SetFocus(document.Get()),"focus");
        ComPtr<ITfKeyEventSink> keys;
        require(service.As(&keys),"key sink");
        BOOL eaten=TRUE;
        require(keys->OnTestKeyDown(context.Get(),VK_OEM_MINUS,0,&eaten),"plain minus");
        if (eaten) throw std::runtime_error("minus intercepted while idle");
        for (const char key : std::string("XUEXI")) {
            require(keys->OnKeyDown(context.Get(),static_cast<WPARAM>(key),0,&eaten),"composing key");
            if (!eaten) throw std::runtime_error("letter not handled");
        }
        if (read(context.Get(),id)!=L"xuexi") throw std::runtime_error("composition text mismatch");
        require(keys->OnKeyDown(context.Get(),VK_RETURN,0,&eaten),"commit");
        if (read(context.Get(),id)!=L"学习") throw std::runtime_error("committed text mismatch");
        for (const char key : std::string("NIHAO")) require(keys->OnKeyDown(context.Get(),static_cast<WPARAM>(key),0,&eaten),"second composition");
        require(keys->OnKeyDown(context.Get(),VK_ESCAPE,0,&eaten),"cancel");
        if (read(context.Get(),id)!=L"学习") throw std::runtime_error("cancel changed committed text");
        require(service->Deactivate(),"service deactivation");
        document->Pop(TF_POPF_ALL);
        manager->Deactivate();
        std::cout << "Real TSF context: composition, Chinese commit, cancel and idle key routing passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
