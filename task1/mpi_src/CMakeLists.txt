cmake_minimum_required(VERSION 3.16.3)
project(luna_graduate_work)

set(CMAKE_CXX_STANDARD 14)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
include_directories(src/luna_src)

set(LUNA_DF_CLASS src/luna_src/df.cpp src/luna_src/common.cpp src/luna_src/serializable.cpp)

add_executable(prog src/vectors_add.cpp ${LUNA_DF_CLASS})