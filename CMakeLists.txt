cmake_minimum_required(VERSION 3.16)
project(sunposition-evaluator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(sun_position_lib
    src/sun_position_algorithm.cpp
    src/sun_position_evaluator.cpp
)

target_include_directories(sun_position_lib PUBLIC include)

add_executable(build_mica_binary_dataset
    apps/build_mica_binary_dataset.cpp
)

target_include_directories(build_mica_binary_dataset PRIVATE include)

add_executable(sun_position_evaluator
    apps/sun_position_evaluator_main.cpp
)

target_link_libraries(sun_position_evaluator PRIVATE sun_position_lib)
target_include_directories(sun_position_evaluator PRIVATE include)
