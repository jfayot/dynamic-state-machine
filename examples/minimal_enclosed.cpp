#define DSM_LOGGER Log::ConsoleLogger

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

    return 0;
}
