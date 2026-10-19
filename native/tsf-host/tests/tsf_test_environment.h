#pragma once
#include "common.h"

// Context tests own an in-memory document, not a foreground input window. Do not load the
// user's selected TIP as a second service alongside the fixture's language-bar item.
inline HRESULT activate_isolated_tsf(ITfThreadMgr* manager, TfClientId* client) {
    if (!manager || !client) return E_POINTER;
    ComPtr<ITfThreadMgrEx> extended;
    auto result = manager->QueryInterface(IID_PPV_ARGS(&extended));
    if (FAILED(result)) return result;
    result = extended->ActivateEx(client,
        TF_TMAE_NOACTIVATETIP | TF_TMAE_NOACTIVATEKEYBOARDLAYOUT);
    if (FAILED(result)) return result;
    DWORD flags = 0;
    result = extended->GetActiveFlags(&flags);
    if (FAILED(result) || !(flags & TF_TMF_NOACTIVATETIP)) {
        extended->Deactivate();
        *client = TF_CLIENTID_NULL;
        return FAILED(result) ? result : E_UNEXPECTED;
    }
    return S_OK;
}
