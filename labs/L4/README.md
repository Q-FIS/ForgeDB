# L4 · A chat server: one thread, many clients
**Week 4 · C++ · about 5 hours**

## What is this, in plain words?
A chat room in your terminal: people connect with `nc`, and whatever one person
types, everyone else sees. With **one thread** and `poll()`, the server watches
every connection at once and only touches the ones that have something to say.
That's exactly how ForgeDB's `--io epoll` mode works (CONN-5), and how Nginx and Redis work. You'll
also watch TCP itself in Wireshark: the handshake, the data, the goodbye.

## Build
`chat_server --port N`:
1. Listen on 127.0.0.1:N. Make the listening socket and every client socket **non-blocking** (`fcntl` with `O_NONBLOCK`).
2. Write a small `Socket` class that closes its fd in its destructor (RAII, move-only), like ForgeDB's `Socket` (CONN-1).
3. Loop on `poll()` over the listener plus every client:
   - the listener is readable → `accept` new clients (until `EAGAIN`);
   - a client is readable → `recv` into **that client's own buffer**. For each complete line (ending in `\n`) in the buffer:
     - `/nick NAME` → this client's name becomes NAME (nothing is sent);
     - `/quit` → close this client;
     - anything else → send `NAME: line\n` to **every other** client.
   - `recv` returns 0 → the client left: close and remove it.
4. A new client's name is `guest<N>` until it uses `/nick`.
5. **Partial lines:** a message can arrive in pieces. Only handle complete lines, and keep the rest in the buffer. **Partial writes:** if `send` can't take everything (`EAGAIN`), keep the rest in that client's output buffer, and also watch for `POLLOUT` until it's sent.

Try it: `./chat_server --port 7000`, then `nc localhost 7000` in three terminals.

## Experiment
1. **Wireshark** (or `sudo tcpdump -i lo -w chat.pcap port 7000`, then open the file in Wireshark): connect with `nc`, send two messages, quit. Label the three-way handshake (SYN, SYN-ACK, ACK), each data segment with its sequence and acknowledgement numbers, and the teardown (FIN, ACK). Screenshot it into `NOTES.md`.
2. `kill -9` one `nc` client. What does the server see? (`recv` returns 0? An error? Or nothing at all?) Then pull the network instead: why can a dead peer go unnoticed for a long time? (Look up TCP keepalive.)

## The test
```bash
python3 L4/test_l4.py build/debug/L4
```
50 clients connect, set their names, and each sends 20 messages at the same time. Every client must receive every other client's 980 messages, each one complete and in its sender's order.

## Done when
- [ ] The test passes on the debug build
- [ ] `NOTES.md`: the labelled Wireshark capture, and the `kill -9` answer

## Extra
Rewrite the loop with `epoll` instead of `poll`. Why does `epoll` scale better to 10,000 clients?

## Read
- Beej's Guide to Network Programming, chapter 7.2 ("poll()").
- *Computer Networking: A Top-Down Approach*, chapter 3.5 (TCP: connection management).
