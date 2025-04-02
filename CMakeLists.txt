cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")

project(uvvm_cosim VERSION 0.0.1 DESCRIPTION "UVVM Co-simulation library and JSON-RPC server")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(NVC_PATH "/opt/nvc")
set(VSIM_PATH "/opt/intelFPGA_pro/24.3.1/questa_fse")

###############################################################################
# Cosim library target for VHPI
###############################################################################
add_library(uvvm_cosim_vhpi SHARED
            src/cpp/uvvm_cosim_server.cpp
	    src/cpp/uvvm_cosim_common.cpp
            src/cpp/uvvm_cosim_foreign_vhpi.cpp)
target_compile_definitions(uvvm_cosim_vhpi PRIVATE VHPI)
target_include_directories(uvvm_cosim_vhpi PRIVATE thirdparty/json-rpc-cxx/include thirdparty/json-rpc-cxx/vendor thirdparty/json-rpc-cxx/examples ${NVC_PATH}/include)
target_link_options(uvvm_cosim_vhpi PRIVATE -static-libgcc -static-libstdc++)
set_property(TARGET uvvm_cosim_vhpi PROPERTY POSITION_INDEPENDENT_CODE ON)

###############################################################################
# Cosim library target for FLI
###############################################################################
add_library(uvvm_cosim_fli SHARED
            src/cpp/uvvm_cosim_server.cpp
	    src/cpp/uvvm_cosim_common.cpp
            src/cpp/uvvm_cosim_foreign_fli.cpp)
target_compile_definitions(uvvm_cosim_fli PRIVATE FLI)
target_include_directories(uvvm_cosim_fli PRIVATE thirdparty/json-rpc-cxx/include thirdparty/json-rpc-cxx/vendor thirdparty/json-rpc-cxx/examples ${VSIM_PATH}/include)

# Some compile + link options that are used in Modelsim/Questasim examples
# Probably not all necessary (but static libstdc++ is required to build unless the gcc toolchain bundled with Model/Questasim is used)
target_compile_options(uvvm_cosim_fli PRIVATE -ansi -pedantic -freg-struct-return)
target_link_options(uvvm_cosim_fli PRIVATE -lpthread -Bsymbolic -export-dynamic -static-libstdc++)
set_property(TARGET uvvm_cosim_fli PROPERTY POSITION_INDEPENDENT_CODE ON)

# TODO: Off by default for now since we don't bundle mti.h
option(BUILD_VSIM_FLI "Compile Modelsim FLI version of library" OFF)

if (NOT BUILD_VSIM_FLI)
  set_target_properties(uvvm_cosim_fli PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
endif()


###############################################################################
# C++-based JSON RPC cosim example client
###############################################################################
add_executable(uvvm_cosim_example_client
               src/cpp/uvvm_cosim_client_example.cpp)
target_include_directories(uvvm_cosim_example_client PRIVATE thirdparty/json-rpc-cxx/include thirdparty/json-rpc-cxx/vendor thirdparty/json-rpc-cxx/examples)


###############################################################################
# HDL build + sim targets
###############################################################################

# Build example simulation using HDLregression
add_custom_target(hdl_build_nvc
                  DEPENDS uvvm_cosim_vhpi
                  COMMAND python ${CMAKE_SOURCE_DIR}/run.py --no_sim -s nvc -v)

# Run/simulate example using HDLregression
add_custom_target(hdl_sim_nvc
                  DEPENDS uvvm_cosim_vhpi
                  COMMAND python ${CMAKE_SOURCE_DIR}/run.py -fr -s nvc -v)

# Run/simulate example using HDLregression and Modelsim/Questasim
add_custom_target(hdl_sim_vsim
                  DEPENDS uvvm_cosim_fli
                  COMMAND python ${CMAKE_SOURCE_DIR}/run.py -fr -s modelsim -v)

add_custom_target(hdl_sim_vsim_gui
                  DEPENDS uvvm_cosim_fli
                  COMMAND python ${CMAKE_SOURCE_DIR}/run.py -fr -s modelsim --gui)


###############################################################################
# C++ unit tests and coverage
###############################################################################

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
