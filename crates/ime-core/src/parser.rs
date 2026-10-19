use crate::normalize;
use ime_types::AppResult;

pub struct PinyinInput {
    pub key: String,
    pub boundaries: Vec<usize>,
}

impl PinyinInput {
    pub fn parse(input: &str) -> AppResult<Self> {
        let key = normalize(input)?;
        let mut boundaries = Vec::new();
        let mut length = 0;
        for segment in input
            .split(['\'', ' '])
            .filter(|segment| !segment.is_empty())
        {
            length += normalize(segment)?.len();
            if length > 0 && length < key.len() {
                boundaries.push(length);
            }
        }
        Ok(Self { key, boundaries })
    }

    pub fn matches_reading(&self, reading: &str) -> bool {
        if self.boundaries.is_empty() {
            return true;
        }
        let mut length = 0;
        let positions: Vec<_> = reading
            .split_whitespace()
            .filter_map(|segment| {
                length += normalize(segment).ok()?.len();
                Some(length)
            })
            .collect();
        self.boundaries
            .iter()
            .all(|position| positions.contains(position))
    }

    pub fn crosses_boundary(&self, start: usize, end: usize) -> bool {
        self.boundaries
            .iter()
            .any(|position| start < *position && *position < end)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn apostrophes_preserve_syllable_boundaries() {
        let input = PinyinInput::parse("xi'an").expect("parse");
        assert_eq!(input.key, "xian");
        assert!(input.matches_reading("Xi1 an1"));
        assert!(!input.matches_reading("xian1"));
        assert!(input.crosses_boundary(0, 4));
        assert!(!input.crosses_boundary(0, 2));
    }
}
