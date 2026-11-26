"""Smoke test: the core of the API works, end to end.   (owner: lead)

    python3 tests/blackbox/smoke_test.py fake
    python3 tests/blackbox/smoke_test.py build/debug/forgedb [--io epoll ...]

This is the "v0.1 First search" milestone: when it passes against forgedb,
ForgeDB works. LOAD-1 writes the full API test suite (api_test.py).
"""

import socket
import sys

from harness import Check, build_request, forgedb, http_request, parse_body

CHUNKS = [
    {"id": "infosys:p1", "doc_id": "infosys_fy25", "text": "Revenue rose to ₹1,45,000 crore in FY2025.",
     "vector": [1, 0, 0, 0], "meta": {"company": "infosys"}},
    {"id": "tcs:p1", "doc_id": "tcs_fy25", "text": "TCS revenue grew; profit fell.",
     "vector": [0, 1, 0, 0], "meta": {"company": "tcs"}},
    {"id": "infosys:p2", "doc_id": "infosys_fy25", "text": "Profit and profit margin improved.",
     "vector": [0.9, 0.1, 0, 0], "meta": {"company": "infosys"}},
]


def same_chunk(got, want) -> bool:
    if not isinstance(got, dict):
        return False
    if any(got.get(k) != want[k] for k in ("id", "doc_id", "text", "meta")):
        return False
    vec = got.get("vector")
    return isinstance(vec, list) and len(vec) == len(want["vector"]) and all(
        abs(a - b) < 1e-6 for a, b in zip(vec, want["vector"]))


def ids(body) -> list:
    try:
        return [h["id"] for h in body["hits"]]
    except (TypeError, KeyError):
        return [f"<bad response: {body!r:.200}>"]


