# [KW-1] The tokenizer: text into words
labels: comp:kw, size:S
milestone: v0.1 First search

**Component:** KW (keyword search) · **Size:** S (1 day) · **Needs:** nothing, start on day 1
**Contract:** `include/forge/keyword_index.hpp` · **Your code:** `src/kw/tokenizer.cpp` · **Tests:** `tests/contract/kw/tokenizer_test.cpp`

## What is this, in plain words?
Keyword search starts by cutting text into words ("tokens"). The rules decide
what can be found: if "₹1,45,000" became "1", "45" and "000", nobody could
search for the amount. Our documents are Indian annual reports, so the rules
keep numbers with commas and dots whole.

## What you must give back
`tokenize`, following these rules exactly (the fake server, `tools/fake_server.py`, has the same rules in Python: check it if a case is unclear):

1. Work on bytes. A **word byte** is a letter `a-z`, `A-Z`, a digit `0-9`, or any byte ≥ 0x80 (so UTF-8 words in any script, like "नमस्ते", stay whole).
2. A run of word bytes is a token. Letters `A-Z` are lowercased; nothing else changes.
3. A `,` or `.` is kept **inside** a token when the byte before it is a digit, the byte after it is a digit, and a token has already started: "1,45,000", "3.5", "v2.0.1".
4. The rupee sign `₹` (the three bytes `E2 82 B9`) separates tokens, like a space.
5. Every other byte separates tokens: spaces, punctuation, `-`, `_`, `'`.

| Text | Tokens |
| --- | --- |
| `Revenue rose to ₹1,45,000 crore in FY2025.` | revenue, rose, to, 1,45,000, crore, in, fy2025 |
| `3.5% growth` | 3.5, growth |
| `U.S.A.` | u, s, a |
| `e-mail x_y don't` | e, mail, x, y, don, t |
| `1..2` | 1, 2 |
| `₹500` | 500 |

## Step by step
1. Delete `src/kw/stub_tokenizer.cpp`; create `src/kw/tokenizer.cpp`.
2. One loop over the bytes, with a `std::string current`. Add a byte, or end the token.
3. Check for `₹` with `text.compare(i, 3, "\xE2\x82\xB9") == 0` before the other rules.

## Watch out for
- `char` may be signed: bytes ≥ 0x80 are negative. Convert to `unsigned char` before comparing.
- Don't use `std::isalpha` / `std::tolower` on non-ASCII bytes: their result depends on the locale.
- Rule 3 looks at the byte *after* the comma: don't read past the end of the text.

## How to test it
`ctest --preset debug -R contract.kw.Tokenize`

## Done when
- [ ] Every `contract.kw.Tokenize.*` test passes on `debug`

## Learn first
- How UTF-8 encodes characters as 1 to 4 bytes, and why "every byte ≥ 0x80 is part of a multi-byte character" makes rule 1 safe.
