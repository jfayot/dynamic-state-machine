#define DSM_LOGGER Log::ConsoleLogger

#include "dsm/dsm.hpp"

using namespace dsm;

struct e1 : Event<e1>{
    bool guard_flag;
    explicit e1(bool guard_flag) : guard_flag{ guard_flag } {}
};

struct sm : StateMachine<sm>{};
struct s0 : State<s0, sm>{
    void onEvent1(const e1& evt) { std::cout << "Received event " << evt.name() << std::endl; }
    bool guard(const e1& evt)
    {
        std::cout << "Guard " << (evt.guard_flag ? "accepted" : "rejected") << std::endl;
        return evt.guard_flag;
    }
};
struct s1 : State<s1, sm>{};

int main()
{
    sm sm;

    sm.addState<s0, Entry>();
    sm.addState<s1>();
    sm.addTransition<s0, e1, s0, &s0::onEvent1, &s0::guard, s1>();

    sm.start();
    sm.processEvent(e1{false});

    std::cout << sm << std::endl;

    sm.processEvent(e1{true});

    std::cout << sm << std::endl;

    return 0;
}
