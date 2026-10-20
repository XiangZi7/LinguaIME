//! Offline Rime candidate generation with LinguaIME metadata and shared SQLite learning.
use ime_core::{PinyinEngine, normalize};
use ime_types::{
    AppError, AppResult, Candidate, CandidatePage, EngineRequest, SCHEMA_VERSION, UserPhrase,
};
use std::{
    collections::{HashMap, HashSet},
    ffi::{CStr, CString, c_char, c_int},
    path::{Path, PathBuf},
    sync::Arc,
};

#[repr(C)]
#[derive(Clone, Copy)]
struct RawCandidate {
    text: [c_char; 256],
    reading: [c_char; 256],
}

#[cfg(windows)]
unsafe extern "C" {
    fn lingua_rime_module_path(output: *mut u16, capacity: u32) -> u32;
    fn lingua_rime_open(directory: *const u16, user_directory: *const c_char) -> c_int;
    fn lingua_rime_close();
    fn lingua_rime_query(input: *const c_char, output: *mut RawCandidate, capacity: u32) -> c_int;
}

pub struct InputEngine {
    dictionary: Arc<PinyinEngine>,
}

impl InputEngine {
    pub fn new(
        dictionary: Arc<PinyinEngine>,
        directory: &Path,
        data_directory: &Path,
    ) -> AppResult<Self> {
        #[cfg(windows)]
        {
            use std::os::windows::ffi::OsStrExt;
            let bundle_version =
                std::fs::read_to_string(directory.join("runtime-id.txt")).unwrap_or_default();
            if bundle_version.trim() != env!("LINGUAIME_RIME_BUNDLE") || bundle_version.is_empty() {
                return Err(AppError::new(
                    "RIME_BUNDLE",
                    "Rime 资源与输入核心版本不一致，请重新构建并更新输入法",
                ));
            }
            // Only immutable precompiled dictionaries are shared by processes. No deployment,
            // network, logs or LevelDB writes occur on the per-keystroke path.
            let user = data_directory
                .join("rime-runtime")
                .join(std::process::id().to_string());
            std::fs::create_dir_all(&user)
                .map_err(|_| AppError::new("RIME_DATA", "无法创建 Rime 运行目录"))?;
            let user = CString::new(user.to_string_lossy().as_bytes())
                .map_err(|_| AppError::new("RIME_PATH", "Rime 运行路径无效"))?;
            let directory: Vec<u16> = directory.as_os_str().encode_wide().chain(Some(0)).collect();
            // SAFETY: both paths are NUL-terminated and live throughout initialization.
            let result = unsafe { lingua_rime_open(directory.as_ptr(), user.as_ptr()) };
            if result != 0 {
                return Err(AppError::new(
                    "RIME_INIT",
                    format!("Rime 初始化失败（{result}），请运行 pnpm native:build 后重启管理应用"),
                ));
            }
            Ok(Self { dictionary })
        }
        #[cfg(not(windows))]
        {
            let _ = (dictionary, directory, data_directory);
            Err(AppError::new(
                "RIME_PLATFORM",
                "Rime 原生输入目前仅支持 Windows",
            ))
        }
    }

    pub fn count(&self) -> usize {
        self.dictionary.count()
    }

