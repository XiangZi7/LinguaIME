#pragma once
#include <cstdint>

namespace lingua {
inline wchar_t punctuation(uint32_t key, bool shift) noexcept {
    switch (key) {
    case 0xBC: return shift ? L'《' : L'，';
    case 0xBE: return shift ? L'》' : L'。';
    case 0xBA: return shift ? L'：' : L'；';
    case 0xBF: return shift ? L'？' : L'、';
    case '1': return shift ? L'！' : 0;
    default: return 0;
    }
}
// Pure key policy can be tested without registering an input method.
inline bool handles_key(uint32_t key, bool composing, bool modified,
                        uint32_t previous = 0xBD, uint32_t next = 0xBB) noexcept {
    if (modified) return false;
    if (key >= 'A' && key <= 'Z') return true;
    if (!composing) return false;
    return (key >= '1' && key <= '9') || key == 0x08 || key == 0x0D || key == 0x20 ||
           key == 0x1B || key == 0x26 || key == 0x28 || key == previous || key == next || key == 0xDE;
}
}
