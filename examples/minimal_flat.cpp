#define DSM_LOGGER Log::ConsoleLogger

#include "dsm/dsm.hpp"

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
    sm.processEvent(e1{});

    std::cout << sm << std::endl;

    return 0;
}
