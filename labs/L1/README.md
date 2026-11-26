# L1 · Talking to the kernel
**Week 1 · C, then a little C++ · about 8 hours · in your own labs repo**

## What is this, in plain words?
Your program can't touch a file, the screen or the network by itself. Only the
**kernel** (the core of the operating system) can. When your program wants
something, it asks the kernel with a **system call**. Reading a file is
`open`, then `read` again and again, then `close`. Printing is `write`.

`printf` and `fopen` are only helpers in the C library: underneath, they make
these same system calls. In this lab you skip the helpers and make the calls
yourself. You'll write your own `cat` and `wc`, measure what each system call
costs, and watch the kernel at work with `strace`. Then you rewrite `cat` in
C++ and learn the one C++ idea the whole week's build depends on: **RAII**.

## What you'll learn
By the end you can explain, without notes:

- [ ] what a **system call** is, and how it differs from a normal function call (user mode vs kernel mode)
- [ ] what a **file descriptor** is: a small number (0, 1, 2, 3…) that stands for an open file, and why 0, 1 and 2 are always standard input, output and error
- [ ] why `read` and `write` can do **less than you asked** (a "short read/write"), and how to handle it with a loop
- [ ] how errors work: a `-1` return, `errno`, `strerror`, `perror`, and exit codes
- [ ] why **buffer size** changes speed by 1000×, and why it stops mattering after a point
- [ ] what the **page cache** is (why reading a file the second time is faster)
- [ ] how to read `strace` output, and what a program does before `main` even starts
- [ ] **RAII** in C++: a class whose destructor frees the resource, and why such a class must be **move-only**
- [ ] 4 more system calls, in depth (the side quest), plus the 16 the others teach you

## What you'll make
All in `L1/` in your labs repo:

| File | What | Language |
| --- | --- | --- |
| `mycat.c` | your `cat` | C |
| `mywc.c` | your `wc` | C |
| `mycatpp.cpp` | your `cat` again, in C++, with an RAII class | C++20 |
| `NOTES.md` | your measurements, `strace` findings and the copy experiment | Markdown |
| `syscalls.md` + `side/` | your 4 system-call cards and their tiny programs (the side quest) | Markdown + C |

## Start

```bash
~/Finforge-CoreCS/labs/start_lab.sh L1 ~/qfis-labs     # copies this handout, the starter files and the test
cd ~/qfis-labs
cmake --preset debug && cmake --build --preset debug -j
python3 L1/test_l1.py build/debug/L1                   # every check FAILS for now: that's your job
```
Every time you change code: `cmake --build --preset debug -j`, then run the test again.

**Read first (45 minutes):**
- *Operating Systems: Three Easy Pieces* (OSTEP, free online), chapter 6, "Limited Direct Execution": sections 6.1 and 6.2 only. This explains what a system call really is.
- `man 2 open`, `man 2 read`, `man 2 write`. Read DESCRIPTION and **RETURN VALUE** of each. (The `2` means "section 2: system calls". `man 3 printf` is section 3: library functions.)

---

## Part 1 · `mycat` in C (about 1.5 hours)

**What it does:** `mycat [-b SIZE] [FILE...]` prints each file to standard output, in order. With no files, it copies standard input. `-b SIZE` sets the buffer size in bytes (default 4096).

**The rule:** only `open`, `read`, `write` and `close` for the data. No `printf`, `fopen`, `fread` or `fwrite`. (For error messages, `fprintf(stderr, ...)` is allowed.)

