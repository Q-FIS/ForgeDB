"""A fake ForgeDB: the whole API (docs/API.md) in plain Python.   (owner: lead)

Why it exists:
  * CLI and LOAD can build and test their tools against it from day 1,
    before the C++ server works.
  * The black-box tests run against it too, which proves the tests themselves
    are right: if a test fails against the fake, the test is wrong.

It takes the same flags as forgedb (the ones it doesn't need are ignored):

    python3 tools/fake_server.py --port 8080 --dim 4 [--data-dir DIR]

With --data-dir, chunks are kept in DIR/fake-chunks.jsonl, so they survive a
restart. Python standard library only. Not fast, and not meant to be.
"""

import argparse
import json
import math
import os
import re
import signal
import sys
import threading
import time
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

MAX_K = 1000
CHUNK_KEYS = {"id", "doc_id", "text", "vector", "meta"}
SEARCH_KEYS = {"mode", "vector", "text", "k", "filter"}
FILTER_KEYS = {"doc_id", "meta"}
RRF_C = 60.0
HYBRID_DEPTH = 100  # hybrid search takes the best max(k, 100) of each kind, then merges


class BadRequest(Exception):
    pass


# ---------------------------------------------------------------------------
# Tokenizer: the same rules as KW-1 (issues/KW-1.md)
# ---------------------------------------------------------------------------

RUPEE = "₹"


def tokenize(text: str) -> list[str]:
    data = text.replace(RUPEE, " ").encode("utf-8")
    tokens, current = [], bytearray()

    def is_word_byte(b: int) -> bool:
        return (48 <= b <= 57) or (65 <= b <= 90) or (97 <= b <= 122) or b >= 0x80

    for i, b in enumerate(data):
        if is_word_byte(b):
            current.append(b + 32 if 65 <= b <= 90 else b)
        elif b in (ord(","), ord(".")) and current and 48 <= data[i - 1] <= 57 \
                and i + 1 < len(data) and 48 <= data[i + 1] <= 57:
            current.append(b)
        else:
            if current:
                tokens.append(current.decode("utf-8", errors="replace"))
                current = bytearray()
    if current:
        tokens.append(current.decode("utf-8", errors="replace"))
    return tokens


# ---------------------------------------------------------------------------
# The database
# ---------------------------------------------------------------------------


class Database:
    def __init__(self, dim: int, data_dir: str | None):
        self.dim = dim
        self.lock = threading.Lock()
        self.chunks: list[dict] = []
        self.rows: dict[str, int] = {}
        self.tokens: list[list[str]] = []
        self.file = None
        if data_dir:
            os.makedirs(data_dir, exist_ok=True)
            path = os.path.join(data_dir, "fake-chunks.jsonl")
            if os.path.exists(path):
                with open(path, encoding="utf-8") as f:
                    for line in f:
                        if line.strip():
                            self._add(json.loads(line))
            self.file = open(path, "a", encoding="utf-8")

    def _add(self, chunk: dict) -> int:
        row = len(self.chunks)
        self.chunks.append(chunk)
        self.rows[chunk["id"]] = row
        self.tokens.append(tokenize(chunk["text"]))
        return row

    def put(self, chunk: dict) -> int:
        if len(chunk["vector"]) != self.dim:
            raise BadRequest(f"vector has {len(chunk['vector'])} numbers, expected {self.dim}")
        if all(x == 0 for x in chunk["vector"]):
            raise BadRequest("vector is all zeros")
        with self.lock:
            if chunk["id"] in self.rows:
                raise KeyError(chunk["id"])
            row = self._add(chunk)
            if self.file:
                self.file.write(json.dumps(chunk, ensure_ascii=False) + "\n")
                self.file.flush()
            return row

    def get(self, chunk_id: str) -> dict | None:
        with self.lock:
            row = self.rows.get(chunk_id)
            return None if row is None else self.chunks[row]

    def size(self) -> int:
        with self.lock:
            return len(self.chunks)

    # -- search ---------------------------------------------------------------

    @staticmethod
    def _matches(chunk: dict, flt: dict) -> bool:
        if flt.get("doc_id") and chunk["doc_id"] != flt["doc_id"]:
            return False
        meta = chunk.get("meta", {})
        return all(meta.get(k) == v for k, v in flt.get("meta", {}).items())

    @staticmethod
    def _best(scored: list[tuple[int, float]], k: int) -> list[tuple[int, float]]:
        return sorted(scored, key=lambda rs: (-rs[1], rs[0]))[:k]

    def _vector_search(self, query: list[float], k: int, flt: dict) -> list[tuple[int, float]]:
        qn = math.sqrt(sum(x * x for x in query))
        scored = []
        for row, c in enumerate(self.chunks):
            if not self._matches(c, flt):
                continue
            v = c["vector"]
            vn = math.sqrt(sum(x * x for x in v))
            scored.append((row, sum(a * b for a, b in zip(query, v)) / (qn * vn)))
        return self._best(scored, k)

    def _keyword_search(self, text: str, k: int, flt: dict) -> list[tuple[int, float]]:
        words = set(tokenize(text))
        scored = []
        for row, toks in enumerate(self.tokens):
            if not self._matches(self.chunks[row], flt):
                continue
            score = sum(1 for t in toks if t in words)  # count-based, like KW-2
            if score > 0:
                scored.append((row, float(score)))
        return self._best(scored, k)

    def search(self, req: dict) -> list[dict]:
        mode, k, flt = req["mode"], req["k"], req["filter"]
        with self.lock:
            if mode == "vector":
                found = self._vector_search(req["vector"], k, flt)
            elif mode == "keyword":
                found = self._keyword_search(req["text"], k, flt)
            else:
                depth = max(k, HYBRID_DEPTH)
                merged: dict[int, float] = {}
                for ranked in (self._vector_search(req["vector"], depth, flt),
                               self._keyword_search(req["text"], depth, flt)):
                    for rank, (row, _) in enumerate(ranked, start=1):
                        merged[row] = merged.get(row, 0.0) + 1.0 / (RRF_C + rank)
                found = self._best(list(merged.items()), k)
            return [{"id": self.chunks[r]["id"], "doc_id": self.chunks[r]["doc_id"],
                     "text": self.chunks[r]["text"], "score": s} for r, s in found]


