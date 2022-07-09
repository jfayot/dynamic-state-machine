#include "dsm/dsm.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;

#include <string>

using namespace dsm;

struct Store
{
    std::string data;
};

struct e0 : Event<e0>{};
struct e1 : Event<e1> {};
struct e2 : Event<e2> {};
struct e3 : Event<e3> {};
struct s0;
struct s1;
struct s2;
struct s3;
struct s4;

struct sm : StateMachine<sm, Store>
{
    MOCK_METHOD(void, onEntry, ());
    MOCK_METHOD(void, onExit, ());
    MOCK_METHOD(void, onError, (std::exception_ptr));
    MOCK_METHOD(TStates, getStates, ());
    MOCK_METHOD(TTransitions, getTransitions, ());
};

struct s0 : State<s0, sm>
{
    MOCK_METHOD(void, onEntry, ());
    MOCK_METHOD(void, onExit, ());
    MOCK_METHOD(void, onError, (std::exception_ptr));
    MOCK_METHOD(TStates, getStates, ());
    MOCK_METHOD(TTransitions, getTransitions, ());
    MOCK_METHOD(bool, guard, (const e0&));
    MOCK_METHOD(void, onEvent0, (const e0&));
};

struct s1 : State<s1, sm>
{
    MOCK_METHOD(void, onEntry, ());
    MOCK_METHOD(void, onExit, ());
    MOCK_METHOD(void, onError, (std::exception_ptr));
    MOCK_METHOD(TStates, getStates, ());
    MOCK_METHOD(TTransitions, getTransitions, ());
    MOCK_METHOD(void, onEvent1, (const e1&));
};

struct s2 : State<s2, sm>
{
    MOCK_METHOD(void, onEntry, ());
    MOCK_METHOD(void, onExit, ());
    MOCK_METHOD(void, onError, (std::exception_ptr));
    MOCK_METHOD(TTransitions, getTransitions, ());
};

struct s3 : State<s3, sm>
{
    MOCK_METHOD(void, onError, (std::exception_ptr));
    MOCK_METHOD(TTransitions, getTransitions, ());
};

struct s4 : State<s4, sm>
{
    MOCK_METHOD(void, onError, (std::exception_ptr));
    MOCK_METHOD(TTransitions, getTransitions, ());
};

struct s5 : State<s5, sm>
{
};

struct Visitor : IStateVisitor
{
    std::string searchedState;
    std::vector<std::string> states;
    bool found = false;

    Visitor() {}
    explicit Visitor(const std::string& state) : searchedState{ state } {}

    void visit(const StateBase* state) override
    {
        states.push_back(state->name());
        if (!searchedState.empty() && state->name() == searchedState) found = true;
    }
};

class Common_StateMachine : public ::testing::Test
{
    virtual void SetUp() override
    {
    }

    virtual void TearDown() override
    {
        _sm.stop();
        _sm.clear();
    }

protected:
    NiceMock<sm> _sm;
};

TEST_F(Common_StateMachine, test_check_states)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<s0, NiceMock<s1>, Entry>();
    _sm.addState<s1, NiceMock<s2>, Entry>();

    ASSERT_FALSE((_sm.checkStates<>()));
    ASSERT_FALSE((_sm.checkStates<sm>()));
    ASSERT_FALSE((_sm.checkStates<s0>()));
    ASSERT_FALSE((_sm.checkStates<s1>()));
    ASSERT_FALSE((_sm.checkStates<s2>()));
    ASSERT_FALSE((_sm.checkStates<s3>()));

    _sm.start();

    ASSERT_FALSE((_sm.checkStates<>()));
    ASSERT_FALSE((_sm.checkStates<s3>()));

    ASSERT_TRUE((_sm.checkStates<sm>()));
    ASSERT_TRUE((_sm.checkStates<s0>()));
    ASSERT_TRUE((_sm.checkStates<s1>()));
    ASSERT_TRUE((_sm.checkStates<s2>()));

    ASSERT_FALSE((_sm.checkStates<sm, sm>()));
    ASSERT_FALSE((_sm.checkStates<sm, s1>()));
    ASSERT_FALSE((_sm.checkStates<sm, s2>()));

    ASSERT_FALSE((_sm.checkStates<s0, sm>()));
    ASSERT_FALSE((_sm.checkStates<s0, s0>()));
    ASSERT_FALSE((_sm.checkStates<s0, s2>()));

    ASSERT_FALSE((_sm.checkStates<s1, sm>()));
    ASSERT_FALSE((_sm.checkStates<s1, s0>()));
    ASSERT_FALSE((_sm.checkStates<s1, s1>()));

    ASSERT_FALSE((_sm.checkStates<s2, sm>()));
    ASSERT_FALSE((_sm.checkStates<s2, s0>()));
    ASSERT_FALSE((_sm.checkStates<s2, s1>()));
    ASSERT_FALSE((_sm.checkStates<s2, s2>()));

    ASSERT_TRUE((_sm.checkStates<sm, s0>()));
    ASSERT_TRUE((_sm.checkStates<s0, s1>()));
    ASSERT_TRUE((_sm.checkStates<s1, s2>()));

    ASSERT_FALSE((_sm.checkStates<sm, s0, s2>()));
    ASSERT_FALSE((_sm.checkStates<sm, s1, s2>()));

    ASSERT_TRUE((_sm.checkStates<sm, s0, s1>()));

    ASSERT_TRUE((_sm.checkStates<sm, s0, s1, s2>()));
}

