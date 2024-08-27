#define DSM_LOGGER Log::ConsoleLogger

#include "dsm/dsm.hpp"

using namespace dsm;

struct e1 : Event<e1>{};
struct e2 : Event<e2>{};
struct e3 : Event<e3>{};
struct e4 : Event<e4>{};

struct sm : StateMachine<sm>{};
struct s0 : State<s0, sm>{};
struct s1 : State<s1, sm>{};
struct s2 : State<s2, sm>{};
struct s3 : State<s3, sm>{};
struct s4 : State<s4, sm>{};

int main()
{
    sm sm;

    sm.addState<s0, Entry>();
    sm.addState<s0, s1, 0, Entry>();
    sm.addState<s0, s2, 0>();
    sm.addState<s0, s3, 1, Entry>();
    sm.addState<s0, s4, 1>();

    sm.addTransition<s1, e1, s2>();
    sm.addTransition<s2, e2, s1>();
    sm.addTransition<s3, e3, s4>();
    sm.addTransition<s4, e4, s3>();

    sm.start();

    std::cout << sm << std::endl;
    
    sm.processEvent(e1{});

    std::cout << sm << std::endl;

    sm.processEvent(e3{});

    std::cout << sm << std::endl;

    return 0;
}
