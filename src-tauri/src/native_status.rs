pub fn native_registered() -> bool {
    #[cfg(windows)]
    {
        use windows::{
            Win32::{
                System::Com::{
                    CLSCTX_INPROC_SERVER, COINIT_APARTMENTTHREADED, CoCreateInstance,
                    CoInitializeEx, CoUninitialize,
                },
                UI::TextServices::{
                    CLSID_TF_InputProcessorProfiles, ITfInputProcessorProfiles,
                    ITfTextInputProcessorEx,
                },
            },
            core::GUID,
        };
        const CLASS: GUID = GUID::from_u128(0x49a6e853_66a7_45ac_981f_ecb7f74f50cf);
        const PROFILE: GUID = GUID::from_u128(0x2079bc15_856c_4cbb_9b2e_5472c9903242);
        // A pre-initialized COM apartment remains usable even if its model differs.
        unsafe {
            let initialized = CoInitializeEx(None, COINIT_APARTMENTTHREADED).is_ok();
            let enabled = {
                let profiles: windows::core::Result<ITfInputProcessorProfiles> =
                    CoCreateInstance(&CLSID_TF_InputProcessorProfiles, None, CLSCTX_INPROC_SERVER);
                profiles.is_ok_and(|profiles| {
                    profiles
                        .IsEnabledLanguageProfile(&CLASS, 0x0804, &PROFILE)
                        .is_ok_and(|enabled| enabled.as_bool())
                }) && CoCreateInstance::<_, ITfTextInputProcessorEx>(
                    &CLASS,
                    None,
                    CLSCTX_INPROC_SERVER,
                )
                .is_ok()
            };
            if initialized {
                CoUninitialize();
            }
            enabled
        }
    }
    #[cfg(not(windows))]
    {
        false
    }
}
