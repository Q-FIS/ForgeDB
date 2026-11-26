# [VEC-4] SIMD: 8 multiplications per instruction
labels: comp:vec, size:M
milestone: v1.0 Fast, and powers the RAG

**Component:** VEC · **Size:** M · **Needs:** VEC-2
**Your code:** `src/vec/dot.cpp` (+ a small header)

## What is this, in plain words?
A modern x86 CPU can multiply 8 floats in one AVX2 instruction, and add them
to 8 running sums in another (FMA). Write the dot product with these
**intrinsics** by hand, and compare with what the compiler did on its own.

## What you must give back
1. `float dot_avx2(const float* a, const float* b, std::size_t n)` using `_mm256_loadu_ps`, `_mm256_fmadd_ps`, and a horizontal sum at the end; a plain C++ `dot_scalar`; and a `dot` that picks AVX2 at **runtime** if the CPU has it (`__builtin_cpu_supports("avx2")` and `"fma"`). Use `__attribute__((target("avx2,fma")))` on the AVX2 function, so the rest of the program still runs on any x86 CPU.
2. Unit tests: `dot_avx2` and `dot_scalar` agree (within 1e-5, relative) for lengths 0 to 100, including lengths not divisible by 8.
3. The flat index uses `dot`. Numbers before/after from `bench_flat_index`.

## Watch out for
- Lengths not divisible by 8: handle the leftover elements with a scalar loop.
- Floating-point addition isn't associative: the SIMD sum adds in a different order, so results differ in the last bits. That's why the tests compare within a tolerance, and why ties may now break differently. Think about whether that matters (the contract tests will tell you).
- On ARM (e.g. Apple Silicon in a VM), there's no AVX2: the scalar path must still work.

## Done when
- [ ] Unit tests and contract tests pass; the PR has the speed-up numbers

## Learn first
- Intel Intrinsics Guide: look up `_mm256_fmadd_ps`. What does each lane do?
