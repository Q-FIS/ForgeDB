"""L2 test: qsh gives the same output as bash.   python3 L2/test_l2.py <folder with qsh>"""
import os
import subprocess
import sys
import tempfile

bindir = sys.argv[1] if len(sys.argv) > 1 else "build/debug/L2"
QSH = os.path.abspath(os.path.join(bindir, "qsh"))

SCRIPT = """\
echo hello world
echo one two three | wc -w
seq 1 10 | sort -r | head -3
seq 1 100 > numbers.txt
wc -l < numbers.txt
grep 7 numbers.txt | wc -l

echo first > out.txt
echo second >> out.txt
cat out.txt
mkdir sub
cd sub
pwd | xargs basename
echo inside > file.txt
cd ..
ls sub
cat < out.txt | tr a-z A-Z
seq 1 1000 | grep 99 | sort | tail -2
ls this_file_does_not_exist
echo still running after an error
seq 1 5 | tr -d 3 | wc -c
exit
echo this line must not run
"""


def run(shell: list[str]) -> tuple[str, int]:
    with tempfile.TemporaryDirectory() as d:
        r = subprocess.run(shell, input=SCRIPT, capture_output=True, text=True, cwd=d, timeout=30)
        return r.stdout, r.returncode


expected, _ = run(["bash"])
try:
    got, code = run([QSH])
except subprocess.TimeoutExpired:
    print("FAILED: qsh did not finish within 30 s (a pipeline that never ends? check you close every pipe end)")
    sys.exit(1)

exp_lines, got_lines = expected.splitlines(), got.splitlines()
failures = 0
for i, (e, g) in enumerate(zip(exp_lines, got_lines), 1):
    if e != g:
        print(f"  FAIL  output line {i}: bash printed {e!r}, qsh printed {g!r}")
        failures += 1
if len(exp_lines) != len(got_lines):
    print(f"  FAIL  bash printed {len(exp_lines)} lines, qsh printed {len(got_lines)}")
    failures += 1
if code != 0:
    print(f"  FAIL  qsh exited with code {code} after 'exit' (expected 0)")
    failures += 1
if failures:
    print("FAILED")
    sys.exit(1)
print(f"  ok    all {len(exp_lines)} lines of output match bash")
print("PASSED")
