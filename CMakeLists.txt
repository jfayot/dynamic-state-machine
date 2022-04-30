cmake_minimum_required(VERSION 3.16)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMakePredefinedTargets")

project(DynamicStateMachine
  VERSION 1.0.0
  LANGUAGES CXX)

option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_COVERAGE "Enable coverage" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake)

add_library(dsm INTERFACE)
add_library(dsm::dsm ALIAS dsm)

target_sources(dsm
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include/dsm/dsm.hpp
)
  
target_include_directories(dsm
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_features(dsm
  INTERFACE
    cxx_std_17
)

add_custom_target(DynamicStateMachine
  SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/include/dsm/dsm.hpp
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    ${CMAKE_CURRENT_LIST_DIR}/LICENSE
    ${CMAKE_CURRENT_LIST_DIR}/.github/workflows/windows.yml
    ${CMAKE_CURRENT_LIST_DIR}/.github/workflows/linux.yml
    ${CMAKE_CURRENT_LIST_DIR}/.github/workflows/macos.yml
    ${CMAKE_CURRENT_LIST_DIR}/.github/workflows/tests.yml
)

if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if (BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
