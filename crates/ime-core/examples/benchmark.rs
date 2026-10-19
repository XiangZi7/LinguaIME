use ime_core::PinyinEngine;
use ime_types::EngineRequest;
use std::{collections::HashMap, time::Instant};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let start = Instant::now();
    let engine = PinyinEngine::default();
    println!(
        "{} dictionary entries; cold initialization {:?}",
        engine.count(),
        start.elapsed()
    );
    let mut durations = Vec::new();
    for key in [
        "nihao",
        "xuexi",
        "zhongwen",
        "woshi",
        "woshishuo",
        "jintian",
        "woaizhongguo",
    ] {
        let request = EngineRequest {
            composition: key.into(),
            page: 0,
            page_size: 7,
        };
        let page = engine.query(&request, &[], &HashMap::new())?;
        println!(
            "{key}: {:?}",
            page.items
                .iter()
                .map(|entry| &entry.text)
                .collect::<Vec<_>>()
        );
        for _ in 0..100 {
            let start = Instant::now();
            std::hint::black_box(engine.query(&request, &[], &HashMap::new())?);
            durations.push(start.elapsed());
        }
    }
    durations.sort();
    println!(
        "Warm query P95: {:?} (700 queries)",
        durations[durations.len() * 95 / 100]
    );
    Ok(())
}
