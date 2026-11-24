# L1 · Talking to the kernel: `mycat` and `mywc`
**Week 1 · C · about 4 hours**

## What is this, in plain words?
Every program that reads a file asks the **kernel** to do it, through **system
calls**: `open`, `read`, `write`, `close`. In this lab you use nothing else: no
`printf`, no `fopen`. You'll see exactly how much work one system call is, and
why the size of your buffer matters enormously.

## Build
1. **`mycat [-b SIZE] [FILE...]`**: print each file's bytes to standard output (fd 1), in order. With no files, copy standard input (fd 0). `-b SIZE` sets the buffer size in bytes (default 4096).
   - `open(path, O_RDONLY)`, then loop `read(fd, buf, size)` until it returns 0, and `write` everything you read to fd 1. **`write` may write fewer bytes than asked: loop until it's all written.**
   - A file that can't be opened: print `mycat: <file>: <reason>` to stderr (use `strerror(errno)`), keep going with the other files, and exit with code 1 at the end.
2. **`mywc FILE`**: print `<lines> <words> <bytes> <file>`. Lines = the number of `\n` bytes. Words = runs of non-whitespace bytes (whitespace = space, `\t`, `\n`, `\r`, `\v`, `\f`). Same `read` loop; count as you go.
   - You may use `snprintf` to format the numbers, then `write` the result.

## Measure
Make a 1 GB file: `head -c 1G /dev/urandom > /tmp/big`. Then, with the **release** build:
- time `mycat -b <size> /tmp/big > /dev/null` for sizes 1, 512, 4096, 65536 and 1048576 (`time`, or `hyperfine` if you have it);
- count the system calls: `strace -c -f ./mycat -b 4096 /tmp/big > /dev/null`. (1-byte buffers make a billion calls: try it on a 10 MB file instead.)

Make a table of size → seconds → number of `read` calls, and explain the curve. Why does it stop improving after a certain size?

## Observe
`strace ./mycat /etc/hostname`: find the `openat`, `read`, `write` and `close` calls for **your** code, among everything else the program does at startup. What are all those other calls?

## The test
```bash
python3 L1/test_l1.py build/debug/L1
```
It compares your programs with the right answer on 10 files: empty, binary, no final newline, 5 MB, only newlines, and more.

## Done when
- [ ] The test passes on the debug build (with AddressSanitizer)
- [ ] `NOTES.md` has your table, the explanation, and your `strace` observation

## Extra
Write `mycat_stdio` with `fopen`/`fread`/`fwrite`, and time it with a 1-byte buffer. Why is it so much faster than `mycat -b 1`? (Look up user-space buffering.)

## Read
- *Operating Systems: Three Easy Pieces*, chapter 39 ("Files and Directories").
- `man 2 read`, `man 2 write`. Read the RETURN VALUE sections carefully.
