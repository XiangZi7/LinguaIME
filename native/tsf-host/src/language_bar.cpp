#include "language_bar.h"
#include "runtime_contract.h"
#include <algorithm>
#include <utility>

namespace {
HICON make_mode_icon(const wchar_t* text) {
    const int size = std::clamp(GetSystemMetrics(SM_CXSMICON), 16, 48);
    HDC screen = GetDC(nullptr);
    HDC dc = screen ? CreateCompatibleDC(screen) : nullptr;
    HBITMAP mask = CreateBitmap(size, size * 2, 1, 1, nullptr);
    HFONT font = CreateFontW(-size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
    HICON icon = nullptr;
    if (dc && mask && font) {
        const auto previous_bitmap = SelectObject(dc, mask);
        const auto previous_font = SelectObject(dc, font);
        // Monochrome icon: transparent background, black glyph. Windows recolors it for its theme.
        PatBlt(dc, 0, 0, size, size, WHITENESS);
        PatBlt(dc, 0, size, size, size, BLACKNESS);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(0, 0, 0));
        RECT bounds{0, 0, size, size};
        DrawTextW(dc, text, 1, &bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(dc, previous_font);
        SelectObject(dc, previous_bitmap);
        ICONINFO info{};
        info.fIcon = TRUE;
        info.hbmMask = mask;
        icon = CreateIconIndirect(&info);
    }
    if (font) DeleteObject(font);
    if (mask) DeleteObject(mask);
    if (dc) DeleteDC(dc);
    if (screen) ReleaseDC(nullptr, screen);
    return icon;
}
}

LanguageBarButton::LanguageBarButton(std::function<HRESULT()> toggle)
    : toggle_(std::move(toggle)) {
    ++g_objects;
    info_.clsidService = kClsid;
    info_.guidItem = GUID_LBI_INPUTMODE;
    info_.dwStyle = TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_SHOWNINTRAY |
                    TF_LBI_STYLE_TEXTCOLORICON;
    wcscpy_s(info_.szDescription, L"LinguaIME 中英文切换");
}

LanguageBarButton::~LanguageBarButton() {
    --g_objects;
}

HRESULT LanguageBarButton::QueryInterface(REFIID id, void** output) {
    if (!output) return E_POINTER;
    *output = nullptr;
    if (id == IID_IUnknown || id == IID_ITfLangBarItem || id == IID_ITfLangBarItemButton)
        *output = static_cast<ITfLangBarItemButton*>(this);
    else if (id == IID_ITfSource)
        *output = static_cast<ITfSource*>(this);
    else
        return E_NOINTERFACE;
    AddRef();
    return S_OK;
}

ULONG LanguageBarButton::AddRef() {
    return ++references_;
}

ULONG LanguageBarButton::Release() {
    const auto count = --references_;
    if (!count) delete this;
    return count;
}

HRESULT LanguageBarButton::GetInfo(TF_LANGBARITEMINFO* info) {
    if (!info) return E_POINTER;
    *info = info_;
    return S_OK;
}

HRESULT LanguageBarButton::GetStatus(DWORD* status) {
    if (!status) return E_POINTER;
    *status = status_;
    return S_OK;
}

HRESULT LanguageBarButton::Show(BOOL show) {
    // Visibility is controlled by Windows' input indicator and the active profile.
    static_cast<void>(show);
    notify(TF_LBI_STATUS);
    return S_OK;
}

HRESULT LanguageBarButton::GetTooltipString(BSTR* text) {
    if (!text) return E_POINTER;
    *text = SysAllocString(chinese_ ? L"LinguaIME · 中文模式，单击切换英文"
                                  : L"LinguaIME · 英文模式，单击切换中文");
    return *text ? S_OK : E_OUTOFMEMORY;
}

HRESULT LanguageBarButton::OnClick(TfLBIClick click, POINT, const RECT*) {
    RuntimeCall call;
    if (click != TF_LBI_CLK_LEFT || (status_ & TF_LBI_STATUS_DISABLED)) return S_OK;
    try {
        const auto toggle = toggle_;
        return toggle ? toggle() : S_FALSE;
    } catch (...) {
        return E_FAIL;
    }
}

HRESULT LanguageBarButton::InitMenu(ITfMenu*) {
    return S_OK;
}

HRESULT LanguageBarButton::OnMenuSelect(UINT) {
    return S_OK;
}

HRESULT LanguageBarButton::GetIcon(HICON* icon) {
    if (!icon) return E_POINTER;
    // The language bar takes ownership of the returned icon.
    *icon = make_mode_icon(chinese_ ? L"中" : L"英");
    return *icon ? S_OK : E_FAIL;
}

HRESULT LanguageBarButton::GetText(BSTR* text) {
    if (!text) return E_POINTER;
    *text = SysAllocString(chinese_ ? L"中" : L"英");
    return *text ? S_OK : E_OUTOFMEMORY;
}

HRESULT LanguageBarButton::AdviseSink(REFIID id, IUnknown* object, DWORD* cookie) {
    if (!cookie) return E_POINTER;
    *cookie = TF_INVALID_COOKIE;
    if (id != IID_ITfLangBarItemSink) return CONNECT_E_CANNOTCONNECT;
    if (!object) return E_INVALIDARG;
    if (sink_) return CONNECT_E_ADVISELIMIT;
    const auto result = object->QueryInterface(IID_PPV_ARGS(&sink_));
    if (FAILED(result)) return result;
    if (++cookie_ == TF_INVALID_COOKIE) ++cookie_;
    *cookie = cookie_;
    return S_OK;
}

HRESULT LanguageBarButton::UnadviseSink(DWORD cookie) {
    if (!sink_ || cookie != cookie_) return CONNECT_E_NOCONNECTION;
    sink_.Reset();
    return S_OK;
}

void LanguageBarButton::notify(DWORD flags) {
    // Keep the sink alive if a notification causes Windows to unadvise it synchronously.
    const auto sink = sink_;
    if (sink) sink->OnUpdate(flags);
}

void LanguageBarButton::set_mode(bool chinese) {
    if (chinese_ == chinese) return;
    chinese_ = chinese;
    notify(TF_LBI_BTNALL);
}

void LanguageBarButton::detach() {
    toggle_ = {};
    status_ |= TF_LBI_STATUS_DISABLED;
    notify(TF_LBI_STATUS);
    sink_.Reset();
}
