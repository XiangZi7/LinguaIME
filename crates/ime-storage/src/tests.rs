use super::*;
use ime_types::{Candidate, Settings, UserPhrase};

fn storage() -> Storage { Storage::initialize(Connection::open_in_memory().expect("memory")).expect("migration") }
fn phrase(id: &str, text: &str) -> UserPhrase { UserPhrase { id:id.into(),text:text.into(),reading:"ni hao".into(),translation:None } }

#[test]
fn migration_settings_and_privacy_are_persistent() {
    let mut storage = storage();
    assert_eq!(storage.settings().expect("settings"),Settings::default());
    storage.record_selection("a").expect("select");
    assert_eq!(storage.frequencies().expect("frequency")["a"],1);
    assert_eq!(storage.statistics().expect("stats").selections,0);
    let settings = Settings { auto_learn:false,collect_stats:true,..Settings::default() };
    storage.save_settings(&settings).expect("save");
    storage.record_selection("b").expect("select");
    assert!(!storage.frequencies().expect("frequency").contains_key("b"));
    assert_eq!(storage.statistics().expect("stats").selections,1);
    assert!(storage.save_settings(&Settings { page_size:0,..settings }).is_err());
}

#[test]
fn invalid_imports_do_not_partially_commit() {
    let mut storage = storage();
    let invalid = UserPhrase { reading:"no=key".into(),..phrase("bad","坏") };
    assert!(storage.import_phrases(&[phrase("a","你好"),invalid]).is_err());
    assert!(storage.phrases().expect("phrases").is_empty());
    storage.import_phrases(&[phrase("a","你好")]).expect("import");
    storage.import_phrases(&[UserPhrase { translation:Some("hello".into()),..phrase("b","你好") }]).expect("merge");
    let phrases=storage.phrases().expect("phrases");
    assert_eq!(phrases.len(),1);
    assert_eq!(phrases[0].translation.as_deref(),Some("hello"));
}

#[test]
fn clearing_learning_preserves_dictionary_and_phrases() {
    let mut storage = storage();
    storage.save_phrase(&phrase("a","你好")).expect("phrase");
    let candidate=Candidate { id:"a".into(),text:"你好".into(),reading:"ni hao".into(),key:"nihao".into(),translation:None,source:"个人词库".into(),frequency:1 };
    storage.save_learning(&candidate,"review").expect("favorite");
    storage.save_learning(&candidate,"mastered").expect("master");
    assert_eq!(storage.statistics().expect("stats").mastered,1);
    storage.clear_data("learning").expect("clear");
    assert!(storage.learning().expect("learning").is_empty());
    assert_eq!(storage.phrases().expect("phrases").len(),1);
}
