#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiCandidate {
    pub id: [u8; 256],
    pub text: [u8; 256],
    pub reading: [u8; 256],
    pub translation: [u8; 768],
}

#[repr(C)]
pub struct FfiPage {
    pub schema_version: u32,
    pub item_count: u32,
    pub page_index: u32,
    pub page_count: u32,
    pub items: [FfiCandidate; 9],
}

#[repr(C)]
pub struct FfiConfig {
    pub page_size: u32,
    pub display_mode: u32,
    pub theme: u32,
    pub accent: u32,
    pub font_size: u32,
    pub compact: u32,
    pub chinese_punctuation: u32,
    pub enter_raw: u32,
    pub previous_key: u32,
    pub next_key: u32,
}

pub fn copy_utf8<const N: usize>(text: &str) -> [u8; N] {
    let mut buffer = [0; N];
    let mut length = text.len().min(N.saturating_sub(1));
    while !text.is_char_boundary(length) { length -= 1; }
    buffer[..length].copy_from_slice(&text.as_bytes()[..length]);
    buffer
}
