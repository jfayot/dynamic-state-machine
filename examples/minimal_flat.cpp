#include "dsm/dsm.hpp"
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
