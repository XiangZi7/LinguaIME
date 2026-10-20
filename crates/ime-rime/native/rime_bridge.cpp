// Rime C API adapter. The runtime and immutable dictionaries are loaded by absolute path.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "rime_api.h"
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>

namespace {
std::mutex mutex;
HMODULE library = nullptr;
RimeApi* api = nullptr;
unsigned references = 0;

struct Session {
    RimeSessionId id = api->create_session();
    ~Session() { if (id) api->destroy_session(id); }
    bool ready() {
        return id && api->select_schema(id, "lingua_pinyin");
    }
};

struct Context {
    RimeContext value{};
    Context() { RIME_STRUCT_INIT(RimeContext, value); }
    ~Context() { api->free_context(&value); }
};

std::string string(const char* value) { return value ? value : ""; }
}

extern "C" {
struct LinguaRimeCandidate { char text[256]; char reading[256]; };

// Locate the DLL containing this bridge, rather than the application loading the IME.
unsigned lingua_rime_module_path(wchar_t* output, unsigned capacity) {
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&lingua_rime_module_path), &module)) return 0;
    return GetModuleFileNameW(module, output, capacity);
}

int lingua_rime_open(const wchar_t* directory, const char* user_directory) {
    std::lock_guard<std::mutex> lock(mutex);
    if (references) { ++references; return 0; }
    try {
        const auto root = std::filesystem::path(directory);
        library = LoadLibraryExW((root / L"rime.dll").c_str(), nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!library) return -1;
        const auto get_api = reinterpret_cast<RimeApi*(*)()>(GetProcAddress(library, "rime_get_api"));
        api = get_api ? get_api() : nullptr;
        if (!api || !RIME_API_AVAILABLE(api, highlight_candidate)) {
            FreeLibrary(library); library = nullptr; api = nullptr; return -2;
        }
        const auto shared = (root / L"data").u8string();
        const auto prebuilt = (root / L"data" / L"build").u8string();
        RIME_STRUCT(RimeTraits, traits);
        traits.shared_data_dir = shared.c_str();
        traits.prebuilt_data_dir = prebuilt.c_str();
        traits.user_data_dir = user_directory;
        traits.distribution_name = "LinguaIME";
        traits.distribution_code_name = "lingua";
        traits.distribution_version = "0.1.0";
        traits.app_name = "rime.lingua";
        traits.min_log_level = 3;
        traits.log_dir = "";
        api->setup(&traits);
        api->initialize(&traits);
        bool ready = false;
        { Session session; ready = session.ready(); }
        if (!ready) {
            api->finalize(); FreeLibrary(library); library = nullptr; api = nullptr; return -3;
        }
        references = 1;
        return 0;
    } catch (...) { return -4; }
}

void lingua_rime_close() {
    std::lock_guard<std::mutex> lock(mutex);
    if (references && --references == 0) {
        api->finalize();
        FreeLibrary(library);
        api = nullptr; library = nullptr;
    }
}

int lingua_rime_query(const char* input, LinguaRimeCandidate* output, unsigned capacity) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!api || !references) return -1;
    try {
        Session source;
        if (!source.ready()) return -2;
        if (!api->set_input(source.id, input)) return -3;
        unsigned count = 0;
        for (unsigned index = 0; count < capacity && index < 512; ++index) {
            // Highlighting is non-destructive: the commit preview includes any unconsumed
            // raw suffix. It avoids reparsing the input for each possible candidate.
            api->highlight_candidate(source.id, index);
            Context context;
            if (!api->get_context(source.id, &context.value)) return -3;
            const auto& menu = context.value.menu;
            if (menu.num_candidates <= 0 || menu.highlighted_candidate_index < 0 ||
                menu.highlighted_candidate_index >= menu.num_candidates ||
                static_cast<unsigned>(menu.page_no * menu.page_size + menu.highlighted_candidate_index) != index) break;
            const auto& candidate = menu.candidates[menu.highlighted_candidate_index];
            const auto text = string(candidate.text);
            const auto reading = string(candidate.comment);
            if (text.empty() || text.size() >= sizeof(output[0].text)) continue;
            // The host commits the entire composition. Never expose a prefix candidate that
            // would silently discard the remaining keystrokes (e.g. 你 for nihao).
            const bool complete = string(context.value.commit_text_preview) == text;
            if (!complete) continue;
            auto& item = output[count++];
            std::memset(&item, 0, sizeof(item));
            std::memcpy(item.text, text.c_str(), text.size());
            if (reading.size() < sizeof(item.reading)) {
                std::memcpy(item.reading, reading.c_str(), reading.size());
            }
        }
        return static_cast<int>(count);
    } catch (...) { return -4; }
}
}
