fn main() {
    let runtime_id = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../build/native/rime/runtime-id.txt");
    println!("cargo:rerun-if-changed={}", runtime_id.display());
    let version = std::fs::read_to_string(runtime_id).unwrap_or_else(|_| "unprepared".into());
    println!("cargo:rustc-env=LINGUAIME_RIME_BUNDLE={}", version.trim());
    println!("cargo:rerun-if-changed=native/rime_bridge.cpp");
    println!("cargo:rerun-if-changed=native/rime_api.h");
    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("windows") {
        cc::Build::new()
            .cpp(true)
            .std("c++17")
            .flag_if_supported("/utf-8")
            .file("native/rime_bridge.cpp")
            .compile("lingua_rime_bridge");
    }
}
