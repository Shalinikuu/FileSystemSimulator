cmake_minimum_required(VERSION 3.10)

# Project Name
project(FileSystemSimulator)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add Source Files
add_executable(FileSystemSimulator main.cpp)

# Include Dependencies
include_directories(${CMAKE_SOURCE_DIR}/../dependencies/crow/Crow-1.2.1.2/include)
include_directories(${CMAKE_SOURCE_DIR}/../dependencies/asio-1.30.2/include)

# Link Required Libraries
find_package(Threads REQUIRED)
target_link_libraries(FileSystemSimulator PRIVATE Threads::Threads ws2_32 mswsock)  # <-- Added `mswsock`
