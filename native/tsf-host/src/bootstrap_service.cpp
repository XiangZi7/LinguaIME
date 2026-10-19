#include "bootstrap_service.h"
#include "diagnostics.h"
#include <utility>

namespace {
constexpr wchar_t kLoaderWindow[] = L"LinguaIME.UpdateLoader.v1";
constexpr UINT_PTR kUpdateTimer = 1;
}

BootstrapService::BootstrapService() : root_(installed_runtime_root()) { ++g_objects; }
#ifdef LINGUA_TESTING
BootstrapService::BootstrapService(const std::filesystem::path& root) : root_(root) { ++g_objects; }
#endif
BootstrapService::~BootstrapService() {
    Deactivate();
    --g_objects;
}

HRESULT BootstrapService::QueryInterface(REFIID id, void** output) {
    if (!output) return E_POINTER;
    *output = nullptr;
    if (id == IID_IUnknown || id == IID_ITfTextInputProcessor || id == IID_ITfTextInputProcessorEx)
        *output = static_cast<ITfTextInputProcessorEx*>(this);
    else if (id == __uuidof(ILinguaUpdateStatus))
        *output = static_cast<ILinguaUpdateStatus*>(this);
    else return E_NOINTERFACE;
    AddRef();
    return S_OK;
}

ULONG BootstrapService::AddRef() { return ++references_; }
ULONG BootstrapService::Release() {
    const auto count = --references_;
    if (!count) delete this;
    return count;
}

HRESULT BootstrapService::Activate(ITfThreadMgr* manager, TfClientId client) {
    return ActivateEx(manager, client, 0);
}

HRESULT BootstrapService::ActivateEx(ITfThreadMgr* manager, TfClientId client, DWORD flags) {
    if (!manager || (flags & TF_TMAE_SECUREMODE)) return E_INVALIDARG;
    if (manager_) return E_UNEXPECTED;
    try {
        manager_ = manager;
        client_ = client;
        flags_ = flags;
        auto result = load_initial();
        if (SUCCEEDED(result)) result = start_timer();
        if (FAILED(result)) Deactivate();
        return result;
    } catch (...) {
        Deactivate();
        return E_FAIL;
    }
}

HRESULT BootstrapService::load_initial() {
    HRESULT result = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    for (const bool previous : {false, true}) {
        const auto version = read_runtime_version(root_, previous);
        if (version.empty()) continue;
        auto candidate = std::make_unique<RuntimeGeneration>(version);
        result = candidate->load(root_);
        if (SUCCEEDED(result)) result = candidate->activate(manager_.Get(), client_, flags_);
        if (SUCCEEDED(result)) {
            current_ = std::move(candidate);
            return S_OK;
        }
        if (!previous) failed_version_ = version;
        retire(std::move(candidate));
    }
    return result;
}

HRESULT BootstrapService::Deactivate() {
    if (timer_window_) {
        KillTimer(timer_window_, kUpdateTimer);
        SetWindowLongPtrW(timer_window_, GWLP_USERDATA, 0);
        DestroyWindow(timer_window_);
        timer_window_ = nullptr;
    }
    if (current_) retire(std::move(current_));
    collect_retired();
    manager_.Reset();
    client_ = TF_CLIENTID_NULL;
    failed_version_.clear();
    return S_OK;
}

HRESULT BootstrapService::GetRuntimeVersion(BSTR* version) {
    if (!version) return E_POINTER;
    *version = SysAllocString(current_ ? current_->version().c_str() : L"");
    return *version ? S_OK : E_OUTOFMEMORY;
}

void BootstrapService::retire(std::unique_ptr<RuntimeGeneration> runtime) {
    runtime->retire();
    if (runtime->try_unload()) return;
    try {
        retired_.push_back(std::move(runtime));
    } catch (...) {
        // The generation destructor pins a still-referenced module rather than invalidating COM.
    }
}

void BootstrapService::collect_retired() {
    std::erase_if(retired_, [](auto& runtime) { return runtime->try_unload(); });
}

HRESULT BootstrapService::CheckForUpdate() {
    if (checking_ || !current_ || !manager_) return S_FALSE;
    struct CheckScope {
        bool& active;
        explicit CheckScope(bool& value) : active(value) { active = true; }
        ~CheckScope() { active = false; }
    } scope(checking_);
    try {
        collect_retired();
        const auto version = read_runtime_version(root_);
        if (version.empty() || version == current_->version() || version == failed_version_)
            return S_FALSE;
        RuntimeState state{};
        auto result = current_->control()->GetState(&state);
        if (FAILED(result)) return result;
        if (state.abi != kRuntimeAbi || !state.idle) return S_FALSE;
        auto candidate = std::make_unique<RuntimeGeneration>(version);
        result = candidate->load(root_);
        if (SUCCEEDED(result)) result = current_->deactivate();
        if (SUCCEEDED(result)) result = candidate->activate(manager_.Get(), client_, flags_);
        if (SUCCEEDED(result)) result = candidate->control()->RestoreMode(state.chinese);
        if (FAILED(result)) {
            candidate->deactivate();
            const auto restored = current_->activate(manager_.Get(), client_, flags_);
            if (SUCCEEDED(restored)) current_->control()->RestoreMode(state.chinese);
            else log_error("RUNTIME_ROLLBACK_FAILED", restored);
            failed_version_ = version;
            retire(std::move(candidate));
            log_error("RUNTIME_UPDATE_FAILED", result);
            return result;
        }
        retire(std::move(current_));
        current_ = std::move(candidate);
        failed_version_.clear();
        collect_retired();
        return S_OK;
    } catch (...) {
        return E_FAIL;
    }
}

HRESULT BootstrapService::start_timer() {
    WNDCLASSEXW klass{sizeof(klass)};
    klass.lpfnWndProc = window_proc;
    klass.hInstance = g_module;
    klass.lpszClassName = kLoaderWindow;
    if (!RegisterClassExW(&klass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return HRESULT_FROM_WIN32(GetLastError());
    timer_window_ = CreateWindowExW(0, kLoaderWindow, L"", 0, 0, 0, 0, 0,
                                   HWND_MESSAGE, nullptr, g_module, this);
    if (!timer_window_) return HRESULT_FROM_WIN32(GetLastError());
    return SetTimer(timer_window_, kUpdateTimer, 1000, nullptr) ? S_OK : E_FAIL;
}

LRESULT CALLBACK BootstrapService::window_proc(HWND window, UINT message,
                                              WPARAM wparam, LPARAM lparam) {
    if (message == WM_NCCREATE) {
        const auto* creation = reinterpret_cast<CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(creation->lpCreateParams));
    }
    const auto service = reinterpret_cast<BootstrapService*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_TIMER && wparam == kUpdateTimer && service) {
        const ComPtr<BootstrapService> keep_alive(service);
        service->CheckForUpdate();
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
