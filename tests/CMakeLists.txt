cmake_minimum_required(VERSION 3.16)

include(FetchContent)
FetchContent_Declare(googletest
  QUIET
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz
)

# configure build of googletest
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set_target_properties(gtest gmock gtest_main gmock_main
  PROPERTIES
    FOLDER googletest
)

add_executable(tests
  main.cpp
)

target_link_libraries(tests
  PRIVATE
    dsm::dsm
    gtest
    gmock
)

if (ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES GNU AND CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(tests
      PRIVATE
        "--coverage"
      )
    target_link_options(tests
      PRIVATE
        "--coverage"
      )
  endif()
endif()

include(GoogleTest)
gtest_discover_tests(tests
  DISCOVERY_TIMEOUT 240
)
