cmake_minimum_required(VERSION 3.5.1)
project(p2p)

set(CMAKE_CXX_STANDARD 11)


add_executable(p2pServer p2pServer/Proto.pb.cc p2pServer/p2pServer.cpp)
target_link_libraries(p2pServer -lprotobuf -lpthread)