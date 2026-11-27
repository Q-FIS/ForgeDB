"""CLI test: the `forge` command against the fake server.   (owner: lead; checks CLI-1)

    python3 tests/blackbox/cli_test.py build/debug/forge

The rules (issue CLI-1): `forge` prints the server's response body on stdout,
then a newline. Exit code 0 for a 2xx status, 1 for any other status or if it
can't reach the server (with a message on stderr), 2 for a bad command line.
"""

import json
import subprocess
import sys

from harness import Check, forgedb, skip


def main() -> None:
    forge = sys.argv[1]
    check = Check()

    def run(*args: str) -> tuple[int, object, str]:
        r = subprocess.run([forge, *args], capture_output=True, text=True, timeout=30)
        if r.returncode == 77:
            skip("forge is not implemented yet")
        try:
            out = json.loads(r.stdout)
        except ValueError:
            out = r.stdout
        return r.returncode, out, r.stderr

    with forgedb("fake", 3) as db:
        p = ["--port", str(db.port)]
        print("commands")
        code, out, _ = run(*p, "health")
        check(code == 0 and isinstance(out, dict) and out.get("status") == "ok",
              f"forge health -> exit 0 and the JSON body (got {code} {out!r:.100})")
        code, out, _ = run(*p, "put", "--id", "a:1", "--doc", "d1", "--text", "hello world",
                           "--vector", "1,0,0", "--meta", "company=x", "--meta", "fy=FY2025")
        check(code == 0 and out == {"id": "a:1", "row": 0}, f"forge put (got {code} {out!r:.100})")
        code, out, _ = run("--host", "127.0.0.1", *p, "put", "--id", "a/2", "--doc", "d2",
                           "--text", "goodbye world", "--vector", "0,1,0.5")
        check(code == 0 and out == {"id": "a/2", "row": 1}, f"forge --host ... put without --meta (got {code} {out!r:.100})")
        code, out, _ = run(*p, "put", "--id", "a:1", "--doc", "d1", "--text", "again", "--vector", "1,0,0")
        check(code == 1 and isinstance(out, dict) and "error" in out,
              f"a 409 from the server -> exit 1, and the error body is printed (got {code})")
        code, out, _ = run(*p, "get", "a:1")
        check(code == 0 and isinstance(out, dict) and out.get("text") == "hello world"
              and out.get("meta") == {"company": "x", "fy": "FY2025"}, f"forge get (got {code} {out!r:.150})")
        code, out, _ = run(*p, "get", "a/2")
        check(code == 0 and isinstance(out, dict) and out.get("id") == "a/2",
              f"forge get of an id with '/': it must be percent-encoded (got {code} {out!r:.100})")
        code, out, _ = run(*p, "get", "nope")
        check(code == 1, f"forge get of an unknown id -> exit 1 (got {code})")

        def hit_ids(out) -> list:
            return [h["id"] for h in out["hits"]] if isinstance(out, dict) and "hits" in out else [repr(out)[:80]]

        code, out, _ = run(*p, "search", "--vector", "1,0,0", "--k", "1")
        check(code == 0 and hit_ids(out) == ["a:1"], f"forge search --vector (got {code} {hit_ids(out)})")
        code, out, _ = run(*p, "search", "--text", "goodbye")
        check(code == 0 and hit_ids(out) == ["a/2"], f"forge search --text is a keyword search (got {hit_ids(out)})")
        code, out, _ = run(*p, "search", "--text", "goodbye world", "--vector", "0,1,0")
        check(code == 0 and hit_ids(out)[:1] == ["a/2"], f"--text and --vector together: hybrid (got {hit_ids(out)})")
        code, out, _ = run(*p, "search", "--text", "world", "--mode", "keyword", "--k", "5")
        check(code == 0 and sorted(hit_ids(out)) == ["a/2", "a:1"], f"--mode keyword (got {hit_ids(out)})")

        print("bad command lines -> exit 2")
        for args in ([], ["fly"], ["put", "--doc", "d", "--text", "t", "--vector", "1,0,0"],
                     ["put", "--id", "x", "--doc", "d", "--text", "t", "--vector", "1,abc,0"],
                     ["search"], ["search", "--vector", "1,0,0", "--k", "ten"], ["get"],
                     ["--port", "abc", "health"], ["--port", "70000", "health"],
                     ["put", "--id", "x", "--doc", "d", "--text", "t", "--vector", "1,0,0", "--meta", "novalue"]):
            code, _, err = run(*(p if args[:1] != ["--port"] else []), *args)
            check(code == 2 and err.strip() != "", f"forge {' '.join(args) or '(nothing)'} -> exit 2 with a message (got {code})")

        port = db.port
    code, _, err = run("--port", str(port), "health")
    check(code == 1 and err.strip() != "", f"no server listening -> exit 1 with a message (got {code})")
    check.finish()


if __name__ == "__main__":
    main()
