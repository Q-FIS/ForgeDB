"""L3 test: counter and queue_test.   python3 L3/test_l3.py <folder with counter and queue_test>"""
import os
import re
import subprocess
import sys

bindir = sys.argv[1] if len(sys.argv) > 1 else "build/debug/L3"
COUNTER, QUEUE = os.path.join(bindir, "counter"), os.path.join(bindir, "queue_test")
failures = 0


def check(ok: bool, what: str) -> None:
    global failures
    print(("  ok    " if ok else "  FAIL  ") + what)
    failures += not ok


def run(argv: list[str], timeout: int = 300) -> str:
    r = subprocess.run(argv, capture_output=True, text=True, timeout=timeout)
    if r.returncode != 0:
        return f"<exit code {r.returncode}: {r.stderr.strip()[-300:]}>"
    return r.stdout.strip()


print("counter")
for mode in ("mutex", "atomic"):
    for threads in (1, 4, 8):
        out = run([COUNTER, mode, str(threads), "100000"])
        check(out == str(threads * 100000), f"counter {mode} {threads} 100000 printed {out} (expected {threads * 100000})")
out = run([COUNTER, "none", "8", "100000"])
print(f"  info  counter none 8 100000 printed {out}: the right answer would be 800000")

print("queue_test")
PATTERN = re.compile(r"^produced (\d+) consumed (\d+) missing (\d+) duplicates (\d+)$")
for producers, consumers, items in ((1, 1, 1000), (4, 4, 10_000_000), (8, 2, 100_000), (2, 8, 100_000)):
    out = run([QUEUE, str(producers), str(consumers), str(items)])
    m = PATTERN.match(out)
    ok = bool(m) and m.groups() == (str(items), str(items), "0", "0")
    check(ok, f"queue_test {producers} {consumers} {items}: {out}")

if failures:
    print(f"FAILED: {failures} check(s)")
    sys.exit(1)
print("PASSED")
