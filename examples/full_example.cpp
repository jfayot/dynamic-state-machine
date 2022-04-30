#include "dsm/dsm.hpp"

#include <vector>
#include <iostream>
#include <assert.h>

using namespace dsm;

struct Event0 : Event<Event0> {};
struct Event1 : Event<Event1>
{
    std::string m_data;
    Event1(const std::string& data) : m_data{ data } {}
};
struct Event2 : Event<Event2> {};
struct Event3 : Event<Event3> {};
struct Event4 : Event<Event4> {};
struct ConnectEvt : Event<ConnectEvt> {};
struct DisconnectEvt : Event<DisconnectEvt> {};
struct DebriefEvt : Event<DebriefEvt> {};
struct PlayEvt : Event<PlayEvt> {};
struct PauseEvt : Event<PauseEvt> {
    bool m_allow;
    PauseEvt(bool allow) : Event<PauseEvt>{}, m_allow{ allow } {}
};
struct Tactical1Evt : Event<Tactical1Evt> {};
struct Tactical2Evt : Event<Tactical2Evt> {};
struct TacticalHandleEvt : Event<TacticalHandleEvt> {};
struct SafetyEvt : Event<SafetyEvt> {};
struct OrthoEvt : Event<OrthoEvt> {};
struct ExceptEvt : Event<ExceptEvt> {};

struct Store
{
    std::string data = "first";
};

struct Waiting;
struct Connected;
struct MissionManagement;
struct Debriefing;
struct PlayPause;
struct Play;
struct Pause;
struct Standard;
struct Tactical;
struct Safety;

struct SM : StateMachine<SM, Store>
{
    SM() : StateMachine<SM, Store>{ "topSm" } {}

    TStates getStates() override
    {
        return {
            createState<Waiting>("Waiting", true),
            createState<Connected>("Connected")
        };
    }
    TTransitions getTransitions() override
    {
        return {
            createTransition<Event0, SM, &SM::onEvent0>()
        };
    }
    void onEvent0(const Event0& ev)
    {
        std::cout << "SM::onEvent0 " << store()->data << std::endl;
    }
};

struct Waiting : State<Waiting, SM>
{
    Waiting()
    {
        assert(nullptr == store());
    }
    TTransitions getTransitions() override
    {
        return {
            createTransition<Event1, Waiting, &Waiting::onEvent1>(),
            createTransition<ConnectEvt, Connected>()
        };
    }
    void onEvent1(const Event1& ev)
    {
        std::cout << "Waiting::onEvent1 " << store()->data << " " << ev.m_data << std::endl;
        transit<Connected>();
    }
};

struct Connected : State<Connected, SM>
{
    TStates getStates() override
    {
        return {
            createState<MissionManagement>("MissionManagement", true),
            createState<Debriefing>("Debriefing")
        };
    }
    TTransitions getTransitions() override
    {
        return {
            createTransition<Event2, Connected, &Connected::onEvent2>(),
            createTransition<Event3, Connected, &Connected::onEvent3>(),
            createTransition<DisconnectEvt, Waiting>()
        };
    }
    History getHistory() override
    {
        return History::Shallow;
    }
    void onEvent2(const Event2& ev)
    {
        std::cout << "Connected::onEvent2" << std::endl;
        postEvent(Event3{});
    }
    void onEvent3(const Event3& ev)
    {
        std::cout << "Connected::onEvent3" << std::endl;
    }
    void onEvent4(const Event4& ev)
    {
        std::cout << "Connected::onEvent4" << std::endl;
    }
};

struct MissionManagement : State<MissionManagement, SM>
{
    TTransitions getTransitions() override
    {
        return {
            createTransition<DebriefEvt, MissionManagement, &MissionManagement::onDebrief, Debriefing>()
        };
    }
    void onEvent3(const Event3& ev)
    {
        std::cout << "MissionManagement::onEvent3" << std::endl;
    }
    void onDebrief(const DebriefEvt& ev)
    {
        std::cout << "MissionManagement::onDebrief" << std::endl;
    }
};

struct Debriefing : State<Debriefing, SM>
{
    TStates getStates() override
    {
        return {
            createState<PlayPause>("PlayPause", true)
        };
    }
    History getHistory() override
    {
        return History::Deep;
    }
};

struct PlayPause : State<PlayPause, SM>
{
    TStates getStates() override
    {
        return {
            createState<Pause>("Pause", true),
            createState<Play>("Play"),
            createState<Standard, 1>("Standard", true),
            createState<Tactical, 1>("Tactical"),
            createState<Safety, 1>("Safety")
        };
    }
};

struct Play : State<Play, SM>
{
public:
    static bool m_allow;
    bool allow()
    {
        return m_allow;
    }
    TTransitions getTransitions() override
    {
        return {
            createTransition<ExceptEvt, Play, &Play::onExcept>(),
            createTransition<PauseEvt, Play, &Play::allow, Pause>()
        };
    }
    void onExcept(const ExceptEvt& evt)
    {
        std::cout << "Play::onExcept" << std::endl;
        throw "you've been thrown";
    }
    void onError(std::exception_ptr ex) override
    {
        try
        {
            std::rethrow_exception(ex);
        }
        catch (const char* ex)
        {
            std::cout << "Play::onError " << ex << std::endl;
        }
    }
};
bool Play::m_allow = false;

struct Pause : State<Pause, SM>
{
    TTransitions getTransitions() override
    {
        return {
            createTransition<PlayEvt, Play>(),
            createTransition<OrthoEvt, Pause, &Pause::onOrtho>()
        };
    }
    void onOrtho(const OrthoEvt& evt)
    {
        std::cout << "Pause::onOrtho" << std::endl;
    }
};

