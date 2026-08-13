cmake_minimum_required(VERSION 3.20)
project(bittorrentclient C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(CURL CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(bittorrentclient
    src/main.c
    src/bencode.c
    src/torrent.c
    src/tracker.c
    src/udp_tracker.c
    src/dht.c
    src/peer.c
    src/peer_protocol.c
    src/webseed.c
    src/piece.c
    src/magnet.c
    src/lpd.c
)

target_include_directories(bittorrentclient PRIVATE src)
target_link_libraries(bittorrentclient PRIVATE CURL::libcurl OpenSSL::SSL OpenSSL::Crypto ws2_32 iphlpapi)

if(MSVC)
    target_compile_options(bittorrentclient PRIVATE /W4 /O2 /EHsc)
else()
    target_compile_options(bittorrentclient PRIVATE -Wall -Wextra -O2)
endif()

if(WIN32)
    target_compile_definitions(bittorrentclient PRIVATE _WIN32_WINNT=0x0A00 WIN32_LEAN_AND_MEAN)
endif()
