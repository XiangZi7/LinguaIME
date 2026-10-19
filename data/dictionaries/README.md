# 离线词典

`cedict.tsv` 来源：[CC-CEDICT / MDBG](https://www.mdbg.net/chinese/dictionary?page=cedict)。作者为 CC-CEDICT contributors，原始 CEDICT 由 Paul Denisowski 发起。数据遵循 [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/)，该数据文件及其衍生数据仍按此许可证发布，应用代码许可证不改变词典许可证。

`metadata.json` 记录原始下载摘要、上游版本头与修改方式。转换仅保留简体汉字词条、所有读音和首条英文释义；生成无声调检索键及稳定 ID。不推测词性、音标、例句或使用频率。frequency=1 是中性排序基值，不代表语料统计。`public/preview-dictionary.json` 是同源子集，仅用于明确标记的浏览器预览。

使用 `python scripts/fetch-dictionary.py` 显式更新，不在构建时下载。更新后应提交 metadata 与词典，复核许可证及差异。
