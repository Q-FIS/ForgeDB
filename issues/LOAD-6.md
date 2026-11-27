# [LOAD-6] The final numbers
labels: comp:load, size:S, experiment
milestone: v1.0 Fast, and powers the RAG

**Component:** LOAD · **Size:** S · **Needs:** LOAD-3 (and the other experiments, for context)
**Your output:** `docs/experiments/LOAD-6-final-numbers.md`

## What is this, in plain words?
ForgeDB v1.0's report card, measured carefully, on one machine, in one
session, so the numbers can be compared.

## What you must give back
On a release build, with 100,000 chunks of dim 384 (CLI-4's real data if available):
| Workload | rps | p50 | p99 |
| --- | --- | --- | --- |
| health, 64 connections | | | |
| vector search k = 10, 16 / 64 / 256 connections | | | |
| keyword search, 64 connections | | | |
| hybrid search, 64 connections | | | |
| put, 8 connections, each `--fsync` policy | | | |

plus: the open-loop p99 at 50 % and 90 % of the maximum rate; startup (recovery) time; memory used. And a paragraph: the biggest bottleneck left, and how you know (perf, metrics).

## Done when
- [ ] The report is merged and is the centrepiece of the final presentation

## Learn first
- Re-read every other experiment report: your numbers should agree with theirs, or you should explain why not.
