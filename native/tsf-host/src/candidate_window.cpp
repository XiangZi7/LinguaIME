#include "candidate_window.h"
#include <algorithm>
#include <dwmapi.h>

namespace {
constexpr wchar_t kWindowClass[] = L"LinguaIME.Candidate.v1";
bool system_dark() {
    DWORD light = 1, size = sizeof(light);
    RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                 L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size);
    return light == 0;
}
void draw_text(HDC dc, const std::wstring& text, RECT rect, COLORREF color, UINT flags = DT_LEFT) {
    SetTextColor(dc, color);
    DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &rect,
              flags | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
}
}

CandidateWindow::~CandidateWindow() { if (window_) DestroyWindow(window_); }
void CandidateWindow::hide() noexcept { if (window_) ShowWindow(window_, SW_HIDE); }

bool CandidateWindow::show(const std::string& composition, const LinguaPage& page, unsigned selected,
                           const LinguaConfig& config, RECT caret) {
    if (!window_) {
        WNDCLASSEXW klass{sizeof(klass)};
        klass.lpfnWndProc = window_proc;
        klass.hInstance = g_module;
        klass.lpszClassName = kWindowClass;
        klass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClassExW(&klass);
        window_ = CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                                  kWindowClass, L"LinguaIME candidates", WS_POPUP | WS_BORDER,
                                  0, 0, 0, 0, nullptr, nullptr, g_module, this);
        if (!window_) return false;
        const DWORD corner = 2;
        DwmSetWindowAttribute(window_, 33, &corner, sizeof(corner));
    }
    composition_ = wide(composition.c_str());
    page_ = page;
    config_ = config;
    if (config_.theme == 0) config_.theme = system_dark() ? 2 : 1;
    selected_ = selected;
    const POINT point{caret.left, caret.bottom};
    MONITORINFO monitor{sizeof(monitor)};
    GetMonitorInfoW(MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST), &monitor);
    // The context view supplies physical screen coordinates. Query that monitor's DPI.
    using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
    HMODULE shcore = LoadLibraryW(L"shcore.dll");
    UINT dpi_x = 96, dpi_y = 96;
    if (shcore) {
        const auto get_dpi = reinterpret_cast<GetDpiForMonitorFn>(GetProcAddress(shcore, "GetDpiForMonitor"));
        if (get_dpi) get_dpi(MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST), 0, &dpi_x, &dpi_y);
        FreeLibrary(shcore);
    }
    scale_ = static_cast<int>(dpi_x);
    row_height_ = config.compact ? 32 : 38;
    if (config.display_mode == 2) row_height_ += 20;
    const int width = px(350);
    const int height = px(38 + static_cast<int>(page.item_count) * row_height_ + 28);
    int x = std::clamp(caret.left, monitor.rcWork.left, std::max(monitor.rcWork.left, monitor.rcWork.right - width));
    int y = caret.bottom + px(4);
    if (y + height > monitor.rcWork.bottom) y = caret.top - height - px(4);
    y = std::clamp(y, static_cast<int>(monitor.rcWork.top), static_cast<int>(std::max(monitor.rcWork.top, monitor.rcWork.bottom - height)));
    SetWindowPos(window_, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(window_, nullptr, FALSE);
    return true;
}

LRESULT CALLBACK CandidateWindow::window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<CandidateWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        self = static_cast<CandidateWindow*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (self) {
        if (message == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
        if (message == WM_ERASEBKGND) return 1;
        if (message == WM_PAINT) { self->paint(); return 0; }
        if (message == WM_LBUTTONUP) {
            const int y = MulDiv(static_cast<short>(HIWORD(lparam)), 96, self->scale_) - 38;
            if (y >= 0) {
                const auto index = static_cast<unsigned>(y / self->row_height_);
                if (index < self->page_.item_count && self->on_select) self->on_select(index);
            }
            return 0;
        }
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

void CandidateWindow::paint() {
    PAINTSTRUCT ps{};
    const HDC screen = BeginPaint(window_, &ps);
    RECT bounds{};
    GetClientRect(window_, &bounds);
    const HDC dc = CreateCompatibleDC(screen);
    const HBITMAP bitmap = CreateCompatibleBitmap(screen, bounds.right, bounds.bottom);
    const auto old_bitmap = SelectObject(dc, bitmap);
    const bool dark = config_.theme == 2;
    const COLORREF surface = dark ? RGB(32,33,36) : RGB(255,255,255);
    const COLORREF text = dark ? RGB(243,243,243) : RGB(37,37,37);
    const COLORREF muted = dark ? RGB(182,182,182) : RGB(112,112,112);
    const auto background = CreateSolidBrush(surface);
    FillRect(dc, &bounds, background);
    DeleteObject(background);
    SetBkMode(dc, TRANSPARENT);
    const HFONT font = CreateFontW(-px(static_cast<int>(config_.font_size)),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
                                   DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
    const HFONT small_font = CreateFontW(-px(12),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    const auto old_font = SelectObject(dc, small_font);
    draw_text(dc, composition_, {px(14),0,bounds.right-px(14),px(36)}, muted);
    for (unsigned index=0; index<page_.item_count; ++index) {
        const int top = px(38 + static_cast<int>(index)*row_height_);
        const bool selected = index == selected_;
        if (selected) {
            const COLORREF color = config_.accent ? (dark ? RGB(40,57,78) : RGB(221,235,251))
                                                  : (dark ? RGB(57,73,54) : RGB(220,235,200));
            const auto brush=CreateSolidBrush(color);
            RECT row{px(5),top,bounds.right-px(5),top+px(row_height_-2)};
            FillRect(dc,&row,brush);
            DeleteObject(brush);
        }
        const int line_height = config_.display_mode == 2 ? row_height_-20 : row_height_;
        SelectObject(dc,small_font);
        draw_text(dc,std::to_wstring(index+1),{px(13),top,px(32),top+px(line_height)},muted);
        SelectObject(dc,font);
        const int text_right=config_.display_mode == 0 ? 330 : 160;
        draw_text(dc,wide(page_.items[index].text),{px(36),top,px(text_right),top+px(line_height)},text);
        SelectObject(dc,small_font);
        if (config_.display_mode != 0) draw_text(dc,wide(page_.items[index].translation),
            {px(168),top,bounds.right-px(12),top+px(line_height)},dark?RGB(243,161,109):RGB(164,76,28));
        if (config_.display_mode == 2) draw_text(dc,wide(page_.items[index].reading),
            {px(36),top+px(line_height),bounds.right-px(12),top+px(row_height_-3)},muted);
    }
    const int footer=px(38+static_cast<int>(page_.item_count)*row_height_);
    SelectObject(dc,small_font);
    draw_text(dc,L"↑↓ 选择   Enter 确认",{px(12),footer,bounds.right-px(60),bounds.bottom},muted);
    const auto number=std::to_wstring(page_.page_count ? page_.page_index+1 : 0)+L"/"+std::to_wstring(page_.page_count);
    draw_text(dc,number,{bounds.right-px(60),footer,bounds.right-px(12),bounds.bottom},muted,DT_RIGHT);
    BitBlt(screen,0,0,bounds.right,bounds.bottom,dc,0,0,SRCCOPY);
    SelectObject(dc,old_font);
    SelectObject(dc,old_bitmap);
    DeleteObject(font); DeleteObject(small_font); DeleteObject(bitmap); DeleteDC(dc);
    EndPaint(window_,&ps);
}