    pub fn query(
        &self,
        request: &EngineRequest,
        phrases: &[UserPhrase],
        frequencies: &HashMap<String, u64>,
    ) -> AppResult<CandidatePage> {
        // Validate before passing input to the native library; preserve explicit syllable boundaries.
        let key = normalize(&request.composition)?;
        let input = request
            .composition
            .split(['\'', ' '])
            .map(normalize)
            .collect::<AppResult<Vec<_>>>()?
            .into_iter()
            .filter(|part| !part.is_empty())
            .collect::<Vec<_>>()
            .join("'");
        let mut candidates = Vec::new();
        if !key.is_empty() {
            for phrase in phrases {
                if matches_phrase(&input, &phrase.reading)? {
                    candidates.push(Candidate {
                        id: phrase.id.clone(),
                        text: phrase.text.clone(),
                        reading: phrase.reading.clone(),
                        key: normalize(&phrase.reading)?,
                        translation: phrase.translation.clone(),
                        source: "个人词库".into(),
                        frequency: 1,
                    });
                }
            }
            for raw in query_rime(&input)? {
                // SAFETY: the bridge zero-initializes rows and rejects oversized strings.
                let text = unsafe { CStr::from_ptr(raw.text.as_ptr()) }
                    .to_string_lossy()
                    .into_owned();
                let hint = unsafe { CStr::from_ptr(raw.reading.as_ptr()) }
                    .to_string_lossy()
                    .into_owned();
                let hint_key = normalize(&hint).unwrap_or_default();
                if let Some(metadata) = self
                    .dictionary
                    .lookup_text(&text, if hint_key.is_empty() { &key } else { &hint_key })
                {
                    candidates.push(Candidate {
                        source: "Rime · CC-CEDICT".into(),
                        ..metadata.clone()
                    });
                } else {
                    let reading = if !hint.is_empty() && normalize(&hint).is_ok() {
                        hint
                    } else {
                        input.clone()
                    };
                    candidates.push(Candidate {
                        id: candidate_id(&text),
                        text,
                        key: normalize(&reading)?,
                        reading,
                        translation: None,
                        source: "Rime".into(),
                        frequency: 1,
                    });
                }
            }
        }
        // Stable sort retains Rime's language-model order among unlearned candidates.
        candidates.sort_by_key(|candidate| {
            std::cmp::Reverse((
                frequencies.get(&candidate.id).copied().unwrap_or(0),
                candidate.source == "个人词库",
            ))
        });
        let mut seen = HashSet::new();
        candidates.retain(|candidate| seen.insert(candidate.text.clone()));
        let total = candidates.len();
        let size = request.page_size.clamp(1, 9);
        let page_count = total.div_ceil(size);
        let page_index = request.page.min(page_count.saturating_sub(1));
        Ok(CandidatePage {
            schema_version: SCHEMA_VERSION,
            composition: request.composition.clone(),
            items: candidates
                .into_iter()
                .skip(page_index * size)
                .take(size)
                .collect(),
            page_index,
            page_count,
            total,
        })
    }
}

impl Drop for InputEngine {
    fn drop(&mut self) {
        #[cfg(windows)]
        // SAFETY: each successful new acquires one native reference; close is serialized.
        unsafe {
            lingua_rime_close()
        };
    }
}

fn query_rime(input: &str) -> AppResult<Vec<RawCandidate>> {
    #[cfg(windows)]
    {
        let input =
            CString::new(input).map_err(|_| AppError::new("INVALID_PINYIN", "拼音包含无效字符"))?;
        let mut output = vec![
            RawCandidate {
                text: [0; 256],
                reading: [0; 256]
            };
            128
        ];
        // SAFETY: input is NUL-terminated; output owns capacity initialized writable rows.
        let count =
            unsafe { lingua_rime_query(input.as_ptr(), output.as_mut_ptr(), output.len() as u32) };
        if count < 0 || count as usize > output.len() {
            return Err(AppError::new(
                "RIME_QUERY",
                "Rime 查询失败，请重新启动输入法",
            ));
        }
        output.truncate(count as usize);
        Ok(output)
    }
    #[cfg(not(windows))]
    {
        let _ = input;
        Err(AppError::new(
            "RIME_PLATFORM",
            "Rime 原生输入目前仅支持 Windows",
        ))
    }
}

fn candidate_id(text: &str) -> String {
    // Stable across composition spellings and processes; never store the typed sentence in an ID.
    let hash = text.bytes().fold(0xcbf29ce484222325_u64, |hash, byte| {
        (hash ^ u64::from(byte)).wrapping_mul(0x100000001b3)
    });
    format!("rime:{hash:016x}")
}

fn matches_phrase(input: &str, reading: &str) -> AppResult<bool> {
    let syllables = reading
        .split(['\'', ' '])
        .map(normalize)
        .collect::<AppResult<Vec<_>>>()?
        .into_iter()
        .filter(|s| !s.is_empty())
        .collect::<Vec<_>>();
    let mut positions = HashSet::from([0]);
    for syllable in syllables {
        let mut next = HashSet::new();
        for position in positions {
            if position == input.len() {
                return Ok(true);
            }
            let remaining = &input[position..];
            // The final typed syllable may be incomplete.
            if !remaining.contains('\'') && syllable.starts_with(remaining) {
                return Ok(true);
            }
            let mut lengths = vec![1, syllable.len()];
            if ["zh", "ch", "sh"]
                .iter()
                .any(|initial| syllable.starts_with(initial))
            {
                lengths.push(2);
            }
            for length in lengths {
                if remaining.starts_with(&syllable[..length]) {
                    let mut end = position + length;
                    if input.as_bytes().get(end) == Some(&b'\'') {
                        end += 1;
                    }
                    next.insert(end);
                }
            }
        }
        positions = next;
    }
    Ok(positions.contains(&input.len()))
}

