cmake_minimum_required(VERSION 3.16)
project(sunposition-evaluator LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

option(SUNPOSITION_BUILD_BENCHMARK "Build the timing benchmark executable" ON)

add_library(sun_position_lib
    src/sun_position_algorithm.cpp
    src/sun_position_evaluator.cpp
)

target_compile_features(sun_position_lib PUBLIC cxx_std_20)
target_include_directories(sun_position_lib PUBLIC include)

add_library(sun_position_fixture_support
    src/basic_test_fixture.cpp
    src/mica_binary_dataset_builder.cpp
)

target_compile_features(sun_position_fixture_support PUBLIC cxx_std_20)
target_include_directories(sun_position_fixture_support PUBLIC include)
target_link_libraries(sun_position_fixture_support PUBLIC sun_position_lib)

add_executable(build_mica_binary_dataset
    apps/build_mica_binary_dataset.cpp
)

target_link_libraries(build_mica_binary_dataset PRIVATE sun_position_fixture_support)

add_executable(sun_position_evaluator
    apps/sun_position_evaluator_main.cpp
)

target_link_libraries(sun_position_evaluator PRIVATE sun_position_lib)
target_include_directories(sun_position_evaluator PRIVATE include)

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
    RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/bin"
    CONFIGURATIONS Release
)

if(CMAKE_CONFIGURATION_TYPES)
    add_custom_target(
        install_release
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config Release
        DEPENDS ${sunposition_install_targets}
        COMMENT "Installing Release executables into ${CMAKE_SOURCE_DIR}/bin"
    )
else()
    add_custom_target(
        install_release
        COMMAND
            "${CMAKE_COMMAND}"
            -DCMAKE_COMMAND="${CMAKE_COMMAND}"
            -DCMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}"
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -P "${CMAKE_SOURCE_DIR}/cmake/install_release_single_config.cmake"
        DEPENDS ${sunposition_install_targets}
        COMMENT "Installing executables into ${CMAKE_SOURCE_DIR}/bin"
    )
endif()
