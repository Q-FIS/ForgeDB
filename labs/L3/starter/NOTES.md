# L3 notes

## Measurement (release build, ITERS = 10,000,000)
| Mode | 1 thread | 2 threads | 4 threads | 8 threads |
| --- | --- | --- | --- | --- |
| none (wrong result!) | | | | |
| mutex | | | | |
| atomic | | | | |

Why mutex gets slower with more threads:

Why atomic is faster than mutex, but still slows down:

## The ThreadSanitizer report, explained

## What the kernel did (5 lines: clone and futex in your strace)

## One thing that surprised me
