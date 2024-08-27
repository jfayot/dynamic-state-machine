# Dynamic StateMachine

A Dynamic StateMachine where states and transitions are created runtime.

## Project status

![GitHub Release](https://img.shields.io/github/v/release/jfayot/dynamic-state-machine)
[![Project Status: Active – The project has reached a stable, usable state and is being actively developed.](https://www.repostatus.org/badges/latest/active.svg)](https://www.repostatus.org/#active)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://github.com/jfayot/dynamic-state-machine/blob/main/LICENSE)

## Build status

[![Windows Build Status](https://github.com/jfayot/dynamic-state-machine/actions/workflows/windows.yml/badge.svg)](https://github.com/jfayot/dynamic-state-machine/actions/workflows/windows.yml)
[![Linux Build Status](https://github.com/jfayot/dynamic-state-machine/actions/workflows/linux.yml/badge.svg)](https://github.com/jfayot/dynamic-state-machine/actions/workflows/linux.yml)
[![MacOS Build Status](https://github.com/jfayot/dynamic-state-machine/actions/workflows/macos.yml/badge.svg)](https://github.com/jfayot/dynamic-state-machine/actions/workflows/macos.yml)

## Quality

[![Tests Status](https://github.com/jfayot/dynamic-state-machine/actions/workflows/tests.yml/badge.svg)](https://github.com/jfayot/dynamic-state-machine/actions/workflows/tests.yml)
[![codecov](https://codecov.io/gh/jfayot/dynamic-state-machine/branch/main/graph/badge.svg)](https://codecov.io/gh/jfayot/dynamic-state-machine)
[![CodeQL](https://github.com/jfayot/dynamic-state-machine/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/jfayot/dynamic-state-machine/actions/workflows/codeql-analysis.yml)
[![Coverity scan](https://scan.coverity.com/projects/25247/badge.svg)](https://scan.coverity.com/projects/dsm)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/94a50b94b2f34494bd7c12426ad3fc88)](https://www.codacy.com/gh/jfayot/dynamic-state-machine/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=jfayot/dynamic-state-machine&amp;utm_campaign=Badge_Grade)

## Features

* Flat-style and enclosed-style setup
* Unlimited number of states and transitions
* Hierarchical states
* Entry and exit actions
* Internal and external transitions
* Transition actions
* Transition guard conditions
* State history (deep and shallow)
* Event processing
* Event deferring
* Event posting
* Orthogonal regions
* Optional shared storage
* Visitable
* Observable (check current active states)
* Exception handling
* Platform independent, C++17
* Header only
* No external dependencies except STL
* Moderate use of templates

## Build and install

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/path/to/install/directory
cmake --build build --target install
```

## Usage

Either build and install dsm package and then use cmake's findPackage to include it in your project:

```cmake
cmake_minimum_required(VERSION 3.16)

project(your_project
  LANGUAGES CXX
)

add_executable(your_target ${CMAKE_CURRENT_LIST_DIR}/main.cpp)

findPackage(dsm VERSION 0.1.1 REQUIRED)

target_link_libraries(your_target PRIVATE dsm::dsm)
```

Or include it directly in you project using CPM:

```cmake
cmake_minimum_required(VERSION 3.16)

project(your_project
  LANGUAGES CXX
)

# download CPM.cmake
file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.2/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH SHA256=c8cdc32c03816538ce22781ed72964dc864b2a34a310d3b7104812a5ca2d835d
)

include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:jfayot/dynamic-state-machine@0.1.1")

add_executable(your_target ${CMAKE_CURRENT_LIST_DIR}/main.cpp)

target_link_libraries(your_target PRIVATE dsm::dsm)
```

## Basic example

Considering the following minimal state machine, it can be coded as follows:

![basic-uml](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/jfayot/dynamic-state-machine/master/resources/basic.puml)

### Flat-style StateMachine setup

[minimal_flat.cpp](./examples/minimal_flat.cpp)

```c++
#include "dsm/dsm.hpp"
#include <cassert>

using namespace dsm;

struct e1 : Event<e1> {};

struct minimal : StateMachine<minimal> {};
struct s0 : State<s0, minimal> {};
struct s1 : State<s1, minimal> {};

int main()
{
    minimal sm;

    sm.addState<s0, Entry>();
    sm.addState<s1>();
    sm.addTransition<s0, e1, s1>();

    sm.start();
    assert(sm.checkStates<s0>());
    std::cout << sm << std::endl;
    sm.processEvent(e1{});
    assert(sm.checkStates<s1>());
    std::cout << sm << std::endl;
}
```

### Enclosed-style StateMachine setup

[minimal_enclosed.cpp](./examples/minimal_enclosed.cpp)

```c++
#include "dsm/dsm.hpp"
#include <cassert>

using namespace dsm;

struct minimal;
struct e1 : Event<e1> {};
struct s1 : State<s1, minimal> {};
struct s0 : State<s0, minimal> {
    TTransitions getTransitions() override {
        return { createTransition<e1, s1>() };
    }
};
struct minimal : StateMachine<minimal> {
    TStates getStates() override {
        return { createState<s0, Entry>(), createState<s1>() };
    }
};

int main()
{
    minimal sm;

    sm.setup();

    sm.start();
    assert(sm.checkStates<s0>());
    std::cout << sm << std::endl;
    sm.processEvent(e1{});
    assert(sm.checkStates<s1>());
    std::cout << sm << std::endl;
}
```

## Other examples

| Title              | UML | Source file |
| ------------------ | --- | ----------- |
| entry/exit actions | ![entry_exit_actions](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/jfayot/dynamic-state-machine/master/resources/entry_exit_actions.puml) | [transition_action.cpp](./examples/transition_action.cpp) |
| transition action  | ![transition_action](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/jfayot/dynamic-state-machine/master/resources/transition_action.puml) | [transition_action.cpp](./examples/transition_action.cpp) |
| transition guard   | ![transition_guard](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/jfayot/dynamic-state-machine/master/resources/transition_guard.puml) | [transition_guard.cpp](./examples/transition_guard.cpp) |
| self transition    | ![self_transition](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/jfayot/dynamic-state-machine/master/resources/self_transition.puml) | [self_transition.cpp](./examples/self_transition.cpp) |
| composite state    | ![composite_state](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/jfayot/dynamic-state-machine/master/resources/composite_state.puml) | [composite_state.cpp](./examples/composite_state.cpp) |
| shallow history    | ![shallow_history](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/jfayot/dynamic-state-machine/master/resources/shallow_history.puml) | [shallow_history.cpp](./examples/shallow_history.cpp) |
| deep history    | ![deep_history](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/jfayot/dynamic-state-machine/master/resources/deep_history.puml) | [deep_history.cpp](./examples/deep_history.cpp) |
