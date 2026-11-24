"""Week 2: the HTTP/1.1 server.

    python3 tests/http_test.py <http_server>

The server is started as:  <http_server> --port <port> --root tests/www
It must support:
    GET  /health          -> 200, Content-Type: application/json, body {"status":"ok"}
    POST /echo            -> 200, the request body sent back
    GET  /static/<file>   -> 200 with the file from --root (never a file outside it)
and answer 400 (malformed), 404 (unknown path), 405 (known path, wrong method),
413 (body over 1 MiB) and 431 (headers over 8 KiB). Keep-alive by default.
"""

import json
import os
import socket
import sys
import time

from harness import Check, Server, free_port, http_request, read_response

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "www")


def conn(port: int) -> socket.socket:
    return socket.create_connection(("127.0.0.1", port), timeout=5)


def one(port: int, raw: bytes):
    with conn(port) as s:
        return http_request(s, raw)


def main() -> None:
    server_bin = sys.argv[1]
    port = free_port()
    check = Check()
    with Server([server_bin, "--port", str(port), "--root", ROOT], port, "http_server") as server:
        status, headers, body = one(port, b"GET /health HTTP/1.1\r\nHost: x\r\n\r\n")
        ok_json = False
        try:
            ok_json = json.loads(body) == {"status": "ok"}
        except ValueError:
            pass
        check(status == 200 and ok_json, 'GET /health -> 200 {"status":"ok"}')
        check(headers.get("content-type", "").startswith("application/json"), "GET /health has Content-Type: application/json")

        status, _, body = one(port, b"POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 11\r\n\r\nhello world")
        check(status == 200 and body == b"hello world", "POST /echo -> 200 with the same body")

        with open(os.path.join(ROOT, "index.html"), "rb") as f:
            page = f.read()
        status, _, body = one(port, b"GET /static/index.html HTTP/1.1\r\nHost: x\r\n\r\n")
        check(status == 200 and body == page, "GET /static/index.html -> 200 with the file")

        status, _, body = one(port, b"GET /static/../../../../etc/passwd HTTP/1.1\r\nHost: x\r\n\r\n")
        check(status in (400, 403, 404) and b"root:" not in body, "GET /static/../../etc/passwd is refused (no path traversal)")

        check(one(port, b"GET /nope HTTP/1.1\r\nHost: x\r\n\r\n")[0] == 404, "unknown path -> 404")
        check(one(port, b"DELETE /health HTTP/1.1\r\nHost: x\r\n\r\n")[0] == 405, "DELETE /health -> 405")
        check(one(port, b"HELLO\r\n\r\n")[0] == 400, "malformed request line -> 400")
        check(
            one(port, b"POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 2097152\r\n\r\n")[0] == 413,
            "Content-Length of 2 MiB -> 413 (decided from the header, before reading the body)",
        )
        huge = b"GET /health HTTP/1.1\r\nHost: x\r\nX-Big: " + b"a" * 20000 + b"\r\n\r\n"
        try:
            status = one(port, huge)[0]
        except ConnectionError:
            status = None
        check(status == 431, "a 20 KB header -> 431")

        with conn(port) as s:
            first = http_request(s, b"GET /health HTTP/1.1\r\nHost: x\r\n\r\n")[0]
            second = http_request(s, b"POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 3\r\n\r\nabc")
            check(first == 200 and second[0] == 200 and second[2] == b"abc", "keep-alive: 2 requests on one connection")

        with conn(port) as s:
            status = http_request(s, b"GET /health HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")[0]
            check(status == 200 and s.recv(1) == b"", "Connection: close -> response, then the server closes")

        with conn(port) as s:  # one request arriving in pieces: TCP has no message boundaries
            s.sendall(b"POST /echo HTTP/1.1\r\nHo")
            time.sleep(0.05)
            s.sendall(b"st: x\r\nContent-Length: 10\r\n\r\n01234")
            time.sleep(0.05)
            s.sendall(b"56789")
            status, _, body = read_response(s)
            check(status == 200 and body == b"0123456789", "a request split across 3 packets still works")

        check(server.alive(), "server process is still running")
    check.finish()


if __name__ == "__main__":
    main()
