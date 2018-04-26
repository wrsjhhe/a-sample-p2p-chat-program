cmake_minimum_required(VERSION 3.9)

project(p2pServer)

set(CMAKE_CXX_STANDARD 11)

add_executable(p2pServer Proto.pb.cc p2pServer.cpp)
