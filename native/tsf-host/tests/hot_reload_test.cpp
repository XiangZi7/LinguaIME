#include "bootstrap_service.h"
#include "tsf_test_environment.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

HINSTANCE g_module = GetModuleHandleW(nullptr);
std::atomic<long> g_objects{0};

namespace {
void require(HRESULT result, const char* step) {
    if (FAILED(result)) {
        std::cerr << step << ": 0x" << std::hex << static_cast<unsigned long>(result) << '\n';
        throw std::runtime_error(step);
    }
}
void expect(bool condition, const char* step) {
    if (!condition) throw std::runtime_error(step);
}
std::wstring version(wchar_t value) {
    return std::wstring(64, value) + L"-" + std::wstring(64, value);
}
void publish(const std::filesystem::path& root, const std::wstring& id, bool previous = false) {
    const auto path = root / (previous ? L"previous-runtime.txt" : L"active-runtime.txt");
    const auto temporary = root / L"manifest.tmp";
    {
        std::ofstream file(temporary, std::ios::binary);
        file << "1\n";
        for (const wchar_t character : id) file.put(static_cast<char>(character));
        file << '\n';
    }
    expect(MoveFileExW(temporary.c_str(), path.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE, "manifest publish");
}
std::filesystem::path stage(const std::filesystem::path& root, wchar_t id, const wchar_t* source) {
    const auto directory = root / L"runtime" / version(id);
    std::filesystem::create_directories(directory);
    const auto path = directory / L"lingua_runtime.dll";
    std::filesystem::copy_file(source, path);
    return path;
}
void expect_version(BootstrapService* service, wchar_t id) {
    BSTR text = nullptr;
    require(service->GetRuntimeVersion(&text), "runtime version");
    const bool matches = text && version(id) == text;
    SysFreeString(text);
    expect(matches, "runtime generation mismatch");
}
void pump(unsigned milliseconds) {
    const auto deadline = GetTickCount64() + milliseconds;
    while (GetTickCount64() < deadline) {
        MsgWaitForMultipleObjectsEx(0, nullptr, 25, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
}
using Setter = void(WINAPI*)(BOOL);
Setter setter(HMODULE module, const char* name) {
    const auto result = reinterpret_cast<Setter>(GetProcAddress(module, name));
    expect(result != nullptr, "test export missing");
    return result;
}
}

int main() {
    try {
        require(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED), "COM");
        const auto root = std::filesystem::absolute(
            L"update-fixture-" + std::to_wstring(GetCurrentProcessId()));
        std::filesystem::create_directories(root);
        const auto path_a = stage(root, L'A', L"mock-runtime-a.dll");
        stage(root, L'B', L"mock-runtime-b.dll");
        const auto path_c = stage(root, L'C', L"mock-runtime-b.dll");
        const auto path_d = stage(root, L'D', L"mock-runtime-b.dll");
        publish(root, version(L'A'));
        ComPtr<ITfThreadMgr> manager;
        require(CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER,
                                 IID_PPV_ARGS(&manager)), "thread manager");
        TfClientId client = TF_CLIENTID_NULL;
        require(activate_isolated_tsf(manager.Get(), &client), "isolated thread activation");
        ComPtr<BootstrapService> service;
        service.Attach(new BootstrapService(root));
        require(service->ActivateEx(manager.Get(), client, 0), "bootstrap activation");
        expect_version(service.Get(), L'A');
        ComPtr<ILinguaRuntimeControl> old = service->runtime_for_test();
        require(old->RestoreMode(FALSE), "English mode");
        const auto module_a = GetModuleHandleW(path_a.c_str());
        expect(module_a != nullptr, "initial module missing");
        const auto idle = setter(module_a, "SetMockIdle");
        idle(FALSE);
        publish(root, version(L'B'));
        expect(service->CheckForUpdate() == S_FALSE, "update interrupted composition");
        pump(1200);
        expect_version(service.Get(), L'A');
        idle(TRUE);
        pump(1200);
        expect_version(service.Get(), L'B');
        RuntimeState state{};
        require(service->runtime_for_test()->GetState(&state), "updated state");
        expect(!state.chinese, "update lost English mode");
        expect(GetModuleHandleW(path_a.c_str()) != nullptr, "retained COM item lost its module");
        old.Reset();
        service->CheckForUpdate();
        expect(GetModuleHandleW(path_a.c_str()) == nullptr, "retired module was not unloaded");

        std::cout << "Testing expected rejections: activation failure, ABI mismatch and missing runtime."
                  << std::endl;
        const auto module_c = LoadLibraryW(path_c.c_str());
        expect(module_c != nullptr, "failure fixture load");
        setter(module_c, "SetMockActivationFailure")(TRUE);
        publish(root, version(L'C'));
        expect(FAILED(service->CheckForUpdate()), "bad activation accepted");
        expect_version(service.Get(), L'B');
        require(service->runtime_for_test()->GetState(&state), "rollback state");
        expect(!state.chinese, "rollback lost English mode");
        FreeLibrary(module_c);

        const auto module_d = LoadLibraryW(path_d.c_str());
        expect(module_d != nullptr, "ABI fixture load");
        setter(module_d, "SetMockWrongAbi")(TRUE);
        publish(root, version(L'D'));
        expect(FAILED(service->CheckForUpdate()), "incompatible ABI accepted");
        expect_version(service.Get(), L'B');
        require(service->runtime_for_test()->RestoreMode(TRUE), "old runtime still active");
        FreeLibrary(module_d);

        publish(root, L"../outside");
        expect(read_runtime_version(root).empty(), "manifest traversal accepted");
        publish(root, version(L'E'));
        expect(FAILED(service->CheckForUpdate()), "missing module accepted");
        expect_version(service.Get(), L'B');
        require(service->Deactivate(), "bootstrap deactivation");
        publish(root, version(L'B'), true);
        require(service->ActivateEx(manager.Get(), client, 0), "startup fallback");
        expect_version(service.Get(), L'B');
        publish(root, version(L'A'));
        pump(1200);
        expect_version(service.Get(), L'A');
        require(service->Deactivate(), "final deactivation");
        manager->Deactivate();
        std::cout << "Live runtime update: timer promotion, busy deferral, mode preservation, COM lifetime, activation rollback, ABI rejection, invalid manifest and startup fallback passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
