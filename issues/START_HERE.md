# Start here: your first issue, step by step

Every issue goes through the same 8 steps:

```
find it → make a branch → write code → build & test → commit & push → open a PR → get a review → merge
```

Below, we follow one real issue all the way: **SNAP-1, the in-memory store**,
as if you owned SNAP. Every other issue works the same way.
(Labs are different, and simpler: see `labs/README.md`.)

---

## Before your first issue (once, about 20 minutes)

```bash
sudo apt install build-essential cmake gdb strace python3 netcat-openbsd
git clone https://github.com/Q-FIS/Finforge-CoreCS.git ~/Finforge-CoreCS
cd ~/Finforge-CoreCS
cmake --preset debug               # prepares the build (the first time it downloads GoogleTest and JSON)
cmake --build --preset debug -j    # compiles everything
ctest --preset debug               # runs every test
```

The last command ends like this:
```
100% tests passed out of 37

The following tests did not run:
      1 - contract.app.api (Skipped)
      2 - contract.app.config (Skipped)
      ...
     17 - contract.snap.memory_store (Skipped)
      ...
```
"Skipped" is good: those parts aren't written yet. **That's your job.**

---

## Step 1: Find your issue

Your issues are already assigned to you. On GitHub, open the **Issues** tab and
type in the search box:

`is:open assignee:@me`

You'll see your component's issues, e.g. `[SNAP-1] The in-memory store`. Do
them **in order**: SNAP-1, then SNAP-2... If one says **Needs: WAL-1** and
WAL-1 isn't merged yet, start what you can, or go on to the next one.

---

## Step 2: Read the whole issue (10 minutes)

Every issue has the same parts:

| Part | What it tells you |
| --- | --- |
| The first lines | Size, what it **Needs**, the **contract** (header), where **your code** goes, which **tests** check it |
| **What is this, in plain words?** | Why this task exists, and where it fits in ForgeDB |
| **What you must give back** | Exactly what to write |
| **Step by step** | How to do it, in order |
| **Watch out for** | The usual mistakes |
| **How to test it** | The commands |
| **Done when** | The checklist your reviewer uses |
| **Learn first** | What to read before writing code |

Do the **Learn first** part first: your reviewer may ask about it. Then read the
contract. For SNAP-1 that's `include/forge/store.hpp`: the comments there are the rules.

---

## Step 3: Make a branch

A branch is your own copy of the code, where you can work without disturbing anyone.

```bash
git switch main                       # go back to the main version
git pull                              # get everyone's latest work
git switch -c snap-1-memory-store     # your branch: <issue>-<short name>, lowercase
```

---

## Step 4: Switch on the tests, and create your file

Each unfinished part has a placeholder file called `stub_<part>.cpp`. For SNAP-1
it's `src/snap/stub_memory_store.cpp`. **Delete it, and create your own file:**

```bash
git rm src/snap/stub_memory_store.cpp       # removes the placeholder (and tells git)
touch src/snap/memory_store.cpp             # your code goes here
```

Deleting the stub switches on its tests (`tests/contract/snap/memory_store_test.cpp`):
they'll run, and fail, until your code is right. **You never edit a CMake file:**
any `.cpp` under `src/` is built automatically, and so is any test you add in
`tests/unit/snap/`.

---

## Step 5: Write a little, build, test, repeat

Don't write everything and then test. Write one function, then build and test.

```bash
cmake --build --preset debug -j                         # compile; read errors from the top
ctest --preset debug -R contract.snap.MemoryStore       # only your tests
ctest --preset debug                                    # everything (do this before Step 6)
```

What you'll see:
- `Passed`: it works.
- `***Failed`: add `--output-on-failure` to see which check failed, and why.
- A long report starting with `ERROR: AddressSanitizer`: a memory bug (reading past an array, using freed memory...). It shows the exact file and line: start reading at the first line that mentions **your** file.

If your code uses threads, also run the ThreadSanitizer build: `cmake --preset tsan`
once, then `cmake --build --preset tsan -j && ctest --preset tsan -R <your tests>`.

> **Stuck for an hour?** Ask your partner or the channel: what you tried, and the exact error. That's normal, not a failure.

---

## Step 6: Commit and push

```bash
git add -A                                        # include every changed file
git commit -m "SNAP-1: the in-memory store"       # save a snapshot, with a short message
git push -u origin snap-1-memory-store            # send your branch to GitHub
```
(If `git commit` complains about your name or email, run once:
`git config --global user.name "Your Name"` and `git config --global user.email you@example.com`.)

You can commit and push many times while you work. Small commits are good.

---

## Step 7: Open a pull request (PR)

A PR asks: "please add my branch to `main`".

1. On GitHub, a yellow banner appears: **Compare & pull request**. Click it.
2. The description starts with `Closes #`. Add the issue's number (from its URL), e.g. `Closes #46`. GitHub then closes the issue when the PR is merged.
3. Tick the checklist, and click **Create pull request**.
4. Wait for **CI** (the tests run on GitHub, on `debug` and `tsan`). A green ✓ means good. A red ✗: click **Details**, read the error, fix it on your branch, and push again. The PR updates by itself.

---

## Step 8: Get a review, then merge

1. Ask your **partner** to review (on the right, under **Reviewers**). If you changed files owned by someone else, GitHub asks them automatically too.
2. They read your code and may ask questions or request changes. Answer in the PR; to change something, edit, commit and push again.
3. When they **approve** and CI is green, click **Merge pull request**. Done!
4. Tell the channel what's now possible, and get ready for the next issue:
   ```bash
   git switch main && git pull
   ```

---

## That's all

Next issue: back to Step 1. Need a command, the folder layout, or a word
explained? See [`REFERENCE.md`](REFERENCE.md).
