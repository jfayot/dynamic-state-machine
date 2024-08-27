cmake_minimum_required(VERSION 3.16)

CPMGetPackage(googletest)

add_executable(tests
  main.cpp
)

target_link_libraries(tests
  PRIVATE
    dsm::dsm
    gtest_main
    gmock_main
)

if (ENABLE_DSM_COVERAGE)
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
enable_testing()
gtest_discover_tests(tests)