/// The DLL and the desktop executable both locate their own bundled runtime.
pub fn default_runtime_directory() -> PathBuf {
    if let Some(directory) = std::env::var_os("LINGUAIME_RIME_DIR") {
        return directory.into();
    }
    #[cfg(windows)]
    {
        use std::os::windows::ffi::OsStringExt;
        let mut path = vec![0_u16; 32768];
        // SAFETY: path is a writable UTF-16 buffer of the declared size.
        let length =
            unsafe { lingua_rime_module_path(path.as_mut_ptr(), path.len() as u32) } as usize;
        if length > 0 && length < path.len() {
            let module = PathBuf::from(std::ffi::OsString::from_wide(&path[..length]));
            if let Some(parent) = module.parent() {
                if module
                    .file_name()
                    .is_some_and(|name| name.eq_ignore_ascii_case("ime_ffi.dll"))
                {
                    return parent.join("rime");
                }
                for directory in [parent.join("rime"), parent.join("native/rime")] {
                    if directory.join("rime.dll").is_file() {
                        return directory;
                    }
                }
            }
        }
    }
    // Source-tree commands and debug builds. Installed DLLs use the sibling above.
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../build/native/rime")
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn personal_phrases_accept_initials_mixed_spelling_and_boundaries() {
        for input in ["zg", "zhg", "zhongg", "zguo", "z'g", "zhong'guo"] {
            assert!(
                matches_phrase(input, "zhong1 guo2").expect("reading"),
                "{input}"
            );
        }
        assert!(!matches_phrase("zhong'guoo", "zhong1 guo2").expect("reading"));
        assert!(!matches_phrase("xi'an", "xian1").expect("reading"));
    }

    #[cfg(windows)]
    #[test]
    fn real_rime_supports_initials_sentences_metadata_learning_and_paging() -> AppResult<()> {
        let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../build/rime-test");
        let engine = InputEngine::new(
            Arc::new(PinyinEngine::default()),
            &default_runtime_directory(),
            &root,
        )?;
        let query = |composition: &str| {
            engine.query(
                &EngineRequest {
                    composition: composition.into(),
                    page: 0,
                    page_size: 9,
                },
                &[],
                &HashMap::new(),
            )
        };
        for (input, expected) in [
            ("nh", "你好"),
            ("xx", "学习"),
            ("xuex", "学习"),
            ("zhg", "中国"),
            ("srf", "输入法"),
            ("xi'an", "西安"),
            ("woaizhongguo", "我爱中国"),
        ] {
            let page = query(input)?;
            assert!(
                page.items.iter().any(|item| item.text == expected),
                "{input}: {:?}",
                page.items
            );
        }
        let page = query("nihao")?;
        assert!(
            page.items.iter().all(|item| item.text != "你"),
            "partial candidates must not discard hao"
        );
        let hello = page
            .items
            .iter()
            .find(|item| item.text == "你好")
            .expect("hello");
        assert!(hello.translation.is_some());
        let request = EngineRequest {
            composition: "nh".into(),
            page: 0,
            page_size: 9,
        };
        let first = engine.query(&request, &[], &HashMap::new())?;
        let learned = first.items.last().expect("candidates");
        let learned_page =
            engine.query(&request, &[], &HashMap::from([(learned.id.clone(), 10)]))?;
        assert_eq!(learned_page.items[0].id, learned.id);
        let last = engine.query(
            &EngineRequest {
                page: usize::MAX,
                page_size: 2,
                ..request
            },
            &[],
            &HashMap::new(),
        )?;
        assert_eq!(last.page_index, last.page_count.saturating_sub(1));
        assert!(query("")?.items.is_empty());
        assert!(query("abc=").is_err());
        assert!(query(&"a".repeat(129)).is_err());
        Ok(())
    }
}
