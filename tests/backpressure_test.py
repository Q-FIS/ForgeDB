"""Week 3, Group A: a full queue answers 503 instead of waiting forever.

    python3 tests/backpressure_test.py <http_server>

Started as: <http_server> --port <port> --root tests/www --workers 1 --queue 1
Then 10 clients ask for GET /sleep?ms=500 at the same moment. With 1 worker
and room for 1 waiting connection, most must get 503 quickly, and at least
one must get 200.
"""

import os
import socket
import sys
import threading
import time

from harness import Check, Server, free_port, http_request


def main() -> None:
    server_bin = sys.argv[1]
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "www")
    port = free_port()
    argv = [server_bin, "--port", str(port), "--root", root, "--workers", "1", "--queue", "1"]
    check = Check()
    with Server(argv, port, "http_server") as server:
        results: list[tuple[int, float]] = []
        lock = threading.Lock()

        def one() -> None:
            start = time.time()
            try:
                with socket.create_connection(("127.0.0.1", port), timeout=10) as s:
                    status = http_request(s, b"GET /sleep?ms=500 HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")[0]
            except Exception:  # noqa: BLE001
                status = -1
            with lock:
                results.append((status, time.time() - start))

        threads = [threading.Thread(target=one) for _ in range(10)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        codes = [s for s, _ in results]
        check(codes.count(200) >= 1, f"at least one request was served (codes: {sorted(codes)})")
        check(codes.count(503) >= 5, "most requests got 503 Service Unavailable")
        fast_503 = all(t < 0.3 for s, t in results if s == 503)
        check(fast_503, "each 503 came back quickly, without waiting for a worker")
        check(server.alive(), "server process is still running")
    check.finish()


if __name__ == "__main__":
    main()
