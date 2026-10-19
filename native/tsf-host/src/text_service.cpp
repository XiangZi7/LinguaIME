#include "text_service.h"
#include <algorithm>
#include <new>
#include <utility>

namespace {
class EditSession final : public ITfEditSession {
public:
    EditSession(TextService* service, ITfContext* context, std::wstring text, bool commit, bool cancel)
        : service_(service), context_(context), text_(std::move(text)), commit_(commit), cancel_(cancel) { service_->AddRef(); ++g_objects; }
    ~EditSession() { service_->Release(); --g_objects; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** output) override {
        if (!output) return E_POINTER;
        *output = nullptr;
        if (id == IID_IUnknown || id == IID_ITfEditSession) { *output=static_cast<ITfEditSession*>(this); AddRef(); return S_OK; }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references_; }
    ULONG STDMETHODCALLTYPE Release() override { const auto n=--references_; if (!n) delete this; return n; }
    HRESULT STDMETHODCALLTYPE DoEditSession(TfEditCookie cookie) override {
        try { return service_->edit(cookie,context_.Get(),text_,commit_,cancel_); }
        catch (...) { return E_FAIL; }
    }
private:
    std::atomic<ULONG> references_{1};
    TextService* service_;
    ComPtr<ITfContext> context_;
    std::wstring text_;
    bool commit_, cancel_;
};
bool modifier_pressed() { return (GetKeyState(VK_CONTROL)&0x8000) || (GetKeyState(VK_MENU)&0x8000) ||
                                  (GetKeyState(VK_LWIN)&0x8000) || (GetKeyState(VK_RWIN)&0x8000) || (GetKeyState(VK_SHIFT)&0x8000); }
}

TextService::TextService() { ++g_objects; window_.on_select=[this](unsigned index) { choose(index); }; }
TextService::~TextService() { Deactivate(); --g_objects; }
ULONG TextService::AddRef() { return ++references_; }
ULONG TextService::Release() { const auto n=--references_; if (!n) delete this; return n; }
HRESULT TextService::QueryInterface(REFIID id, void** output) {
    if (!output) return E_POINTER;
    *output=nullptr;
    if (id==IID_IUnknown || id==IID_ITfTextInputProcessor || id==IID_ITfTextInputProcessorEx) *output=static_cast<ITfTextInputProcessorEx*>(this);
    else if (id==IID_ITfKeyEventSink) *output=static_cast<ITfKeyEventSink*>(this);
    else if (id==IID_ITfThreadMgrEventSink) *output=static_cast<ITfThreadMgrEventSink*>(this);
    else if (id==IID_ITfCompositionSink) *output=static_cast<ITfCompositionSink*>(this);
    else return E_NOINTERFACE;
    AddRef(); return S_OK;
}
HRESULT TextService::Activate(ITfThreadMgr* manager, TfClientId id) { return ActivateEx(manager,id,0); }
HRESULT TextService::ActivateEx(ITfThreadMgr* manager, TfClientId id, DWORD flags) {
    // Secure desktops are not supported; allow Windows to retain its normal input path.
    if (!manager || (flags & TF_TMAE_SECUREMODE)) return E_FAIL;
    try {
        thread_manager_=manager; client_id_=id;
        if (lingua_abi_version()!=1 || !(engine_=lingua_create())) return E_FAIL;
        lingua_refresh(engine_,&config_);
        ComPtr<ITfKeystrokeMgr> keys;
        HRESULT result=manager->QueryInterface(IID_PPV_ARGS(&keys));
        if (SUCCEEDED(result)) result=keys->AdviseKeyEventSink(id,this,TRUE);
        if (FAILED(result)) { Deactivate(); return result; }
        ComPtr<ITfSource> source;
        result=manager->QueryInterface(IID_PPV_ARGS(&source));
        if (SUCCEEDED(result)) result=source->AdviseSink(IID_ITfThreadMgrEventSink,static_cast<ITfThreadMgrEventSink*>(this),&thread_cookie_);
        if (FAILED(result)) { Deactivate(); return result; }
        return S_OK;
    } catch (...) { Deactivate(); return E_FAIL; }
}
HRESULT TextService::Deactivate() {
    window_.hide();
    if (context_ && composition_) request_edit(context_.Get(),L"",false,true);
    clear_state();
    if (thread_manager_) {
        ComPtr<ITfKeystrokeMgr> keys;
        if (SUCCEEDED(thread_manager_.As(&keys))) keys->UnadviseKeyEventSink(client_id_);
        ComPtr<ITfSource> source;
        if (thread_cookie_!=TF_INVALID_COOKIE && SUCCEEDED(thread_manager_.As(&source))) source->UnadviseSink(thread_cookie_);
    }
    thread_cookie_=TF_INVALID_COOKIE; thread_manager_.Reset(); client_id_=TF_CLIENTID_NULL;
    if (engine_) { lingua_destroy(engine_); engine_=nullptr; }
    return S_OK;
}
bool TextService::context_available(ITfContext* context) const {
    if (!context) return false;
    TF_STATUS status{};
    if (FAILED(context->GetStatus(&status)) || (status.dwDynamicFlags & TF_SD_READONLY)) return false;
    ComPtr<ITfCompartmentMgr> compartments;
    if (SUCCEEDED(context->QueryInterface(IID_PPV_ARGS(&compartments)))) {
        for (const GUID* id : {&GUID_COMPARTMENT_KEYBOARD_DISABLED,&GUID_COMPARTMENT_EMPTYCONTEXT}) {
            ComPtr<ITfCompartment> compartment;
            VARIANT value{};
            if (SUCCEEDED(compartments->GetCompartment(*id,&compartment)) && SUCCEEDED(compartment->GetValue(&value))) {
                const bool disabled=value.vt==VT_I4 && value.lVal!=0;
                VariantClear(&value);
                if (disabled) return false;
            }
        }
    }
    GUITHREADINFO gui{sizeof(gui)};
    if (GetGUIThreadInfo(GetCurrentThreadId(),&gui) && gui.hwndFocus) {
        wchar_t name[32]{}; GetClassNameW(gui.hwndFocus,name,32);
        if ((_wcsicmp(name,L"Edit")==0 || wcsstr(name,L"RichEdit")) && (GetWindowLongPtrW(gui.hwndFocus,GWL_STYLE)&ES_PASSWORD)) return false;
    }
    return true;
}
bool TextService::handles(ITfContext* context, WPARAM key) const {
    return engine_ && context_available(context) && lingua::handles_key(static_cast<uint32_t>(key),!buffer_.empty(),modifier_pressed(),config_.previous_key,config_.next_key);
}
HRESULT TextService::OnTestKeyDown(ITfContext* context, WPARAM key, LPARAM, BOOL* eaten) { if (!eaten) return E_POINTER; *eaten=handles(context,key); return S_OK; }
HRESULT TextService::OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) { if (!eaten) return E_POINTER; *eaten=FALSE; return S_OK; }
HRESULT TextService::OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) { if (!eaten) return E_POINTER; *eaten=FALSE; return S_OK; }
HRESULT TextService::OnPreservedKey(ITfContext*, REFGUID, BOOL* eaten) { if (!eaten) return E_POINTER; *eaten=FALSE; return S_OK; }

