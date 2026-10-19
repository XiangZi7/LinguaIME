"""Build a reproducible offline lexicon from CC-CEDICT (CC BY-SA 4.0)."""

import gzip
import hashlib
import json
from pathlib import Path
import re
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
URL = 'https://www.mdbg.net/chinese/export/cedict/cedict_1_0_ts_utf-8_mdbg.txt.gz'
target = ROOT / 'data' / 'dictionaries'
target.mkdir(parents=True, exist_ok=True)
archive = urllib.request.urlopen(URL, timeout=60).read()
source = gzip.decompress(archive).decode('utf-8')
FREQUENCY_URL = 'https://raw.githubusercontent.com/fxsjy/jieba/master/jieba/dict.txt'
frequency_source = urllib.request.urlopen(FREQUENCY_URL, timeout=60).read()
frequencies = {}
for frequency_line in frequency_source.decode('utf-8').splitlines():
    frequency_parts = frequency_line.split(' ')
    if len(frequency_parts) >= 2 and frequency_parts[1].isdigit():
        frequencies[frequency_parts[0]] = int(frequency_parts[1])
rows = []
preview = []
pattern = re.compile(r'^(\S+) (\S+) \[(.+?)\] /(.+)/$')
common = set('我是说你好中国中文英语学习输入法谢谢再见今天明天世界苹果电脑工作喜欢我们你们他们北京上海朋友天气时间快乐学校学生老师语言词典卧室准备图书馆冷热吃喝水人大小多少左右上下东西日月山河爱家')
for line in source.splitlines():
    match = pattern.match(line)
    if not match:
        continue
    _, text, reading, definitions = match.groups()
    key = re.sub('[1-5 ]', '', reading.lower()).replace('u:', 'v').replace('ü', 'v')
    if not re.fullmatch('[a-zv]+', key) or not all('\u3400' <= char <= '\u9fff' for char in text):
        continue
    gloss = definitions.split('/')[0]
    if gloss.startswith(('CL:', 'variant of ', 'old variant of ')):
        continue
    entry_id = hashlib.sha256(f'{text}|{reading}'.encode()).hexdigest()[:16]
    row = {'id': entry_id, 'text': text, 'reading': reading, 'key': key,
           'translation': gloss, 'source': 'CC-CEDICT', 'frequency': max(1, frequencies.get(text, 1))}
    rows.append(row)
    if len(text) <= 4 and all(char in common for char in text):
        preview.append(row)

# TSV avoids JSON parsing overhead in the native critical path. Keep all readings.
with (target / 'cedict.tsv').open('w', encoding='utf-8', newline='\n') as stream:
    for row in rows:
        stream.write('\t'.join(str(row[key]).replace('\t', ' ') for key in
                               ('id', 'text', 'reading', 'key', 'translation', 'frequency')) + '\n')
(ROOT / 'public' / 'preview-dictionary.json').write_text(
    json.dumps(preview, ensure_ascii=False, separators=(',', ':')), encoding='utf-8')
metadata = {'name': 'CC-CEDICT', 'license': 'CC BY-SA 4.0', 'url': URL,
            'attribution': 'CC-CEDICT contributors; original CEDICT by Paul Denisowski; distributed by MDBG',
            'sha256': hashlib.sha256(archive).hexdigest(), 'entries': len(rows),
            'previewEntries': len(preview),
            'sourceHeader': [line for line in source.splitlines() if line.startswith('#')][:40],
            'modifications': 'Simplified Han-only entries; tone-free lookup keys; first English sense; stable SHA-256 IDs. POS omitted. Frequency from jieba dictionary; missing frequencies use neutral weight 1.',
            'frequencySource': {'url': FREQUENCY_URL, 'license': 'MIT', 'author': 'Sun Junyi / jieba contributors', 'sha256': hashlib.sha256(frequency_source).hexdigest()}}
(target / 'metadata.json').write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(f'Imported {len(rows)} entries; browser preview {len(preview)} entries; sha256 {metadata["sha256"]}')
