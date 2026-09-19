cmake_minimum_required(VERSION 3.15)
project(widevine-cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(WIDEVINE_BUILD_TESTS "Build tests" OFF)
option(WIDEVINE_BUILD_EXAMPLES "Build examples" ON)

# Find dependencies
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

# Set protobuf output directory
set(PROTO_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto)
file(MAKE_DIRECTORY ${PROTO_OUTPUT_DIR})

# Generate protobuf files
set(PROTO_FILES proto/license_protocol.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    EXPORT_MACRO ""
    PROTOC_OUT_DIR ${PROTO_OUTPUT_DIR}
    ${PROTO_FILES}
)

# Library sources
set(WIDEVINE_SOURCES
    src/device.cpp
    src/cdm.cpp
    src/pssh.cpp
    src/key.cpp
    src/session.cpp
    src/crypto.cpp
    ${PROTO_SRCS}
)

# Create library
add_library(widevine STATIC ${WIDEVINE_SOURCES})

target_include_directories(widevine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROTO_OUTPUT_DIR}>  # For generated protobuf headers
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Protobuf_INCLUDE_DIRS}
)

target_link_libraries(widevine
    PUBLIC
        protobuf::libprotobuf
        OpenSSL::SSL
        OpenSSL::Crypto
)

# Compiler warnings
if(MSVC)
    target_compile_options(widevine PRIVATE /W4)
else()
    target_compile_options(widevine PRIVATE -Wall -Wextra -pedantic)
endif()

# Examples
if(WIDEVINE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
install(TARGETS widevine
    EXPORT widevine-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/widevine
    DESTINATION include
)

install(FILES ${PROTO_HDRS}
    DESTINATION include/widevine
)

# Export targets
install(EXPORT widevine-targets
    FILE widevine-targets.cmake
    NAMESPACE widevine::
    DESTINATION lib/cmake/widevine
)

# Create config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/widevine-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/widevine-config-version.cmake"
    DESTINATION lib/cmake/widevine
)
