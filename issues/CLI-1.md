# [CLI-1] The `forge` command and a C++ client
labels: comp:cli, size:M
milestone: v0.1 First search

**Component:** CLI (clients) · **Size:** M (2-3 days) · **Needs:** CONN-1 (`Socket`). Start with the command line and the response reader on day 1.
**Contract:** `include/forge/client.hpp` · **Your code:** `src/cli/client.cpp`, `apps/forge.cpp` · **Tests:** `tests/blackbox/cli_test.py` (already written)

## What is this, in plain words?
`forge` is how people use ForgeDB from a terminal:
```bash
forge health
forge put --id a:1 --doc report --text "net profit rose" --vector 0.1,0.9,0 --meta fy=FY2025
forge get a:1
forge search --text "net profit" --k 5
```
Underneath is `forge::Client`: it opens a TCP connection, writes an HTTP
request by hand, and reads the response: the client side of everything CONN
and HTTP do on the server. You don't wait for the real server: the tests run
`forge` against the fake one (`tools/fake_server.py`).

## What you must give back
1. `Client::request` and `url_encode` in `src/cli/client.cpp`, as `include/forge/client.hpp` describes. One keep-alive connection; if the server closed it, reconnect once and retry.
2. `apps/forge.cpp` (replace the placeholder):

| Command | Request |
| --- | --- |
| `health` | `GET /health` |
| `put --id ID --doc DOC --text TEXT --vector 1,2,3 [--meta K=V]...` | `POST /v1/chunks` |
| `get ID` | `GET /v1/chunks/<url_encode(ID)>` |
| `search [--vector 1,2,3] [--text TEXT] [--mode M] [--k N]` | `POST /v1/search` |

   - Global options before the command: `--host` (default `127.0.0.1`), `--port` (default `8080`).
   - `search` without `--mode`: `hybrid` if both `--vector` and `--text`, else whichever was given.
   - Print the response body on stdout, then a newline. Exit **0** for a 2xx status, **1** for any other status or if the server can't be reached (message on stderr), **2** for a bad command line (message and usage on stderr).
   - Build the JSON bodies with nlohmann/json (so quotes in `--text` are escaped correctly).

## Step by step
1. Day 1: delete `src/cli/stub_client.cpp` and create `src/cli/client.cpp`. Start with `apps/forge.cpp`'s argument parsing, and the request body for each command. Print the request instead of sending it, to check.
2. The response reader: read until `\r\n\r\n`; parse the status line and `Content-Length`; read exactly that many body bytes (they may arrive in several pieces).
3. When CONN-1 is merged: connect with `connect_tcp`, send with `send_all`, read with `recv_some`.
4. Run `ctest --preset debug -R blackbox.cli` until it passes.

## Watch out for
- The body may arrive in pieces, or together with the headers in one `recv`. Keep a buffer.
- An id like `a/2` must be sent as `a%2F2`, or the server sees a different path.
- `std::stof("1abc")` returns 1: check that the whole piece was used (its second argument tells you how much).

## How to test it
```bash
ctest --preset debug -R blackbox.cli
python3 tools/fake_server.py --port 8080 --dim 3 &     # by hand
./build/debug/forge put --id x --doc d --text hello --vector 1,0,0
./build/debug/forge search --text hello
```

## Done when
- [ ] `blackbox.cli` passes on `debug`
- [ ] `forge` also works against the real `forgedb` once APP-4 is merged

## Learn first
- Run `curl -v localhost:8080/health` against the fake server, and read every line it prints: that's what your client must send and read.
