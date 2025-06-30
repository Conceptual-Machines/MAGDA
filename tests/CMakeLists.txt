# Test sources
set(TEST_SOURCES
    test_command.cpp
    test_interfaces.cpp
)

# Create test executable
add_executable(magica_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(magica_tests 
    PRIVATE 
    magica_daw
    Catch2::Catch2WithMain
)

# Include directories
target_include_directories(magica_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Add the test to CTest
add_test(NAME magica_unit_tests COMMAND magica_tests)

# For Catch2 test discovery (optional - automatically discovers individual test cases)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.10")
    include(Catch)
    catch_discover_tests(magica_tests)
endif() 