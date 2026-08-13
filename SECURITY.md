# Security Policy

## Supported versions

Until a 1.0 release is published, `main` is development software and is **not considered production-ready**.

## Reporting a vulnerability

Please do not publish an exploitable vulnerability as a public issue. Report it privately through GitHub's Security Advisories for this repository.

Include:
- affected commit/version
- operating system and architecture
- minimal reproduction or torrent metadata needed to reproduce
- impact assessment
- logs/backtraces with secrets and private data removed

Do not attach copyrighted torrent payloads unless you have permission to redistribute them.

## Security requirements

Before a production release, the project must pass:

- AddressSanitizer and UndefinedBehaviorSanitizer tests
- fuzzing of bencode, peer messages, tracker responses, DHT/KRPC packets, and torrent metadata
- malformed-network-input tests
- integer-overflow and allocation-limit tests
- path traversal tests for multi-file torrents
- private-torrent isolation tests
- resource-exhaustion tests
- dependency and secret scanning
- reproducible release builds

A passing CI build alone does not constitute a security audit or production certification.
