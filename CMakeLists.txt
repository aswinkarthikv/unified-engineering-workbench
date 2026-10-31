cmake_minimum_required(VERSION 3.16)
project(UnifiedEngineeringWorkbench VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization & warnings
if(MSVC)
    add_compile_options(/W4 /O2 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3)
    # Static runtime linking for self-contained redistributable execution
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

# Include directories
include_directories(src/backend/include)

# Backend Core Library
set(BACKEND_SOURCES
    src/backend/fea/fea_solver.cpp
    src/backend/parsers/cad_ansys_parser.cpp
    src/backend/multicore/multicore_engine.cpp
    src/backend/embedded/embedded_mocks.cpp
    src/backend/backprop/backprop_engine.cpp
    src/backend/server/http_server.cpp
)

add_library(workbench_core STATIC ${BACKEND_SOURCES})

# Windows System Libraries
if(WIN32)
    target_link_libraries(workbench_core PUBLIC ws2_32 shlwapi)
endif()

# Main Application Executable (Launch.exe)
add_executable(Launch src/backend/main.cpp)
target_link_libraries(Launch PRIVATE workbench_core)

# Set output name to Launch.exe in root directory
set_target_properties(Launch PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_NAME "Launch"
)

# Test Runner Executable
add_executable(test_runner tests/test_all.cpp)
target_link_libraries(test_runner PRIVATE workbench_core)
set_target_properties(test_runner PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    OUTPUT_NAME "test_runner"
)

enable_testing()
add_test(NAME AllWorkbenchTests COMMAND test_runner)
