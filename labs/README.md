# Labs: done alone, in your own GitHub repo

Labs are yours: your code, your repo, your GitHub profile. This folder only
holds the handouts, the starter files and the tests.

## Once: make your labs repo

1. On GitHub, create a new repo on **your own account**, e.g. `qfis-labs` (public or private, your choice).
2. Set it up from the template, and push:
   ```bash
   git clone https://github.com/<you>/qfis-labs.git ~/qfis-labs
   cp -r ~/Finforge-CoreCS/labs/template/. ~/qfis-labs/
   cd ~/qfis-labs && git add -A && git commit -m "Set up labs repo" && git push
   ```

## Every week: start the lab

```bash
~/Finforge-CoreCS/labs/start_lab.sh L1 ~/qfis-labs
```
This copies the handout (`README.md`), the starter files and the test into
`~/qfis-labs/L1/`. Then:

```bash
cd ~/qfis-labs
cmake --preset debug && cmake --build --preset debug -j     # build every lab
python3 L1/test_l1.py build/debug/L1                         # run the lab's test
```

**Done = the test passes, and `NOTES.md` is filled in.** Commit and push. No review.
Then, at the weekly meeting, show it for 3 minutes: your measurement, your
`strace` (or Wireshark) output, and one thing that surprised you.

## The labs

| Lab | Week | Language | What | Test ready |
| --- | --- | --- | --- | --- |
| L1 | 1 | C, then C++ | `mycat`, `mywc` and `mycatpp`: talking to the kernel, RAII, 4 system calls of your own | yes |
| L2 | 2 | C | `qsh`: a Unix shell | yes |
| L3 | 3 | C++ | Threads: races, mutexes, atomics, a bounded queue | yes |
| L4 | 4 | C++ | A chat server with `poll()` | yes |
| L5 | 5 | C | Your own `malloc` | before week 5 |
| L6 | 6 | C++ | How the CPU runs your code: caches | before week 6 |
| L7 | 7 | C | `qdig`: a DNS resolver | before week 7 |
| L8 | 8 | C++ | Processes talking: an IPC benchmark | before week 8 |

## The rules for labs
- **You write the code.** AI may explain and review, not write. This is where you learn.
- **Check every system call**, and print the error with `perror` (in C) when one fails.
- **Look at what the kernel did:** every lab's `NOTES.md` has a 5-line section about your `strace` or Wireshark output.
