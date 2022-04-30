#include "dsm/dsm.hpp"
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