**Step by step** (the starter file already has these functions, empty):
1. **`write_all(fd, buf, n)`**: `write` can write fewer bytes than you asked. Loop: write what's left, move forward by what was written, until all `n` bytes are out. Return -1 if `write` fails.
2. **`copy_fd(in_fd, buf, size)`**: loop `read(in_fd, buf, size)`. It returns the number of bytes read, **0 at the end of the file**, or -1 on error. Hand exactly the bytes you read (not `size`!) to `write_all(1, ...)`.
3. **`main`**:
   - Read the arguments. If `argv[1]` is `-b`, turn `argv[2]` into a number with `strtol`. Not a number, or less than 1? Print a message and exit with code 2.
   - `malloc` the buffer. (Not an array on the stack: `-b 100000000` would overflow the stack.)
   - No files left? `copy_fd(0, ...)`: fd 0 is standard input.
   - For each file: `open(path, O_RDONLY)`. If that fails, print `mycat: <path>: <reason>` to stderr (`strerror(errno)` gives the reason), remember that something failed, and **carry on with the next file**. Otherwise `copy_fd`, then `close`.
   - `free` the buffer. Exit with 1 if anything failed, else 0.

**Try it by hand:**
```bash
./build/debug/L1/mycat L1/README.md | head          # should print this handout
echo hello | ./build/debug/L1/mycat                 # standard input
./build/debug/L1/mycat nope L1/README.md; echo "exit code: $?"
./build/debug/L1/mycat /etc                         # a directory: open works, but read fails. Did you handle it?
```

## Part 2 · `mywc` in C (about 45 minutes)

**What it does:** `mywc FILE` prints `<lines> <words> <bytes> <file>`, like `wc FILE`.

