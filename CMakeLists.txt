cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")

project(uvvm_cosim VERSION 0.0.1 DESCRIPTION "UVVM Co-simulation JSON-RPC server for use with VHDL VHPI")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(NVC_PATH "/opt/nvc")

# VHPI cosim library
add_library(uvvm_cosim_vhpi SHARED
            src/cpp/uvvm_cosim_server.cpp
            src/cpp/uvvm_cosim_vhpi.cpp)
target_include_directories(uvvm_cosim_vhpi PRIVATE thirdparty/json-rpc-cxx/include thirdparty/json-rpc-cxx/vendor thirdparty/json-rpc-cxx/examples ${NVC_PATH}/include)
set_property(TARGET uvvm_cosim_vhpi PROPERTY POSITION_INDEPENDENT_CODE ON)


# Build example simulation using HDLregression
add_custom_target(hdl_build
                  DEPENDS uvvm_cosim_vhpi
                  COMMAND python ${CMAKE_SOURCE_DIR}/run.py --no_sim -s nvc -v)

# Run/simulate example using HDLregression
add_custom_target(hdl_sim
                  DEPENDS uvvm_cosim_vhpi
                  COMMAND python ${CMAKE_SOURCE_DIR}/run.py -fr -s nvc)


# JSON RPC example client
add_executable(uvvm_cosim_example_client
               src/cpp/uvvm_cosim_client_example.cpp)
target_include_directories(uvvm_cosim_example_client PRIVATE thirdparty/json-rpc-cxx/include thirdparty/json-rpc-cxx/vendor thirdparty/json-rpc-cxx/examples)

# Options to enable unit tests and coverage
option(ENABLE_UNIT_TESTS "Run Catch2 Unit tests" ON)
option(ENABLE_COVERAGE "Enable code coverage for unit tests" OFF)

# Add unit test target and coverage targets (when enabled)
if(ENABLE_UNIT_TESTS)
  if(ENABLE_COVERAGE)
    include(coverage)
    set(COVERAGE_INCLUDE_DIRS
      "${CMAKE_SOURCE_DIR}/src/cpp/*"
    )
    add_coverage_target(${COVERAGE_INCLUDE_DIRS})
  endif()

  enable_testing()
  add_subdirectory(test/cpp)
endif()

# Note: Run "make clean-gcov" to remove gcov .gcno and .gcda files.
# Haven't found a way to setup ADDITIONAL_CLEAN_FILES to cleanup all
# .gcno and .gcda files by running make clean.
add_custom_target(clean-gcov
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcno" -delete
    COMMENT "Cleaning gcov build and coverage files..."
)
