# Fuzz targets (HTTP-5)

Each `.cpp` here is one libFuzzer program, built only with the `fuzz` preset
(it needs clang: `sudo apt install clang`):

```bash
cmake --preset fuzz
cmake --build --preset fuzz --target fuzz_http_parser
./build/fuzz/fuzz_http_parser -max_total_time=300 fuzz/corpus/
```
