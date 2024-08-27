#define DSM_LOGGER Log::ConsoleLogger

#include "dsm/dsm.hpp"
#include <cassert>
#include <iostream>

using namespace dsm;

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
