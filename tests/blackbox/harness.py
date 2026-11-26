"""Shared helpers for the black-box tests (Python standard library only).   (owner: lead)

Every test starts a server, talks to it over real sockets, and checks what
comes back. A test gets the server to use as its first argument:

    python3 tests/blackbox/smoke_test.py fake                    # tools/fake_server.py
    python3 tests/blackbox/smoke_test.py build/debug/forgedb     # the real one

If the program is still a placeholder (it exits with code 77), the test
reports SKIPPED instead of FAILED.
"""

import json
import os
import resource
import signal
import socket
import subprocess
import sys
import tempfile
import time

SKIP = 77
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FAKE_SERVER = os.path.join(ROOT, "tools", "fake_server.py")


def skip(reason: str) -> None:
    print(f"SKIPPED: {reason}")
    sys.exit(SKIP)


def fail(reason: str) -> None:
    print(f"FAILED: {reason}")
    sys.exit(1)


def free_port() -> int:
    """Ask the kernel for a free TCP port on localhost."""
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def raise_fd_limit(wanted: int = 8192) -> None:
    """Allow this test (and the server it starts) to open many sockets."""
    soft, hard = resource.getrlimit(resource.RLIMIT_NOFILE)
    resource.setrlimit(resource.RLIMIT_NOFILE, (min(wanted, hard), hard))


def server_argv(server: str, port: int, dim: int, data_dir: str, *extra: str) -> list[str]:
    """The command line that starts `server` ("fake" or a path to forgedb)."""
    program = [sys.executable, FAKE_SERVER] if server == "fake" else [server]
    return program + ["--port", str(port), "--dim", str(dim), "--data-dir", data_dir, *extra]


def forgedb(server: str, dim: int = 4, *extra: str, data_dir: str | None = None) -> "Server":
    """Start ForgeDB ("fake" or a path) on a free port, with a fresh data directory
    (or `data_dir`, to reuse one). Use it with `with`:

        with forgedb(sys.argv[1], dim=4) as db:
            status, body = db.call("GET", "/health")
    """
    port = free_port()
    if data_dir is None:
        data_dir = tempfile.mkdtemp(prefix="forge-data-")
    name = "fake server" if server == "fake" else os.path.basename(server)
    srv = Server(server_argv(server, port, dim, data_dir, *extra), port, name)
    srv.data_dir = data_dir
    return srv


class Server:
    """Start a server program, wait until it accepts connections, stop it at the end.

        with Server([path, "--port", str(port)], port) as server:
            ...talk to 127.0.0.1:port...
    """

    def __init__(self, argv: list[str], port: int, name: str = "server"):
        self.argv = argv
        self.port = port
        self.name = name
        self.log = tempfile.NamedTemporaryFile(prefix=f"{name}-", suffix=".log", delete=False)

    def __enter__(self) -> "Server":
        self.proc = subprocess.Popen(
            self.argv, stdout=subprocess.DEVNULL, stderr=self.log, start_new_session=True
        )
        deadline = time.time() + 10
        while time.time() < deadline:
            if self.proc.poll() is not None:
                if self.proc.returncode == SKIP:
                    skip(f"{self.name} is not implemented yet")
                fail(f"{self.name} exited with code {self.proc.returncode}:\n{self.stderr()}")
            try:
                socket.create_connection(("127.0.0.1", self.port), timeout=0.2).close()
                return self
            except OSError:
                time.sleep(0.05)
        self.stop()
        fail(f"{self.name} did not accept connections on port {self.port} within 10 s")

    def alive(self) -> bool:
        return self.proc.poll() is None

    def stderr(self) -> str:
        with open(self.log.name, errors="replace") as f:
            return f.read()[-4000:]

    def stop(self) -> None:
        if self.proc.poll() is None:
            self.proc.send_signal(signal.SIGTERM)
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()

    def __exit__(self, exc_type, exc, tb) -> None:
        self.stop()
        log = self.stderr()
        if "Sanitizer" in log and ("ERROR" in log or "WARNING" in log):
            fail(f"{self.name}: a sanitizer found a bug:\n{log}")
        os.unlink(self.log.name)

    def call(self, method: str, path: str, body=None, timeout: float = 10) -> tuple[int, object]:
        """One request on a new connection. `body` (a dict or list) is sent as
        JSON. Returns (status, parsed JSON body, or the raw text if not JSON)."""
        with socket.create_connection(("127.0.0.1", self.port), timeout=timeout) as s:
            status, _, data = http_request(s, build_request(method, path, body, keep_alive=False))
            return status, parse_body(data)


def build_request(method: str, path: str, body=None, keep_alive: bool = True) -> bytes:
    """The raw bytes of one HTTP/1.1 request, with `body` as JSON."""
    payload = b"" if body is None else json.dumps(body).encode()
    head = f"{method} {path} HTTP/1.1\r\nHost: localhost\r\n"
    if body is not None:
        head += "Content-Type: application/json\r\n"
    head += f"Content-Length: {len(payload)}\r\n"
    if not keep_alive:
        head += "Connection: close\r\n"
    return (head + "\r\n").encode() + payload


def parse_body(data: bytes):
    try:
        return json.loads(data)
    except ValueError:
        return data.decode("utf-8", errors="replace")


def http_request(sock: socket.socket, raw: bytes) -> tuple[int, dict[str, str], bytes]:
    """Send one raw HTTP request on `sock` and read one response."""
    sock.sendall(raw)
    return read_response(sock)


def read_response(sock: socket.socket) -> tuple[int, dict[str, str], bytes]:
    data = b""
    while b"\r\n\r\n" not in data:
        part = sock.recv(65536)
        if not part:
            raise ConnectionError(f"connection closed before a full response; got {data[:200]!r}")
        data += part
    head, _, rest = data.partition(b"\r\n\r\n")
    lines = head.decode("latin-1").split("\r\n")
    parts = lines[0].split(" ", 2)
    if len(parts) < 2 or not parts[0].startswith("HTTP/1.") or not parts[1].isdigit():
        raise ValueError(f"bad status line: {lines[0]!r}")
    headers = {}
    for line in lines[1:]:
        name, _, value = line.partition(":")
        headers[name.strip().lower()] = value.strip()
    length = int(headers.get("content-length", "0"))
    body = rest
    while len(body) < length:
        part = sock.recv(65536)
        if not part:
            raise ConnectionError("connection closed in the middle of the response body")
        body += part
    return int(parts[1]), headers, body[:length]


def get(port: int, path: str, timeout: float = 5) -> tuple[int, dict[str, str], bytes]:
    """One GET request on a new connection."""
    with socket.create_connection(("127.0.0.1", port), timeout=timeout) as s:
        req = f"GET {path} HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
        return http_request(s, req.encode())


class Check:
    """Collects named checks, prints them, and exits non-zero if any failed."""

    def __init__(self) -> None:
        self.failures = 0

    def __call__(self, ok: bool, what: str) -> None:
        print(("  ok    " if ok else "  FAIL  ") + what)
        if not ok:
            self.failures += 1

    def finish(self) -> None:
        if self.failures:
            fail(f"{self.failures} check(s) failed")
        print("PASSED")
