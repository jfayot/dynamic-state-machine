#define DSM_LOGGER Log::ConsoleLogger

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

    return 0;
}
