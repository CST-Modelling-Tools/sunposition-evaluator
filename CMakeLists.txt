cmake_minimum_required(VERSION 3.16)
project(sunposition-evaluator LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

option(SUNPOSITION_BUILD_BENCHMARK "Build the timing benchmark executable" ON)
option(SUNPOSITION_ENABLE_OPENMP "Enable OpenMP when the toolchain supports it" ON)

set(SUNPOSITION_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(sun_position_lib
    src/sun_position_algorithm.cpp
    src/sun_position_evaluator.cpp
)

target_compile_features(sun_position_lib PUBLIC cxx_std_20)
target_include_directories(sun_position_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(SUNPOSITION_ENABLE_OPENMP)
    find_package(OpenMP QUIET)
endif()

if(OpenMP_CXX_FOUND)
    target_link_libraries(sun_position_lib PUBLIC OpenMP::OpenMP_CXX)
endif()

add_library(sun_position_fixture_support
    src/basic_test_fixture.cpp
    src/mica_binary_dataset_builder.cpp
)

target_compile_features(sun_position_fixture_support PUBLIC cxx_std_20)
target_include_directories(sun_position_fixture_support PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(sun_position_fixture_support PUBLIC sun_position_lib)
target_compile_definitions(sun_position_fixture_support
    PUBLIC SUNPOSITION_SOURCE_DIR="${SUNPOSITION_SOURCE_DIR}"
)

add_executable(build_mica_binary_dataset
    apps/build_mica_binary_dataset.cpp
)

target_link_libraries(build_mica_binary_dataset PRIVATE sun_position_fixture_support)

add_executable(sun_position_evaluator
    apps/sun_position_evaluator_main.cpp
)

target_link_libraries(sun_position_evaluator PRIVATE sun_position_lib)
target_include_directories(sun_position_evaluator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(sun_position_evaluator PRIVATE
    SUNPOSITION_SOURCE_DIR="${SUNPOSITION_SOURCE_DIR}"
)

if(SUNPOSITION_BUILD_BENCHMARK)
    add_executable(sun_position_benchmark
        apps/sun_position_benchmark_main.cpp
    )

    target_link_libraries(sun_position_benchmark PRIVATE sun_position_fixture_support)
endif()

if(BUILD_TESTING)
    add_executable(sun_position_regression_test
        tests/sun_position_regression_test_main.cpp
    )

    target_link_libraries(sun_position_regression_test PRIVATE sun_position_fixture_support)

    add_test(
        NAME sun_position_regression
        COMMAND sun_position_regression_test --fixture-dir "${CMAKE_SOURCE_DIR}/basic_test"
    )

    if(SUNPOSITION_BUILD_BENCHMARK)
        add_test(
            NAME sun_position_benchmark_smoke
            COMMAND sun_position_benchmark --fixture-dir "${CMAKE_SOURCE_DIR}/basic_test" --repetitions 1
        )
    endif()
endif()

set(sunposition_install_targets
    build_mica_binary_dataset
    sun_position_evaluator
)

if(SUNPOSITION_BUILD_BENCHMARK)
    list(APPEND sunposition_install_targets sun_position_benchmark)
endif()

install(
    TARGETS ${sunposition_install_targets}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP support enabled via OpenMP::OpenMP_CXX")
elseif(SUNPOSITION_ENABLE_OPENMP)
    message(STATUS "OpenMP support not found; building with sequential fallback")
else()
    message(STATUS "OpenMP support disabled by SUNPOSITION_ENABLE_OPENMP=OFF")
endif()
