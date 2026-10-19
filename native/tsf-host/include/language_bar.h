#pragma once
#include "common.h"
#include <ctfutb.h>
#include <ctffunc.h>
#include <olectl.h>
#include <functional>

// Windows owns the presentation; this item supplies the mode, icon and click action.
class LanguageBarButton final : public ITfLangBarItemButton, public ITfSource {
public:
    explicit LanguageBarButton(std::function<HRESULT()> toggle);
    ~LanguageBarButton();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE GetInfo(TF_LANGBARITEMINFO*) override;
    HRESULT STDMETHODCALLTYPE GetStatus(DWORD*) override;
    HRESULT STDMETHODCALLTYPE Show(BOOL) override;
    HRESULT STDMETHODCALLTYPE GetTooltipString(BSTR*) override;
    HRESULT STDMETHODCALLTYPE OnClick(TfLBIClick, POINT, const RECT*) override;
    HRESULT STDMETHODCALLTYPE InitMenu(ITfMenu*) override;
    HRESULT STDMETHODCALLTYPE OnMenuSelect(UINT) override;
    HRESULT STDMETHODCALLTYPE GetIcon(HICON*) override;
    HRESULT STDMETHODCALLTYPE GetText(BSTR*) override;
    HRESULT STDMETHODCALLTYPE AdviseSink(REFIID, IUnknown*, DWORD*) override;
    HRESULT STDMETHODCALLTYPE UnadviseSink(DWORD) override;
    void set_mode(bool chinese);
    void detach();

private:
    void notify(DWORD flags);
    std::atomic<ULONG> references_{1};
    TF_LANGBARITEMINFO info_{};
    ComPtr<ITfLangBarItemSink> sink_;
    std::function<HRESULT()> toggle_;
    DWORD status_ = 0;
    DWORD cookie_ = 0;
    bool chinese_ = true;
};
