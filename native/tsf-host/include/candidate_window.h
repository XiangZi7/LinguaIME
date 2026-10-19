#pragma once
#include "common.h"
#include "lingua_ffi.h"
#include <functional>

class CandidateWindow final {
public:
    ~CandidateWindow();
    bool show(const std::string& composition, const LinguaPage& page, unsigned selected,
              const LinguaConfig& config, RECT caret);
    void hide() noexcept;
    std::function<void(unsigned)> on_select;
private:
    static LRESULT CALLBACK window_proc(HWND, UINT, WPARAM, LPARAM);
    void paint();
    HWND window_ = nullptr;
    LinguaPage page_{};
    LinguaConfig config_{};
    std::wstring composition_;
    unsigned selected_ = 0;
    int scale_ = 96;
    int row_height_ = 36;
    int px(int value) const noexcept { return MulDiv(value, scale_, 96); }
};
