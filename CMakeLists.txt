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

# NVC simulation target
add_custom_target(nvc_sim
                  DEPENDS uvvm_cosim_vhpi
                  COMMAND nvc -a ${CMAKE_SOURCE_DIR}/test/vhpi_testbench.vhd
                  COMMAND nvc -e tb
                  COMMAND nvc -r --load ./libuvvm_cosim_vhpi.so tb)

add_custom_target(hdl_run
                  DEPENDS uvvm_cosim_vhpi
                  COMMAND python ${CMAKE_SOURCE_DIR}/run.py -fr -s nvc -v)


# JSON RPC example client
add_executable(uvvm_cosim_example_client
               src/cpp/uvvm_cosim_client_example.cpp)
target_include_directories(uvvm_cosim_example_client PRIVATE thirdparty/json-rpc-cxx/include thirdparty/json-rpc-cxx/vendor thirdparty/json-rpc-cxx/examples)

option(ENABLE_UNIT_TESTS "Run Catch2 Unit tests" ON)
option(ENABLE_COVERAGE "Enable code coverage for unit tests" OFF)

if(ENABLE_UNIT_TESTS)
  if(ENABLE_COVERAGE)
    include(coverage)
    set(COVERAGE_EXCLUDE_DIRS
      "${CMAKE_SOURCE_DIR}/test/cpp/*"
      "${CMAKE_SOURCE_DIR}/thirdparty/*"
      "*/catch2/*"
      "/usr/*"
      "/opt/*"
    )
    add_coverage_target(${COVERAGE_EXCLUDE_DIRS})
  endif()

  enable_testing()
  add_subdirectory(test/cpp)
endif()
