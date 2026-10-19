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
        window_ = CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
                                  kWindowClass, L"LinguaIME candidates", WS_POPUP,
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
    row_height_ = config.compact ? 32 : 36;
    const int width = px(368);
    const int detail_height=config.display_mode==2 && page.item_count?54:0;
    const int height = px(18 + 42 + static_cast<int>(page.item_count) * row_height_ + detail_height + 34);
    int x = std::clamp(caret.left, monitor.rcWork.left, std::max(monitor.rcWork.left, monitor.rcWork.right - width));
    int y = caret.bottom + px(4);
    if (y + height > monitor.rcWork.bottom) y = caret.top - height - px(4);
    y = std::clamp(y, static_cast<int>(monitor.rcWork.top), static_cast<int>(std::max(monitor.rcWork.top, monitor.rcWork.bottom - height)));
    SetWindowPos(window_, HWND_TOPMOST, x-px(9), y, width, height, SWP_NOACTIVATE);
    paint();
    ShowWindow(window_, SW_SHOWNOACTIVATE);
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
        if (message == WM_PAINT) {
            PAINTSTRUCT paint{};
            BeginPaint(window,&paint);
            EndPaint(window,&paint);
            self->paint(); return 0;
        }
        if (message == WM_LBUTTONUP) {
            const int y = MulDiv(static_cast<short>(HIWORD(lparam)), 96, self->scale_) - 51;
            if (y >= 0) {
                for (unsigned index=0;index<self->page_.item_count;++index) {
                    const int top=static_cast<int>(index)*self->row_height_+((self->config_.display_mode==2 && index>self->selected_)?54:0);
                    if (y>=top && y<top+self->row_height_ && self->on_select) { self->on_select(index); break; }
                }
            }
            return 0;
        }
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
