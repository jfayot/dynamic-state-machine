# Dynamic StateMachine

A Dynamic StateMachine where states and transitions are created runtime.

[![GitHub version](https://badge.fury.io/gh/jfayot%2Fdynamic-state-machine.svg)](https://badge.fury.io/gh/jfayot%2Fdynamic-state-machine)
[![Project Status: Active – The project has reached a stable, usable state and is being actively developed.](https://www.repostatus.org/badges/latest/active.svg)](https://www.repostatus.org/#active)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://github.com/jfayot/dynamic-state-machine/blob/main/LICENSE)

[![Windows Build Status](https://github.com/jfayot/dynamic-state-machine/actions/workflows/windows.yml/badge.svg)](https://github.com/jfayot/dynamic-state-machine/actions/workflows/windows.yml)
[![Linux Build Status](https://github.com/jfayot/dynamic-state-machine/actions/workflows/linux.yml/badge.svg)](https://github.com/jfayot/dynamic-state-machine/actions/workflows/linux.yml)
[![MacOS Build Status](https://github.com/jfayot/dynamic-state-machine/actions/workflows/macos.yml/badge.svg)](https://github.com/jfayot/dynamic-state-machine/actions/workflows/macos.yml)

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

## Minimal examples

Considering the following minimal state machine, it can be coded as follows:

![basic-uml](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/jfayot/dynamic-state-machine/master/resources/basic.puml)

### Flat-style StateMachine setup

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

## Specify entry/exit actions

![entry_exit_actions](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/jfayot/dynamic-state-machine/master/resources/entry_exit_actions.puml)

```c++
struct e1 : Event<e1>{};

struct sm : StateMachine<sm>{};
struct s0 : State<s0, sm>
{
    void onExit() override { std::cout << "Leaving state " << this->name() << std::endl; }
};
struct s1 : State<s1, sm>
{
    void onEntry() override { std::cout << "Leaving state " << this->name() << std::endl; }
};

int main()
{
    sm sm;

    sm.addState<s0, Entry>();
    sm.addState<s1>();
    sm.addTransition<s0, e1, s1>();

    sm.start();
    sm.processEvent(e1{});

    return 0;
}
```
