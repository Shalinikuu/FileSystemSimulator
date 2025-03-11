cmake_minimum_required(VERSION 3.10)
project(FileSystemSimulator)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define Source Files
set(SRC_FILES main.cpp auth.cpp globals.cpp)
add_executable(FileSystemSimulator ${SRC_FILES})

# Set Dependency Paths
set(DEPS_DIR "${CMAKE_SOURCE_DIR}/../dependencies")

# Include Dependencies
include_directories(
    ${DEPS_DIR}/crow/include
    ${DEPS_DIR}/asio-1.30.2/include
    ${DEPS_DIR}/jwt-cpp-0.7.1/include
    ${DEPS_DIR}/json-3.11.3/include
    ${DEPS_DIR}/json-3.11.3/include/nlohmann
    ${DEPS_DIR}/bcrypt   # Add bcrypt directory
)

# Add bcrypt source file if needed
set(BCRYPT_SRC ${DEPS_DIR}/bcrypt/bcrypt.cpp) # Change if your bcrypt implementation file has a different name

# OpenSSL and Zlib Paths (Ensure MinGW paths are correct)
set(OPENSSL_ROOT "C:/msys64/mingw64")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT}/include")
set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT}/lib/libcrypto.a")
set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT}/lib/libssl.a")
set(ZLIB_INCLUDE_DIR "${OPENSSL_ROOT}/include")
set(ZLIB_LIBRARY "${OPENSSL_ROOT}/lib/libz.a")

# Find OpenSSL and Zlib
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# Link Libraries
target_link_libraries(FileSystemSimulator PRIVATE
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${ZLIB_LIBRARY}
    bcrypt  # <-- Add this
    ws2_32
    mswsock
)


# Debug Messages
message(STATUS "OpenSSL Include Dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL Libraries: ${OPENSSL_CRYPTO_LIBRARY}, ${OPENSSL_SSL_LIBRARY}")
message(STATUS "Zlib Include Dir: ${ZLIB_INCLUDE_DIR}")
message(STATUS "Zlib Libraries: ${ZLIB_LIBRARY}")
message(STATUS "bcrypt directory: ${DEPS_DIR}/bcrypt")
