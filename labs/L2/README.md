# L2 · A Unix shell: `qsh`
**Week 2 · C · about 5 hours**

## What is this, in plain words?
A shell is a small program: read a line, start the programs it names, wire
their inputs and outputs together, and wait for them to finish. Writing one
teaches you how **every** program on Linux gets started: `fork` (copy the
process), `exec` (replace the copy with another program) and `wait` (collect it
when it ends), plus `pipe` and `dup2` to connect programs together.

## Build
`qsh` reads commands from standard input, one per line, until end of input or `exit`.
Print a prompt (`qsh> `) **only** if standard input is a terminal (`isatty(0)`), so the test's output stays clean.

1. **Run a command:** split the line on spaces into words, then `fork`. In the child, `execvp(words[0], words)`; if that fails, print the error and `_exit(127)`. In the parent, `waitpid` for the child. (No quotes or variables needed.)
2. **Built-ins:** `cd <dir>` (must run in the shell itself, not in a child: why?) and `exit`.
3. **Redirection:** `cmd > file`, `cmd >> file`, `cmd < file`. In the child, before `exec`: `open` the file, `dup2` it onto fd 1 (or fd 0), and `close` the original.
4. **Pipes:** `a | b | c`, any number of stages. Create a `pipe()` between each pair, fork every stage, `dup2` each pipe end onto the right fd, and **close every unused pipe end** in every process (otherwise the reader never sees end-of-file, and the pipeline hangs). Then wait for all the stages.
5. **Ctrl-C** stops the running command, but not the shell: the shell ignores `SIGINT`, and each child restores the default (`signal(SIGINT, SIG_DFL)`) before `exec`.
6. Empty lines are ignored. Errors (a command not found, a file that can't be opened) print a message and the shell carries on.

## Experiment
1. **A zombie:** write a 10-line C program that forks a child that exits at once, while the parent sleeps 30 s without calling `wait`. Find the child in `ps -el` (state `Z`). Why does it still exist? Fix it with `waitpid`, then again with a `SIGCHLD` handler.
2. **Observe:** `strace -f -e trace=process,pipe2,dup2,execve ./qsh` and type `ls | wc -l`. Find each `clone`/`fork`, the `pipe2`, the `dup2`s and the `execve`s.

## The test
```bash
python3 L2/test_l2.py build/debug/L2
```
It runs 20 command lines through both `bash` and your `qsh`, in empty folders, and compares what they print.

## Done when
- [ ] The test passes on the debug build
- [ ] Ctrl-C works as described (try it by hand: `sleep 10`, then Ctrl-C)
- [ ] `NOTES.md`: the zombie experiment and your `strace` observation

## Extra
Background jobs with `&`: don't wait, print the child's pid, and collect finished children with a `SIGCHLD` handler. Then process groups (`setpgid`), so Ctrl-C only reaches the foreground job.

## Read
- *Operating Systems: Three Easy Pieces*, chapter 5 ("Process API"). It builds a tiny shell too.
- `man 2 fork`, `man 3 exec`, `man 2 pipe`, `man 2 dup2`.
