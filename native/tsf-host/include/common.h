#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <msctf.h>
#include <wrl/client.h>
#include <atomic>
#include <string>

using Microsoft::WRL::ComPtr;
inline constexpr GUID kClsid = {0x49a6e853, 0x66a7, 0x45ac, {0x98, 0x1f, 0xec, 0xb7, 0xf7, 0x4f, 0x50, 0xcf}};
inline constexpr GUID kProfile = {0x2079bc15, 0x856c, 0x4cbb, {0x9b, 0x2e, 0x54, 0x72, 0xc9, 0x90, 0x32, 0x42}};
inline constexpr LANGID kLanguage = 0x0804;
extern HINSTANCE g_module;
extern std::atomic<long> g_objects;

inline std::wstring wide(const char* value) {
    if (!value || !*value) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, nullptr, 0);
    if (size <= 0) return {};
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, result.data(), size);
    result.pop_back();
    return result;
}
