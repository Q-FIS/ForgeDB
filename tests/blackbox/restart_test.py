"""Restart test: chunks survive stopping and starting the server.   (owner: lead)

    python3 tests/blackbox/restart_test.py fake
    python3 tests/blackbox/restart_test.py build/debug/forgedb

Against forgedb this needs the write-ahead log and recovery: WAL-4 switches it
on in tests/blackbox/CMakeLists.txt.
"""

import sys

from harness import Check, forgedb

CHUNKS = [
    {"id": f"doc:{i}", "doc_id": "doc", "text": f"chunk number {i} about profit",
     "vector": [1, i, 0, 0], "meta": {"n": str(i)}}
    for i in range(50)
]


def main() -> None:
    check = Check()
    server = sys.argv[1]
    with forgedb(server, 4, *sys.argv[2:]) as db:
        data_dir = db.data_dir
        for chunk in CHUNKS:
            status, _ = db.call("POST", "/v1/chunks", chunk)
            if status != 201:
                check(False, f"POST {chunk['id']} -> 201 (got {status})")
                check.finish()
    print("stopped with SIGTERM; starting again on the same data directory")
    for attempt in (1, 2):
        with forgedb(server, 4, *sys.argv[2:], data_dir=data_dir) as db:
            status, body = db.call("GET", "/health")
            check(status == 200 and body.get("chunks") == 50 + (attempt - 1),
                  f"restart {attempt}: every chunk is back (health says {body!r:.100})")
            status, body = db.call("GET", "/v1/chunks/doc:7")
            check(status == 200 and body.get("text") == CHUNKS[7]["text"] and body.get("meta") == {"n": "7"},
                  f"restart {attempt}: a chunk reads back exactly")
            status, body = db.call("POST", "/v1/search", {"mode": "keyword", "text": "number", "k": 100})
            check(status == 200 and len(body.get("hits", [])) == 50,
                  f"restart {attempt}: the keyword index was rebuilt (50 hits, got {len(body.get('hits', []))})")
            status, body = db.call("POST", "/v1/chunks", CHUNKS[0])
            check(status == 409, f"restart {attempt}: an old id is still taken (409, got {status})")
            if attempt == 1:
                extra = {"id": "after-restart", "doc_id": "doc", "text": "added later", "vector": [0, 0, 1, 0]}
                status, body = db.call("POST", "/v1/chunks", extra)
                check(status == 201 and body.get("row") == 50, f"restart 1: the next row is 50 (got {body!r:.100})")
    check.finish()


if __name__ == "__main__":
    main()
