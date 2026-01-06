cmake_minimum_required(VERSION 3.15...3.27)
project(mesh2sdf VERSION 0.1.0 LANGUAGES CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Python 3
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# Find pybind11
# First try to find it in the current Python environment
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -m pybind11 --cmakedir
    OUTPUT_VARIABLE pybind11_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(pybind11_DIR)
    find_package(pybind11 CONFIG REQUIRED PATHS ${pybind11_DIR} NO_DEFAULT_PATH)
else()
    find_package(pybind11 CONFIG REQUIRED)
endif()

# Find OpenMP（可选，用于多线程加速）
find_package(OpenMP)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source file list
set(MESH2SDF_SOURCES
    src/mesh.cpp
    src/plane_cutter.cpp
    src/distance_transform.cpp
    src/sdf_generator.cpp
)

# Create pybind11 module
pybind11_add_module(_core
    ${MESH2SDF_SOURCES}
    src/bindings.cpp
)

# Link OpenMP（如果可用）
if(OpenMP_CXX_FOUND)
    target_link_libraries(_core PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(_core PRIVATE USE_OPENMP)
endif()

# Compilation options
target_compile_options(_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/O2 /W4>
)

# Install target
install(TARGETS _core DESTINATION mesh2sdf_fast)
