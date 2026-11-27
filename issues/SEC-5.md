# [SEC-5] Threat model and security review
labels: comp:sec, size:M
milestone: v1.0 Fast, and powers the RAG

**Component:** SEC (with every owner) · **Size:** M · **Needs:** SEC-2, HTTP-5
**Your output:** `docs/SECURITY.md`, issues for every finding, and new checks in `tests/blackbox/security_test.py`

## What is this, in plain words?
Look at ForgeDB the way an attacker would. Write down what it must protect,
who might attack it, and how; then try each attack. Every weakness you find
becomes an issue for the component's owner. This is a real job ("security
reviewer"), and it teaches you everyone else's code.

## What you must give back
1. `docs/SECURITY.md`: what ForgeDB protects (the data, the availability of the service, the API keys); where untrusted input comes in (every byte of every request; the files in the data dir; the replica's log, after CLI-8); and for each, the threats (the **STRIDE** list: spoofing, tampering, repudiation, information disclosure, denial of service, elevation of privilege), what defends against it today, and what doesn't.
2. Try at least ten attacks and record the result of each. For example:
   - huge `k`, huge vectors (`dim` = 4096 × 1 MiB bodies), deeply nested JSON;
   - ids with `../`, NUL bytes, invalid UTF-8;
   - thousands of connections (with and without `--io epoll`);
   - very slow clients; a request that never ends;
   - a damaged log or snapshot file at startup;
   - timing the 401 response for right and wrong keys.
3. One issue per real weakness, labelled with the component, with steps to reproduce.
4. The repeatable attacks added to `security_test.py`.

## Done when
- [ ] `docs/SECURITY.md` is merged; the findings are issues; presented at a weekly meeting

## Learn first
- OWASP's "Threat Modeling Cheat Sheet", and the STRIDE model (one page each).
