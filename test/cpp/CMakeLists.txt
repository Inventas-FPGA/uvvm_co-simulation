include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.0)
FetchContent_MakeAvailable(Catch2)

# Common include dirs for all test executables
set(UVVM_COSIM_TEST_INCLUDES
  "${PROJECT_SOURCE_DIR}/src/cpp"
  "${PROJECT_SOURCE_DIR}/thirdparty/json-rpc-cxx/vendor"
)

# Helper: register a test executable, link Catch2WithMain, set common includes,
# discover its Catch2 tests, and (when coverage is enabled) instrument it.
# Usage: add_uvvm_cosim_test(<name> <source.cpp> [extra_sources...])
function(add_uvvm_cosim_test name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain)
  target_include_directories(${name} PUBLIC ${UVVM_COSIM_TEST_INCLUDES})
  catch_discover_tests(${name})
  if (ENABLE_COVERAGE)
    append_coverage_compiler_flags_to_target(${name})
  endif()
endfunction()

include(Catch)
set(CMAKE_CATCH_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

add_uvvm_cosim_test(test_byte_queue       test_byte_queue.cpp)
add_uvvm_cosim_test(test_packet_queue     test_packet_queue.cpp)
add_uvvm_cosim_test(test_uvvm_cosim_types test_uvvm_cosim_types.cpp)
add_uvvm_cosim_test(test_vvc_base         test_vvc_base.cpp)
add_uvvm_cosim_test(test_vvc_byte_stream  test_vvc_byte_stream.cpp)
add_uvvm_cosim_test(test_vvc_packet_stream test_vvc_packet_stream.cpp)
add_uvvm_cosim_test(test_vvc_mem_mapped   test_vvc_mem_mapped.cpp)
add_uvvm_cosim_test(test_shared_map       test_shared_map.cpp)

# Pthreads for the shared_map thread-safety test
find_package(Threads REQUIRED)
target_link_libraries(test_shared_map PRIVATE Threads::Threads)

# Tests that pull vvc_factory.cpp into their translation unit
add_uvvm_cosim_test(test_vvc_factory
  test_vvc_factory.cpp
  ${PROJECT_SOURCE_DIR}/src/cpp/vvc/vvc_factory.cpp)

add_uvvm_cosim_test(test_uvvm_cosim_data
  test_uvvm_cosim_data.cpp
  ${PROJECT_SOURCE_DIR}/src/cpp/uvvm_cosim_data.cpp
  ${PROJECT_SOURCE_DIR}/src/cpp/vvc/vvc_factory.cpp)


if (ENABLE_COVERAGE)
  # All test executables must be in DEPENDENCIES so they get built (with
  # coverage instrumentation) before ctest runs against them.
  setup_target_for_coverage_lcov(NAME cov
                                 EXECUTABLE ctest -j ${PROCESSOR_COUNT}
                                 DEPENDENCIES
                                   test_byte_queue
                                   test_packet_queue
                                   test_uvvm_cosim_types
                                   test_uvvm_cosim_data
                                   test_vvc_base
                                   test_vvc_byte_stream
                                   test_vvc_packet_stream
                                   test_vvc_mem_mapped
                                   test_vvc_factory
                                   test_shared_map
                                 # Capture from the build tree (not just src/cpp) so
                                 # .gcda files for header-only classes resolve back to
                                 # their source paths.  The EXCLUDE filters keep
                                 # thirdparty + system headers out of the report.
                                 BASE_DIRECTORY "${PROJECT_SOURCE_DIR}"
                                 EXCLUDE
                                   "/usr/include/*"
                                   "${PROJECT_SOURCE_DIR}/thirdparty/*"
                                   "${CMAKE_BINARY_DIR}/_deps/*")

endif()
