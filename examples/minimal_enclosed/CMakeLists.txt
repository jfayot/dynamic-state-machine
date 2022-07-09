cmake_minimum_required(VERSION 3.16)

add_executable(minimal_enclosed
  minimal_enclosed.cpp
)

target_link_libraries(minimal_enclosed
  PRIVATE
    dsm::dsm
)

set_target_properties(minimal_enclosed
  PROPERTIES
    FOLDER examples
)