HRESULT TextService::OnKeyDown(ITfContext* context, WPARAM key, LPARAM, BOOL* eaten) {
    if (!eaten) return E_POINTER;
    *eaten=FALSE;
    try {
        if (!handles(context,key)) {
            if (!buffer_.empty()) cancel();
            return S_OK;
        }
        *eaten=TRUE;
        if (key==VK_ESCAPE) { cancel(); return S_OK; }
        if (key==VK_RETURN || key==VK_SPACE) {
            if (key==VK_RETURN && config_.enter_raw) {
                const auto result=request_edit(context,wide(buffer_.c_str()),true,false);
                if (SUCCEEDED(result)) clear_state();
                return result;
            }
            if (page_.item_count) return choose(selected_);
            const auto result=request_edit(context,wide(buffer_.c_str()),true,false);
            if (SUCCEEDED(result)) clear_state();
            return result;
        }
        if (key>='1' && key<='9') return choose(static_cast<unsigned>(key-'1'));
        if (key==VK_UP || key==VK_DOWN) {
            if (page_.item_count) selected_=(selected_+page_.item_count+(key==VK_UP?-1:1))%page_.item_count;
        } else if (key==config_.previous_key || key==config_.next_key) {
            unsigned page=page_.page_index;
            if (key==config_.previous_key && page>0) --page;
            if (key==config_.next_key && page+1<page_.page_count) ++page;
            if (lingua_query(engine_,buffer_.c_str(),page,&page_)!=0) { cancel(); return S_OK; }
            selected_=0;
        } else {
            if (buffer_.empty()) { lingua_refresh(engine_,&config_); context_=context; }
            if (key==VK_BACK) { if (!buffer_.empty()) buffer_.pop_back(); }
            else if (key==VK_OEM_7) { if (!buffer_.empty() && buffer_.back()!='\'') buffer_+='\''; }
            else if (buffer_.size()<128) buffer_+=static_cast<char>(key-'A'+'a');
            if (buffer_.empty()) { cancel(); return S_OK; }
            refresh_candidates();
        }
        const auto result=request_edit(context,wide(buffer_.c_str()),false,false);
        if (FAILED(result)) { clear_state(); *eaten=FALSE; }
        return S_OK;
    } catch (...) { clear_state(); *eaten=FALSE; return S_OK; }
}
void TextService::refresh_candidates() { selected_=0; if (lingua_query(engine_,buffer_.c_str(),0,&page_)!=0) page_={}; }
HRESULT TextService::choose(unsigned index) {
    if (index>=page_.item_count || !context_) return S_OK;
    const auto candidate=page_.items[index];
    const auto result=request_edit(context_.Get(),wide(candidate.text),true,false);
    if (SUCCEEDED(result)) { lingua_record(engine_,candidate.id); clear_state(); }
    return result;
}
void TextService::clear_state() { buffer_.clear(); page_={}; selected_=0; composition_.Reset(); context_.Reset(); window_.hide(); }
void TextService::cancel() { if (context_) request_edit(context_.Get(),L"",false,true); clear_state(); }
HRESULT TextService::OnSetFocus(BOOL foreground) { if (!foreground) cancel(); return S_OK; }
HRESULT TextService::OnSetFocus(ITfDocumentMgr*, ITfDocumentMgr*) { cancel(); return S_OK; }
HRESULT TextService::OnUninitDocumentMgr(ITfDocumentMgr*) { cancel(); return S_OK; }
HRESULT TextService::OnPushContext(ITfContext*) { cancel(); return S_OK; }
HRESULT TextService::OnPopContext(ITfContext*) { cancel(); return S_OK; }
HRESULT TextService::OnCompositionTerminated(TfEditCookie cookie, ITfComposition* composition) {
    if (composition_.Get()==composition) {
        ComPtr<ITfRange> range;
        if (SUCCEEDED(composition->GetRange(&range))) range->SetText(cookie,0,L"",0);
        clear_state();
    }
    return S_OK;
}
HRESULT TextService::request_edit(ITfContext* context, std::wstring text, bool commit, bool cancel) {
    auto* session=new(std::nothrow) EditSession(this,context,std::move(text),commit,cancel);
    if (!session) return E_OUTOFMEMORY;
    HRESULT session_result=E_FAIL;
    const auto result=context->RequestEditSession(client_id_,session,TF_ES_SYNC|TF_ES_READWRITE,&session_result);
    session->Release();
    return FAILED(result)?result:session_result;
}
HRESULT TextService::edit(TfEditCookie cookie, ITfContext* context, const std::wstring& text, bool commit, bool cancel) {
    ComPtr<ITfRange> range;
    if (composition_) {
        const auto result=composition_->GetRange(&range);
        if (FAILED(result)) return result;
    } else {
        if (cancel) return S_OK;
        ComPtr<ITfInsertAtSelection> insertion;
        auto result=context->QueryInterface(IID_PPV_ARGS(&insertion));
        if (FAILED(result)) return result;
        result=insertion->InsertTextAtSelection(cookie,TF_IAS_QUERYONLY,L"",0,&range);
        if (FAILED(result)) return result;
        ComPtr<ITfContextComposition> compositions;
        result=context->QueryInterface(IID_PPV_ARGS(&compositions));
        if (FAILED(result)) return result;
        result=compositions->StartComposition(cookie,range.Get(),this,&composition_);
        if (FAILED(result)) return result;
    }
    auto result=range->SetText(cookie,0,cancel?L"":text.c_str(),cancel?0:static_cast<LONG>(text.size()));
    if (FAILED(result)) return result;
    ComPtr<ITfRange> caret;
    result=range->Clone(&caret);
    if (FAILED(result)) return result;
    caret->Collapse(cookie,TF_ANCHOR_END);
    TF_SELECTION selection{caret.Get(),{TF_AE_NONE,FALSE}};
    context->SetSelection(cookie,1,&selection);
    if (commit || cancel) {
        ComPtr<ITfComposition> ending=composition_;
        composition_.Reset();
        result=ending->EndComposition(cookie);
        window_.hide();
        return result;
    }
    ComPtr<ITfContextView> view;
    RECT position{}; BOOL clipped=FALSE;
    if (SUCCEEDED(context->GetActiveView(&view)) && SUCCEEDED(view->GetTextExt(cookie,caret.Get(),&position,&clipped))) {
        window_.show(buffer_,page_,selected_,config_,position);
    } else {
        GUITHREADINFO info{sizeof(info)};
        if (GetGUIThreadInfo(GetCurrentThreadId(),&info) && info.hwndCaret) {
            position=info.rcCaret;
            MapWindowPoints(info.hwndCaret,nullptr,reinterpret_cast<POINT*>(&position),2);
        } else {
            POINT cursor{}; GetCursorPos(&cursor); position={cursor.x,cursor.y,cursor.x,cursor.y+16};
        }
        window_.show(buffer_,page_,selected_,config_,position);
    }
    return S_OK;
}
