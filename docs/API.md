# The ForgeDB API

ForgeDB speaks HTTP/1.1 and JSON. This page is the contract: `forgedb`, the
fake server (`tools/fake_server.py`) and every client follow it. Changing it
needs the lead's OK.

Every response body is JSON (`Content-Type: application/json`), except
`/metrics`. Every error is `{"error": "<a readable message>"}`.

## The endpoints

| Method and path | What | Success |
| --- | --- | --- |
| `GET /health` | Is it up? How big is it? | 200 |
| `POST /v1/chunks` | Add one chunk | 201 |
| `GET /v1/chunks/{id}` | Read one chunk back | 200 |
| `POST /v1/search` | Search: by vector, by keyword, or both | 200 |
| `GET /metrics` | Counters and timings, as text (APP-6) | 200 |

A path that doesn't exist: **404**. A path that exists with the wrong method:
**405**, with an `Allow` header listing the methods that work (e.g. `Allow: POST`).

### `GET /health`

```json
{"status": "ok", "chunks": 1520, "dim": 384}
```

### `POST /v1/chunks`

Request:

```json
{
  "id": "infosys_ar_fy2025:p12:c0003",
  "doc_id": "infosys_ar_fy2025",
  "text": "Revenue rose to ₹1,45,000 crore in FY2025.",
  "vector": [0.021, -0.114, 0.087],
  "meta": {"company": "infosys", "fy": "FY2025"}
}
```

| Field | Rules |
| --- | --- |
| `id` | string, required, not empty, unique |
| `doc_id` | string, required (may be empty) |
| `text` | string, required (may be empty) |
| `vector` | array of numbers, required, exactly `dim` of them, not all zero |
| `meta` | object whose values are all strings; optional (missing = `{}`) |

Any other field is an error: a typo like `"vectr"` must fail loudly, not be ignored.

Response, **201**: `{"id": "infosys_ar_fy2025:p12:c0003", "row": 1519}`. The
row is the chunk's position: 0 for the first chunk ever added, then 1, 2...

Errors: **400** for invalid JSON, a missing or wrong field, or a wrong vector
length; **409** if the id is already taken.

### `GET /v1/chunks/{id}`

The id is percent-encoded in the path when needed (`a/b` is `a%2Fb`; `:` may
stay as it is). Response, **200**: the chunk, exactly as it was added, with
`meta` always present (`{}` if empty). **404** if there's no such id.

### `POST /v1/search`

```json
{
  "mode": "hybrid",
  "vector": [0.021, -0.114, 0.087],
  "text": "net profit FY2025",
  "k": 5,
  "filter": {"doc_id": "infosys_ar_fy2025", "meta": {"fy": "FY2025"}}
}
```

| Field | Rules |
| --- | --- |
| `mode` | `"vector"` (default), `"keyword"` or `"hybrid"` |
| `vector` | needed for `vector` and `hybrid`: exactly `dim` numbers, not all zero |
| `text` | needed for `keyword` and `hybrid`: a non-empty string |
| `k` | integer from 1 to 1000; default 10 |
| `filter` | optional. `doc_id`: only this document. `meta`: every pair must match |

Response, **200**:

```json
{
  "hits": [
    {"id": "infosys_ar_fy2025:p12:c0003", "doc_id": "infosys_ar_fy2025",
     "text": "Revenue rose to ₹1,45,000 crore in FY2025.", "score": 0.83}
  ],
  "took_ms": 1.7
}
```

Hits are best first; equal scores are ordered by row (the older chunk first).
Fewer than `k` hits if fewer chunks match; `"hits": []` if none.

What `score` means depends on the mode:

| Mode | Score |
| --- | --- |
| `vector` | cosine similarity, from -1 to 1 |
| `keyword` | BM25 (higher is better; only positive scores are returned). Before KW-3: a count of matching words |
| `hybrid` | Reciprocal Rank Fusion of the two rankings (KW-5). Before KW-5: **501** |

Errors: **400** for any rule above broken.

### `GET /metrics` (APP-6)

Plain text, one value per line, in the Prometheus format:

```
forge_requests_total{route="/v1/search",status="200"} 1342
forge_request_seconds_bucket{route="/v1/search",le="0.001"} 1200
forge_chunks 1520
```

## Status codes, all in one place

| Code | When |
| --- | --- |
| 200, 201 | Success |
| 400 | Bad JSON, a bad field, a malformed HTTP request |
| 404 | No such path, or no such chunk |
| 405 | The path exists, but not with this method (`Allow` header) |
| 409 | The chunk id is taken |
| 413 | The body is over 1 MiB |
| 431 | The request line and headers are over 8 KiB |
| 500 | A bug in ForgeDB. It should never happen; the server keeps running |
| 501 | Not built yet (hybrid before KW-5; `Transfer-Encoding: chunked`) |
| 503 | Too busy: the queue is full, or too many connections. `Retry-After: 1` |

## HTTP details

- HTTP/1.1, with keep-alive: a connection stays open for more requests unless
  the client sends `Connection: close` (HTTP/1.0: unless it sends `Connection: keep-alive`).
- Bodies use `Content-Length`. `Transfer-Encoding` is not supported (501).
- Requests may be pipelined: several sent at once, answered in order.
- A connection silent for `--idle-timeout-ms` (default 10 s) is closed.