TEST_F(Common_StateMachine, test_add_existing_state)
{
    EXPECT_CALL(_sm, onError(_)).Times(2);
    _sm.addState<NiceMock<s0>>();

    _sm.addState<NiceMock<s0>>();

    ON_CALL(_sm, getStates()).WillByDefault(Return(TStates{ _sm.createState<NiceMock<s0>>() }));
    _sm.setup();
}

TEST_F(Common_StateMachine, test_add_existing_state_in_ancestor)
{
    EXPECT_CALL(_sm, onError(_)).Times(1);
    _sm.addState<NiceMock<s0>>();
    _sm.addState<s0, NiceMock<s1>>();

    _sm.addState<s1, NiceMock<s0>>();

    s1* _s1 = _sm.getState<s1>();
    EXPECT_CALL(*_s1, onError(_)).Times(1);
    ON_CALL(*_s1, getStates()).WillByDefault(Return(TStates{ _s1->createState<NiceMock<s0>>() }));
    _sm.setup();
}

TEST_F(Common_StateMachine, test_add_existing_state_in_descendant)
{
    EXPECT_CALL(_sm, onError(_)).Times(2);
    _sm.addState<NiceMock<s0>>();
    _sm.addState<s0, NiceMock<s1>>();

    _sm.addState<NiceMock<s1>>();

    ON_CALL(_sm, getStates()).WillByDefault(Return(TStates{ _sm.createState<NiceMock<s1>>() }));
    _sm.setup();
}

TEST_F(Common_StateMachine, test_add_existing_transition)
{
    EXPECT_CALL(_sm, onError(_)).Times(1);
    _sm.addState<NiceMock<s0>>();
    _sm.addState<NiceMock<s1>>();

    _sm.addTransition<s0, e0, s1>();
    _sm.addTransition<s0, e0, s1>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onError(_)).Times(1);
    ON_CALL(*_s0, getTransitions()).WillByDefault(Return(TTransitions{ _s0->createTransition<e0, s1>() }));
    _sm.setup();
}

TEST_F(Common_StateMachine, test_add_existing_transition_in_sibling)
{
    EXPECT_CALL(_sm, onError(_)).Times(0);
    _sm.addState<NiceMock<s0>>();
    _sm.addState<NiceMock<s1>>();
    _sm.addState<NiceMock<s2>>();
    _sm.addState<NiceMock<s3>>();

    _sm.addTransition<s0, e3, s3>();
    _sm.addTransition<s1, e3, s3>();

    s2* _s2 = _sm.getState<s2>();
    EXPECT_CALL(*_s2, onError(_)).Times(0);
    ON_CALL(*_s2, getTransitions()).WillByDefault(Return(TTransitions{ _s2->createTransition<e2, s3>() }));
    _sm.setup();
}

TEST_F(Common_StateMachine, test_add_existing_transition_in_ancestor)
{
    EXPECT_CALL(_sm, onError(_)).Times(0);
    _sm.addState<NiceMock<s0>>();
    _sm.addState<NiceMock<s1>>();
    _sm.addState<s0, NiceMock<s2>>();
    _sm.addState<s0, NiceMock<s3>>();

    _sm.addTransition<s0, e0, s1>();
    _sm.addTransition<s2, e0, s3>();

    s3* _s3 = _sm.getState<s3>();
    EXPECT_CALL(*_s3, onError(_)).Times(0);
    ON_CALL(*_s3, getTransitions()).WillByDefault(Return(TTransitions{ _s3->createTransition<e0, s2>() }));
    _sm.setup();
}

