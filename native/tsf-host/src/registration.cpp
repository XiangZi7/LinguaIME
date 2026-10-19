#include "common.h"
#include "diagnostics.h"
#include <iterator>

namespace {
constexpr wchar_t kRegistry[] = L"Software\\Classes\\CLSID\\{49A6E853-66A7-45AC-981F-ECB7F74F50CF}";
HRESULT set_value(const std::wstring& path, const wchar_t* name, const wchar_t* value) {
    HKEY key=nullptr;
    const auto opened=RegCreateKeyExW(HKEY_CURRENT_USER,path.c_str(),0,nullptr,0,KEY_WRITE,nullptr,&key,nullptr);
    if (opened!=ERROR_SUCCESS) return HRESULT_FROM_WIN32(opened);
    const auto result=RegSetValueExW(key,name,0,REG_SZ,reinterpret_cast<const BYTE*>(value),static_cast<DWORD>((wcslen(value)+1)*sizeof(wchar_t)));
    RegCloseKey(key); return HRESULT_FROM_WIN32(result);
}
struct ComScope {
    HRESULT result=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    ~ComScope() { if (SUCCEEDED(result)) CoUninitialize(); }
};
}
STDAPI DllUnregisterServer() {
    ComScope com;
    ComPtr<ITfInputProcessorProfiles> profiles;
    HRESULT result=CoCreateInstance(CLSID_TF_InputProcessorProfiles,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&profiles));
    if (SUCCEEDED(result)) { profiles->RemoveLanguageProfile(kClsid,kLanguage,kProfile); result=profiles->Unregister(kClsid); }
    ComPtr<ITfCategoryMgr> category;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&category)))) {
        for (const GUID* id : {&GUID_TFCAT_TIP_KEYBOARD, &GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
                              &GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT})
            category->UnregisterCategory(kClsid,*id,kClsid);
    }
    const auto removed=RegDeleteTreeW(HKEY_CURRENT_USER,kRegistry);
    if (removed!=ERROR_SUCCESS && removed!=ERROR_FILE_NOT_FOUND) return HRESULT_FROM_WIN32(removed);
    return result;
}
STDAPI DllRegisterServer() {
    try {
        wchar_t module_path[MAX_PATH]{};
        const DWORD length=GetModuleFileNameW(g_module,module_path,MAX_PATH);
        if (length==0 || length>=MAX_PATH) return E_FAIL;
        auto result=set_value(kRegistry,nullptr,L"LinguaIME Text Service");
        const auto inproc=std::wstring(kRegistry)+L"\\InprocServer32";
        if (SUCCEEDED(result)) result=set_value(inproc,nullptr,module_path);
        if (SUCCEEDED(result)) result=set_value(inproc,L"ThreadingModel",L"Apartment");
        if (FAILED(result)) { RegDeleteTreeW(HKEY_CURRENT_USER,kRegistry); return result; }
        ComScope com;
        ComPtr<ITfInputProcessorProfiles> profiles;
        result=CoCreateInstance(CLSID_TF_InputProcessorProfiles,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&profiles));
        if (SUCCEEDED(result)) result=profiles->Register(kClsid);
        if (FAILED(result)) log_error("PROFILE_REGISTER_FAILED",result);
        constexpr wchar_t description[]=L"LinguaIME · 中英双语";
        if (SUCCEEDED(result)) result=profiles->AddLanguageProfile(kClsid,kLanguage,kProfile,description,
            static_cast<ULONG>(std::size(description)-1),module_path,length,0);
        if (FAILED(result)) log_error("LANGUAGE_PROFILE_FAILED",result);
        if (SUCCEEDED(result)) result=profiles->EnableLanguageProfile(kClsid,kLanguage,kProfile,TRUE);
        ComPtr<ITfCategoryMgr> category;
        if (SUCCEEDED(result)) result=CoCreateInstance(CLSID_TF_CategoryMgr,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&category));
        for (const GUID* id : {&GUID_TFCAT_TIP_KEYBOARD, &GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
                              &GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT}) {
            if (SUCCEEDED(result)) result=category->RegisterCategory(kClsid,*id,kClsid);
        }
        if (FAILED(result)) log_error("CATEGORY_REGISTER_FAILED",result);
        if (FAILED(result)) DllUnregisterServer();
        return result;
    } catch (...) { return E_FAIL; }
}
