cmake_minimum_required(VERSION 3.16)

add_executable(transition_action
  transition_action.cpp
)

target_link_libraries(transition_action
  PRIVATE
    dsm::dsm
)

set_target_properties(transition_action
  PROPERTIES
    FOLDER examples
)
