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
    src/dht.c
    src/peer.c
    src/webseed.c
    src/piece.c
)

target_include_directories(bittorrentclient PRIVATE src)

target_link_libraries(bittorrentclient PRIVATE
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    ws2_32
)

if(MSVC)
    target_compile_options(bittorrentclient PRIVATE /W4 /O2)
else()
    target_compile_options(bittorrentclient PRIVATE -Wall -Wextra -O2)
endif()
