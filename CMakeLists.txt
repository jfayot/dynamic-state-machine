cmake_minimum_required(VERSION 3.16)

# for IDE tree structure
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMakePredefinedTargets")

# lib's version
set(version 0.1.1)

project(DynamicStateMachine
  VERSION ${version}
  LANGUAGES CXX)

# make cache variables for install destinations
include(GNUInstallDirs)

# download CPM.cmake
file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.2/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH SHA256=c8cdc32c03816538ce22781ed72964dc864b2a34a310d3b7104812a5ca2d835d
)

include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)
CPMUsePackageLock(package-lock.cmake)

# check if project is standalone or embedded
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(IS_ROOT_PROJECT ON)
else()
    set(IS_ROOT_PROJECT OFF)
endif()

option(BUILD_DSM_EXAMPLES "Build examples" ${IS_ROOT_PROJECT})
option(BUILD_DSM_TESTS "Build tests" ${IS_ROOT_PROJECT})
option(ENABLE_DSM_INSTALL "Enable package installation" ${IS_ROOT_PROJECT})
option(ENABLE_DSM_COVERAGE "Enable coverage" ${IS_ROOT_PROJECT})

add_library(dsm
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include/dsm/dsm.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/dsm/log.hpp
)

add_library(dsm::dsm ALIAS dsm)

target_include_directories(dsm
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# C++17 required for structured binding
target_compile_features(dsm
  INTERFACE
    cxx_std_17
)

# mainly useful for VS IDE to be able to edit all project's files
add_custom_target(DynamicStateMachine
  SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/include/dsm/dsm.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/dsm/log.hpp
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    ${CMAKE_CURRENT_LIST_DIR}/LICENSE
    ${CMAKE_CURRENT_LIST_DIR}/.github/workflows/windows.yml
    ${CMAKE_CURRENT_LIST_DIR}/.github/workflows/linux.yml
    ${CMAKE_CURRENT_LIST_DIR}/.github/workflows/macos.yml
    ${CMAKE_CURRENT_LIST_DIR}/.github/workflows/tests.yml
)

if (BUILD_DSM_EXAMPLES)
  add_subdirectory(examples)
endif()

if (BUILD_DSM_TESTS)
  add_subdirectory(tests)
endif()

if(ENABLE_DSM_INSTALL)
  CPMGetPackage(CPMPackageProject)

  packageProject(
    NAME dsm
    VERSION ${PROJECT_VERSION}
    NAMESPACE dsm
    BINARY_DIR ${PROJECT_BINARY_DIR}
    INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
    INCLUDE_DESTINATION include
    VERSION_HEADER dsm/version.h
    DISABLE_VERSION_SUFFIX YES
    COMPATIBILITY SameMajorVersion
  )
endif()
