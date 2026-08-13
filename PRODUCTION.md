# Production-readiness gate

This file is intentionally strict. A release must not be called production-ready until every required item below is implemented and covered by automated tests.

## Core transfer engine

- [ ] Concurrent peer manager
- [ ] Per-peer request queues and pipelining
- [ ] Rarest-first piece selection
- [ ] End-game mode
- [ ] Choke/unchoke and optimistic unchoke
- [ ] Upload/seeding engine
- [ ] Peer connection limits and backpressure
- [ ] Fast extension
- [ ] Request timeout/retry policy
- [ ] Corrupt-peer handling and banning
- [ ] Atomic piece completion
- [ ] Crash-safe resume state
- [ ] Multi-file torrents and safe path normalization

## Discovery and protocols

- [ ] HTTP tracker lifecycle/retry
- [ ] UDP tracker protocol
- [ ] BEP 5 DHT routing buckets, maintenance, tokens and iterative lookup
- [ ] IPv6 tracker/DHT support
- [ ] BEP 9 metadata exchange
- [ ] BEP 10 extension handshake
- [ ] BEP 11 PEX
- [ ] BEP 14 Local Service Discovery
- [ ] WebSeed behavior and retry/fallback
- [ ] Private-torrent isolation
- [ ] uTP transport
- [ ] NAT-PMP/UPnP where supported

## Torrent formats

- [ ] BitTorrent v1 SHA-1 torrents
- [ ] Magnet metadata acquisition
- [ ] BitTorrent v2 Merkle-piece verification
- [ ] Hybrid v1/v2 torrents

## Storage and performance

- [ ] Sparse/preallocated files where appropriate
- [ ] Bounded memory usage
- [ ] Disk I/O queue
- [ ] Configurable cache
- [ ] Bandwidth scheduling
- [ ] Connection scheduling
- [ ] Safe shutdown and recovery

## GUI

- [ ] Real engine integration
- [ ] Add/remove torrents
- [ ] Pause/resume/force recheck
- [ ] Per-torrent and global speed limits
- [ ] Peer list
- [ ] Tracker/DHT status
- [ ] Piece availability
- [ ] Logs and diagnostics
- [ ] Settings persistence
- [ ] Accessible keyboard controls

## Quality and release engineering

- [ ] Unit tests
- [ ] Integration tests with real tracker/peer fixtures
- [ ] Interoperability tests against established BitTorrent clients
- [ ] Malformed-input tests
- [ ] Fuzzing
- [ ] ASan/UBSan/clang-tidy
- [ ] Static analysis
- [ ] Dependency scanning
- [ ] Secret scanning
- [ ] Windows and Linux release builds
- [ ] Reproducible release artifacts
- [ ] Signed release artifacts
- [ ] Versioned changelog
- [ ] Crash reporting/diagnostic policy

**Release rule:** if any checkbox above is incomplete, do not label the release `production-ready`.
