include(ExternalProject)

# Add an external project that will be the context our gtest tests run out of
# and make sure it's always up-to-date
ExternalProject_Add(
    tests

    SOURCE_DIR
        "${CMAKE_CURRENT_LIST_DIR}/testing"

    INSTALL_COMMAND
        ""

    BUILD_ALWAYS
        TRUE
)

# Get where the gtest files will end up
ExternalProject_Get_Property(
    tests
        BINARY_DIR
)

# Add a custom target that runs the tests
add_custom_target(
    __run_tests

    COMMAND
        ctest --test-dir "${BINARY_DIR}" --output-on-failure

    USES_TERMINAL
)

# Have the test running depend on the test application being up-to-date
add_dependencies(
    __run_tests
        tests
)

# Add a target for running the tests
add_custom_target(
    run_tests
)

# Have the target depend on the "real" test runner
add_dependencies(
    run_tests
        __run_tests
)
