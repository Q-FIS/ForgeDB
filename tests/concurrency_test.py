"""Week 3: many clients at the same time.

    python3 tests/concurrency_test.py <http_server>

100 clients each send 20 keep-alive requests at the same time (mixing
GET /health, POST /echo and GET /sleep?ms=5). Every response must be correct.
Run it with the tsan build too: ThreadSanitizer reports any data race.
"""

import os
import socket
import sys
import threading

from harness import Check, Server, free_port, http_request, raise_fd_limit

CLIENTS, REQUESTS = 100, 20


def client(port: int, n: int, errors: list[str]) -> None:
    try:
        with socket.create_connection(("127.0.0.1", port), timeout=30) as s:
            for i in range(REQUESTS):
                if i % 3 == 0:
                    status, _, _ = http_request(s, b"GET /health HTTP/1.1\r\nHost: x\r\n\r\n")
                    ok = status == 200
                elif i % 3 == 1:
                    payload = f"client {n} request {i}".encode()
                    raw = b"POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: %d\r\n\r\n" % len(payload) + payload
                    status, _, body = http_request(s, raw)
                    ok = status == 200 and body == payload  # a mixed-up response would show here
                else:
                    status, _, _ = http_request(s, b"GET /sleep?ms=5 HTTP/1.1\r\nHost: x\r\n\r\n")
                    ok = status == 200
                if not ok:
                    errors.append(f"client {n}, request {i}: status {status}")
                    return
    except Exception as e:  # noqa: BLE001 - report any failure
        errors.append(f"client {n}: {e!r}")


def main() -> None:
    raise_fd_limit()
    server_bin = sys.argv[1]
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "www")
    port = free_port()
    check = Check()
    with Server([server_bin, "--port", str(port), "--root", root], port, "http_server") as server:
        errors: list[str] = []
        threads = [threading.Thread(target=client, args=(port, n, errors)) for n in range(CLIENTS)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        check(not errors, f"{CLIENTS} clients x {REQUESTS} requests all correct" + (f" (first error: {errors[0]})" if errors else ""))
        check(server.alive(), "server process is still running")
    check.finish()


if __name__ == "__main__":
    main()
