BEGIN IMMEDIATE;
CREATE TABLE app_settings (key TEXT PRIMARY KEY, value_json TEXT NOT NULL);
CREATE TABLE user_phrases (id TEXT PRIMARY KEY, text TEXT NOT NULL, reading TEXT NOT NULL, translation TEXT, UNIQUE(text, reading));
CREATE TABLE user_word_frequency (entry_id TEXT PRIMARY KEY, selection_count INTEGER NOT NULL DEFAULT 0, last_selected_at INTEGER NOT NULL);
CREATE TABLE learning_items (id TEXT PRIMARY KEY, candidate_json TEXT NOT NULL, status TEXT NOT NULL CHECK(status IN ('review', 'mastered')), created_at INTEGER NOT NULL, updated_at INTEGER NOT NULL);
CREATE TABLE learning_events (id INTEGER PRIMARY KEY, event_type TEXT NOT NULL, created_at INTEGER NOT NULL);
PRAGMA user_version = 1;
COMMIT;
