use crate::normalize;
use ime_types::{AppResult, Candidate, CandidatePage, EngineRequest, SCHEMA_VERSION, UserPhrase};
use std::collections::{BTreeMap, HashMap, HashSet};

const DICTIONARY: &str = include_str!("../../../data/dictionaries/cedict.tsv");

pub struct PinyinEngine {
    index: BTreeMap<String, Vec<Candidate>>,
    count: usize,
    log_total_frequency: f64,
}

impl Default for PinyinEngine {
    fn default() -> Self {
        Self::from_tsv(DICTIONARY)
    }
}

impl PinyinEngine {
    pub fn from_tsv(data: &str) -> Self {
        let mut index: BTreeMap<String, Vec<Candidate>> = BTreeMap::new();
        let mut count = 0;
        for line in data.lines() {
            let fields: Vec<_> = line.split('\t').collect();
            if fields.len() != 6 {
                continue;
            }
            index.entry(fields[3].into()).or_default().push(Candidate {
                id: fields[0].into(),
                text: fields[1].into(),
                reading: fields[2].into(),
                key: fields[3].into(),
                translation: Some(fields[4].into()),
                source: "CC-CEDICT".into(),
                frequency: fields[5].parse().unwrap_or(1),
            });
            count += 1;
        }
        let total: u64 = index.values().flatten().map(|entry| entry.frequency.max(1)).sum();
        Self { index, count, log_total_frequency: (total.max(1) as f64).ln() }
    }

    pub fn count(&self) -> usize {
        self.count
    }

    pub fn query(
        &self,
        request: &EngineRequest,
        phrases: &[UserPhrase],
        frequencies: &HashMap<String, u64>,
    ) -> AppResult<CandidatePage> {
        let key = normalize(&request.composition)?;
        let page_size = request.page_size.clamp(1, 9);
        let mut candidates: Vec<Candidate> = Vec::new();
        if !key.is_empty() {
            for phrase in phrases {
                let reading = normalize(&phrase.reading)?;
                if reading.starts_with(&key) {
                    candidates.push(Candidate {
                        id: phrase.id.clone(),
                        text: phrase.text.clone(),
                        reading: phrase.reading.clone(),
                        key: reading,
                        translation: phrase.translation.clone(),
                        source: "个人词库".into(),
                        frequency: 1,
                    });
                }
            }
            // A lexicographic range provides bounded prefix lookup, never a full dictionary scan.
            for (_, entries) in self.index.range(key.clone()..) {
                if !entries[0].key.starts_with(&key) {
                    break;
                }
                candidates.extend(entries.iter().cloned());
                if candidates.len() >= 512 {
                    break;
                }
            }
            if let Some((text, weight)) = self.compose(&key)
                && !candidates.iter().any(|candidate| candidate.text == text)
            {
                candidates.push(Candidate {
                    id: format!("composed:{key}"),
                    text,
                    reading: request.composition.clone(),
                    key: key.clone(),
                    translation: None,
                    source: "本地组合".into(),
                    frequency: weight,
                });
            }
        }
        candidates.sort_by(|a, b| {
            let rank = |c: &Candidate| {
                (
                    c.key == key,
                    frequencies.get(&c.id).copied().unwrap_or(0),
                    c.source == "个人词库",
                    std::cmp::Reverse(c.key.len()),
                    c.frequency,
                )
            };
            rank(b).cmp(&rank(a)).then_with(|| a.id.cmp(&b.id))
        });
        let mut seen = HashSet::new();
        candidates.retain(|c| seen.insert(c.text.clone()));
        let total = candidates.len();
        let page_count = total.div_ceil(page_size);
        let page_index = request.page.min(page_count.saturating_sub(1));
        let items = candidates
            .into_iter()
            .skip(page_index * page_size)
            .take(page_size)
            .collect();
        Ok(CandidatePage {
            schema_version: SCHEMA_VERSION,
            composition: request.composition.clone(),
            items,
            page_index,
            page_count,
            total,
        })
    }

    fn compose(&self, key: &str) -> Option<(String, u64)> {
        // Unigram log probabilities favor common words without fabricating a phrase translation.
        let mut paths: Vec<Option<(f64, String)>> = vec![None; key.len() + 1];
        paths[0] = Some((0.0, String::new()));
        for end in 1..=key.len() {
            for start in (0..end).rev().take(24) {
                let Some((cost, prefix)) = &paths[start] else {
                    continue;
                };
                let Some(entries) = self.index.get(&key[start..end]) else {
                    continue;
                };
                let Some(entry) = entries.iter().max_by_key(|e| e.frequency) else {
                    continue;
                };
                let next = cost + (entry.frequency.max(1) as f64).ln() - self.log_total_frequency;
                if paths[end]
                    .as_ref()
                    .is_none_or(|(current, _)| next > *current)
                {
                    paths[end] = Some((next, format!("{prefix}{}", entry.text)));
                }
            }
        }
        paths.pop().flatten().map(|(score, text)| (text, (score + self.log_total_frequency).exp().round() as u64))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    const SAMPLE: &str = "a\t学习\txue2 xi2\txuexi\tto study\t1\nb\t学习\txue2 xi2\txuexi\tto learn\t1\nc\t学\txue2\txue\tto learn\t1\nd\t西\txi1\txi\twest\t1\ne\t学校\txue2 xiao4\txuexiao\tschool\t1";
    #[test]
    fn ranks_exact_deduplicates_and_clamps_pages() {
        let engine = PinyinEngine::from_tsv(SAMPLE);
        let request = EngineRequest {
            composition: "xue".into(),
            page: 99,
            page_size: 2,
        };
        let page = engine.query(&request, &[], &HashMap::new()).expect("query");
        assert_eq!((page.total, page.page_count, page.page_index), (3, 2, 1));
        let page = engine
            .query(&EngineRequest { page: 0, ..request }, &[], &HashMap::new())
            .expect("query");
        assert_eq!(page.items[0].text, "学");
    }
    #[test]
    fn respects_learning_and_custom_phrases() {
        let engine = PinyinEngine::from_tsv(SAMPLE);
        let phrases = [UserPhrase {
            id: "user-1".into(),
            text: "学戏".into(),
            reading: "xue xi".into(),
            translation: None,
        }];
        let request = EngineRequest {
            composition: "xuexi".into(),
            page: 0,
            page_size: 9,
        };
        let page = engine
            .query(&request, &phrases, &HashMap::new())
            .expect("query");
        assert_eq!(page.items[0].text, "学戏");
        let page = engine
            .query(&request, &phrases, &HashMap::from([("a".into(), 10)]))
            .expect("query");
        assert_eq!(page.items[0].text, "学习");
    }
    #[test]
    fn empty_and_composed_results_have_honest_metadata() {
        let engine = PinyinEngine::from_tsv(SAMPLE);
        let request = EngineRequest {
            composition: String::new(),
            page: 8,
            page_size: 0,
        };
        let empty = engine.query(&request, &[], &HashMap::new()).expect("query");
        assert_eq!((empty.page_count, empty.page_index), (0, 0));
        let page = engine
            .query(
                &EngineRequest {
                    composition: "xixue".into(),
                    ..request
                },
                &[],
                &HashMap::new(),
            )
            .expect("query");
        assert_eq!(page.items[0].text, "西学");
        assert!(page.items[0].translation.is_none());
    }
}
