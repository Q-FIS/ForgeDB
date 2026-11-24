# L7 · A DNS resolver: `qdig`
**Week 7 · C · about 5 hours** · *The test arrives before week 7.*

## What is this, in plain words?
Before your browser can connect to `example.com`, it asks a DNS server "what's
the IP address?". The question and the answer are small binary packets sent
over **UDP**. You'll build the question byte by byte, send it, and decode the
answer, including DNS's clever trick for not repeating names ("compression").

## Build
`qdig NAME [TYPE] [SERVER]`: TYPE is `A` (default), `AAAA` or `CNAME`; SERVER defaults to `8.8.8.8`.
1. **Build the query:** a 12-byte header (an id, flags with "recursion desired" set, 1 question), then the name as labels (`7example3com0`), then the type and class (`IN` = 1). All numbers are **big-endian**: use `htons`.
2. **Send it** with `sendto` on a UDP socket to port 53. Wait at most 2 seconds for the reply (`poll`, or `SO_RCVTIMEO`), and retry once.
3. **Decode the answer:** check the id matches, read the answer count, then each record: its name (follow **compression pointers**: a byte starting with `11` binary points back to an earlier name), type, TTL and data. Print each one like `example.com. 3600 IN A 93.184.215.14`.

## Experiment
Compare with `dig +short NAME` on 20 domains (include ones with CNAMEs, like `www.github.com`). Capture your query in Wireshark: it must decode as valid DNS.

## Done when
- [ ] Same addresses as `dig` for the 20 domains
- [ ] `NOTES.md`: a Wireshark screenshot of your query and answer, with the header fields labelled

## Extra
`qtrace HOST`: traceroute. Send UDP probes with TTL 1, 2, 3... and read the ICMP "time exceeded" replies (with `IP_RECVERR`, or a raw socket and `sudo`). Trace three servers in different countries, and explain the latency jumps.

## Read
- RFC 1035, sections 4.1 (message format) and 4.1.4 (compression).
- *Computer Networking: A Top-Down Approach*, section 2.4 (DNS).
