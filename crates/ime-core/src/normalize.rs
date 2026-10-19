use ime_types::{AppError, AppResult};

pub fn normalize(input: &str) -> AppResult<String> {
    if input.len() > 128 {
        return Err(AppError::new("COMPOSITION_TOO_LONG", "组合串不能超过 128 字节"));
    }
    let key: String = input.trim().to_lowercase().replace("u:", "v").replace('ü', "v")
        .chars().filter(|c| !matches!(c, '\'' | ' ' | '1'..='5')).collect();
    if !key.bytes().all(|c| c.is_ascii_lowercase()) {
        return Err(AppError::new("INVALID_PINYIN", "请使用拼音字母、空格或单引号"));
    }
    Ok(key)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn normalizes_pinyin_without_accepting_symbols() {
        assert_eq!(normalize(" NÜ3 ' peng2 you3 ").expect("pinyin"), "nvpengyou");
        assert_eq!(normalize("lu:4").expect("pinyin"), "lv");
        assert!(normalize("abc=def").is_err());
        assert!(normalize(&"a".repeat(129)).is_err());
    }
}
