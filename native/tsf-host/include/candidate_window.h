#pragma once
#include "common.h"
#include "lingua_ffi.h"
#include <functional>
#include <d2d1.h>
#include <dwrite.h>
#include <vector>

class CandidateWindow final {
public:
    ~CandidateWindow();
    bool show(const std::string& composition, const LinguaPage& page, unsigned selected,
              const LinguaConfig& config, RECT caret);
    void hide() noexcept;
    bool save_preview(const wchar_t* path) const;
    std::function<void(unsigned)> on_select;
private:
    static LRESULT CALLBACK window_proc(HWND, UINT, WPARAM, LPARAM);
    void paint();
    bool draw(HDC dc, const RECT& bounds);
    HWND window_ = nullptr;
    LinguaPage page_{};
    LinguaConfig config_{};
    std::wstring composition_;
    unsigned selected_ = 0;
    int scale_ = 96;
    int row_height_ = 36;
    ComPtr<ID2D1Factory> drawing_factory_;
    ComPtr<ID2D1DCRenderTarget> target_;
    ComPtr<IDWriteFactory> text_factory_;
    std::vector<unsigned char> pixels_;
    int bitmap_width_ = 0;
    int bitmap_height_ = 0;
    int px(int value) const noexcept { return MulDiv(value, scale_, 96); }
};
