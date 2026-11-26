# [KW-6] Experiment: keyword vs vector vs hybrid, on real questions
labels: comp:kw, size:M, experiment
milestone: v1.0 Fast, and powers the RAG

**Component:** KW (with the ML wing) · **Size:** M · **Needs:** KW-5, CLI-4
**Your output:** `tools/eval_retrieval.py`, `docs/experiments/KW-6-hybrid.md`

## What is this, in plain words?
The ML wing has real annual reports, chunked and embedded, and a set of
questions whose answers they've marked. Load them into ForgeDB (CLI-4) and
measure which search mode finds the right chunks most often.

## What you must give back
1. `tools/eval_retrieval.py`: for each question, search in each mode (k = 10), and compute **recall@5**, **recall@10** and **MRR** against the marked relevant chunks.
2. A report: the three modes side by side, overall and per question category; three questions where hybrid wins and why; where keyword alone beats vectors (hint: exact numbers and years).
3. Ask the ML wing's evaluation team how they compute these metrics, and use the same definitions, so the numbers are comparable with theirs.

## Done when
- [ ] The script and the report are merged, and presented to **both** wings

## Learn first
- Recall@k and MRR: what each measures, and when they disagree.
