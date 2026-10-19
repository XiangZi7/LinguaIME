#pragma once
#include "runtime_loader.h"
#include <vector>

// This small TSF entry stays loaded. Input and candidate code live in runtime generations.
class BootstrapService final : public ITfTextInputProcessorEx, public ILinguaUpdateStatus {
public:
    BootstrapService();
    ~BootstrapService();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE Activate(ITfThreadMgr*, TfClientId) override;
    HRESULT STDMETHODCALLTYPE ActivateEx(ITfThreadMgr*, TfClientId, DWORD) override;
    HRESULT STDMETHODCALLTYPE Deactivate() override;
    HRESULT STDMETHODCALLTYPE GetRuntimeVersion(BSTR*) override;
    HRESULT STDMETHODCALLTYPE CheckForUpdate() override;
#ifdef LINGUA_TESTING
    explicit BootstrapService(const std::filesystem::path& root);
    ILinguaRuntimeControl* runtime_for_test() const { return current_->control(); }
#endif

private:
    static LRESULT CALLBACK window_proc(HWND, UINT, WPARAM, LPARAM);
    HRESULT start_timer();
    HRESULT load_initial();
    void collect_retired();
    void retire(std::unique_ptr<RuntimeGeneration>);
    std::atomic<ULONG> references_{1};
    std::filesystem::path root_;
    ComPtr<ITfThreadMgr> manager_;
    TfClientId client_ = TF_CLIENTID_NULL;
    DWORD flags_ = 0;
    HWND timer_window_ = nullptr;
    bool checking_ = false;
    std::wstring failed_version_;
    std::unique_ptr<RuntimeGeneration> current_;
    std::vector<std::unique_ptr<RuntimeGeneration>> retired_;
};
