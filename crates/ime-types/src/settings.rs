use crate::{AppError, AppResult};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(default, rename_all = "camelCase", deny_unknown_fields)]
pub struct Settings {
    pub display_mode: String,
    pub page_size: usize,
    pub auto_learn: bool,
    pub collect_stats: bool,
    pub theme: String,
    pub accent: String,
    pub font_size: u32,
    pub row_spacing: String,
    pub chinese_punctuation: bool,
    pub enter_behavior: String,
    pub previous_page_key: String,
    pub next_page_key: String,
}

impl Default for Settings {
    fn default() -> Self {
        Self {
            display_mode: "bilingual".into(),
            page_size: 7,
            auto_learn: true,
            collect_stats: false,
            theme: "system".into(),
            accent: "green".into(),
            font_size: 17,
            row_spacing: "comfortable".into(),
            chinese_punctuation: true,
            enter_behavior: "candidate".into(),
            previous_page_key: "-".into(),
            next_page_key: "=".into(),
        }
    }
}

impl Settings {
    pub fn validate(&self) -> AppResult<()> {
        let valid = ["chinese", "bilingual", "detailed"].contains(&self.display_mode.as_str())
            && (3..=9).contains(&self.page_size)
            && ["system", "light", "dark"].contains(&self.theme.as_str())
            && ["green", "blue"].contains(&self.accent.as_str())
            && (14..=22).contains(&self.font_size)
            && ["compact", "comfortable"].contains(&self.row_spacing.as_str())
            && ["candidate", "raw"].contains(&self.enter_behavior.as_str())
            && ["-", "PageUp", "["].contains(&self.previous_page_key.as_str())
            && ["=", "PageDown", "]"].contains(&self.next_page_key.as_str());
        if valid {
            Ok(())
        } else {
            Err(AppError::new("INVALID_SETTINGS", "设置值不在允许范围内"))
        }
    }
}
