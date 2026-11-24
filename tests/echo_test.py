"""Week 1: the echo server and client.

    python3 tests/echo_test.py <echo_server> <echo_client>

The server is started as:  <echo_server> --port <port>
The client is started as:  <echo_client> --host 127.0.0.1 --port <port> --file <path>
and must exit with code 0 when the echo matched the file, 1 when it did not.
"""

import hashlib
import os
import socket
import struct
import subprocess
import sys
import tempfile
import threading

from harness import Check, Server, free_port, SKIP


def roundtrip(port: int, data: bytes, piece: int | None = None) -> bytes:
    """Send `data` (all at once, or `piece` bytes at a time) and read the echo.
    Sending and receiving happen at the same time, as they must: if we sent
    100 MB before reading, both sides' buffers would fill up and block."""
    with socket.create_connection(("127.0.0.1", port), timeout=30) as s:

        def send() -> None:
            if piece is None:
                s.sendall(data)
            else:
                for i in range(0, len(data), piece):
                    s.sendall(data[i : i + piece])
            s.shutdown(socket.SHUT_WR)  # "I'm done sending"

        sender = threading.Thread(target=send)
        sender.start()
        received = bytearray()
        while True:
            part = s.recv(1 << 20)
            if not part:
                break
            received += part
        sender.join()
        return bytes(received)


def main() -> None:
    server_bin, client_bin = sys.argv[1], sys.argv[2]
    port = free_port()
    check = Check()
    with Server([server_bin, "--port", str(port)], port, "echo_server") as server:
        data = os.urandom(100 * 1024 * 1024)
        echo = roundtrip(port, data)
        check(
            hashlib.sha256(echo).digest() == hashlib.sha256(data).digest(),
            f"100 MB echoes back identically (got {len(echo):,} of {len(data):,} bytes)",
        )

        small = b"hello, one byte at a time\n" * 40
        check(roundtrip(port, small, piece=1) == small, "data sent 1 byte at a time echoes back")

        # A client that vanishes mid-message (RST instead of a clean close).
        # Writing to it gives the server EPIPE/ECONNRESET, or SIGPIPE if not handled.
        s = socket.create_connection(("127.0.0.1", port))
        s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
        s.sendall(os.urandom(1024 * 1024))
        s.close()
        check(roundtrip(port, b"still alive?") == b"still alive?", "server survives a client vanishing mid-message")
        check(server.alive(), "server process is still running")

        with tempfile.NamedTemporaryFile() as f:
            f.write(os.urandom(5 * 1024 * 1024))
            f.flush()
            run = subprocess.run(
                [client_bin, "--host", "127.0.0.1", "--port", str(port), "--file", f.name],
                capture_output=True, text=True, timeout=60,
            )
            if run.returncode == SKIP:
                print("  skip  echo_client is not implemented yet")
            else:
                check(run.returncode == 0, f"echo_client sends a 5 MB file and confirms the echo (exit code {run.returncode})")
    check.finish()


if __name__ == "__main__":
    main()
