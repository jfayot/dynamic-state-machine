# Dynamic StateMachine

A Dynamic StateMachine where states and transitions are created runtime.

[![GitHub version](https://badge.fury.io/gh/jfayot%2FDynamic-State-Machine.svg)](https://badge.fury.io/gh/jfayot%2FDynamic-State-Machine)
[![Project Status: Active – The project has reached a stable, usable state and is being actively developed.](https://www.repostatus.org/badges/latest/active.svg)](https://www.repostatus.org/#active)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://github.com/jfayot/Dynamic-State-Machine/blob/main/LICENSE)

[![Windows Build Status](https://github.com/jfayot/Dynamic-State-Machine/actions/workflows/windows.yml/badge.svg)](https://github.com/jfayot/Dynamic-State-Machine/actions/workflows/windows.yml)
[![Linux Build Status](https://github.com/jfayot/Dynamic-State-Machine/actions/workflows/linux.yml/badge.svg)](https://github.com/jfayot/Dynamic-State-Machine/actions/workflows/linux.yml)
[![MacOS Build Status](https://github.com/jfayot/Dynamic-State-Machine/actions/workflows/macos.yml/badge.svg)](https://github.com/jfayot/Dynamic-State-Machine/actions/workflows/macos.yml)

[![Tests Status](https://github.com/jfayot/Dynamic-State-Machine/actions/workflows/tests.yml/badge.svg)](https://github.com/jfayot/Dynamic-State-Machine/actions/workflows/tests.yml)
[![codecov](https://codecov.io/gh/jfayot/Dynamic-State-Machine/branch/main/graph/badge.svg)](https://codecov.io/gh/jfayot/Dynamic-State-Machine)
[![CodeQL](https://github.com/jfayot/Dynamic-state-machine/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/jfayot/Dynamic-state-machine/actions/workflows/codeql-analysis.yml)
[![Coverity scan](https://scan.coverity.com/projects/25036/badge.svg)](https://scan.coverity.com/projects/jfayot-dynamic-state-machine)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/jfayot/Dynamic-State-Machine.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/jfayot/Dynamic-State-Machine/context:cpp)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/jfayot/Dynamic-State-Machine.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/jfayot/Dynamic-State-Machine/alerts/)

## Features

* Flat-style and enclosed-style setup
* Hierarchical states
* Entry and exit actions
* Simple internal and external transitions
* Transition actions
* Transition guards conditions
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
* Fast compile time
* No external dependencies except STL

## Minimal examples

### Flat-style StateMachine setup

```c++
#include "DynamicStateMachine/StateMachine.hpp"
#include <cassert>

using namespace dsm;

struct e1 : Event<e1> {};

struct minimal : StateMachine<minimal> {
    minimal() : StateMachine<minimal>{ "minimal" } {}
};

struct s0 : State<s0, minimal> {};
struct s1 : State<s1, minimal> {};

int main() {
    minimal sm;

    sm.addState<s0>("s0", true);
    sm.addState<s1>("s1");
    sm.addTransition<s0, e1, s1>();

    sm.start();
    assert(sm.checkStates<s0>());
    sm.processEvent(e1{});
    assert(sm.checkStates<s1>());
}
```
### Enclosed-style StateMachine setup

```c++
#include "DynamicStateMachine/StateMachine.hpp"
#include <cassert>

using namespace dsm;

struct e1 : Event<e1> {};

struct s0;
struct s1;

struct minimal : StateMachine<minimal> {
    minimal() : StateMachine<minimal>{ "minimal" } {}
    TStates getStates() override {
        return { createState<s0>("s0", true), createState<s1>("s1") };
    }
};

struct s0 : State<s0, minimal> {
    TTransitions getTransitions() override {
        return { createTransition<e1, s1>() };
    }
};

struct s1 : State<s1, minimal> {};

int main() {
    minimal sm;

    sm.setup();

    sm.start();
    assert(sm.checkStates<s0>());
    sm.processEvent(e1{});
    assert(sm.checkStates<s1>());
}
```

