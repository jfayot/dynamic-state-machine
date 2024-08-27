#define DSM_LOGGER Log::ConsoleLogger

#include "dsm/dsm.hpp"

using namespace dsm;

struct e1 : Event<e1>{};

struct sm : StateMachine<sm>{};
struct s0 : State<s0, sm>{};
struct s1 : State<s1, sm>{};
struct s2 : State<s2, sm>{};

int main()
{
    sm sm;

    sm.addState<s0, Entry>();
    sm.addState<s0, s1, Entry>();
    sm.addState<s0, s2>();
    sm.addTransition<s1, e1, s2>();

    sm.start();
    sm.processEvent(e1{});

    return 0;
}