def main() -> None:
    check = Check()
    with forgedb(sys.argv[1], 4, *sys.argv[2:]) as db:
        print("health and adding chunks")
        status, body = db.call("GET", "/health")
        check(status == 200 and body == {"status": "ok", "chunks": 0, "dim": 4},
              f"GET /health on an empty database (got {status} {body!r:.200})")
        for row, chunk in enumerate(CHUNKS):
            status, body = db.call("POST", "/v1/chunks", chunk)
            check(status == 201 and body == {"id": chunk["id"], "row": row},
                  f"POST /v1/chunks {chunk['id']} -> 201, row {row} (got {status} {body!r:.200})")
        status, body = db.call("POST", "/v1/chunks", CHUNKS[0])
        check(status == 409 and "error" in body, f"the same id again -> 409 (got {status})")
        bad = [
            ("a vector of the wrong length", {**CHUNKS[0], "id": "x1", "vector": [1, 0]}),
            ("an all-zero vector", {**CHUNKS[0], "id": "x2", "vector": [0, 0, 0, 0]}),
            ("no id", {k: v for k, v in CHUNKS[0].items() if k != "id"}),
            ("an empty id", {**CHUNKS[0], "id": ""}),
            ("an unknown field", {**CHUNKS[0], "id": "x3", "colour": "red"}),
            ("text that isn't a string", {**CHUNKS[0], "id": "x4", "text": 5}),
        ]
        for what, chunk in bad:
            status, body = db.call("POST", "/v1/chunks", chunk)
            check(status == 400 and isinstance(body, dict) and "error" in body,
                  f"a chunk with {what} -> 400 with an error message (got {status})")
        with socket.create_connection(("127.0.0.1", db.port), timeout=10) as s:
            raw = b"POST /v1/chunks HTTP/1.1\r\nContent-Length: 9\r\nConnection: close\r\n\r\n{not json"
            status, headers, _ = http_request(s, raw)
            check(status == 400, f"a body that isn't JSON -> 400 (got {status})")
        status, body = db.call("GET", "/health")
        check(status == 200 and body.get("chunks") == 3, "GET /health counts 3 chunks")

        print("reading chunks back")
        status, body = db.call("GET", "/v1/chunks/infosys:p1")
        check(status == 200 and same_chunk(body, CHUNKS[0]), f"GET /v1/chunks/infosys:p1 (got {status} {body!r:.200})")
        status, body = db.call("GET", "/v1/chunks/infosys%3Ap2")
        check(status == 200 and same_chunk(body, CHUNKS[2]), "GET with a percent-encoded id (%3A is ':')")
        status, body = db.call("GET", "/v1/chunks/nope")
        check(status == 404 and "error" in body, f"GET an unknown id -> 404 (got {status})")

        print("vector search")
        status, body = db.call("POST", "/v1/search", {"vector": [1, 0, 0, 0], "k": 2})
        check(status == 200 and ids(body) == ["infosys:p1", "infosys:p2"],
              f"nearest 2 to [1,0,0,0] (got {status} {ids(body)})")
        if status == 200:
            scores = [h["score"] for h in body["hits"]]
            check(abs(scores[0] - 1.0) < 1e-5 and scores[0] >= scores[1],
                  f"scores are cosine similarities, best first (got {scores})")
            check(isinstance(body.get("took_ms"), (int, float)), "the response has took_ms")
            hit = body["hits"][0]
            check(hit.get("doc_id") == "infosys_fy25" and hit.get("text") == CHUNKS[0]["text"],
                  "a hit has its doc_id and text")
        status, body = db.call("POST", "/v1/search",
                               {"mode": "vector", "vector": [1, 0, 0, 0], "k": 5, "filter": {"doc_id": "tcs_fy25"}})
        check(ids(body) == ["tcs:p1"], f"filter by doc_id (got {ids(body)})")
        status, body = db.call("POST", "/v1/search",
                               {"vector": [0, 1, 0, 0], "k": 3, "filter": {"meta": {"company": "infosys"}}})
        check(ids(body) == ["infosys:p2", "infosys:p1"], f"filter by meta (got {ids(body)})")

        print("keyword search")
        status, body = db.call("POST", "/v1/search", {"mode": "keyword", "text": "PROFIT", "k": 5})
        check(status == 200 and ids(body) == ["infosys:p2", "tcs:p1"],
              f"'PROFIT': twice beats once, case doesn't matter (got {status} {ids(body)})")
        status, body = db.call("POST", "/v1/search", {"mode": "keyword", "text": "1,45,000"})
        check(ids(body) == ["infosys:p1"], f"'1,45,000' is one word (got {ids(body)})")
        status, body = db.call("POST", "/v1/search", {"mode": "keyword", "text": "zebra"})
        check(status == 200 and ids(body) == [], f"no match -> no hits (got {ids(body)})")

        print("bad searches")
        bad = [
            ("k = 0", {"vector": [1, 0, 0, 0], "k": 0}),
            ("k = 1001", {"vector": [1, 0, 0, 0], "k": 1001}),
            ("an unknown mode", {"mode": "magic", "vector": [1, 0, 0, 0]}),
            ("keyword mode without text", {"mode": "keyword"}),
            ("vector mode without a vector", {"mode": "vector", "text": "profit"}),
            ("a vector of the wrong length", {"vector": [1, 0]}),
        ]
        for what, req in bad:
            status, body = db.call("POST", "/v1/search", req)
            check(status == 400 and isinstance(body, dict) and "error" in body, f"{what} -> 400 (got {status})")

        print("HTTP")
        status, body = db.call("GET", "/nope")
        check(status == 404 and isinstance(body, dict) and "error" in body, f"unknown path -> 404 JSON error (got {status})")
        with socket.create_connection(("127.0.0.1", db.port), timeout=10) as s:
            status, headers, _ = http_request(s, build_request("GET", "/v1/search", keep_alive=False))
            check(status == 405 and headers.get("allow") == "POST",
                  f"GET /v1/search -> 405 with 'Allow: POST' (got {status}, allow={headers.get('allow')!r})")
        with socket.create_connection(("127.0.0.1", db.port), timeout=10) as s:
            first = http_request(s, build_request("GET", "/health"))
            second = http_request(s, build_request("GET", "/v1/chunks/tcs:p1"))
            check(first[0] == 200 and second[0] == 200 and same_chunk(parse_body(second[2]), CHUNKS[1]),
                  "two requests on one kept-alive connection")
            check(first[1].get("content-type") == "application/json", "responses are application/json")
        check(db.alive(), "the server is still running")
    check.finish()


if __name__ == "__main__":
    main()
