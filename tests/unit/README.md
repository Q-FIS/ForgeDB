# Your unit tests

Put your own GoogleTest files in `tests/unit/<component>/` (e.g.
`tests/unit/wal/test_recovery.cpp`). Every `.cpp` in a component's folder is
built into one test program, `unit_<component>`, and run by `ctest`.
No CMake edits needed. Copy the shape of any file in `tests/contract/`.
