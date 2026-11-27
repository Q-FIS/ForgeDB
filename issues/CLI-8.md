# [CLI-8] A read replica: copy the primary by following its log
labels: comp:cli, size:L
milestone: v2 Stretch

**Component:** CLI (touches WAL's and APP's code: both review) · **Size:** L (1-2 weeks) · **Needs:** CLI-7, WAL-5, SNAP-4
**Your code:** `src/cli/replica.cpp`, a new endpoint in `src/app/api.cpp`, `docs/API.md` (agreed with the lead) · **Tests:** `tests/blackbox/replica_test.py` (replaces the placeholder)

## What is this, in plain words?
One server is one machine that can fail, and one machine's worth of search
capacity. A **replica** is a second `forgedb` that copies everything the
primary writes, a moment later, and answers searches too. The cleanest way to
copy a database is to ship its **write-ahead log**: the replica asks "give me
the log from byte X", applies the records exactly as recovery does, and asks
again. This is how PostgreSQL streaming replication and Redis replicas work.

## What you must give back
1. On the primary, two new endpoints (in `docs/API.md`):
   - `GET /v1/log?from=<offset>`: the raw log bytes from `offset` (whole records only, at most 4 MiB), plus headers with the log's current size and a **log id** that changes at every checkpoint. If there's nothing new, wait up to 1 s for new records before answering (long polling). An offset from before the last checkpoint: **410 Gone**.
   - `GET /v1/snapshot`: the current snapshot file, plus the log id and offset it corresponds to.
2. `forgedb --replica-of HOST:PORT`: starts by downloading the snapshot, then follows the log with `GET /v1/log`, applying each record with the same code recovery uses (no copy of that logic!). It answers `GET` and search requests; a `POST /v1/chunks` gets **409** with `{"error": "read-only replica; write to HOST:PORT"}`. On 410 (the primary checkpointed past it), it downloads a fresh snapshot and starts again.
3. `/health` on a replica adds `"replica_of"` and `"lag_bytes"`.
4. `replica_test.py`: puts on the primary appear on the replica within 2 s; a replica started late catches up; a checkpoint on the primary doesn't break the replica; killing and restarting the replica resumes where it left off (it saves its offset durably).
5. `docs/experiments/CLI-8-replication.md`: replication lag (ms) under 1,000 puts/s, and search throughput with one primary vs primary + one replica.

## Watch out for
- The replica must apply records **in order**, exactly once: save "applied up to offset O of log id L" durably, and only advance it after the record is applied.
- What a client reads from a replica may be slightly old ("eventual consistency"). Write in `docs/API.md` what is and isn't promised.
- This is the biggest issue in the project. Agree the endpoint design with the lead, the WAL owner and the APP owner **before** writing code, in this issue's comments.

## Done when
- [ ] `replica_test.py` passes on `debug` and `tsan`; the report is merged and presented

## Learn first
- *Designing Data-Intensive Applications*, chapter 5 ("Replication"): leaders and followers, replication lag, "reading your own writes".
- PostgreSQL docs: "Log-Shipping Standby Servers", the overview.
