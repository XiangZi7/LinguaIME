#include "text_service.h"
#include "tsf_test_environment.h"
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
void expect_mode(ITfLangBarItemButton* button, const wchar_t* expected) {
    if (!button) throw std::runtime_error("language bar item missing");
    BSTR text=nullptr;
    require(button->GetText(&text),"language bar text");
    const bool matches=text && wcscmp(text,expected)==0;
    SysFreeString(text);
    if (!matches) throw std::runtime_error("language bar mode mismatch");
}
class ButtonSink final : public ITfLangBarItemSink {
public:
    DWORD updates=0;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** output) override {
        if (!output) return E_POINTER;
        *output=nullptr;
        if (id!=IID_IUnknown && id!=IID_ITfLangBarItemSink) return E_NOINTERFACE;
        *output=static_cast<ITfLangBarItemSink*>(this);
        AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
    ULONG STDMETHODCALLTYPE Release() override { const auto n=--refs_; if (!n) delete this; return n; }
    HRESULT STDMETHODCALLTYPE OnUpdate(DWORD flags) override { updates|=flags; return S_OK; }
private:
    ULONG refs_=1;
};
void test_button_contract() {
    unsigned clicks=0;
    ComPtr<LanguageBarButton> button;
    button.Attach(new LanguageBarButton([&clicks]() { ++clicks; return S_OK; }));
    ComPtr<ButtonSink> sink; sink.Attach(new ButtonSink());
    DWORD cookie=TF_INVALID_COOKIE;
    require(button->AdviseSink(IID_ITfLangBarItemSink,sink.Get(),&cookie),"button sink");
    expect_mode(button.Get(),L"中");
    button->set_mode(false);
    expect_mode(button.Get(),L"英");
    if ((sink->updates & TF_LBI_BTNALL)!=TF_LBI_BTNALL)
        throw std::runtime_error("mode change did not notify Windows");
    HICON icon=nullptr;
    require(button->GetIcon(&icon),"mode icon");
    ICONINFO info{};
    if (!GetIconInfo(icon,&info) || !info.hbmMask || info.hbmColor)
        throw std::runtime_error("mode icon is not monochrome");
    DeleteObject(info.hbmMask);
    DestroyIcon(icon);
    require(button->OnClick(TF_LBI_CLK_RIGHT,POINT{},nullptr),"right click");
    if (clicks) throw std::runtime_error("right click switched mode");
    require(button->OnClick(TF_LBI_CLK_LEFT,POINT{},nullptr),"left click");
    if (clicks!=1) throw std::runtime_error("left click did not switch mode");
    require(button->UnadviseSink(cookie),"button unadvise");
    if (button->UnadviseSink(cookie)!=CONNECT_E_NOCONNECTION)
        throw std::runtime_error("stale sink cookie accepted");
    button->detach();
    require(button->OnClick(TF_LBI_CLK_LEFT,POINT{},nullptr),"detached click");
    if (clicks!=1) throw std::runtime_error("detached button accessed service");
    ComPtr<LanguageBarButton> reentrant;
    reentrant.Attach(new LanguageBarButton([&reentrant]() {
        reentrant->detach();
        return S_OK;
    }));
    require(reentrant->OnClick(TF_LBI_CLK_LEFT,POINT{},nullptr),"deactivation during click");
}
}

HRESULT TextService::initialize_for_test(ITfThreadMgr* manager, TfClientId id) {
    thread_manager_=manager;
    client_id_=id;
    engine_=lingua_create();
    if (!engine_) {
        std::cerr << "Test Rust engine creation failed.\n";
        return E_FAIL;
    }
    if (lingua_refresh(engine_,&config_)!=0) {
        std::cerr << "Test engine settings refresh failed.\n";
        return E_FAIL;
    }
    const auto result=initialize_language_bar();
    require(result,"test language bar initialization");
    return result;
}

