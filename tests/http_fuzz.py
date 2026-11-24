"""Week 2: throw malformed requests at the HTTP server. It must never crash.

    python3 tests/http_fuzz.py <http_server> [number_of_requests]

Sends random and broken requests (random bytes, missing CRLF, huge headers,
silly Content-Lengths, NUL bytes...). Every 500 requests it checks that
GET /health still works. Run it against the debug build: AddressSanitizer
then turns any memory bug into a crash that this test reports.
"""

import os
import random
import socket
import sys

from harness import Check, Server, free_port, get

random.seed(12345)  # the same "random" requests every run, so failures can be reproduced

METHODS = [b"GET", b"POST", b"PUT", b"DELETE", b"", b"G\x00T", b"GETGETGETGET" * 50]
PATHS = [b"/health", b"/echo", b"/static/index.html", b"", b"/" + b"a" * 9000, b"/%00", b"/../../x", b"*"]
VERSIONS = [b"HTTP/1.1", b"HTTP/1.0", b"HTTP/9.9", b"HTTP/", b"", b"http/1.1"]


def fuzz_request() -> bytes:
    kind = random.randrange(8)
    if kind == 0:
        return os.urandom(random.randrange(1, 4096))
    m, p, v = random.choice(METHODS), random.choice(PATHS), random.choice(VERSIONS)
    line = m + b" " + p + b" " + v
    headers = [b"Host: x"]
    if kind == 1:
        headers.append(b"Content-Length: " + random.choice([b"-1", b"abc", b"99999999999999999999", b"", b"10 10"]))
    if kind == 2:
        headers.append(b"X-Big: " + b"b" * random.randrange(5000, 70000))
    if kind == 3:
        headers = [b"NoColonHere", b":empty-name", b"A" * 200 + b": v"]
    if kind == 4:
        return line + b"\n" + b"\n".join(headers) + b"\n\n"  # LF only, no CR
    if kind == 5:
        return line + b"\r\n" + b"\r\n".join(headers)  # never finishes the headers
    if kind == 6:
        headers += [b"Content-Length: 5"]
        return line + b"\r\n" + b"\r\n".join(headers) + b"\r\n\r\nab"  # body shorter than promised
    body = os.urandom(random.randrange(0, 64))
    headers.append(b"Content-Length: " + str(len(body)).encode())
    return line + b"\r\n" + b"\r\n".join(headers) + b"\r\n\r\n" + body


def main() -> None:
    server_bin = sys.argv[1]
    total = int(sys.argv[2]) if len(sys.argv) > 2 else 10_000
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "www")
    port = free_port()
    check = Check()
    with Server([server_bin, "--port", str(port), "--root", root], port, "http_server") as server:
        for i in range(1, total + 1):
            try:
                with socket.create_connection(("127.0.0.1", port), timeout=2) as s:
                    s.sendall(fuzz_request())
                    s.settimeout(0.05)
                    try:
                        s.recv(4096)
                    except OSError:
                        pass
            except OSError:
                pass  # the server may close or reset the connection: that's allowed
            if i % 500 == 0:
                if not server.alive():
                    check(False, f"server crashed after {i} fuzz requests")
                    break
                try:
                    healthy = get(port, "/health")[0] == 200
                except OSError:
                    healthy = False
                if not healthy:
                    check(False, f"GET /health stopped working after {i} fuzz requests")
                    break
        check(server.alive(), f"server survived {total:,} malformed requests")
    check.finish()


if __name__ == "__main__":
    main()
