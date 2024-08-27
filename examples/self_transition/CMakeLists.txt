cmake_minimum_required(VERSION 3.16)

add_executable(self_transition
  self_transition.cpp
)

target_link_libraries(self_transition
  PRIVATE
    dsm::dsm
)

set_target_properties(self_transition
  PROPERTIES
    FOLDER examples
)
