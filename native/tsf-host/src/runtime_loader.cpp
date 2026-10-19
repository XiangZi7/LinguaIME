#include "runtime_loader.h"
#include <algorithm>
#include <utility>

namespace {
bool valid_version(const std::string& version) {
    if (version.size() != 129 || version[64] != '-') return false;
    for (size_t i = 0; i < version.size(); ++i) {
        if (i == 64) continue;
        const char value = version[i];
        if (!(value >= '0' && value <= '9') && !(value >= 'A' && value <= 'F')) return false;
    }
    return true;
}
}

std::filesystem::path installed_runtime_root() {
    wchar_t path[32768]{};
    const auto size = GetModuleFileNameW(g_module, path, ARRAYSIZE(path));
    if (!size || size >= ARRAYSIZE(path)) return {};
    // native/host-v1-<hash>/lingua_tsf.dll -> native/
    return std::filesystem::path(path).parent_path().parent_path();
}

std::wstring read_runtime_version(const std::filesystem::path& root, bool previous) {
    const auto path = root / (previous ? L"previous-runtime.txt" : L"active-runtime.txt");
    const auto file = CreateFileW(path.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return {};
    char buffer[256]{};
    DWORD count = 0;
    const bool read = ReadFile(file, buffer, sizeof(buffer), &count, nullptr) != FALSE;
    CloseHandle(file);
    if (!read || count == sizeof(buffer)) return {};
    std::string record(buffer, count);
    record.erase(std::remove(record.begin(), record.end(), '\r'), record.end());
    if (!record.starts_with("1\n")) return {};
    auto version = record.substr(2);
    if (!version.empty() && version.back() == '\n') version.pop_back();
    return valid_version(version) ? wide(version.c_str()) : std::wstring{};
}

RuntimeGeneration::RuntimeGeneration(std::wstring version) : version_(std::move(version)) {}

RuntimeGeneration::~RuntimeGeneration() {
    retire();
    if (try_unload() || !module_) return;
    // A client can retain a removed language-bar COM item. Keep its code valid until process exit.
    HMODULE pinned = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                      reinterpret_cast<LPCWSTR>(can_unload_), &pinned);
    FreeLibrary(module_);
}

HRESULT RuntimeGeneration::load(const std::filesystem::path& root) {
    const auto directory = root / L"runtime" / version_;
    const auto path = directory / L"lingua_runtime.dll";
    if (module_) return E_UNEXPECTED;
    module_ = LoadLibraryExW(path.c_str(), nullptr,
                            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!module_) return HRESULT_FROM_WIN32(GetLastError());
    using AbiVersion = uint32_t(WINAPI*)();
    using GetFactory = HRESULT(WINAPI*)(REFCLSID, REFIID, void**);
    const auto abi = reinterpret_cast<AbiVersion>(GetProcAddress(module_, "LinguaRuntimeAbiVersion"));
    const auto factory = reinterpret_cast<GetFactory>(GetProcAddress(module_, "DllGetClassObject"));
    can_unload_ = reinterpret_cast<CanUnload>(GetProcAddress(module_, "DllCanUnloadNow"));
    prepare_unload_ = reinterpret_cast<CanUnload>(
        GetProcAddress(module_, "LinguaRuntimePrepareUnload"));
    if (!abi || !factory || !can_unload_ || !prepare_unload_ || abi() != kRuntimeAbi)
        return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
    ComPtr<IClassFactory> object_factory;
    auto result = factory(kClsid, IID_PPV_ARGS(&object_factory));
    if (SUCCEEDED(result)) {
        result = object_factory->CreateInstance(nullptr, IID_PPV_ARGS(&service_));
        instantiated_ = SUCCEEDED(result);
    }
    if (SUCCEEDED(result)) result = service_.As(&control_);
    return result;
}

HRESULT RuntimeGeneration::activate(ITfThreadMgr* manager, TfClientId client, DWORD flags) {
    if (active_) return S_OK;
    const auto result = service_->ActivateEx(manager, client, flags);
    active_ = SUCCEEDED(result);
    if (!active_) service_->Deactivate();
    return result;
}

HRESULT RuntimeGeneration::deactivate() {
    if (!active_) return S_OK;
    const auto result = service_->Deactivate();
    if (SUCCEEDED(result)) active_ = false;
    return result;
}

void RuntimeGeneration::retire() {
    deactivate();
    control_.Reset();
    service_.Reset();
}

bool RuntimeGeneration::try_unload() {
    if (!module_) return true;
    if (service_ || control_ || active_) return false;
    if (instantiated_ && (can_unload_() != S_OK || prepare_unload_() != S_OK)) return false;
    FreeLibrary(module_);
    module_ = nullptr;
    return true;
}
