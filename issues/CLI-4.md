# [CLI-4] Bring the ML wing's data into ForgeDB
labels: comp:cli, size:S
milestone: v0.2 Survives a crash

**Component:** CLI (with the ML wing's Retrieval team) · **Size:** S · **Needs:** CLI-2
**Your code:** `tools/export_rag.py`

## What is this, in plain words?
The ML wing (repo `Q-FIS/Finforge-vanilla`) has turned six annual reports into
chunks and embeddings. Turn their files into a ForgeDB JSONL file, load it
with `forge load`, and suddenly ForgeDB holds real data. This is the bridge
between the two wings.

## What you must give back
`tools/export_rag.py --chunks <their chunks file> --vectors <their embeddings file> --out chunks.jsonl`:
- One line per chunk: `id` = their `chunk_id`, `doc_id`, `text`, `vector`, and `meta` = `company`, `fy`, `basis`, `page_start` (as strings).
- Check that the number of vectors equals the number of chunks, and that every vector has the same length; refuse otherwise.
- Print a summary: chunks, dimension, documents.

## Step by step
1. Ask the ML wing's Retrieval team where their chunks and embeddings are saved, and in which format. (Their data shapes are in `src/rag/schema.py`: look at the `Chunk` class.) This script may run in their environment (with numpy).
2. Write the script; export; `forge load` it into a `forgedb --dim <their dim>`.
3. Sanity check: search for a chunk's own vector: it must come back first, with a score of about 1.0.

## Done when
- [ ] The script is merged; the PR shows the summary, the load rate, and the sanity check

## Learn first
- Read the ML wing's plan doc and `schema.py`: what's a chunk for them, and where do the embeddings come from?