TEST_F(Common_StateMachine, test_add_existing_transition_in_descendant)
{
    EXPECT_CALL(_sm, onError(_)).Times(0);
    _sm.addState<NiceMock<s0>>();
    _sm.addState<NiceMock<s1>>();
    _sm.addState<s0, NiceMock<s2>>();
    _sm.addState<s0, NiceMock<s3>>();

    _sm.addTransition<s2, e0, s3>();
    _sm.addTransition<s0, e0, s1>();

    s1* _s1 = _sm.getState<s1>();
    EXPECT_CALL(*_s1, onError(_)).Times(0);
    ON_CALL(*_s1, getTransitions()).WillByDefault(Return(TTransitions{ _s1->createTransition<e0, s0>() }));
    _sm.setup();
}

TEST_F(Common_StateMachine, test_add_existing_transition_in_orthogonal_region)
{
    EXPECT_CALL(_sm, onError(_)).Times(0);
    _sm.addState<NiceMock<s0>>();
    _sm.addState<NiceMock<s1>>();
    _sm.addState<NiceMock<s2>, 1>();
    _sm.addState<NiceMock<s3>, 1>();
    _sm.addState<NiceMock<s4>, 2>();
    _sm.addState<NiceMock<s5>, 2>();

    _sm.addTransition<s0, e0, s1>();
    _sm.addTransition<s2, e0, s3>();

    s4* _s4 = _sm.getState<s4>();
    EXPECT_CALL(*_s4, onError(_)).Times(0);
    ON_CALL(*_s4, getTransitions()).WillByDefault(Return(TTransitions{ _s4->createTransition<e0, s5>() }));
    _sm.setup();
}

TEST_F(Common_StateMachine, test_add_transition_crossing_region)
{
    EXPECT_CALL(_sm, onError(_)).Times(1);
    _sm.addState<NiceMock<s0>, 0>();
    _sm.addState<NiceMock<s1>, 1>();

    _sm.addTransition<s0, e0, s1>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onError(_)).Times(1);
    ON_CALL(*_s0, getTransitions()).WillByDefault(Return(TTransitions{ _s0->createTransition<e0, s1>() }));
    _sm.setup();
}

TEST_F(Common_StateMachine, test_add_transition_to_parent)
{
    EXPECT_CALL(_sm, onError(_)).Times(1);
    _sm.addState<NiceMock<s0>>();
    _sm.addState<s0, NiceMock<s1>>();

    _sm.addTransition<s1, e0, s0>();

    s1* _s1 = _sm.getState<s1>();
    EXPECT_CALL(*_s1, onError(_)).Times(1);
    ON_CALL(*_s1, getTransitions()).WillByDefault(Return(TTransitions{ _s1->createTransition<e0, s0>() }));
    _sm.setup();
}

TEST_F(Common_StateMachine, test_add_transition_to_child)
{
    EXPECT_CALL(_sm, onError(_)).Times(1);
    _sm.addState<NiceMock<s0>>();
    _sm.addState<s0, NiceMock<s1>>();

    _sm.addTransition<s0, e0, s1>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onError(_)).Times(1);
    ON_CALL(*_s0, getTransitions()).WillByDefault(Return(TTransitions{ _s0->createTransition<e0, s1>() }));
    _sm.setup();
}

TEST_F(Common_StateMachine, test_entry_exit)
{
    _sm.addState<s0, Entry>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onEntry()).Times(1);
    EXPECT_CALL(*_s0, onExit()).Times(1);

    ASSERT_FALSE(_sm.checkStates<s0>());

    _sm.start();

    ASSERT_TRUE((_sm.checkStates<s0>()));

    _sm.stop();

    ASSERT_FALSE(_sm.checkStates<s0>());
}

TEST_F(Common_StateMachine, test_start_sm_without_initial_state)
{
    _sm.addState<NiceMock<s0>>();
    _sm.addState<NiceMock<s1>>();

    _sm.start();

    ASSERT_TRUE((_sm.checkStates<sm>()));
    ASSERT_FALSE(_sm.checkStates<s0>());
    ASSERT_FALSE(_sm.checkStates<s1>());
}

