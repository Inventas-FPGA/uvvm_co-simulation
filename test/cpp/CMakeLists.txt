find_package(Catch2 2 REQUIRED)

add_executable(test_uvvm_cosim_types test_uvvm_cosim_types.cpp)
target_link_libraries(test_uvvm_cosim_types PRIVATE Catch2::Catch2WithMain)
target_include_directories(test_uvvm_cosim_types PUBLIC
  "${PROJECT_SOURCE_DIR}/src/cpp"
  "${PROJECT_SOURCE_DIR}/thirdparty/json-rpc-cxx/vendor"
)

include(Catch)
catch_discover_tests(test_uvvm_cosim_types)

