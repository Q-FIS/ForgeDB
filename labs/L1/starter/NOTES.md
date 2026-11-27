# L1 notes

## Measurement (release build)
| Buffer size | Time (s) | read() calls: predicted | read() calls: strace |
| --- | --- | --- | --- |
| 1 B (10 MB file, ×10) | | | |
| 512 B | | | |
| 4 KB | | | |
| 64 KB | | | |
| 1 MB | | | |

1. Why is a 1-byte buffer so slow? What does one system call cost, roughly (ns)?

2. Why does it stop getting faster after some size?

3. First run vs second run: what changed, and why?

## What the kernel did (5 lines: your strace output, explained)

## The copy experiment (what happened when Fd could be copied)

## One thing that surprised me
