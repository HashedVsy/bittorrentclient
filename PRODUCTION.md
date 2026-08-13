# Production-readiness gate

This file is intentionally strict. A release must not be called production-ready until every required item below is implemented, exercised by automated tests, and verified in a real end-to-end download/seeding test.

## Current architecture

The project uses a production-oriented libtorrent backend for the transfer engine. This checklist therefore tracks both project-owned functionality and the backend integration that must be verified in CI/release testing.

## Core transfer engine

- [ ] Verify concurrent multi-peer operation in an end-to-end test
- [ ] Verify per-peer request queues and pipelining
- [ ] Verify rarest-first piece selection
- [ ] Verify end-game mode
- [ ] Verify choke/unchoke and optimistic unchoke
- [ ] Verify upload/seeding engine
- [ ] Verify peer connection limits and backpressure
- [ ] Verify fast extension
- [ ] Verify request timeout/retry policy
- [ ] Verify corrupt-peer handling and banning
- [ ] Verify atomic piece completion
- [ ] Verify crash-safe resume state
- [ ] Verify multi-file torrents and safe path normalization

## Discovery and protocols

- [ ] Verify HTTP tracker lifecycle/retry
- [ ] Verify UDP tracker protocol
- [ ] Verify BEP 5 DHT routing, maintenance, tokens and iterative lookup
- [ ] Verify IPv6 tracker/DHT support
- [ ] Verify BEP 9 metadata exchange
- [ ] Verify BEP 10 extension handshake
- [ ] Verify BEP 11 PEX
- [ ] Verify BEP 14 Local Service Discovery
- [ ] Verify WebSeed behavior and retry/fallback
- [ ] Verify private-torrent isolation
- [ ] Verify uTP transport
- [ ] Verify NAT-PMP/UPnP where supported

## Torrent formats

- [ ] Verify BitTorrent v1 SHA-1 torrents
- [ ] Verify Magnet metadata acquisition
- [ ] Verify BitTorrent v2 Merkle-piece verification
- [ ] Verify hybrid v1/v2 torrents

## Storage and performance

- [ ] Verify sparse/preallocated files where appropriate
- [ ] Verify bounded memory usage on large torrents
- [ ] Verify disk I/O queue behavior
- [ ] Verify configurable disk cache
- [ ] Verify bandwidth scheduling
- [ ] Verify connection scheduling
- [ ] Verify safe shutdown and recovery

## GUI

- [ ] Real engine integration verified end-to-end
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
- [ ] Integration tests with deterministic tracker/peer fixtures
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
- [ ] Real-world Arch Linux torrent download test
- [ ] Real-world seeding/upload test

## CI status

The repository has GitHub Actions coverage for Windows and Linux builds, static analysis, sanitizer checks, and security/integrity checks. A queued workflow is **not** considered a passing workflow; all required jobs must complete successfully before release.

## Release rule

**If any checkbox above is incomplete, do not label the release `production-ready`.**

The project must also pass a complete end-to-end download and recheck test before a production release is tagged.