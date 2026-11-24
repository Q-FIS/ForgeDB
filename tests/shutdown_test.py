"""Graceful shutdown: on SIGTERM, finish the requests in progress, then exit 0.

    python3 tests/shutdown_test.py <http_server>

Starts 5 requests to GET /sleep?ms=1000, sends SIGTERM 200 ms later, and checks
that all 5 still get 200, that the server exits with code 0 within 3 seconds,
and that it stopped accepting new connections.
"""

import os
import signal
import socket
import sys
import threading
import time

from harness import Check, Server, free_port, http_request


def main() -> None:
    server_bin = sys.argv[1]
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "www")
    port = free_port()
    check = Check()
    with Server([server_bin, "--port", str(port), "--root", root], port, "http_server") as server:
        codes: list[int] = []

        def slow() -> None:
            try:
                with socket.create_connection(("127.0.0.1", port), timeout=10) as s:
                    codes.append(http_request(s, b"GET /sleep?ms=1000 HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")[0])
            except Exception:  # noqa: BLE001
                codes.append(-1)

        threads = [threading.Thread(target=slow) for _ in range(5)]
        for t in threads:
            t.start()
        time.sleep(0.2)
        server.proc.send_signal(signal.SIGTERM)
        for t in threads:
            t.join()
        try:
            code = server.proc.wait(timeout=3)
        except Exception:  # noqa: BLE001
            code = None
        check(codes == [200] * 5, f"all 5 requests in progress finished with 200 (got {codes})")
        check(code == 0, f"the server exited with code 0 within 3 s (got {code})")
        try:
            socket.create_connection(("127.0.0.1", port), timeout=1).close()
            refused = False
        except OSError:
            refused = True
        check(refused, "no new connections are accepted after shutdown")
    check.finish()


if __name__ == "__main__":
    main()
