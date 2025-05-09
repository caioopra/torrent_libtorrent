cmake_minimum_required(VERSION 3.12)
project(local_torrent)
set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBTORRENT REQUIRED libtorrent-rasterbar)

include_directories(${LIBTORRENT_INCLUDE_DIRS})

add_executable(peer peer/main.cpp peer/peer.h peer/peer.cpp)
target_link_libraries(peer ${LIBTORRENT_LIBRARIES})
