# Example: Basic Usage
add_executable(basic_usage basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE widevine)

# Example: License Server Test
find_package(CURL)
if(CURL_FOUND)
    add_executable(test_license_server test_license_server.cpp)
    target_link_libraries(test_license_server PRIVATE widevine CURL::libcurl)
    set_target_properties(test_license_server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
    )
endif()

# Set output directory
set_target_properties(basic_usage PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
)
