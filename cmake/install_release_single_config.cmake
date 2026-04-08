if(NOT DEFINED CMAKE_BUILD_TYPE OR NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(FATAL_ERROR
        "The 'install_release' target requires a Release-configured single-config build tree.\n"
        "Reconfigure with: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
    RESULT_VARIABLE install_result
)

if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "Release install failed with exit code ${install_result}.")
endif()
