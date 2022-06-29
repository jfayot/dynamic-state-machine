cmake_minimum_required(VERSION 3.16)

# for IDE tree structure
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMakePredefinedTargets")

# lib's version
set(version 0.0.1)

project(DynamicStateMachine
  VERSION ${version}
  LANGUAGES CXX)

# make cache variables for install destinations
include(GNUInstallDirs)

# check if project is standalone or embedded
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(IS_ROOT_PROJECT ON)
else()
    set(IS_ROOT_PROJECT OFF)
endif()

option(BUILD_EXAMPLES "Build examples" ${IS_ROOT_PROJECT})
option(BUILD_TESTS "Build tests" ${IS_ROOT_PROJECT})
option(ENABLE_COVERAGE "Enable coverage" ${IS_ROOT_PROJECT})

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

if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if (BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(ENABLE_INSTALL)
  install(TARGETS dsm
     EXPORT dsmTargets
     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(FILES include/dsm/dsm.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dsm)

  install(EXPORT dsmTargets
     FILE dsmTargets.cmake
     NAMESPACE dsm::
     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dsm
  )

  include(CMakePackageConfigHelpers)

  # generate package's config file
  configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/dsmConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dsm
  )

  # generate the version file for the config file
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/dsmConfigVersion.cmake
    VERSION ${version}
    COMPATIBILITY AnyNewerVersion
  )

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dsmConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/dsmConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dsm
  )
endif()
