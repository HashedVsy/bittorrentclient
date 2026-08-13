# BitTorrent Client

Windows-first C17 BitTorrent client using Winsock2, libcurl and OpenSSL.

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

## Current scope

- bencode parsing
- torrent metadata and SHA-1 info hash
- HTTP/HTTPS tracker discovery
- WebSeed HTTP Range downloads
- BitTorrent peer handshake/request path
- piece verification and resume
- DHT networking scaffold

DHT routing/iterative lookup and a full production peer scheduler are still under development.
