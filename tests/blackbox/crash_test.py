"""kill -9 in the middle of writes: every acknowledged chunk survives.   (owner: WAL)

Placeholder: WAL-5 replaces this file with the real test. Until then it
reports SKIPPED. It is already registered in tests/blackbox/CMakeLists.txt,
so you only need to write it.
"""

import sys

print("SKIPPED: WAL-5 hasn't written this test yet")
sys.exit(77)
