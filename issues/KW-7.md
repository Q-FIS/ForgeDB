# [KW-7] Phrase search: "net profit", in that order
labels: comp:kw, size:M
milestone: v2 Stretch

**Component:** KW · **Size:** M (3 days) · **Needs:** KW-6 (your v1 work is done)
**Your code:** `src/kw/`; `docs/API.md` and `tools/fake_server.py` (with the lead) · **Tests:** `tests/unit/kw/`

## What is this, in plain words?
A keyword search for `net profit` today finds chunks with "net" anywhere and
"profit" anywhere, like "net loss ... profit before tax". In financial text
the difference matters. **Phrase search** finds the words next to each
other, in order. It needs a **positional index**: for each word and chunk,
not just how many times it appears, but *where*.

## What you must give back
1. Postings that store positions (token numbers) as well as counts, compressed like KW-4 (gaps + varints).
2. Query syntax, in `docs/API.md` (agreed with the lead): text in double quotes is a phrase. `"net profit" FY2025` means: the phrase "net profit" must appear, and FY2025 adds to the score.
3. Scoring: a chunk without every phrase in the query isn't returned; a matching phrase counts in BM25 like one term, with its frequency = how many times the phrase occurs.
4. The same rules in `tools/fake_server.py`, so the black-box tests can check both (the lead reviews that part).
5. Unit tests: phrases at the start and end of a chunk; a phrase that appears twice; words present but not adjacent (not a match); a one-word "phrase"; an unclosed quote (treat it as if closed at the end).

## Watch out for
- Positions make the index 2-4× bigger: measure it, before and after.
- Intersect the postings of a phrase's words **before** checking positions: only chunks containing all the words can match.

## Done when
- [ ] Phrase search works in `forgedb` and the fake server, with tests; the PR has the memory cost

## Learn first
- *Introduction to Information Retrieval*, section 2.4, "Positional postings and phrase queries".
