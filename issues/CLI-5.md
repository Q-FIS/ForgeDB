# [CLI-5] The RAG uses ForgeDB
labels: comp:cli, size:M
milestone: v1.0 Fast, and powers the RAG

**Component:** CLI (a pull request in the ML wing's repo, reviewed by them) · **Size:** M · **Needs:** CLI-3, CLI-4, APP-4 (and WAL-4, so the data survives restarts)
**Your code:** in `Q-FIS/Finforge-vanilla`: a ForgeDB retrieval backend

## What is this, in plain words?
The finish line of the whole project: the ML wing's question-answering
system retrieves its passages from **ForgeDB**, the database you built from
raw sockets up, instead of its own numpy search. Then you check that the
answers are just as good, and see how much faster or slower retrieval is.

## What you must give back
1. In the ML wing's repo (with their Retrieval team): a backend for their retrieval step that calls ForgeDB through `forgedb.py`, chosen by a config setting (their default stays unchanged).
2. Their filters (company, year, basis) mapped to ForgeDB's `filter.meta`.
3. Their evaluation run twice, numpy vs ForgeDB: the retrieval metrics must match (vector mode: the same top-k, apart from exact ties), plus the retrieval time per question.
4. A short note in both repos: how to switch, and the comparison.

## Watch out for
- Their embeddings and the query embeddings must come from the same model and normalisation, or nothing matches. Check with CLI-4's sanity test.

## Done when
- [ ] The PR is merged in the ML wing's repo, with the comparison in its description
- [ ] Demoed to both wings

## Learn first
- The ML wing's `retrieve()` function and its config: read them with their Retrieval team.
