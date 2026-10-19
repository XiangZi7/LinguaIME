#pragma once
#include "candidate_window.h"
#include "key_policy.h"

class TextService final : public ITfTextInputProcessorEx, public ITfKeyEventSink,
                          public ITfThreadMgrEventSink, public ITfCompositionSink {
public:
    TextService();
    ~TextService();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE Activate(ITfThreadMgr*, TfClientId) override;
    HRESULT STDMETHODCALLTYPE ActivateEx(ITfThreadMgr*, TfClientId, DWORD) override;
    HRESULT STDMETHODCALLTYPE Deactivate() override;
    HRESULT STDMETHODCALLTYPE OnSetFocus(BOOL) override;
    HRESULT STDMETHODCALLTYPE OnTestKeyDown(ITfContext*, WPARAM, LPARAM, BOOL*) override;
    HRESULT STDMETHODCALLTYPE OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL*) override;
    HRESULT STDMETHODCALLTYPE OnKeyDown(ITfContext*, WPARAM, LPARAM, BOOL*) override;
    HRESULT STDMETHODCALLTYPE OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL*) override;
    HRESULT STDMETHODCALLTYPE OnPreservedKey(ITfContext*, REFGUID, BOOL*) override;
    HRESULT STDMETHODCALLTYPE OnInitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnUninitDocumentMgr(ITfDocumentMgr*) override;
    HRESULT STDMETHODCALLTYPE OnSetFocus(ITfDocumentMgr*, ITfDocumentMgr*) override;
    HRESULT STDMETHODCALLTYPE OnPushContext(ITfContext*) override;
    HRESULT STDMETHODCALLTYPE OnPopContext(ITfContext*) override;
    HRESULT STDMETHODCALLTYPE OnCompositionTerminated(TfEditCookie, ITfComposition*) override;
    HRESULT edit(TfEditCookie, ITfContext*, const std::wstring&, bool commit, bool cancel);
#ifdef LINGUA_TESTING
    // The harness uses a real TSF context without claiming to be the OS-activated keyboard TIP.
    HRESULT initialize_for_test(TfClientId id) {
        client_id_=id;
        engine_=lingua_create();
        return engine_ && lingua_refresh(engine_,&config_)==0 ? S_OK : E_FAIL;
    }
#endif
private:
    HRESULT request_edit(ITfContext*, std::wstring text, bool commit, bool cancel);
    void refresh_candidates();
    HRESULT choose(unsigned index);
    void cancel();
    void clear_state();
    bool handles(ITfContext*, WPARAM) const;
    bool context_available(ITfContext*) const;
    std::atomic<ULONG> references_{1};
    ComPtr<ITfThreadMgr> thread_manager_;
    ComPtr<ITfContext> context_;
    ComPtr<ITfComposition> composition_;
    TfClientId client_id_ = TF_CLIENTID_NULL;
    DWORD thread_cookie_ = TF_INVALID_COOKIE;
    EngineContext* engine_ = nullptr;
    LinguaConfig config_{7,1,0,0,17,0,1,0,0xBD,0xBB};
    LinguaPage page_{};
    std::string buffer_;
    unsigned selected_ = 0;
    CandidateWindow window_;
};