# ---------------------------------------------------------------------------
# JSON checking: the same rules as APP-2 (issues/APP-2.md)
# ---------------------------------------------------------------------------


def parse_json_object(body: bytes) -> dict:
    try:
        value = json.loads(body.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as e:
        raise BadRequest(f"invalid JSON: {e}") from None
    if not isinstance(value, dict):
        raise BadRequest("the body must be a JSON object")
    return value


def check_keys(obj: dict, allowed: set[str], what: str) -> None:
    unknown = sorted(set(obj) - allowed)
    if unknown:
        raise BadRequest(f"unknown field in {what}: {unknown[0]}")


def is_number(x) -> bool:
    return isinstance(x, (int, float)) and not isinstance(x, bool)


def read_vector(value, what: str) -> list[float]:
    if not isinstance(value, list) or not value or not all(is_number(x) for x in value):
        raise BadRequest(f"{what} must be a non-empty array of numbers")
    return [float(x) for x in value]


def read_string_map(value, what: str) -> dict:
    if not isinstance(value, dict) or not all(isinstance(v, str) for v in value.values()):
        raise BadRequest(f"{what} must be an object of strings")
    return value


def chunk_from_json(body: bytes) -> dict:
    obj = parse_json_object(body)
    check_keys(obj, CHUNK_KEYS, "chunk")
    for key in ("id", "doc_id", "text"):
        if not isinstance(obj.get(key), str):
            raise BadRequest(f"{key} must be a string")
    if not obj["id"]:
        raise BadRequest("id must not be empty")
    if "vector" not in obj:
        raise BadRequest("vector is missing")
    return {"id": obj["id"], "doc_id": obj["doc_id"], "text": obj["text"],
            "vector": read_vector(obj["vector"], "vector"),
            "meta": read_string_map(obj.get("meta", {}), "meta")}


def search_from_json(body: bytes, dim: int) -> dict:
    obj = parse_json_object(body)
    check_keys(obj, SEARCH_KEYS, "search")
    mode = obj.get("mode", "vector")
    if mode not in ("vector", "keyword", "hybrid"):
        raise BadRequest("mode must be vector, keyword or hybrid")
    k = obj.get("k", 10)
    if not isinstance(k, int) or isinstance(k, bool) or not 1 <= k <= MAX_K:
        raise BadRequest(f"k must be an integer from 1 to {MAX_K}")
    req = {"mode": mode, "k": k, "vector": [], "text": "", "filter": {}}
    if "vector" in obj:
        req["vector"] = read_vector(obj["vector"], "vector")
    if "text" in obj:
        if not isinstance(obj["text"], str):
            raise BadRequest("text must be a string")
        req["text"] = obj["text"]
    if "filter" in obj:
        flt = obj["filter"]
        if not isinstance(flt, dict):
            raise BadRequest("filter must be an object")
        check_keys(flt, FILTER_KEYS, "filter")
        if "doc_id" in flt and not isinstance(flt["doc_id"], str):
            raise BadRequest("filter.doc_id must be a string")
        req["filter"] = {"doc_id": flt.get("doc_id", ""),
                         "meta": read_string_map(flt.get("meta", {}), "filter.meta")}
    if mode in ("vector", "hybrid"):
        if not req["vector"]:
            raise BadRequest(f"a {mode} search needs a vector")
        if len(req["vector"]) != dim:
            raise BadRequest(f"vector has {len(req['vector'])} numbers, expected {dim}")
        if all(x == 0 for x in req["vector"]):
            raise BadRequest("vector is all zeros")
    if mode in ("keyword", "hybrid") and not req["text"]:
        raise BadRequest(f"a {mode} search needs text")
    return req


# ---------------------------------------------------------------------------
# HTTP
# ---------------------------------------------------------------------------

ROUTES = [  # (method, pattern); {id} matches one path segment
    ("GET", "/health"),
    ("POST", "/v1/chunks"),
    ("GET", "/v1/chunks/{id}"),
    ("POST", "/v1/search"),
]


def match(pattern: str, path: str) -> dict | None:
    p, s = pattern.split("/"), path.split("/")
    if len(p) != len(s):
        return None
    params = {}
    for a, b in zip(p, s):
        if a.startswith("{"):
            if not b:
                return None
            params[a[1:-1]] = b
        elif a != b:
            return None
    return params


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    db: Database = None  # set in main()

    def log_message(self, *args) -> None:  # quiet
        pass

    def reply(self, status: int, obj: dict, extra_headers: dict | None = None) -> None:
        body = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        for k, v in (extra_headers or {}).items():
            self.send_header(k, v)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def error(self, status: int, message: str, extra_headers: dict | None = None) -> None:
        self.reply(status, {"error": message}, extra_headers)

    def body(self) -> bytes:
        n = int(self.headers.get("Content-Length") or 0)
        return self.rfile.read(n) if n else b""

    def dispatch(self) -> None:
        path = self.path.split("?", 1)[0]
        body = self.body()
        allowed = []
        for method, pattern in ROUTES:
            params = match(pattern, path)
            if params is None:
                continue
            if method != self.command:
                allowed.append(method)
                continue
            try:
                params = {k: urllib.parse.unquote(v, errors="strict") for k, v in params.items()}
            except UnicodeDecodeError:
                return self.error(400, "bad percent-escape in the path")
            try:
                return self.route(method, pattern, params, body)
            except BadRequest as e:
                return self.error(400, str(e))
        if allowed:
            return self.error(405, "method not allowed", {"Allow": ", ".join(allowed)})
        return self.error(404, "not found")

    def route(self, method: str, pattern: str, params: dict, body: bytes) -> None:
        if pattern == "/health":
            return self.reply(200, {"status": "ok", "chunks": self.db.size(), "dim": self.db.dim})
        if pattern == "/v1/chunks":
            chunk = chunk_from_json(body)
            try:
                row = self.db.put(chunk)
            except KeyError:
                return self.error(409, f"a chunk with id {chunk['id']} already exists")
            return self.reply(201, {"id": chunk["id"], "row": row})
        if pattern == "/v1/chunks/{id}":
            chunk = self.db.get(params["id"])
            if chunk is None:
                return self.error(404, f"no chunk with id {params['id']}")
            return self.reply(200, chunk)
        if pattern == "/v1/search":
            req = search_from_json(body, self.db.dim)
            start = time.perf_counter()
            hits = self.db.search(req)
            took = (time.perf_counter() - start) * 1000
            return self.reply(200, {"hits": hits, "took_ms": took})

    do_GET = do_POST = do_PUT = do_DELETE = do_PATCH = dispatch


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=8080)
    ap.add_argument("--dim", type=int, required=True)
    ap.add_argument("--data-dir")
    for ignored in ("--threads", "--queue", "--io", "--fsync", "--idle-timeout-ms"):
        ap.add_argument(ignored)
    args = ap.parse_args()
    if not 1 <= args.dim <= 4096:
        sys.exit("--dim must be from 1 to 4096")
    Handler.db = Database(args.dim, args.data_dir)
    server = ThreadingHTTPServer((args.host, args.port), Handler)
    server.daemon_threads = True
    signal.signal(signal.SIGTERM, lambda *_: threading.Thread(target=server.shutdown).start())
    print(f"fake ForgeDB listening on {args.host}:{args.port} (dim {args.dim})", file=sys.stderr)
    server.serve_forever()
    server.server_close()


if __name__ == "__main__":
    main()