- **Lines** = the number of `\n` bytes. (A last line with no `\n` doesn't count: real `wc` does the same.)
- **Words** = runs of non-whitespace bytes. Whitespace = space, `\t`, `\n`, `\r`, `\v`, `\f`.
- **Bytes** = the total `read` returned.

**The trick for words:** keep one variable, `in_word` (0 or 1). For each byte: if it's whitespace, set `in_word = 0`. If it isn't, and `in_word` was 0, a new word starts: count it, and set `in_word = 1`. This works even when a word is split between two `read` calls. (Why does that matter? Think about it: TCP gives you the same problem in the build.)

Format the numbers with `snprintf` into a small array, then `write_all` it to fd 1.

Check it against the real thing: `wc L1/README.md` and `./build/debug/L1/mywc L1/README.md` should agree.

## Part 3 · `mycatpp`: the same program in C++ (about 2 hours)

This part is **this week's C++**. Your build issue uses exactly this idea, on sockets.

**The problem with C:** every `open` needs a `close`, on every path out of the
function, including the error paths. Forget one, and you leak a file descriptor.
A real server that leaks one per client dies after about 1,000 clients.

**The C++ fix, RAII** ("Resource Acquisition Is Initialization"): put the
`int` inside a class whose **destructor** calls `close`. C++ runs the destructor
automatically whenever the object goes out of scope: at the end of the
function, after a `return`, even when an exception is thrown. You can't forget.

The starter `mycatpp.cpp` declares the class. You write the bodies:

```cpp
class Fd {
public:
    Fd() = default;                         // empty: holds no file (fd_ = -1)
    explicit Fd(int fd) : fd_(fd) {}        // takes ownership of an open fd
    ~Fd();                                  // closes fd_, if it's open
    Fd(Fd&& other) noexcept;                // move: take other's fd, leave other empty
    Fd& operator=(Fd&& other) noexcept;     // move-assign: close yours first, then take other's
    Fd(const Fd&) = delete;                 // no copies (step 5 shows why)
    Fd& operator=(const Fd&) = delete;
    int get() const { return fd_; }
private:
    int fd_ = -1;
};
```

**Step by step:**
1. The destructor: `if (fd_ != -1) ::close(fd_);`
2. The move constructor: take `other.fd_`, and set `other.fd_ = -1`. Now only one object owns the fd. (`std::exchange(other.fd_, -1)` does both in one line.) The move assignment: if `this != &other`, close your own fd (if any), then take the other's the same way.
3. `Fd open_read(const char* path)`: call `::open`. On failure, `throw std::system_error(errno, std::generic_category(), path);`. On success, return `Fd(fd)`.
4. Rewrite `mycat`:
   - the buffer is a `std::vector<char>` of the chosen size (no `malloc`, no `free`);
   - `write_all` takes a `std::span<const char>` (a pointer and a length in one object);
   - read `-b` with `std::from_chars`, which tells you when the text isn't a number (unlike `atoi`, which quietly returns 0);
   - for each file, put a `try { ... } catch (const std::system_error& e) { ... }` around it. Print `mycat++: ` followed by `e.what()` to `std::cerr`, and carry on. Notice: **there's no `close` anywhere in `main`**.
5. **The copy experiment.** Why `= delete` the copy? Find out by breaking it:
   - Temporarily, replace the two `= delete` lines with a copy constructor that simply copies `fd_`.
   - In `main`, right after opening a file, write `Fd copy = f;`.
   - Build, and run `strace -e trace=openat,close ./build/debug/L1/mycatpp L1/README.md > /dev/null`.
   - You'll see the same fd closed twice, and the second `close` fails with `EBADF`. In a server, between those two closes, the kernel could have given that fd number to a **new client**, and the second `close` would hang up on them. Write this down in `NOTES.md`, then undo the change.

**Learn for this part:** on learncpp.com, the lesson on destructors (chapter 15, "More on classes"), and chapter 22, "Move semantics and smart pointers": lessons 22.1 to 22.4. On cppreference: `std::span`, `std::from_chars`, `std::system_error`.

## Part 4 · Measure (about 45 minutes)

Measure with the **release** build: no sanitizers, so the timings are real.

```bash
cmake --preset release && cmake --build --preset release -j
head -c 100M /dev/urandom > /tmp/big          # a 100 MB file
head -c 10M  /dev/urandom > /tmp/small        # 10 MB, for the 1-byte test
time ./build/release/L1/mycat -b 4096 /tmp/big > /dev/null
```
Do this for buffer sizes **1, 512, 4096, 65536 and 1048576**. A 1-byte buffer
makes 100 million `read` calls on the big file: use `/tmp/small` for it, and
multiply the time by 10.

Before you count the `read` calls, **predict** how many there should be (file size ÷ buffer size, plus one more: why?). Then check:
```bash
strace -c -e trace=read,write ./build/release/L1/mycat -b 4096 /tmp/big > /dev/null
```
Fill in the table in `NOTES.md`, and answer:
1. Why is a 1-byte buffer so slow? Use your table to estimate what one system call costs, in nanoseconds.
2. Why does it stop getting faster after some size?
3. Run the same command twice. Is the first run slower? Why? (Look up "page cache".)

## Part 5 · Watch the kernel (about 30 minutes)

```bash
strace ./build/release/L1/mycat /etc/hostname
```
Find **your** calls: the `openat` of `/etc/hostname`, the `read`s, the `write`, the `close`. (`open` shows up as `openat`: the modern version of the same call.)

Everything before them is the program **starting up**: `execve` loads it, then the loader opens and maps the C library (`openat`, `mmap`, `mprotect`) and sets up memory (`brk`). Count them. In `NOTES.md`, explain in 5 lines what you saw.

Then compare with the real `cat`: `strace cat /etc/hostname`. Does it do anything differently from yours?

## Part 6 · Side quest: 4 system calls behind the commands you use (about 2 hours)

There are five themes and ten of us, so each theme has two people: do yours
alone, then compare cards with your theme-mate. Together we cover 20 system
calls, and everyone learns from everyone. **Claim a theme in the channel: at
most two people per theme, first come, first served.**

| # | Theme | Your 4 system calls | Where to catch them |
| --- | --- | --- | --- |
| 1 | How files really work | `stat`, `lseek`, `pread`, `fsync` | `stat file`, `tail file`, `strace /bin/true` (the loader), `dd if=/dev/zero of=x bs=1M count=1 conv=fsync` |
| 2 | What `ls`, `mkdir`, `mv`, `rm` do | `getdents64`, `mkdir`, `rename`, `unlink` | `ls`, `mkdir d`, `mv a b`, `rm b` |
| 3 | Where memory comes from | `brk`, `mmap`, `munmap`, `mprotect` | `strace /bin/true`, and your own program that `malloc`s 100 bytes, then 1 MB |
| 4 | Time and identity | `clock_gettime`, `nanosleep`, `getpid`, `uname` | `date`, `sleep 1`, `uname -a`, your own program |
| 5 | Copying and devices | `sendfile`, `copy_file_range`, `ioctl`, `getrandom` | `cp bigfile copy`, `stty size`, `python3 -c "import os; os.urandom(8)"`, your own program |

Real programs often use a **newer version** of a call: `statx` for `stat`,
`unlinkat` for `unlink`, `renameat2` for `rename`, `clock_nanosleep` for
`nanosleep`. Finding that out is part of the job. And if a call **doesn't show
up at all** where you expected it (try `date` with theme 4), find out why: that's
the best kind of finding.

**For each of your 4 calls, write a card in `syscalls.md`** (the starter has the template):
1. **What it does**, in one sentence of your own words.
2. **Its signature**, from `man 2 <name>`.
3. **Caught in the wild:** the command you ran, and the `strace` line where it appeared. Explain the arguments.
4. **My tiny program:** under 30 lines of C that calls it and prints something. Put it in `L1/side/<name>.c` (it's built automatically, as `build/debug/L1/side_<name>`), and paste its output on the card.
5. **One surprise.**

At the weekly meeting you present **your best card** in 2 minutes. Afterwards, read the other cards: by the end of the week you know 20 system calls, not 4.

---

## The test
```bash
python3 L1/test_l1.py build/debug/L1
```
It runs `mycat`, `mycatpp` and `mywc` on 10 files (empty, binary, no final newline, 5 MB, only newlines, Hindi and ₹ text…) and compares the output with the right answer. It also checks standard input, two files in a row, a missing file, and a bad `-b`. It uses the **debug** build on purpose: AddressSanitizer stops your program on any memory bug.

## Done when
- [ ] `python3 L1/test_l1.py build/debug/L1` prints `PASSED`
- [ ] `NOTES.md` is filled in: the table and its 3 answers, your `strace` story, the copy experiment
- [ ] `syscalls.md` has your 4 cards, and `L1/side/` has the 4 tiny programs
- [ ] Committed and pushed to your labs repo. Nobody reviews labs: the test and the show-and-tell are the check.

## Show-and-tell at the weekly meeting (3 minutes, plus 2 for your card)
Show your table and say why the curve looks like that; show one `strace` line
that surprised you; then present your best system-call card.

## Stuck?
| You see | Probably |
| --- | --- |
| Output is right for small files, wrong for big ones | You write `size` bytes instead of the number `read` returned |
| `mywc` word count is a little off on the 5 MB file | A word split across two `read`s is counted twice: keep `in_word` **outside** the read loop |
| `mycat` hangs with no file arguments | It's waiting for standard input: type something and press Ctrl-D |
| `ERROR: AddressSanitizer: heap-buffer-overflow` | You read or wrote past the end of your buffer. The report names the line: read it from the top |
| `ERROR: AddressSanitizer: stack-overflow` | A huge array on the stack: use `malloc` (C) or `std::vector` (C++) |
| `mycatpp`: "use of deleted function" | You tried to **copy** an `Fd`. Pass it as `const Fd&`, or `std::move` it |

Stuck for an hour? Ask in the channel, with what you tried and the exact error.

## Extra (optional, only if you have time)
- Write `mycat_stdio.c` with `fopen`/`fread`/`fwrite`, and time it reading 1 byte at a time. Why is it so much faster than `mycat -b 1`? (Look up user-space buffering, and count its `read` calls with `strace -c`.)
- Make `mycat` use `sendfile` when the output is a file. How much faster does it get, and why? (Look up "zero-copy".)

## Read more
- OSTEP chapter 39, "Interlude: Files and Directories".
- *The Linux Programming Interface* (Kerrisk), chapters 3 to 5, if you can get it: the best book on this whole topic.
