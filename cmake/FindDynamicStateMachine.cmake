include(FindPackageHandleStandardArgs)

set(_DYNAMIC_STATE_MACHINE_SEARCHES)

# Search DYNAMIC_STATE_MACHINE_ROOT first if it is set.
if(DYNAMIC_STATE_MACHINE_ROOT)
  set(_DYNAMIC_STATE_MACHINE_SEARCH_ROOT PATHS ${DYNAMIC_STATE_MACHINE_ROOT} NO_DEFAULT_PATH)
  list(APPEND _DYNAMIC_STATE_MACHINE_SEARCHES _DYNAMIC_STATE_MACHINE_SEARCH_ROOT)
endif()

# Try each search configuration.
foreach(search ${_DYNAMIC_STATE_MACHINE_SEARCHES})
  find_path(DYNAMIC_STATE_MACHINE_INCLUDE_DIR NAMES dsm/dsm.hpp ${${search}} PATH_SUFFIXES include)
endforeach()

mark_as_advanced(DYNAMIC_STATE_MACHINE_INCLUDE_DIR)

find_package_handle_standard_args(DynamicStateMachine REQUIRED_VARS DYNAMIC_STATE_MACHINE_INCLUDE_DIR)

if(DynamicStateMachine_FOUND)
    if(NOT TARGET DynamicStateMachine::DynamicStateMachine)
      add_library(DynamicStateMachine::DynamicStateMachine INTERFACE IMPORTED)
      set_target_properties(DynamicStateMachine::DynamicStateMachine PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${DYNAMIC_STATE_MACHINE_INCLUDE_DIR}")
    endif()
endif()
