# [APP-2] The API's JSON: chunks, searches, hits
labels: comp:app, size:M
milestone: v0.1 First search

**Component:** APP · **Size:** M (2 days) · **Needs:** nothing, start on day 1
**Contract:** `include/forge/app.hpp`, `docs/API.md` · **Your code:** `src/app/json.cpp` · **Tests:** `tests/contract/app/json_test.cpp`

## What is this, in plain words?
Clients send JSON; ForgeDB works with C++ structs. These four functions are
the border between them: `chunk_from_json`, `chunk_to_json`,
`search_from_json`, `hits_to_json`. They're strict on the way in: a missing
field, a wrong type or an unknown field is a clear error, because a typo
silently ignored is the worst kind of bug.

## What you must give back
The four functions, following `docs/API.md` field by field. Every problem
throws `std::invalid_argument` with a message naming the field.

## Step by step
1. Delete `src/app/stub_json.cpp`; create `src/app/json.cpp`. Use nlohmann/json (`#include <nlohmann/json.hpp>`): it's already downloaded and linked.
2. Parse without exceptions: `auto j = nlohmann::json::parse(body, nullptr, false); if (j.is_discarded()) ...`
3. Check it's an object; reject unknown keys (loop over `j.items()`).
4. For each field: present? right type (`is_string()`, `is_number()`, `is_array()`, `is_object()`, `is_number_integer()`)? Then convert.
5. `search_from_json`: defaults (mode `vector`, k 10); `k` must be an **integer** from 1 to 1000 (`2.5`, `"5"` and `true` are errors); the mode decides which of `vector` / `text` are required.
6. Output: `meta` is always an object (`{}` when empty), `hits` always an array.

## Watch out for
- In nlohmann/json, `true` is not a number, but an integer is: `is_number()` is true for both `1` and `1.5`. Check what each `is_...` function really means.
- Strings with invalid UTF-8 make `dump()` throw. Use `dump(-1, ' ', false, nlohmann::json::error_handler_t::replace)`.
- Dimension checks are the Engine's job (it knows `dim`); JSON only checks shapes and types.

## How to test it
`ctest --preset debug -R contract.app.Json`

## Done when
- [ ] Every `contract.app.Json.*` test passes on `debug`

## Learn first
- The nlohmann/json README: "Read JSON from a string", "STL-like access", "Implicit conversions" (and why we avoid them).
