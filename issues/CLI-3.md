# [CLI-3] A Python client, for the ML wing
labels: comp:cli, size:M
milestone: v0.2 Survives a crash

**Component:** CLI · **Size:** M (2 days) · **Needs:** nothing (it's tested against the fake server)
**Your code:** `clients/python/forgedb.py` · **Tests:** `tests/blackbox/python_client_test.py` (replaces the placeholder)

## What is this, in plain words?
The ML wing writes Python. Give them `import forgedb` with a small, friendly
API, using **only the standard library** (`http.client`, `json`,
`urllib.parse`), so they can use it without installing anything.

## What you must give back
```python
class ForgeError(Exception):          # .status (int) and .message (str)
class AlreadyExists(ForgeError):      # 409
class ForgeDB:
    def __init__(self, host="127.0.0.1", port=8080, timeout=10.0): ...
    def health(self) -> dict: ...
    def put(self, id, doc_id, text, vector, meta=None) -> int: ...              # returns the row
    def get(self, id) -> dict | None: ...                                       # None if 404
    def search(self, vector=None, text=None, k=10, mode=None, doc_id=None, meta=None) -> list[dict]: ...
    def close(self): ...                                                        # also usable as `with ForgeDB() as db:`
```
- One `http.client.HTTPConnection`, reused (keep-alive); reconnect once if the server closed it.
- `mode=None` chooses like `forge search` does. Any non-2xx status raises `ForgeError` (409 → `AlreadyExists`).
- `vector` may be a list or a numpy array (convert with `[float(x) for x in vector]`; don't import numpy).
- Docstrings on every method, and a short usage example in `clients/python/README.md`.

`python_client_test.py` (against the fake server, with `harness.py`): every method; the errors; ids with `/` and `:`; 500 puts through one connection; a server that closed the connection between calls.

## Done when
- [ ] `blackbox.python_client` passes; the ML wing's Retrieval team has read the README and said it's clear

## Learn first
- Python docs: `http.client` (HTTPConnection, getresponse, and the keep-alive behaviour).