TEST_F(Common_StateMachine, test_start_sm_with_initial_state)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();

    _sm.start();

    ASSERT_TRUE((_sm.checkStates<s0>()));
    ASSERT_FALSE((_sm.checkStates<s1>()));
}

TEST_F(Common_StateMachine, test_external_transition_not_started)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s1>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onEvent0(_)).Times(0);
    EXPECT_CALL(*_s0, guard(_)).Times(0);

    _sm.processEvent(e0{});

    ASSERT_FALSE(_sm.checkStates<sm>());
    ASSERT_FALSE(_sm.checkStates<s0>());
    ASSERT_FALSE(_sm.checkStates<s1>());
}

TEST_F(Common_StateMachine, test_external_transition)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s1>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onEvent0(_)).Times(0);
    EXPECT_CALL(*_s0, guard(_)).Times(0);

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s1>()));
}

TEST_F(Common_StateMachine, test_external_transition_action)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s0, &s0::onEvent0, s1>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onEvent0(_)).Times(1);
    EXPECT_CALL(*_s0, guard(_)).Times(0);

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s1>()));
}

TEST_F(Common_StateMachine, test_external_transition_guard_true)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s0, &s0::guard, s1>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onEvent0(_)).Times(0);
    EXPECT_CALL(*_s0, guard(_)).Times(1);
    ON_CALL(*_s0, guard(_)).WillByDefault(Return(true));

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s1>()));
}

TEST_F(Common_StateMachine, test_external_transition_guard_false)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s0, &s0::guard, s1>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onEvent0(_)).Times(0);
    EXPECT_CALL(*_s0, guard(_)).Times(1);
    ON_CALL(*_s0, guard(_)).WillByDefault(Return(false));

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s0>()));
}

TEST_F(Common_StateMachine, test_external_transition_action_guard_true)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s0, &s0::onEvent0, &s0::guard, s1>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onEvent0(_)).Times(1);
    EXPECT_CALL(*_s0, guard(_)).Times(1);
    ON_CALL(*_s0, guard(_)).WillByDefault(Return(true));

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s1>()));
}

TEST_F(Common_StateMachine, test_external_transition_action_guard_false)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s0, &s0::onEvent0, &s0::guard, s1>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onEvent0(_)).Times(0);
    EXPECT_CALL(*_s0, guard(_)).Times(1);
    ON_CALL(*_s0, guard(_)).WillByDefault(Return(false));

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s0>()));
}

TEST_F(Common_StateMachine, test_internal_transition_action)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s0, &s0::onEvent0>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onEvent0(_)).Times(1);
    EXPECT_CALL(*_s0, guard(_)).Times(0);

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s0>()));
}

TEST_F(Common_StateMachine, test_internal_transition_action_guard_true)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s0, &s0::onEvent0, &s0::guard>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onEvent0(_)).Times(1);
    EXPECT_CALL(*_s0, guard(_)).Times(1);
    ON_CALL(*_s0, guard(_)).WillByDefault(Return(true));

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s0>()));
}

TEST_F(Common_StateMachine, test_internal_transition_action_guard_false)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s0, &s0::onEvent0, &s0::guard>();

    s0* _s0 = _sm.getState<s0>();
    EXPECT_CALL(*_s0, onEvent0(_)).Times(0);
    EXPECT_CALL(*_s0, guard(_)).Times(1);
    ON_CALL(*_s0, guard(_)).WillByDefault(Return(false));

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s0>()));
}

TEST_F(Common_StateMachine, test_action_with_transit)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s0, &s0::onEvent0>();

    s0* _s0 = _sm.getState<s0>();
    ON_CALL(*_s0, onEvent0(_)).WillByDefault(Invoke([&] () { _s0->transit<s1>(); }));

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s1>()));
}

TEST_F(Common_StateMachine, test_transit_outside_process)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s0, &s0::onEvent0>();

    s0* _s0 = _sm.getState<s0>();
    ON_CALL(*_s0, onEvent0(_)).WillByDefault(Invoke([&] () { _s0->transit<s1>(); }));

    _sm.start();
    _s0->transit<s1>();

    ASSERT_TRUE((_sm.checkStates<s1>()));
}