int main(int argument_count, char** arguments) {
    try {
        require(CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED),"COM");
        test_button_contract();
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
        if (argument_count>1 && strcmp(arguments[1],"--installed-profile")==0) {
            require(manager->Activate(&id),"precondition manager activation");
            ComPtr<ITfInputProcessorProfiles> profiles;
            require(CoCreateInstance(CLSID_TF_InputProcessorProfiles,nullptr,CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&profiles)),"installed profile manager");
            ComPtr<ITfInputProcessorProfileMgr> profile_manager;
            require(profiles.As(&profile_manager),"installed profile activation manager");
            require(profiles->ChangeCurrentLanguage(kLanguage),"process language");
            require(profile_manager->ActivateProfile(TF_PROFILETYPE_INPUTPROCESSOR,kLanguage,
                kClsid,kProfile,nullptr,TF_IPPMF_FORPROCESS),"process-only installed profile");
            std::cout << "Installed LinguaIME selected for the test process.\n";
            require(manager->Deactivate(),"precondition manager deactivation");
        }
        require(activate_isolated_tsf(manager.Get(),&id),"isolated manager activation");
        ComPtr<TextService> service;
        service.Attach(new TextService());
        require(service->initialize_for_test(manager.Get(),id),"test service initialization");
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
        RuntimeState update_state{};
        require(service->GetState(&update_state),"busy runtime state");
        if (update_state.idle) throw std::runtime_error("runtime update allowed during composition");
        require(keys->OnKeyDown(context.Get(),VK_RETURN,0,&eaten),"commit");
        if (read(context.Get(),id)!=L"学习") throw std::runtime_error("committed text mismatch");
        require(service->GetState(&update_state),"idle runtime state");
        if (!update_state.idle) throw std::runtime_error("runtime not idle after commit");
        {
            RuntimeCall callback;
            require(service->GetState(&update_state),"nested callback runtime state");
            if (update_state.idle) throw std::runtime_error("runtime update allowed inside a callback");
        }
        for (const char key : std::string("NIHAO")) require(keys->OnKeyDown(context.Get(),static_cast<WPARAM>(key),0,&eaten),"second composition");
        require(keys->OnKeyDown(context.Get(),VK_ESCAPE,0,&eaten),"cancel");
        if (read(context.Get(),id)!=L"学习") throw std::runtime_error("cancel changed committed text");
        ComPtr<ITfLangBarItemButton> mode_button=service->language_bar_for_test();
        expect_mode(mode_button.Get(),L"中");
        TF_LANGBARITEMINFO item_info{};
        require(mode_button->GetInfo(&item_info),"language bar info");
        if (item_info.guidItem!=GUID_LBI_INPUTMODE || item_info.clsidService!=kClsid ||
            !(item_info.dwStyle & TF_LBI_STYLE_SHOWNINTRAY))
            throw std::runtime_error("Windows input indicator contract mismatch");
        for (const char key : std::string("XUEXI"))
            require(keys->OnKeyDown(context.Get(),static_cast<WPARAM>(key),0,&eaten),"composition before mode switch");
        require(mode_button->OnClick(TF_LBI_CLK_LEFT,POINT{},nullptr),"switch English");
        expect_mode(mode_button.Get(),L"英");
        if (read(context.Get(),id)!=L"学习学习")
            throw std::runtime_error("mode switch lost the Chinese composition");
        for (const WPARAM key : {WPARAM('A'), WPARAM(VK_OEM_PERIOD), WPARAM(VK_OEM_COMMA),
                                WPARAM(VK_OEM_MINUS), WPARAM(VK_OEM_PLUS), WPARAM(VK_SPACE)}) {
            require(keys->OnTestKeyDown(context.Get(),key,0,&eaten),"English test key");
            if (eaten) throw std::runtime_error("English test key intercepted");
            require(keys->OnKeyDown(context.Get(),key,0,&eaten),"English direct input");
            if (eaten) throw std::runtime_error("English key intercepted");
        }
        if (read(context.Get(),id)!=L"学习学习")
            throw std::runtime_error("English mode changed document text");
        require(mode_button->OnClick(TF_LBI_CLK_LEFT,POINT{},nullptr),"switch Chinese");
        expect_mode(mode_button.Get(),L"中");
        require(keys->OnTestKeyDown(context.Get(),'A',0,&eaten),"Chinese key after switch");
        if (!eaten) throw std::runtime_error("Chinese mode did not resume composition");
        ComPtr<ITfCompartmentMgr> compartments;
        require(manager.As(&compartments),"mode compartment manager");
        for (const GUID* guid : {&GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                 &GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION}) {
            ComPtr<ITfCompartment> compartment;
            require(compartments->GetCompartment(*guid,&compartment),"mode compartment");
            VARIANT value{}; value.vt=VT_I4; value.lVal=0;
            require(compartment->SetValue(id,&value),"external English mode");
            expect_mode(mode_button.Get(),L"英");
            require(mode_button->OnClick(TF_LBI_CLK_LEFT,POINT{},nullptr),"restore Chinese mode");
            expect_mode(mode_button.Get(),L"中");
        }
        for (const char key : std::string("NH"))
            require(keys->OnKeyDown(context.Get(),static_cast<WPARAM>(key),0,&eaten),"Rime initials composition");
        require(keys->OnKeyDown(context.Get(),VK_SPACE,0,&eaten),"Rime initials commit");
        if (read(context.Get(),id)!=L"学习学习你好") throw std::runtime_error("Rime initials committed text mismatch");
        require(service->Deactivate(),"service deactivation");
        require(mode_button->OnClick(TF_LBI_CLK_LEFT,POINT{},nullptr),"button after deactivation");
        document->Pop(TF_POPF_ALL);
        manager->Deactivate();
        if (argument_count>1 && strcmp(arguments[1],"--preview")==0) {
            auto* engine=lingua_create();
            LinguaPage page{}; LinguaConfig configuration{};
            lingua_refresh(engine,&configuration);
            lingua_query(engine,"d",0,&page);
            configuration.theme=1; configuration.accent=2;
            CandidateWindow preview;
            for (uint32_t mode=0;mode<3;++mode) {
                configuration.display_mode=mode;
                preview.show("d",page,0,configuration,RECT{40,40,40,60});
                const auto path=L"candidate-mode-"+std::to_wstring(mode)+L".ppm";
                if (!preview.save_preview(path.c_str())) throw std::runtime_error("preview export");
                preview.hide();
            }
            lingua_destroy(engine);
        }
        std::cout << "Real TSF context: composition, commit, cancel, taskbar mode toggle, compartment sync and English key routing passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
