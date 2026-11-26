# Benchmarks

Every `<name>.cpp` here is built as the program `bench_<name>` (no CMake edits).
Measure with the **release** preset only: sanitizers make code 2-10x slower.

```bash
cmake --preset release
cmake --build --preset release --target bench_flat_index
./build/release/bench_flat_index
```
