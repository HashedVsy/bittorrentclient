# BitTorrent Client

Windows-first C17 BitTorrent client using Winsock2, libcurl and OpenSSL. The project is intentionally implemented in C rather than wrapping an existing BitTorrent engine.

## Build

```powershell
vcpkg install curl openssl
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

## Usage

```powershell
.\build\Release\bittorrentclient.exe .\Downloads\file.torrent .\file.iso
```

## Implemented / in-tree protocol work

- BEP 3 torrent metainfo parsing
- Exact bencoded `info` SHA-1 info-hash
- Multi-tracker metadata
- HTTP/HTTPS tracker support
- UDP tracker announce path (BEP 15)
- Compact tracker peer parsing
- WebSeed / HTTP Range support
- BitTorrent peer wire message builders
- Core request/cancel/have/PORT messages
- DHT networking and iterative discovery work (BEP 5)
- DHT node IDs, routing data, bootstrap persistence and peer discovery
- Magnet URI `xt=urn:btih:` / `dn` / `tr` parsing
- Local Peer Discovery announce path (BEP 14)
- Piece SHA-1 verification
- Resume-oriented piece handling
- Windows Winsock2 networking
- IPv4/IPv6-capable address representation

## Planned hardening

The remaining work is mostly integration and production hardening rather than pretending every BEP is finished: complete IPv6 DHT (BEP 32), full LTEP/metadata/PEX integration (BEP 9/10/11), robust piece scheduling and choking, connection management, NAT traversal, torrent file trees, BitTorrent v2/hybrid torrents (BEP 52), encrypted transport, disk cache, rate limiting, tests, and fuzzing.

The official BitTorrent BEP index distinguishes accepted extensions such as DHT, Fast Extension, metadata exchange, extension protocol, PEX, multitracker, LPD and UDP tracker, while newer capabilities such as IPv6 DHT and v2 are separate specifications. citeturn0search2turn0search6

BitTorrent v2 replaces SHA-1 piece addressing with SHA-256 Merkle-tree based structures and can coexist with v1 in hybrid torrents, so it needs a dedicated implementation rather than a superficial flag. citeturn0search1
