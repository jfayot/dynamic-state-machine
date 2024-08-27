#define DSM_LOGGER Log::ConsoleLogger

#include "dsm/dsm.hpp"

using namespace dsm;

struct e0 : Event<e0>{};
struct e1 : Event<e1>{};
struct e2 : Event<e2>{};
struct e3 : Event<e3>{};
struct e4 : Event<e4>{};
struct e5 : Event<e5>{};

struct sm : StateMachine<sm>{};
struct s0 : State<s0, sm>{};
struct s1 : State<s1, sm>{};
struct s2 : State<s2, sm>{};
struct s3 : State<s3, sm>{};
struct s4 : State<s4, sm>{};
struct s5 : State<s5, sm>{};

int main()
{
    sm sm;

    sm.addState<s0, Entry>();
    sm.addState<s1>();
    sm.addState<s1, s2, Entry>();
    sm.addState<s1, s3>();
    sm.addState<s3, s4, Entry>();
    sm.addState<s3, s5>();

    sm.addTransition<s0, e0, s1>();
    sm.addTransition<s1, e1, s0>();
    sm.addTransition<s2, e2, s3>();
    sm.addTransition<s3, e3, s2>();
    sm.addTransition<s4, e4, s5>();
    sm.addTransition<s5, e5, s4>();

    sm.setHistory<s1>(History::Deep);

    sm.start();
    sm.processEvent(e0{});
    sm.processEvent(e2{});
    sm.processEvent(e4{});
    sm.processEvent(e1{});
    sm.processEvent(e0{});

    std::cout << sm << std::endl;

    return 0;
}