TEST_F(Common_StateMachine, test_post_event_on_entry)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s0, &s0::onEvent0>();
    _sm.addTransition<s1, e1, s1, &s1::onEvent1>();

    s0* _s0 = _sm.getState<s0>();
    s1* _s1 = _sm.getState<s1>();

    ON_CALL(*_s0, onEvent0(_)).WillByDefault(Invoke([&] () { _s0->transit<s1>(); }));
    ON_CALL(*_s1, onEntry()).WillByDefault(Invoke([&] () { _s1->postEvent(e1{}); }));
    ON_CALL(*_s1, onEvent1(_)).WillByDefault(Invoke([&] () { std::cout << "toto" << std::endl; }));

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s1>()));
}

TEST_F(Common_StateMachine, test_error_on_entry)
{
    _sm.addState<NiceMock<s0>, Entry>();

    s0* _s0 = _sm.getState<s0>();
    ON_CALL(*_s0, onEntry()).WillByDefault(Invoke([&]() { throw "exception on entry"; }));
    EXPECT_CALL(*_s0, onError(_)).Times(1);

    _sm.start();

    ASSERT_TRUE((_sm.checkStates<s0>()));
}

TEST_F(Common_StateMachine, test_error_on_exit)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s1>();

    s0* _s0 = _sm.getState<s0>();
    ON_CALL(*_s0, onExit()).WillByDefault(Invoke([&]() { throw "exception on exit"; }));
    EXPECT_CALL(*_s0, onError(_)).Times(1);

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s1>()));
}

TEST_F(Common_StateMachine, test_error_on_action)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s0, &s0::onEvent0, s1>();

    s0* _s0 = _sm.getState<s0>();
    ON_CALL(*_s0, onEvent0(_)).WillByDefault(Invoke([&]() { throw "exception on action"; }));
    EXPECT_CALL(*_s0, onError(_)).Times(1);

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s0>()));
}

TEST_F(Common_StateMachine, test_store)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addTransition<s0, e0, s0, &s0::onEvent0>();

    s0* _s0 = _sm.getState<s0>();
    ON_CALL(*_s0, onEvent0(_)).WillByDefault(Invoke([&]() {
        ASSERT_EQ("initial", _s0->store()->data);
        _s0->store()->data = "changed";
    }));

    _sm.store()->data = "initial";
    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_EQ("changed", _sm.store()->data);
}

TEST_F(Common_StateMachine, test_composite_states)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<s0, NiceMock<s1>, Entry>();

    _sm.start();

    ASSERT_TRUE((_sm.checkStates<s0, s1>()));
}

TEST_F(Common_StateMachine, test_ortho_states)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<s0, NiceMock<s1>, Entry>();
    _sm.addState<s0, NiceMock<s2>>();
    _sm.addState<s0, NiceMock<s3>, 1, Entry>();
    _sm.addState<s0, NiceMock<s4>, 1>();

    _sm.start();

    ASSERT_TRUE((_sm.checkStates<s0, s1>()));
    ASSERT_TRUE((_sm.checkStates<s0, s3>()));
}

TEST_F(Common_StateMachine, test_triggering_event)
{
    e0 _e0;
    e1 _e1;
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addState<NiceMock<s2>>();
    _sm.addTransition<s0, e0, s1>();
    _sm.addTransition<s1, e1, s2>();

    s0* _s0 = _sm.getState<s0>();
    s1* _s1 = _sm.getState<s1>();
    s2* _s2 = _sm.getState<s2>();

    ON_CALL(*_s0, onEntry()).WillByDefault(Invoke([&]() {
        ASSERT_TRUE(nullptr == _s0->trigEvent<e0>());
        ASSERT_TRUE(nullptr == _s0->trigEvent<e1>());
    }));

    ON_CALL(*_s1, onEntry()).WillByDefault(Invoke([&]() {
        ASSERT_TRUE(&_e0 == _s1->trigEvent<e0>());
        ASSERT_TRUE(nullptr == _s1->trigEvent<e1>());
    }));

    ON_CALL(*_s2, onEntry()).WillByDefault(Invoke([&]() {
        ASSERT_TRUE(nullptr == _s2->trigEvent<e0>());
        ASSERT_TRUE(&_e1 == _s2->trigEvent<e1>());
    }));

    ON_CALL(*_s0, onExit()).WillByDefault(Invoke([&]() {
        ASSERT_TRUE(&_e0 == _s0->trigEvent<e0>());
        ASSERT_TRUE(nullptr == _s0->trigEvent<e1>());
    }));

    ON_CALL(*_s1, onExit()).WillByDefault(Invoke([&]() {
        ASSERT_TRUE(nullptr == _s1->trigEvent<e0>());
        ASSERT_TRUE(&_e1 == _s1->trigEvent<e1>());
    }));

    ON_CALL(*_s2, onExit()).WillByDefault(Invoke([&]() {
        ASSERT_TRUE(nullptr == _s2->trigEvent<e0>());
        ASSERT_TRUE(nullptr == _s2->trigEvent<e1>());
    }));

    _sm.start();
    _sm.processEvent(_e0);
    _sm.processEvent(_e1);
    _sm.stop();
}

