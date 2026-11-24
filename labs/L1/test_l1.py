"""L1 test: mycat and mywc.   python3 L1/test_l1.py <folder with mycat and mywc>"""
import os
import random
import subprocess
import sys
import tempfile

random.seed(1)
bindir = sys.argv[1] if len(sys.argv) > 1 else "build/debug/L1"
MYCAT, MYWC = os.path.join(bindir, "mycat"), os.path.join(bindir, "mywc")
failures = 0


def check(ok: bool, what: str) -> None:
    global failures
    print(("  ok    " if ok else "  FAIL  ") + what)
    failures += not ok


FILES = {
    "empty": b"",
    "one_line_no_newline": b"hello world",
    "lines": b"first line\nsecond line\nthird\n",
    "only_newlines": b"\n" * 1000,
    "binary_with_nul": bytes(random.randrange(256) for _ in range(100_000)),
    "big_5mb": b"".join(random.choice([b"word ", b"line\n", b"tab\t", b"  "]) for _ in range(1_250_000)),
    "unicode": "₹1,45,000 crore — FY2025\nनमस्ते\n".encode(),
    "long_line_1mb": b"x" * 1_000_000,
    "spaces_and_tabs": b"  a \t b\t\tc   \n\n  d\r\ne\x0bf\x0cg",
    "exactly_4096": b"y" * 4096,
}

with tempfile.TemporaryDirectory() as d:
    paths = {}
    for name, data in FILES.items():
        paths[name] = os.path.join(d, name)
        with open(paths[name], "wb") as f:
            f.write(data)

    print("mycat")
    for name, data in FILES.items():
        r = subprocess.run([MYCAT, paths[name]], capture_output=True, timeout=30)
        check(r.returncode == 0 and r.stdout == data, f"mycat {name}")
    for size in ("1", "7", "1048576"):
        r = subprocess.run([MYCAT, "-b", size, paths["lines"]], capture_output=True, timeout=30)
        check(r.stdout == FILES["lines"], f"mycat -b {size}")
    r = subprocess.run([MYCAT, paths["lines"], paths["unicode"]], capture_output=True, timeout=30)
    check(r.stdout == FILES["lines"] + FILES["unicode"], "mycat with two files concatenates them")
    r = subprocess.run([MYCAT], input=b"from stdin\n", capture_output=True, timeout=30)
    check(r.stdout == b"from stdin\n", "mycat with no files copies standard input")
    r = subprocess.run([MYCAT, os.path.join(d, "missing"), paths["lines"]], capture_output=True, timeout=30)
    check(
        r.returncode != 0 and b"missing" in r.stderr and r.stdout == FILES["lines"],
        "a missing file: error on stderr, carries on with the next file, exit code not 0",
    )

    print("mywc")
    for name, data in FILES.items():
        want = (data.count(b"\n"), len(data.split()), len(data))
        r = subprocess.run([MYWC, paths[name]], capture_output=True, timeout=30)
        try:
            got = tuple(int(x) for x in r.stdout.split()[:3])
        except ValueError:
            got = None
        check(got == want, f"mywc {name}: expected {want}, got {got}")

if failures:
    print(f"FAILED: {failures} check(s)")
    sys.exit(1)
print("PASSED")
