cmake_minimum_required(VERSION 3.9)
project(p2p)

set(CMAKE_CXX_STANDARD 11)

include(FindProtobuf)
find_package(Protobuf REQUIRED)

add_executable(p2pServer p2pServer/Proto.pb.cc p2pServer/p2pServer.cpp)
target_link_libraries(p2pServer protobuf)