TEST_F(Common_StateMachine, test_shallow_history)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addState<s1, NiceMock<s2>, Entry>();
    _sm.addState<s1, NiceMock<s3>>();

    _sm.addTransition<s0, e0, s1>();
    _sm.addTransition<s1, e1, s0>();
    _sm.addTransition<s2, e2, s3>();
    _sm.addTransition<s3, e3, s2>();

    _sm.setHistory<sm>(History::Shallow);

    _sm.start();
    ASSERT_TRUE((_sm.checkStates<s0>()));
    _sm.processEvent(e0{});
    ASSERT_TRUE((_sm.checkStates<s1, s2>()));
    _sm.processEvent(e2{});
    ASSERT_TRUE((_sm.checkStates<s1, s3>()));
    _sm.processEvent(e1{});
    ASSERT_TRUE((_sm.checkStates<s0>()));
    _sm.processEvent(e0{});
    ASSERT_TRUE((_sm.checkStates<s1, s2>()));
}

TEST_F(Common_StateMachine, test_deep_history)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();
    _sm.addState<s1, NiceMock<s2>, Entry>();
    _sm.addState<s1, NiceMock<s3>>();

    _sm.addTransition<s0, e0, s1>();
    _sm.addTransition<s1, e1, s0>();
    _sm.addTransition<s2, e2, s3>();
    _sm.addTransition<s3, e3, s2>();

    _sm.setHistory<sm>(History::Deep);

    _sm.start();
    ASSERT_TRUE((_sm.checkStates<s0>()));
    _sm.processEvent(e0{});
    ASSERT_TRUE((_sm.checkStates<s1, s2>()));
    _sm.processEvent(e2{});
    ASSERT_TRUE((_sm.checkStates<s1, s3>()));
    _sm.processEvent(e1{});
    ASSERT_TRUE((_sm.checkStates<s0>()));
    _sm.processEvent(e0{});
    ASSERT_TRUE((_sm.checkStates<s1, s3>()));
}

TEST_F(Common_StateMachine, test_post_event)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();

    _sm.addTransition<s0, e0, s0, &s0::onEvent0>();
    _sm.addTransition<s0, e1, s1>();

    s0* _s0 = _sm.getState<s0>();
    ON_CALL(*_s0, onEvent0(_)).WillByDefault(Invoke([&]() { return _s0->postEvent(e1{}); }));

    _sm.start();
    _sm.processEvent(e0{});

    ASSERT_TRUE((_sm.checkStates<s1>()));
}

TEST_F(Common_StateMachine, test_defer_event)
{
    _sm.addState<NiceMock<s0>, Entry>();
    _sm.addState<NiceMock<s1>>();

    _sm.addTransition<s0, e0, s1>();
    _sm.addTransition<s1, e1, s1, &s1::onEvent1>();

    bool onEvent1Called = false;
    s1* _s1 = _sm.getState<s1>();
    ON_CALL(*_s1, onEvent1(_)).WillByDefault(Invoke([&]() { onEvent1Called = true; }));

    _sm.start();
    _sm.deferEvent(e1{});
    ASSERT_FALSE(onEvent1Called);
    _sm.processEvent(e0{});
    ASSERT_TRUE(onEvent1Called);

    ASSERT_TRUE((_sm.checkStates<s1>()));
}

TEST_F(Common_StateMachine, test_sm_visitor)
{
    _sm.addState<NiceMock<s0>, Entry>("s0");
    _sm.addState<s0, NiceMock<s1>, Entry>("s1");
    _sm.addState<s1, NiceMock<s2>, Entry>("s2");

    _sm.start();

    Visitor v{ "s2" };
    _sm.visit(v);

    ASSERT_TRUE(v.found);
    ASSERT_TRUE((v.states == std::vector<std::string>{ "sm", "s0", "s1", "s2" }));
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

