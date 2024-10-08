#define DSM_LOGGER Log::ConsoleLogger

#include "dsm/dsm.hpp"

using namespace dsm;

struct e1 : Event<e1>{};

struct sm : StateMachine<sm>{};
struct s0 : State<s0, sm>{
    void onEvent1(const e1& evt) { std::cout << "Received event " << evt.name() << std::endl; }
};

int main()
{
    sm sm;

    sm.addState<s0, Entry>();
    sm.addTransition<s0, e1, s0, &s0::onEvent1>();

    sm.start();
    sm.processEvent(e1{});

    std::cout << sm << std::endl;

    return 0;
}
