# Example: Basic Usage
add_executable(basic_usage basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE widevine)

# Set output directory
set_target_properties(basic_usage PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
)
