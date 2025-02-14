find_package(Catch2 2 REQUIRED)

add_executable(uvvm_cosim_types uvvm_cosim_types_test.cpp)
target_link_libraries(uvvm_cosim_types PRIVATE Catch2::Catch2WithMain)
target_include_directories(uvvm_cosim_types PUBLIC
  "${PROJECT_SOURCE_DIR}/src/cpp"
  "${PROJECT_SOURCE_DIR}/thirdparty/json-rpc-cxx/vendor"
)

include(Catch)
catch_discover_tests(uvvm_cosim_types)

