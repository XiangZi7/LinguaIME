#include "common.h"
#include <ctfutb.h>
#include <ctffunc.h>
#include <iostream>

namespace {
HRESULT verify_mode(ITfThreadMgr* threads, ITfLangBarItemButton* button, bool chinese) {
    BSTR text=nullptr;
    auto result=button->GetText(&text);
    const bool matches=SUCCEEDED(result) && text && wcscmp(text,chinese?L"中":L"英")==0;
    SysFreeString(text);
    if (!matches) return FAILED(result)?result:E_FAIL;
    ComPtr<ITfCompartmentMgr> compartments;
    result=threads->QueryInterface(IID_PPV_ARGS(&compartments));
    if (FAILED(result)) return result;
    for (const GUID* guid : {&GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                             &GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION}) {
        ComPtr<ITfCompartment> compartment;
        result=compartments->GetCompartment(*guid,&compartment);
        VARIANT value{};
        if (SUCCEEDED(result)) result=compartment->GetValue(&value);
        const bool valid=SUCCEEDED(result) && value.vt==VT_I4 &&
            value.lVal==(chinese?1:0);
        VariantClear(&value);
        if (!valid) return FAILED(result)?result:E_FAIL;
    }
    return S_OK;
}

HRESULT probe_language_bar(ITfThreadMgr* threads) {
    ComPtr<ITfLangBarItemMgr> manager;
    auto result=threads->QueryInterface(IID_PPV_ARGS(&manager));
    ComPtr<ITfLangBarItem> item;
    if (SUCCEEDED(result)) result=manager->GetItem(GUID_LBI_INPUTMODE,&item);
    TF_LANGBARITEMINFO info{};
    if (SUCCEEDED(result)) result=item->GetInfo(&info);
    if (SUCCEEDED(result) && (info.clsidService!=kClsid ||
        !(info.dwStyle & TF_LBI_STYLE_SHOWNINTRAY))) result=E_FAIL;
    ComPtr<ITfLangBarItemButton> button;
    if (SUCCEEDED(result)) result=item.As(&button);
    if (SUCCEEDED(result)) result=verify_mode(threads,button.Get(),true);
    if (SUCCEEDED(result)) result=button->OnClick(TF_LBI_CLK_LEFT,POINT{},nullptr);
    if (SUCCEEDED(result)) result=verify_mode(threads,button.Get(),false);
    if (SUCCEEDED(result)) result=button->OnClick(TF_LBI_CLK_LEFT,POINT{},nullptr);
    if (SUCCEEDED(result)) result=verify_mode(threads,button.Get(),true);
    std::cout << "System input-mode item and Chinese/English round trip HRESULT=0x"
              << std::hex << static_cast<unsigned long>(result) << '\n';
    return result;
}
}

int wmain(int count, wchar_t** arguments) {
    if (count>=2 && _wcsicmp(arguments[1],L"probe")==0) {
        CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
        ComPtr<ITfThreadMgr> threads;
        HRESULT result=CoCreateInstance(CLSID_TF_ThreadMgr,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&threads));
        TfClientId client=TF_CLIENTID_NULL;
        if (SUCCEEDED(result)) result=threads->Activate(&client);
        ComPtr<ITfInputProcessorProfiles> profiles;
        if (SUCCEEDED(result)) result=CoCreateInstance(CLSID_TF_InputProcessorProfiles,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&profiles));
        ComPtr<ITfInputProcessorProfileMgr> manager;
        if (SUCCEEDED(result)) result=profiles.As(&manager);
        if (SUCCEEDED(result)) result=profiles->ChangeCurrentLanguage(kLanguage);
        if (SUCCEEDED(result)) result=manager->ActivateProfile(TF_PROFILETYPE_INPUTPROCESSOR,kLanguage,kClsid,kProfile,nullptr,TF_IPPMF_FORPROCESS);
        std::cout << "Process-only TSF activation HRESULT=0x" << std::hex << static_cast<unsigned long>(result) << '\n';
        if (SUCCEEDED(result)) {
            TF_INPUTPROCESSORPROFILE profile{};
            result=manager->GetActiveProfile(GUID_TFCAT_TIP_KEYBOARD,&profile);
            if (SUCCEEDED(result) && profile.clsid!=kClsid) result=E_FAIL;
            if (SUCCEEDED(result) && !(profile.dwCaps & TF_IPP_CAPS_SYSTRAYSUPPORT)) {
                std::cerr << "System tray support category is missing\n";
                result=E_FAIL;
            }
            if (SUCCEEDED(result)) result=probe_language_bar(threads.Get());
        }
        if (threads) threads->Deactivate();
        return SUCCEEDED(result)?0:1;
    }
    if (count>=2 && _wcsicmp(arguments[1],L"status")==0) {
        const HRESULT initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
        ComPtr<ITfInputProcessorProfiles> profiles;
        HRESULT result=CoCreateInstance(CLSID_TF_InputProcessorProfiles,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&profiles));
        BOOL enabled=FALSE;
        if (SUCCEEDED(result)) result=profiles->IsEnabledLanguageProfile(kClsid,kLanguage,kProfile,&enabled);
        std::cout << "profile HRESULT=0x" << std::hex << static_cast<unsigned long>(result) << "; enabled=" << (enabled?"true":"false") << '\n';
        ComPtr<ITfTextInputProcessorEx> service;
        const auto activation=CoCreateInstance(kClsid,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&service));
        std::cout << "COM activation HRESULT=0x" << std::hex << static_cast<unsigned long>(activation) << '\n';
        service.Reset(); profiles.Reset();
        if (SUCCEEDED(initialized)) CoUninitialize();
        return SUCCEEDED(result) && enabled && SUCCEEDED(activation)?0:1;
    }
    if (count<3) { std::cerr << "Usage: profile_tool register|unregister <absolute DLL path>\n"; return 2; }
    const auto module=LoadLibraryExW(arguments[2],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!module) { std::cerr << "DLL_LOAD_FAILED: " << GetLastError() << '\n'; return 3; }
    const char* entry=_wcsicmp(arguments[1],L"unregister")==0?"DllUnregisterServer":"DllRegisterServer";
    const auto action=reinterpret_cast<HRESULT(WINAPI*)()>(GetProcAddress(module,entry));
    if (!action) { std::cerr << "DLL_EXPORT_MISSING\n"; FreeLibrary(module); return 4; }
    const HRESULT result=action();
    std::cout << entry << " HRESULT=0x" << std::hex << static_cast<unsigned long>(result) << '\n';
    FreeLibrary(module);
    return SUCCEEDED(result)?0:5;
}
