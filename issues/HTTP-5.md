# [HTTP-5] Fuzz the parser with libFuzzer
labels: comp:http, size:M
milestone: v0.2 Survives a crash

**Component:** HTTP · **Size:** M · **Needs:** HTTP-1 (better after HTTP-4)
**Your code:** `fuzz/http_parser.cpp`, `fuzz/corpus/` · **Tests:** new regression tests in `tests/unit/http/`

## What is this, in plain words?
Your tests check the cases you thought of. A **fuzzer** checks millions you
didn't: it feeds the parser random bytes, watches which code each input
reaches (coverage), and mutates the interesting ones further. With
AddressSanitizer on, any out-of-bounds read becomes an immediate crash with
the exact input that caused it.

## What you must give back
1. `fuzz/http_parser.cpp`:
   ```cpp
   extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
       std::string_view bytes(reinterpret_cast<const char*>(data), size);
       auto r = forge::http::parse_request(bytes, {/*small limits*/ 1024, 4096});
       // Check what must ALWAYS be true, e.g.:
       //   complete -> consumed <= size, and parsing bytes.substr(0, consumed) gives the same request
       //   error    -> error_status is one of 400, 413, 431, 501
       return 0;
   }
   ```
2. A seed corpus `fuzz/corpus/`: 10-20 small real requests (one per file).
3. A run of at least 30 minutes with no crash, and a regression unit test for every bug it found.
4. A short `fuzz/FINDINGS.md`: what it found, and why each bug happened.

## Step by step
```bash
sudo apt install clang
cmake --preset fuzz
cmake --build --preset fuzz --target fuzz_http_parser
./build/fuzz/fuzz_http_parser -max_total_time=1800 fuzz/corpus/
```
When it crashes, it writes `crash-<hash>`: reproduce with `./build/fuzz/fuzz_http_parser crash-<hash>`, fix, add a unit test with those bytes, and run again.

## Watch out for
- The checks inside the fuzz target ("invariants") are what make fuzzing find logic bugs, not just crashes. Think about what must always hold.
- Don't commit huge generated corpora: only your seed files.

## Done when
- [ ] The fuzz target builds with the `fuzz` preset, and the corpus is committed
- [ ] 30 minutes of fuzzing with no crash (paste libFuzzer's last status line in the PR)
- [ ] `fuzz/FINDINGS.md` is written, even if it says "found nothing" (then explain why you trust that)

## Learn first
- The libFuzzer tutorial (llvm.org/docs/LibFuzzer.html): "Fuzz target", "Corpus", "Options".
- What does coverage-guided mean, and why is it so much better than purely random input?
