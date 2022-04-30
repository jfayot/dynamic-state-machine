cmake_minimum_required(VERSION 3.16)

add_executable(full_example
  full_example.cpp
)

add_executable(minimal_flat
  minimal_flat.cpp
)

add_executable(minimal_enclosed
  minimal_enclosed.cpp
)

target_link_libraries(full_example
  PRIVATE
    dsm::dsm
)

target_link_libraries(minimal_flat
  PRIVATE
    dsm::dsm
)

target_link_libraries(minimal_enclosed
  PRIVATE
    dsm::dsm
)

set_target_properties(full_example minimal_flat minimal_enclosed
  PROPERTIES
    FOLDER examples
)
