include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.0)
FetchContent_MakeAvailable(Catch2)

add_executable(test_uvvm_cosim_types test_uvvm_cosim_types.cpp)
target_link_libraries(test_uvvm_cosim_types PRIVATE Catch2::Catch2WithMain)
target_include_directories(test_uvvm_cosim_types PUBLIC
  "${PROJECT_SOURCE_DIR}/src/cpp"
  "${PROJECT_SOURCE_DIR}/thirdparty/json-rpc-cxx/vendor"
)

add_executable(test_byte_queue test_byte_queue.cpp)
target_link_libraries(test_byte_queue PRIVATE Catch2::Catch2WithMain)
target_include_directories(test_byte_queue PUBLIC
  "${PROJECT_SOURCE_DIR}/src/cpp"
  "${PROJECT_SOURCE_DIR}/thirdparty/json-rpc-cxx/vendor"
)

include(Catch)
set(CMAKE_CATCH_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)
catch_discover_tests(test_uvvm_cosim_types)
catch_discover_tests(test_byte_queue)
