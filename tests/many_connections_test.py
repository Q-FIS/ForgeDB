"""Week 3, Group B: 1,000 open connections, and the server still answers quickly.

    python3 tests/many_connections_test.py <http_server>

Opens 1,000 connections that send nothing, then checks that GET /health on
a new connection still answers in well under 100 ms. A design with one
thread per connection struggles here; an event loop shouldn't.
"""

import os
import socket
import sys
import time

from harness import Check, Server, free_port, get, raise_fd_limit


def main() -> None:
    raise_fd_limit()
    server_bin = sys.argv[1]
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "www")
    port = free_port()
    check = Check()
    with Server([server_bin, "--port", str(port), "--root", root], port, "http_server") as server:
        idle = []
        try:
            for _ in range(1000):
                idle.append(socket.create_connection(("127.0.0.1", port), timeout=5))
            check(True, "opened 1,000 idle connections")
        except OSError as e:
            check(False, f"could only open {len(idle)} connections: {e}")
        times = []
        for _ in range(20):
            start = time.time()
            ok = get(port, "/health")[0] == 200
            times.append(time.time() - start)
            if not ok:
                check(False, "GET /health failed while 1,000 connections were open")
                break
        times.sort()
        check(times[len(times) // 2] < 0.1, f"median GET /health took {times[len(times) // 2] * 1000:.1f} ms (must be < 100 ms)")
        for s in idle:
            s.close()
        check(server.alive(), "server process is still running")
    check.finish()


if __name__ == "__main__":
    main()
