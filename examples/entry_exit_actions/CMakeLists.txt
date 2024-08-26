cmake_minimum_required(VERSION 3.16)

add_executable(entry_exit_actions
  entry_exit_actions.cpp
)

target_link_libraries(entry_exit_actions
  PRIVATE
    dsm::dsm
)

set_target_properties(entry_exit_actions
  PROPERTIES
    FOLDER examples
)
