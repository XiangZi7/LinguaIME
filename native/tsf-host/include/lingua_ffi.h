#pragma once
#include <cstdint>

struct EngineContext;
struct LinguaCandidate {
    char id[256];
    char text[256];
    char reading[256];
    char translation[768];
};
struct LinguaPage {
    uint32_t schema_version, item_count, page_index, page_count;
    LinguaCandidate items[9];
};
struct LinguaConfig {
    uint32_t page_size, display_mode, theme, accent, font_size, compact;
    uint32_t chinese_punctuation, enter_raw, previous_key, next_key;
};
static_assert(sizeof(LinguaCandidate) == 1536);
static_assert(sizeof(LinguaPage) == 13840);
static_assert(sizeof(LinguaConfig) == 40);

extern "C" {
__declspec(dllimport) uint32_t lingua_abi_version();
__declspec(dllimport) void lingua_release_cache();
__declspec(dllimport) EngineContext* lingua_create();
__declspec(dllimport) void lingua_destroy(EngineContext*);
__declspec(dllimport) int32_t lingua_refresh(EngineContext*, LinguaConfig*);
__declspec(dllimport) int32_t lingua_query(EngineContext*, const char*, uint32_t, LinguaPage*);
__declspec(dllimport) int32_t lingua_record(EngineContext*, const char*);
}
