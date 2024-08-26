#include "dsm/dsm.hpp"
#include <cassert>
#include <iostream>

using namespace dsm;

struct e1 : Event<e1>{};

struct sm : StateMachine<sm>{};
struct s0 : State<s0, sm>{
    void onEvent1(const e1& evt) { std::cout << "Received event " << evt.name() << std::endl; }
};
struct s1 : State<s1, sm>{};

int main()
{
    sm sm;

    sm.addState<s0, Entry>();
    sm.addState<s1>();
    sm.addTransition<s0, e1, s0, &s0::onEvent1, s1>();

    sm.start();
    sm.processEvent(e1{});

    return 0;
}
