"""L4 test: 50 clients chat at once.   python3 L4/test_l4.py <folder with chat_server>"""
import os
import socket
import subprocess
import sys
import threading
import time

bindir = sys.argv[1] if len(sys.argv) > 1 else "build/debug/L4"
SERVER = os.path.join(bindir, "chat_server")
CLIENTS, MESSAGES = 50, 20


def free_port() -> int:
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


port = free_port()
proc = subprocess.Popen([SERVER, "--port", str(port)], stderr=subprocess.PIPE)
deadline = time.time() + 10
while True:
    try:
        socket.create_connection(("127.0.0.1", port), timeout=0.2).close()
        break
    except OSError:
        if proc.poll() is not None or time.time() > deadline:
            print("FAILED: chat_server did not start listening")
            sys.exit(1)
        time.sleep(0.05)

socks = [socket.create_connection(("127.0.0.1", port), timeout=20) for _ in range(CLIENTS)]
for i, s in enumerate(socks):
    s.sendall(f"/nick c{i}\n".encode())
time.sleep(0.5)  # let every /nick be handled before anyone talks

received: list[list[str]] = [[] for _ in range(CLIENTS)]
expected_lines = (CLIENTS - 1) * MESSAGES


def reader(i: int) -> None:
    buf = b""
    s = socks[i]
    try:
        while len(received[i]) < expected_lines:
            part = s.recv(65536)
            if not part:
                break
            buf += part
            *lines, buf = buf.split(b"\n")
            received[i].extend(line.decode(errors="replace") for line in lines)
    except OSError:
        pass


def writer(i: int) -> None:
    for k in range(MESSAGES):
        socks[i].sendall(f"m{k}\n".encode())  # whole lines, but many at once: the server must split them


readers = [threading.Thread(target=reader, args=(i,)) for i in range(CLIENTS)]
writers = [threading.Thread(target=writer, args=(i,)) for i in range(CLIENTS)]
for t in readers + writers:
    t.start()
for t in writers + readers:
    t.join(timeout=60)

failures = 0
for i in range(CLIENTS):
    lines = received[i]
    if len(lines) != expected_lines:
        print(f"  FAIL  client c{i} received {len(lines)} lines, expected {expected_lines}")
        failures += 1
        continue
    per_sender: dict[str, list[str]] = {}
    bad = [line for line in lines if ": " not in line]
    for line in lines:
        name, _, text = line.partition(": ")
        per_sender.setdefault(name, []).append(text)
    want = [f"m{k}" for k in range(MESSAGES)]
    wrong = [n for n, texts in per_sender.items() if texts != want]
    if bad or f"c{i}" in per_sender or wrong or len(per_sender) != CLIENTS - 1:
        print(f"  FAIL  client c{i}: mixed-up or out-of-order lines (example: {(bad or wrong or ['own message echoed'])[0]!r})")
        failures += 1

alive = proc.poll() is None
for s in socks:
    s.close()
proc.terminate()
proc.wait(timeout=5)
if not alive:
    print("  FAIL  chat_server crashed")
    failures += 1
if failures:
    print(f"FAILED: {failures} problem(s)")
    sys.exit(1)
print(f"  ok    {CLIENTS} clients each received all {expected_lines} messages, complete and in order")
print("PASSED")
