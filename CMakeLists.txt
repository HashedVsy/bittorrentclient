cmake_minimum_required(VERSION 3.24)
project(bittorrentclient VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LibtorrentRasterbar CONFIG REQUIRED)

add_executable(bittorrentclient
    src/main.c
    src/engine.cpp
    src/gui.c
)

target_include_directories(bittorrentclient PRIVATE src)
target_link_libraries(bittorrentclient PRIVATE LibtorrentRasterbar::torrent-rasterbar)

if(WIN32)
    target_link_libraries(bittorrentclient PRIVATE comctl32 comdlg32 ws2_32)
    target_compile_definitions(bittorrentclient PRIVATE _WIN32_WINNT=0x0A00 WIN32_LEAN_AND_MEAN NOMINMAX)
    set_target_properties(bittorrentclient PROPERTIES WIN32_EXECUTABLE FALSE)
endif()

if(MSVC)
    target_compile_options(bittorrentclient PRIVATE /W4 /WX /O2 /permissive-)
else()
    target_compile_options(bittorrentclient PRIVATE -Wall -Wextra -Wpedantic -Werror -O2)
endif()

include(GNUInstallDirs)
install(TARGETS bittorrentclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
