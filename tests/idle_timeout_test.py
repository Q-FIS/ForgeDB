"""Week 4: idle connections are closed on time; busy ones are not.

    python3 tests/idle_timeout_test.py <http_server>

Started with --idle-timeout-ms 500. A connection that sends nothing must be
closed by the server within about a second. A connection that sends a request
every 200 ms must stay open.
"""

import os
import socket
import sys
import time

from harness import Check, Server, free_port, http_request


def main() -> None:
    server_bin = sys.argv[1]
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "www")
    port = free_port()
    argv = [server_bin, "--port", str(port), "--root", root, "--idle-timeout-ms", "500"]
    check = Check()
    with Server(argv, port, "http_server") as server:
        idle = socket.create_connection(("127.0.0.1", port), timeout=3)
        start = time.time()
        try:
            closed = idle.recv(1) == b""
        except (socket.timeout, ConnectionResetError):
            closed = False
        waited = time.time() - start
        check(closed and 0.4 < waited < 1.5, f"an idle connection was closed after {waited:.2f} s (expected ~0.5 s)")
        idle.close()

        busy = socket.create_connection(("127.0.0.1", port), timeout=3)
        ok = True
        for _ in range(8):  # 8 x 200 ms = 1.6 s, longer than the timeout
            try:
                ok = ok and http_request(busy, b"GET /health HTTP/1.1\r\nHost: x\r\n\r\n")[0] == 200
            except (ConnectionError, OSError):
                ok = False
            time.sleep(0.2)
        check(ok, "a connection that keeps sending requests stays open")
        busy.close()
        check(server.alive(), "server process is still running")
    check.finish()


if __name__ == "__main__":
    main()
