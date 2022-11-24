cmake_minimum_required(VERSION 3.16)
project(SocketServer)

include_directories(include)

file(GLOB INCS "${PROJECT_SOURCE_DIR}/include/*.h")

file(GLOB SRCS "${PROJECT_SOURCE_DIR}/src/*.cpp")

message(${INCS})
message(${SRCS})

add_executable(server ${SRCS})