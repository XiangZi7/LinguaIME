#pragma once
#include "common.h"
#include <cstdio>

inline void log_error(const char* code, HRESULT result) noexcept {
    char message[160]{};
    std::snprintf(message,sizeof(message),"LinguaIME|%s|0x%08lX\n",code,static_cast<unsigned long>(result));
    OutputDebugStringA(message);
    std::fputs(message,stderr);
}
