use ime_core::PinyinEngine;
use ime_rime::{InputEngine, default_runtime_directory};
use ime_types::EngineRequest;
use std::{collections::HashMap, sync::Arc, time::Instant};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../build/rime-benchmark");
    let start = Instant::now();
    let engine = InputEngine::new(
        Arc::new(PinyinEngine::default()),
        &default_runtime_directory(),
        &root,
    )?;
    println!("Rime initialization: {:?}", start.elapsed());
    let mut durations = Vec::new();
    for key in [
        "n",
        "nh",
        "nihao",
        "xx",
        "xuex",
        "xuexi",
        "zg",
        "zhg",
        "shrf",
        "srf",
        "jintian",
        "woaizhongguo",
        "woxiangxuezhongwen",
        "xi'an",
        "xian",
    ] {
        let request = EngineRequest {
            composition: key.into(),
            page: 0,
            page_size: 9,
        };
        let start = Instant::now();
        let page = engine.query(&request, &[], &HashMap::new())?;
        println!(
            "{key}: {:?} ({:?}, {} candidates)",
            page.items
                .iter()
                .map(|entry| &entry.text)
                .collect::<Vec<_>>(),
            start.elapsed(),
            page.total
        );
        for _ in 0..10 {
            let start = Instant::now();
            std::hint::black_box(engine.query(&request, &[], &HashMap::new())?);
            durations.push(start.elapsed());
        }
    }
    durations.sort();
    println!(
        "Warm query P95: {:?} ({} queries)",
        durations[durations.len() * 95 / 100],
        durations.len()
    );
    Ok(())
}