struct Standard : State<Standard, SM>
{
    TTransitions getTransitions() override
    {
        return {
            createTransition<Tactical1Evt, Tactical>(),
            createTransition<OrthoEvt, Standard, &Standard::onOrtho>()
        };
    }
    void onOrtho(const OrthoEvt& evt)
    {
        std::cout << "Standard::onOrtho" << std::endl;
    }
};

struct Tactical : State<Tactical, SM>
{
    TTransitions getTransitions() override
    {
        return {
            createTransition<SafetyEvt, Safety>(),
            createTransition<TacticalHandleEvt, Tactical, &Tactical::onHandleTactical>()
        };
    }
    void onEntry() override
    {
        std::cout << "entering " << name() << " from " << (trigEvent<Tactical1Evt>() ? "Standard" : (trigEvent<Tactical2Evt>() ? "Safety" : "Unknown")) << std::endl;
    }
    void onHandleTactical(const TacticalHandleEvt& evt)
    {
        std::cout << "Tactical::onHandleTactical" << std::endl;
    }
};

struct Safety : State<Safety, SM>
{
    TTransitions getTransitions() override
    {
        return {
            createTransition<Tactical2Evt, Tactical>()
        };
    }
    void onEntry() override
    {
        std::cout << "Safety::onEntry" << std::endl;
        throw "you've been thrown";
    }
    void onError(std::exception_ptr ex) override
    {
        try
        {
            std::rethrow_exception(ex);
        }
        catch (const char* ex)
        {
            std::cout << "Safety::onError " << ex << std::endl;
        }
    }
};

struct Visitor : IStateVisitor
{
    std::string searchedState;
    std::vector<std::string> states;
    bool found = false;

    Visitor(const std::string& state) : searchedState{ state } {}

    void visit(const StateBase* state) override
    {
        states.push_back(state->name());
        if (state->name() == searchedState) found = true;
    }
};

int main()
{
    SM sm;
    sm.setup();

    std::cout << sm.store()->data << std::endl;

    sm.store()->data = "second";

    std::cout << sm << std::endl << std::endl;

    std::cout << "sending ev0..." << std::endl;
    sm.processEvent(Event0{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "starting..." << std::endl;
    sm.start();
    std::cout << sm << std::endl << std::endl;

    std::cout << "sending ev0..." << std::endl;
    sm.processEvent(Event0{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "stopping..." << std::endl;
    sm.stop();
    std::cout << sm << std::endl << std::endl;

    std::cout << "starting..." << std::endl;
    sm.start();
    std::cout << sm << std::endl << std::endl;

    std::cout << "sending ev1..." << std::endl;
    sm.processEvent(Event1{ "pouic" });
    std::cout << sm << std::endl << std::endl;

    std::cout << "sending disconnect..." << std::endl;
    sm.processEvent(DisconnectEvt{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "sending ev2..." << std::endl;
    sm.processEvent(Event2{});
    std::cout << std::endl;

    std::cout << "sending connect..." << std::endl;
    sm.processEvent(ConnectEvt{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "visiting Debriefing" << "..." << std::endl;
    Visitor visitor1{ "Debriefing" };
    sm.visit(visitor1);
    for (const auto& state : visitor1.states)
    {
        std::cout << state << ".";
    }
    std::cout << std::endl << visitor1.found << std::endl;
    Visitor visitor2{ "MissionManagement" };
    std::cout << "visiting MissionManagement" << "..." << std::endl;
    sm.visit(visitor2);
    for (const auto& state : visitor2.states)
    {
        std::cout << state << ".";
    }
    std::cout << std::endl << visitor2.found << std::endl << std::endl;

    std::cout << "sending ev1..." << std::endl;
    sm.processEvent(Event1{ "pouic" });
    std::cout << "sending ev2..." << std::endl;
    sm.processEvent(Event2{});
    std::cout << "sending ev3..." << std::endl;
    sm.processEvent(Event3{});
    std::cout << std::endl;

    std::cout << "sending debrief..." << std::endl;
    sm.processEvent(DebriefEvt{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "sending ortho..." << std::endl;
    sm.processEvent(OrthoEvt{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "sending play..." << std::endl;
    sm.processEvent(PlayEvt{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "sending tactic1..." << std::endl;
    sm.processEvent(Tactical1Evt{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "sending safety..." << std::endl;
    sm.processEvent(SafetyEvt{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "defer tactical" << std::endl;
    sm.deferEvent(TacticalHandleEvt{});

    std::cout << "sending tactic2..." << std::endl;
    sm.processEvent(Tactical2Evt{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "sending disconnect..." << std::endl;
    sm.processEvent(DisconnectEvt{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "sending connect..." << std::endl;
    sm.processEvent(ConnectEvt{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "disallow pause..." << std::endl;
    Play::m_allow = false;
    std::cout << "sending pause..." << std::endl;
    sm.processEvent(PauseEvt{ false });
    std::cout << sm << std::endl << std::endl;

    std::cout << "allow pause..." << std::endl;
    Play::m_allow = true;
    std::cout << "sending pause..." << std::endl;
    sm.processEvent(PauseEvt{ true });
    std::cout << sm << std::endl << std::endl;

    std::cout << "posting play..." << std::endl;
    sm.postEvent(PlayEvt{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "sending except..." << std::endl;
    sm.processEvent(ExceptEvt{});
    std::cout << sm << std::endl << std::endl;

    std::cout << "stopping..." << std::endl;
    sm.stop();
    std::cout << sm << std::endl;

    return 0;
}
