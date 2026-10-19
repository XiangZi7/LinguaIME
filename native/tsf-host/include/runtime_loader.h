#pragma once
#include "runtime_contract.h"
#include <filesystem>
#include <memory>

// Each generation owns its COM objects and its LoadLibrary reference.
class RuntimeGeneration final {
public:
    explicit RuntimeGeneration(std::wstring version);
    ~RuntimeGeneration();
    RuntimeGeneration(const RuntimeGeneration&) = delete;
    RuntimeGeneration& operator=(const RuntimeGeneration&) = delete;
    HRESULT load(const std::filesystem::path& root);
    HRESULT activate(ITfThreadMgr*, TfClientId, DWORD);
    HRESULT deactivate();
    void retire();
    bool try_unload();
    const std::wstring& version() const { return version_; }
    ILinguaRuntimeControl* control() const { return control_.Get(); }
#ifdef LINGUA_TESTING
    ITfTextInputProcessorEx* service_for_test() const { return service_.Get(); }
#endif

private:
    using CanUnload = HRESULT(WINAPI*)();
    HMODULE module_ = nullptr;
    CanUnload can_unload_ = nullptr;
    CanUnload prepare_unload_ = nullptr;
    ComPtr<ITfTextInputProcessorEx> service_;
    ComPtr<ILinguaRuntimeControl> control_;
    std::wstring version_;
    bool instantiated_ = false;
    bool active_ = false;
};

std::wstring read_runtime_version(const std::filesystem::path& root, bool previous = false);
std::filesystem::path installed_runtime_root();
