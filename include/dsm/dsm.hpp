#ifndef STATE_MACHINE_HPP_INCLUDED
#define STATE_MACHINE_HPP_INCLUDED

#include <string>
#include <ostream>
#include <functional>
#include <typeindex>
#include <map>
#include <queue>
#include <type_traits>

namespace dsm
{
    // Forwarded declarations
    class StateBase;
    class TransitionBase;
    template <typename DerivedType, typename SmType>
    class State;
    template <typename SmType, typename StoreType>
    class StateMachine;

    // Concepts
    template <typename ...Ts>
    using is_empty_pack = std::enable_if_t<0 == sizeof ...(Ts)>;

    // Types aliases
    using TIndex = std::type_index;
    using TStates = std::vector<StateBase*>;
    using TTransitions = std::vector<TransitionBase*>;
    template <typename StateType, typename EventType>
    using TGuard = bool(StateType::*)();
    template <typename StateType, typename EventType>
    using TAction = void(StateType::*)(const EventType&);

    /**
     * @brief   Index
     * @details Transforms Type into type_index for associative containers
     * @return  Returns the Types's type_index
     */
    template <typename Type>
    static const TIndex Index() { return TIndex(typeid(Type)); }

    /**
     * @brief   CheckIndex
     * @details Checks whether Type and provided type_index are matching
     * @return  True if there's a match, false otherwise
     */
    template <typename Type>
    static constexpr bool CheckIndex(const TIndex& index) { return index == Index<Type>(); }

    /**
     * @brief   IStateVisitor
     * @details Interface for StateMachine visitors
     */
    struct IStateVisitor
    {
        virtual void visit(const StateBase* state) = 0;
    };

    /**
     * @brief   History
     * @details History type: either Shallow or Deep history
     */
    enum class History : std::uint8_t
    {
        None = 0,
        Shallow,
        Deep
    };

    /**
     * @brief   EventBase
     * @details Base class for all Events. Holds the type index of the final subclass
     */
    class EventBase
    {
    private:
        template <typename DerivedType, typename SmType>
        friend class State;

        /**
         * @brief   m_index
         * @details Event's type index
         */
        TIndex m_index;

    protected:
        EventBase(const TIndex& index)
            : m_index{ index }
        {}

        virtual ~EventBase() {}
    };

    /**
     * @brief   Event
     * @details CRTP base class for Events. Allows type index initialization
     */
    template <typename DerivedType>
    class Event : public EventBase
    {
    protected:
        Event() :
            EventBase{ Index<DerivedType>() }
        {
            static_assert(std::is_base_of_v<Event<DerivedType>, DerivedType>);
        }

        virtual ~Event() {}
    };

    /**
     * @brief   TransitionBase
     * @details Base class for all Callbacks. Holds the type index of the final subclass
     */
    class TransitionBase
    {
    private:
        friend class StateBase;

        template <typename SmType, typename StoreType>
        friend class StateMachine;

        /**
         * @brief   m_index
         * @details Transition's type index
         */
        TIndex m_index;

    protected:
        TransitionBase(const TIndex& index)
            : m_index{ index }
        {}

        virtual ~TransitionBase() {}

    private:
        /**
         * @brief   execPrepared
         * @details Executes a prepared callback
         */
        virtual void execPrepared() const = 0;
    };

    /**
     * @brief   StateBase
     * @details Base class for all States.
     *          - Holds the type index of the final subclass
     *          - Holds the state name
     *          - Holds the region index it belongs to
     *          - Holds the inner regions
     *          - Holds the transitions
     *          - Holds the defered events
     *          - Provides a link to the parent state and to the top state (i.e. the StateMachine)
     */
    class StateBase
    {
    private:
        template <typename DerivedType, typename SmType>
        friend class State;

        template <typename SmType, typename StoreType>
        friend class StateMachine;

        /**
         * @brief   Transition
         * @details CRTP base class. Allows type index initialization
         */
        template <typename EventType>
        struct Transition : public TransitionBase
        {
            using TCallbackFunc = std::function<void(const EventType&)>;

            /**
             * @brief   m_cb
             * @details The stored function object
             */
            TCallbackFunc m_cb;

            Transition(const TCallbackFunc& cb)
                : TransitionBase{ Index<EventType>() }
                , m_cb{ cb }
            {}

            virtual ~Transition() {}

            /**
             * @brief   execPrepared
             * @details Executes a prepared callback. Default implementation does nothing
             */
            virtual void execPrepared() const {}
        };

        /**
         * @brief   PreparedTransition
         * @details Prepared Transition that holds its Event parameter together with the function object to call
         */
        template <typename EventType>
        struct PreparedTransition : public Transition<EventType>
        {
            /**
             * @brief   m_evt
             * @details The stored event
             */
            EventType m_evt;

            PreparedTransition(const typename Transition<EventType>::TCallbackFunc& cb, const EventType& evt)
                : Transition<EventType>{ cb }
                , m_evt{ evt }
            {}

            virtual ~PreparedTransition() {}

            /**
             * @brief   execPrepared
             * @details Executes a prepared callback. Calls the stored function object with the stored event as parameter
             */
            void execPrepared() const override
            {
                Transition<EventType>::m_cb(m_evt);
            }
        };

        /**
         * @brief   Region
         * @details Region inside a state. Holds pointers to the entry point state, the current state and the last visited state
         */
        struct Region
        {
            /**
             * @brief   m_entryState
             * @details The entry sub-state
             */
            StateBase* m_entryState = nullptr;

            /**
             * @brief   m_currentState
             * @details The current sub-state
             */
            StateBase* m_currentState = nullptr;

            /**
             * @brief   m_lastState
             * @details The last visited sub-state
             */
            StateBase* m_lastState = nullptr;

            /**
             * @brief   m_children
             * @details The sub-states mapped with their type index
             */
            std::map<TIndex, StateBase*> m_children;
        };
        using TRegions = std::vector<Region>;

        /**
         * @brief   m_name
         * @details The state's name
         */
        std::string m_name;

        /**
         * @brief   m_regionIndex
         * @details The state's region index it belongs to
         */
        int m_regionIndex = 0;

        /**
         * @brief   m_index
         * @details The state's type index
         */
        TIndex m_index;

        /**
         * @brief   m_regions
         * @details The state's regions container
         */
        TRegions m_regions;

        bool m_entry = false;

        /**
         * @brief   m_started
         * @details Flag indicating whether the state machine is started or not.
         *          - A stopped state machine won't process any event
         *          - A started state machine won't accept any new state nor transition to be added
         */
        bool m_started = false;

        /**
         * @brief   m_parentState
         * @details The state's parent state
         */
        StateBase* m_parentState = nullptr;

        /**
         * @brief   m_topSm
         * @details The top-sm state (i.e. the state machine)
         */
        StateBase* m_topSm = nullptr;

        /**
         * @brief   m_trigEvent
         * @details The current triggering event
         */
        const EventBase* m_trigEvent = nullptr;

        /**
         * @brief   m_history
         * @details The state's history activation flag
         */
        bool m_history = false;

        /**
         * @brief   m_transitions
         * @details The state's allowed transitions mapped with their event's type index
         */
        std::map<TIndex, TransitionBase*> m_transitions;

        /**
         * @brief   m_defered
         * @details The state's defered events stored as prepared callbacks
         */
        mutable std::queue<TransitionBase*> m_defered;

    public:
        /**
         * @brief   name
         * @return  Returns the state's name
         */
        const std::string& name() const { return m_name; }

        /**
         * @brief   started
         * @details Provides the statemachine start/stop flag
         * @return  true if started, false otherwise
         */
        bool started() const
        {
            return m_started;
        }

        /**
         * @brief           operator<<
         * @param[in,out]   stream: output stream
         * @param[in]       state: state to output
         * @details         Performs stream output on states
         * @return          Returns the output stream
         */
        friend std::ostream& operator<<(std::ostream& stream, const StateBase& state)
        {
            stream << state.m_name;
            if (state.m_regions.size() > 1) stream << "[";
            for (TRegions::const_iterator it = state.m_regions.cbegin(); it != state.m_regions.cend(); ++it)
            {
                if (it->m_currentState != nullptr) stream << "->" << *(it->m_currentState);
                if (std::next(it) != state.m_regions.end()) stream << "|";
            }
            if (state.m_regions.size() > 1) stream << "]";

            return stream;
        }

    private:
        /**
         * @brief           visitImpl
         * @param[in,out]   visitor: applied visitor
         * @details         Visits the current state and sub-states
         */
        void visitImpl(IStateVisitor& visitor)
        {
            visitor.visit(this);
            for (const auto& region : m_regions)
            {
                if (region.m_currentState != nullptr) region.m_currentState->visitImpl(visitor);
            }
        }

        /**
         * @brief       StateBase
         * @param[in]   index: sub-class type index
         * @details     StateBase ctor
         */
        StateBase(const TIndex& index)
            : m_index{ index }
        {}

        /**
         * @brief   ~StateBase
         * @details StateBase dtor
         */
        virtual ~StateBase()
        {
            for (const auto&[_, transition] : m_transitions)
            {
                delete transition;
                transition = nullptr;
            }

            for (const auto& region : m_regions)
            {
                for (const auto&[_, child] : region.m_children)
                {
                    delete child;
                    child = nullptr;
                }
            }

            while (!m_defered.empty())
            {
                delete m_defered.front();
                m_defered.pop();
            }
        }

        /**
         * @brief   getStates
         * @details Allows statemachine states setup when overriden by user
         * @return  User defined states
         */
        virtual TStates getStates() { return {}; }

        /**
         * @brief   getTransitions
         * @details Allows statemachine transitions setup when overriden by user
         * @return  User defined transitions
         */
        virtual TTransitions getTransitions() { return {}; }

        /**
         * @brief   getHistory
         * @details Allows statemachine history setup when overriden by user
         * @return  User defined history
         */
        virtual History getHistory() { return History::None; }

        /**
         * @brief   onEntry
         * @details State's entry job
         */
        virtual void onEntry() {}

        /**
         * @brief   onExit
         * @details State's exit job
         */
        virtual void onExit() {}

        /**
         * @brief       onError
         * @param[in]   ex: exception causing this call
         * @details     State's error job
         */
        virtual void onError(std::exception_ptr ex) {}

        /**
         * @brief       startImpl
         * @param[in]   evt: event triggering the state's entry
         * @details     Starts the current state
         *              - Stores the triggering event
         *              - Processes any defered event
         *              - Performs the entry job
         *              - For each region:
         *                  - Restores the current sub-state depending on the history flag
         *                  - Recursively starts the current sub-state if any
         */
        template <typename EventType>
        inline void startImpl(const EventType& evt)
        {
            m_started = true;

            // Backup triggering event
            m_trigEvent = evt;

            // Handle deferred events if any
            while (!m_defered.empty())
            {
                m_defered.front()->execPrepared();
                delete m_defered.front();
                m_defered.pop();
            }

            try
            {
                // State's entry job
                onEntry();
            }
            catch (...)
            {
                // State's error job
                onError(std::current_exception());
            }

            // Loop over orthogonal regions
            for (auto& region : m_regions)
            {
                // Restore last visited or entry depending on history flag
                if (m_history && region.m_lastState != nullptr) region.m_currentState = region.m_lastState;
                else region.m_currentState = region.m_entryState;

                // Start current sub-state if any
                if (region.m_currentState != nullptr) region.m_currentState->startImpl(evt);
            }
        }

        /**
         * @brief       stopImpl
         * @param[in]   evt: event triggering the state's exit
         * @details     Stops the current state
         *              - Stores the triggering event
         *              - For each region:
         *                  - Recursively stops the current sub-state if any
         *                  - Stores the last visited sub-state depending on the history flag
         *                  - Resets the current sub-state
         *              - Performs the exit job
         */
        template <typename EventType>
        inline void stopImpl(const EventType& evt)
        {
            // Backup triggering event
            m_trigEvent = evt;

            // Loop over orthogonal regions
            for (auto& region : m_regions)
            {
                // Stop current sub-state if any
                if (region.m_currentState != nullptr) region.m_currentState->stopImpl(evt);

                // Backup last visited sub-state depending on history flag
                if (m_history) region.m_lastState = region.m_currentState;

                // Reset current sub-state
                region.m_currentState = nullptr;
            }

            try
            {
                // State's exit job
                onExit();
            }
            catch (...)
            {
                // State's error job
                onError(std::current_exception());
            }

            m_started = false;
        }

        /**
         * @brief       addState
         * @param[in]   parent: parent state
         * @param[in]   child: child state
         * @details     Adds the provided child state to the parent state
         */
        void addState(StateBase* parent, StateBase* child)
        {
            if (nullptr == parent) throw std::logic_error("Invalid parent state");
            if (nullptr == child) throw std::logic_error("Invalid child state");
            if (static_cast<int>(child->m_regionIndex - parent->m_regions.size()) > 1) throw std::logic_error("Invalid region index");

            if (parent->m_regions.size() <= child->m_regionIndex) parent->m_regions.push_back({});
            // Add the newly created state to the corresponding region
            auto res = parent->m_regions[child->m_regionIndex].m_children.insert({ child->m_index, child });

            if (res.second)
            {
                // Define the entry state if needed
                if (child->m_entry) parent->m_regions[child->m_regionIndex].m_entryState = child;
            }
        }

        /**
         * @brief       addTransition
         * @param[in]   state: state that receives the transition
         * @param[in]   transition: transition to add
         * @details     Adds the provided transition to the state
         */
        void addTransition(StateBase* state, TransitionBase* transition)
        {
            if (nullptr == state) throw std::logic_error("Invalid state's transition");
            if (nullptr == transition) throw std::logic_error("Invalid transition");

            // Add the newly created Transition to the transitions container
            state->m_transitions.insert({ transition->m_index, transition });
        }

        void setupStatesImpl()
        {
            TStates states = getStates();

            for (const auto& state : states)
            {
                addState(this, state);
            }

            for (const auto& region : m_regions)
            {
                for (const auto&[_, child] : region.m_children)
                {
                    child->setupStatesImpl();
                }
            }
        }

        void setupTransitionsImpl()
        {
            TTransitions transitions = getTransitions();

            for (const auto& transition : transitions)
            {
                addTransition(this, transition);
            }

            for (const auto& region : m_regions)
            {
                for (const auto&[_, child] : region.m_children)
                {
                    child->setupTransitionsImpl();
                }
            }
        }

        void setupHistoryImpl()
        {
            setHistory(getHistory());

            for (const auto& region : m_regions)
            {
                for (const auto&[_, child] : region.m_children)
                {
                    child->setupHistoryImpl();
                }
            }
        }

        void tearDownImpl()
        {
            while (!m_defered.empty())
            {
                delete m_defered.front();
                m_defered.pop();
            }

            for (const auto&[_, transition] : m_transitions)
            {
                delete transition;
                transition = nullptr;
            }
            m_transitions.clear();

            for (const auto& region : m_regions)
            {
                for (const auto&[_, child] : region.m_children)
                {
                    child->tearDownImpl();
                    delete child;
                    child = nullptr;
                }
            }
            m_regions.clear();
        }

        /**
         * @brief   getStateImpl
         * @details Recursively searches for the state or sub-state with the specified type StateType
         * @return  Pointer to the found state if any, nullptr otherwise
         */
        template <typename StateType>
        StateType* getStateImpl()
        {
            static_assert(std::is_base_of_v<StateBase, StateType>);

            // Check self state index
            if (true == CheckIndex<StateType>(m_index)) return static_cast<StateType*>(this);

            // Loop over orthogonal regions
            for (const auto& region : m_regions)
            {
                // Loop over sub-states
                for (const auto&[_, child] : region.m_children)
                {
                    // Recursive call
                    StateType* state = child->template getStateImpl<StateType>();
                    // Recursive stop condition
                    if (state != nullptr) return state;
                }
            }

            return nullptr;
        }

        /**
         * @brief   getSubState
         * @details Searches for the direct sub-state with type StateType
         * @return  Pointer to the found state if any, nullptr otherwise
         */
        template <typename StateType>
        StateBase* getSubState() const
        {
            // Loop over orthogonal regions
            for (const auto& region : m_regions)
            {
                // Search for a sub-state with type StateType
                auto itChild = region.m_children.find(Index<StateType>());
                if (itChild != region.m_children.end()) return itChild->second;
            }

            return nullptr;
        }

        /**
         * @brief   hasAncestorTransition
         * @details Recursively searches upwards (from self to topmost parent) for a transition with the provided type index
         * @return  true if transition found, false otherwise
         */
        bool hasAncestorTransition(const TIndex& index) const
        {
            // Check if self contains the requested transition index
            if (m_transitions.end() != m_transitions.find(index)) return true;

            // Recursive stop condition
            if (nullptr == m_parentState) return false;

            // Recursive call
            return m_parentState->hasAncestorTransition(index);
        }

        /**
         * @brief   hasDescendantTransition
         * @details Recursively searches downwards (from self to innermost child) for a transition with the provided type index
         * @return  true if transition found, false otherwise
         */
        bool hasDescendantTransition(const TIndex& index) const
        {
            // Check if self contains the requested transition index
            if (m_transitions.end() != m_transitions.find(index)) return true;

            // Loop over orthogonal regions
            for (const auto& region : m_regions)
            {
                // Loop over sub-states
                for (const auto& [_, child] : region.m_children)
                {
                    // Recursive call and stop condition
                    if (child->hasDescendantTransition(index)) return true;
                }
            }

            return false;
        }

        /**
         * @brief       transitImpl
         * param[in]    evt: transition's triggering event
         * param[in]    dstState: transition's destination state
         * @details     Performs the transition from current to destination state:
         *              - Exits from current state
         *              - Updates current state
         *              - Enters into destination state
         * @return      true if transition succeeded, false otherwise
         */
        template <typename EventType>
        inline bool transitImpl(const EventType& evt, StateBase* dstState)
        {
            // dstState validity checked by caller (pointer validity and dst is a sibling of current)

            // Reference to destination's region index
            const auto& dstRegion = dstState->m_regionIndex;

            // check we're not already in the destination state
            if (dstState == m_regions[dstRegion].m_currentState) return false;

            // Exit from current
            m_regions[dstRegion].m_currentState->stopImpl(evt);
            // Update the current state with the destination
            m_regions[dstRegion].m_currentState = dstState;
            // Enter into new current state
            m_regions[dstRegion].m_currentState->startImpl(evt);

            return true;
        }

        /**
         * @brief       setHistory
         * param[in]    type: Shallow or Deep history type
         * param[in]    history: history activation flag
         * @details     Defines the history flag recursively or not depending on the history type
         */
        void setHistory(History type)
        {
            if (History::None == type) return;

            // Store history flag
            m_history = true;

            for (auto& region : m_regions)
            {
                // Reset the last visited state when changing history settings
                region.m_lastState = nullptr;

                // Recursive call if Deep history requested
                if (History::Deep == type)
                {
                    for (const auto&[_, child] : region.m_children) child->setHistory(type);
                }
            }
        }

        /**
         * @brief       postPreparedCallback
         * param[in]    cb: Prepared Transition to post
         * @details     Posts a prepared Transition in the top-most state (i.e. StateMachine)
         */
        virtual void postPreparedCallback(TransitionBase* cb) const = 0;

        /**
         * @brief       processEventImpl
         * param[in]    evt: Event to process
         * @details     Processes the provided event:
         *              - Searches for a transition with index corresponding with the provided Event's type index
         *              - If such transition found, execute its callback using the provided event as parameter
         *              - If no transition found, recursively search inside sub-states
         */
        template <typename EventType>
        inline void processEventImpl(const EventType& evt) const
        {
            static_assert(std::is_base_of_v<Event<EventType>, EventType>);

            // Loop over transitions
            auto it = m_transitions.find(Index<EventType>());
            if (it != m_transitions.end())
            {
                // Invoke the corresponding callback
                static_cast<Transition<EventType>*>(it->second)->m_cb(evt);
                // Recursive stop condition
                return;
            }

            // Recursive calls over sub-states if no transition found yet
            for (const auto& region : m_regions)
            {
                if (region.m_currentState != nullptr) region.m_currentState->processEventImpl(evt);
            }
        }

        /**
         * @brief       postEventImpl
         * param[in]    evt: Event to post
         * @details     Posts the provided event:
         *              - Searches for a transition with index corresponding with the provided Event's type index
         *              - If such transition found, post a prepared Transition to the top-sm
         *              - If no transition found, recursively search inside sub-states
         */
        template <typename EventType>
        void postEventImpl(const EventType& evt) const
        {
            static_assert(std::is_base_of_v<Event<EventType>, EventType>);

            // Loop over transitions
            auto it = m_transitions.find(Index<EventType>());
            if (it != m_transitions.end())
            {
                // Post a prepared Transition to the top-sm
                m_topSm->postPreparedCallback(new PreparedTransition<EventType>{ static_cast<Transition<EventType>*>(it->second)->m_cb, evt });
                // Recursive stop condition
                return;
            }

            // Recursive calls over sub-states if no transition found yet
            for (const auto& region : m_regions)
            {
                for (const auto&[_, child] : region.m_children) child->postEventImpl(evt);
            }
        }

        /**
         * @brief       deferEventImpl
         * param[in]    evt: Event to defer
         * @details     Defers the provided event:
         *              - Searches for a transition with index corresponding with the provided Event's type index
         *              - If such transition found, add a prepared Transition to the state's defer queue
         *              - If no transition found, recursively search inside sub-states
         */
        template <typename EventType>
        void deferEventImpl(const EventType& evt) const
        {
            static_assert(std::is_base_of_v<Event<EventType>, EventType>);

            // Loop over transitions
            auto it = m_transitions.find(Index<EventType>());
            if (it != m_transitions.end())
            {
                // Add a prepared Transition to the defered queue
                m_defered.push(new PreparedTransition<EventType>{ static_cast<Transition<EventType>*>(it->second)->m_cb, evt });
                // Recursive stop condition
                return;
            }

            // Recursive calls over sub-states if no transition found yet
            for (const auto& region : m_regions)
            {
                for (const auto&[idx, child] : region.m_children) child->deferEventImpl(evt);
            }
        }

        template <typename ...StateTypes, typename = is_empty_pack<StateTypes...>>
        bool checkStatesImpl(StateBase* previousState = nullptr)
        {
            return m_started;
        }

        template <typename FirstStateType, typename ...OtherStateTypes>
        bool checkStatesImpl(StateBase* previousState = nullptr)
        {
            auto state = this->template getStateImpl<FirstStateType>();
            if (nullptr == state) return false;
            if (state == previousState) return false;
            if (!state->m_started) return false;
            if (previousState != nullptr && state != previousState->m_regions[state->m_regionIndex].m_currentState) return false;
            if constexpr (0 == sizeof ...(OtherStateTypes)) return true;
            else return state->template checkStatesImpl<OtherStateTypes...>(state);
        }
    };

    /**
     * @brief   State
     * @details CRTP base class for States. Allows type index initialization
     */
    template <typename DerivedType, typename SmType>
    class State : public StateBase
    {
    private:
        template <typename OtherSmType, typename StoreType>
        friend class StateMachine;

    protected:
        State()
            : StateBase{ Index<DerivedType>() }
        {
            static_assert(std::is_default_constructible_v<DerivedType>);
            static_assert(std::is_base_of_v<State<DerivedType, SmType>, DerivedType>);
            static_assert(std::is_default_constructible_v<DerivedType>);
            if constexpr (std::is_same_v<DerivedType, SmType>) m_topSm = this;
        }

        virtual ~State() = default;

    public:
        /**
         * @brief   transit
         * @details Performs the transition to the provided StateType
         * @return  true if transition succeeded, false otherwise
         */
        template <typename DstState>
        bool transit()
        {
            static_assert(std::is_base_of_v<StateBase, DstState>);

            // No possible transition from the top-sm
            if (m_parentState == nullptr) return false;

            // check that the destination state is a sibling of this state
            auto dstState = m_parentState->getSubState<DstState>();
            if (nullptr == dstState) return false;

            // Forward to the transition implementation
            return m_parentState->transitImpl(nullptr, dstState);
        }

        /**
         * @brief   trigEvent
         * @details Provides the triggering event if any
         * @return  Pointer to triggering event, or nullptr if none (start case)
         */
        template <typename EventType>
        const EventType* trigEvent() const
        {
            static_assert(std::is_base_of_v<Event<EventType>, EventType>);

            // Check triggering event pointer validity and type index match the provided EventType
            if (m_trigEvent != nullptr && true == CheckIndex<EventType>(m_trigEvent->m_index))
            {
                // Return the upcasted event
                return static_cast<const EventType*>(m_trigEvent);
            }

            return nullptr;
        }

    private:
        /**
         * @brief   IsTopSm
         * @details Check whether the current state corresponds to top-sm or not
         * @return  true if current state is the top-sm, false otherwise
         */
        static constexpr bool IsTopSm()
        {
            if constexpr (std::is_same_v<DerivedType, SmType>) return true;
            else return false;
        }

        /**
         * @brief   topSm
         * @details Provides a pointer to the top-sm
         * @return  Upcasted top-sm
         */
        SmType* topSm() const
        {
            return static_cast<SmType*>(m_topSm);
        }

        /**
         * @brief       postPreparedCallback
         * param[in]    cb: Prepared Transition to post
         * @details     Non top-sm implementation does nothing
         */
        virtual void postPreparedCallback(TransitionBase*) const override {}

        /**
         * @brief       createStateImpl
         * param[in]    name: Child state's name
         * param[in]    entry: Flag indicating if the added child state shall be the entry point or not
         * @details     Creates a child state of type ChildState in the provided RegionIndex
         * @return      true if child state added, false otherwise
         */
        template <typename ParentState, typename ChildState, int RegionIndex>
        StateBase* createStateImpl(const std::string& name, bool entry)
        {
            static_assert(std::is_base_of_v<StateBase, ParentState>);
            static_assert(std::is_base_of_v<StateBase, ChildState>);
            static_assert(RegionIndex >= 0);

            // Retrieve and check the parent state
            auto parentState = topSm()->template getStateImpl<ParentState>();
            if (nullptr == parentState) return nullptr;

            // Don't allow same states at different places in a statemachine
            if (topSm()->template getStateImpl<ChildState>() != nullptr) return nullptr;

            // Create the new child state and initialize it
            auto child = new ChildState();
            child->m_name = name;
            child->m_entry = entry;
            child->m_parentState = parentState;
            child->m_regionIndex = RegionIndex;
            child->m_topSm = m_topSm;

            return child;
        }

        /**
         * @brief       createTransitionImpl
         * param[in]    guard: function object that may be used to discard the execution of the transition
         * param[in]    action: function object that is called prior to the transition
         * @details     Creates a function object that wraps the guard, the action and the transition
         * @return      Pointer to the created transition
         */
        template <typename SrcState, typename EventType, typename StateType, TAction<StateType, EventType> action, TGuard<StateType, EventType> guard, typename DstState>
        TransitionBase* createTransitionImpl()
        {
            static_assert(std::is_base_of_v<Event<EventType>, EventType>);
            static_assert(std::is_base_of_v<StateBase, DstState>);

            // Retrieve and check the source state
            auto srcState = topSm()->template getStateImpl<SrcState>();
            if (nullptr == srcState) return nullptr;

            // Retrieve and check the destination state
            auto dstState = topSm()->template getStateImpl<DstState>();
            if (nullptr == dstState) return nullptr;

            // Retrieve and check the state where the action and guard shall take place
            auto actionState = topSm()->template getStateImpl<StateType>();
            if (nullptr == actionState) return nullptr;

            // Source and destination states must have same parent and must belong to same region (no inter-region transitions)
            // Complex transitions are not yet implemented
            if (srcState->m_parentState != dstState->m_parentState || srcState->m_regionIndex != dstState->m_regionIndex) return nullptr;

            // Check if the requested transition already exists in ancestors
            if (srcState->hasAncestorTransition(Index<EventType>())) return nullptr;
            // Check if the requested transition already exists in descendants
            if (srcState->hasDescendantTransition(Index<EventType>())) return nullptr;

            auto cb = [srcState, dstState, actionState, actionMember = action, guardMember = guard](const EventType& evt)
            {
                try
                {
                    if (guardMember != nullptr && false == std::invoke(guardMember, actionState)) return;
                    if (actionMember != nullptr) std::invoke(actionMember, actionState, evt);

                    // If the destination state is different from the source state, the transition from the one to the other happens here
                    if (static_cast<StateBase*>(srcState) != static_cast<StateBase*>(dstState)) srcState->m_parentState->transitImpl(&evt, dstState);
                }
                catch (...)
                {
                    // Transitions's error job
                    srcState->onError(std::current_exception());
                }
            };
            return new Transition<EventType>{ cb };

        }

    public:
        /**
         * @brief   store
         * @details Provides the state machine's local storage
         * @return  Reference to state machine's local storage
         */
        auto store()
        {
            return topSm() ? topSm()->store() : nullptr;
        }

        /**
         * @brief   store
         * @details Provides the state machine's local storage
         * @return  Const reference to state machine's local storage
         */
        const auto store() const
        {
            return topSm() ? topSm()->store() : nullptr;
        }

        /**
         * @brief       createState
         * param[in]    name: Child state's name
         * param[in]    entry: Flag indicating if the added child state shall be the entry point or not
         * @details     Creates a child state of type ChildType to the parent state in the provided RegionIndex. Forwarded to implementation
         * @return      Pointer to the created child state
         */
        template <typename ChildType, int RegionIndex = 0>
        StateBase* createState(const std::string& name, bool entry = false)
        {
            // Forward to implementation
            return createStateImpl<DerivedType, ChildType, RegionIndex>(name, entry);
        }

        /**
         * @brief       createTransition
         * param[in]    guard: function object that may be used to discard the execution of the transition
         * param[in]    action: function object that is called prior to the transition
         * @details     Creates a transition to the provided DstState with the provided guard and action. Forwarded to implementation
         * @return      Pointer to the created transition
         */
        template <typename EventType, typename StateType, TAction<StateType, EventType> action, TGuard<StateType, EventType> guard, typename DstState = DerivedType>
        TransitionBase* createTransition()
        {
            if (nullptr == action || nullptr == guard) return nullptr;
            // Forward to implementation
            return createTransitionImpl<DerivedType, EventType, StateType, action, guard, DstState>();
        }

        /**
         * @brief       createTransition
         * param[in]    action: function object that is called prior to the transition
         * @details     Creates an external transition to the provided DstState with the provided action. Forwarded to implementation
         * @return      Pointer to the created transition
         */
        template <typename EventType, typename StateType, TAction<StateType, EventType> action, typename DstState>
        TransitionBase* createTransition()
        {
            static_assert(!std::is_same_v<DerivedType, DstState>);

            if (nullptr == action) return nullptr;
            // Forward to implementation
            return createTransitionImpl<DerivedType, EventType, StateType, action, nullptr, DstState>();
        }

        /**
         * @brief       createTransition
         * param[in]    action: function object that is called prior to the transition
         * @details     Creates an inner transition with the provided action. Forwarded to implementation
         * @return      Pointer to the created transition
         */
        template <typename EventType, typename StateType, TAction<StateType, EventType> action>
        TransitionBase* createTransition()
        {
            if (nullptr == action) return nullptr;
            // Forward to implementation
            return createTransitionImpl<DerivedType, EventType, StateType, action, nullptr, DerivedType>();
        }

        /**
         * @brief       createTransition
         * param[in]    guard: function object that may be used to discard the execution of the transition
         * @details     Creates an external transition to the provided DstState with the provided guard. Forwarded to implementation
         * @return      Pointer to the created transition
         */
        template <typename EventType, typename StateType, TGuard<StateType, EventType> guard, typename DstState>
        TransitionBase* createTransition()
        {
            static_assert(!std::is_same_v<DerivedType, DstState>);

            if (nullptr == guard) return nullptr;
            // Forward to implementation
            return createTransitionImpl<DerivedType, EventType, StateType, nullptr, guard, DstState>();
        }

        /**
         * @brief       createTransition
         * @details     Creates an external transition to the provided DstState. Forwarded to implementation
         * @return      Pointer to the created transition
         */
        template <typename EventType, typename DstState>
        TransitionBase* createTransition()
        {
            static_assert(!std::is_same_v<DerivedType, DstState>);

            // Forward to implementation
            return createTransitionImpl<DerivedType, EventType, DerivedType, nullptr, nullptr, DstState>();
        }

        /**
         * @brief       deferEvent
         * param[in]    evt: Event to defer
         * @details     Defers the provided event. The event shall be processed as soon as the state machine enters back into the state that handles this kind of event
         */
        template <typename EventType>
        void deferEvent(const EventType& evt) const
        {
            if (!m_topSm) return;
            if (!topSm()->started()) return;
            m_topSm->deferEventImpl(evt);
        }

        /**
         * @brief       postEvent
         * param[in]    evt: Event to defer
         * @details     Posts the provided event. This might be called from within a state during the processing of another event
         *              The posted event will be processed as soon as the ongoing event processing will be completed
         */
        template <typename EventType>
        void postEvent(const EventType& evt) const
        {
            if (!m_topSm) return;
            if (!topSm()->started()) return;
            // If post from top-sm, it is equivalent to processEvent
            if constexpr (IsTopSm()) processEventImpl(evt);
            else m_topSm->postEventImpl(evt);
        }
    };

    /**
     * @brief   EmptyStore
     * @details Default empty store
     */
    struct EmptyStore {};

    /**
     * @brief   StateMachine
     * @details CRTP base class for State Machines
     *          - Holds the state machine's local storage that is shared among all sub-states to access common data
     *          - Holds the posted events as a queue of prepared callbacks
     */
    template <typename SmType, typename StoreType = EmptyStore>
    class StateMachine : public State<SmType, SmType>
    {
    private:
        static_assert(std::is_default_constructible_v<StoreType>);

        template <typename DerivedType, typename OtherSmType>
        friend class State;

        /**
         * @brief   m_store
         * @details State machine's local storage that is shared among all sub-states to access common data
         */
        StoreType* m_store = nullptr;

        /**
         * @brief   m_posted
         * @details Posted events stored as a queue of prepared callbacks
         */
        mutable std::queue<TransitionBase*> m_posted;

        /**
         * @brief       postPreparedCallback
         * param[in]    cb: Prepared Transition to post
         * @details     Posts a prepared Transition in the top-most state (i.e. StateMachine)
         */
        virtual void postPreparedCallback(TransitionBase* cb) const override
        {
            m_posted.push(cb);
        }

        SmType* Derived()
        {
            return static_cast<SmType*>(this);
        }

    protected:
        template <typename StateType>
        StateType* getInnerState()
        {
            return this->template getStateImpl<StateType>();
        }

    public:
        StateMachine(const std::string& name)
            : State<SmType, SmType>{}
            , m_store{ new StoreType() }
        {
            this->m_name = name;
        }

        virtual ~StateMachine()
        {
            tearDown();
            stop();
            while (!m_posted.empty())
            {
                delete m_posted.front();
                m_posted.pop();
            }

            delete m_store;
        }

        /**
         * @brief   store
         * @details Provides the state machine's local storage
         * @return  Reference to the state machine's local storage
         */
        StoreType* store()
        {
            return m_store;
        }

        /**
         * @brief   store
         * @details Provides the state machine's local storage
         * @return  Const reference to the state machine's local storage
         */
        const StoreType* store() const
        {
            return m_store;
        }

        void visit(IStateVisitor& visitor)
        {
            this->visitImpl(visitor);
        }

        /**
         * @brief   setup
         * @details Initializes the statemachine with the provided states and transitions from overriden getStates/getTransitions/getHistory
         */
        void setup()
        {
            try
            {
                if (this->started()) return;

                this->setupStatesImpl();
                this->setupTransitionsImpl();
                this->setupHistoryImpl();
            }
            catch (...)
            {
                // State's error job
                Derived()->onError(std::current_exception());
            }
        }

        /**
         * @brief   tearDown
         * @details Clears the statemachine: removes all states and transitions
         */
        void tearDown()
        {
            if (this->started()) return;

            this->tearDownImpl();
        }

        /**
         * @brief   start
         * @details Starts the statemachine. Forwarded to implementation
         */
        void start()
        {
            try
            {
                if (this->started()) return;

                // Forward to implementation
                this->startImpl(nullptr);
            }
            catch (...)
            {
                // State's error job
                Derived()->onError(std::current_exception());
            }
        }

        /**
         * @brief   stop
         * @details Stops the statemachine. Forwarded to implementation
         */
        void stop()
        {
            if (!this->started()) return;

            // Forward to implementation
            this->stopImpl(nullptr);
        }

        /**
         * @brief       addState
         * param[in]    name: Child state's name
         * param[in]    entry: Flag indicating if the added child state shall be the entry point or not
         * @details     Adds a child state of type ChildState to the statemachine itself in the provided RegionIndex
         */
        template <typename ChildState, int RegionIndex = 0>
        void addState(const std::string& name, bool entry = false)
        {
            if (this->started()) return;

            try
            {
                auto childState = this->template createStateImpl<SmType, ChildState, RegionIndex>(name, entry);
                StateBase::addState(this, childState);
            }
            catch (...)
            {
                // State's error job
                Derived()->onError(std::current_exception());
            }
        }

        /**
         * @brief       addState
         * param[in]    name: Child state's name
         * param[in]    entry: Flag indicating if the added child state shall be the entry point or not
         * @details     Adds a child state of type ChildState to the provided ParentState in the provided RegionIndex
         */
        template <typename ParentState, typename ChildState, int RegionIndex = 0>
        void addState(const std::string& name, bool entry = false)
        {
            if (this->started()) return;

            try
            {
                auto parentState = this->template getStateImpl<ParentState>();
                auto childState = this->template createStateImpl<ParentState, ChildState, RegionIndex>(name, entry);
                StateBase::addState(parentState, childState);
            }
            catch (...)
            {
                // State's error job
                Derived()->onError(std::current_exception());
            }
        }

        template <History history = History::None>
        void setHistory()
        {
            if (this->started()) return;

            StateBase::setHistory(history);
        }

        template <typename StateType, History history = History::None>
        void setHistory()
        {
            static_assert(!std::is_same_v<SmType, StateType>);

            if (this->started()) return;

            auto state = this->template getStateImpl<StateType>();
            if (state != nullptr) state->setHistory(history);
        }

        /**
         * @brief       addTransition
         * param[in]    guard: function object that may be used to discard the execution of the transition
         * param[in]    action: function object that is called prior to the transition
         * @details     Adds a transition to the provided DstState with the provided guard and action
         */
        template <typename SrcState, typename EventType, typename StateType, TAction<StateType, EventType> action, TGuard<StateType, EventType> guard, typename DstState = SrcState>
        void addTransition()
        {
            if (this->started()) return;
            if (nullptr == action || nullptr == guard) return;

            try
            {
                auto state = this->template getStateImpl<SrcState>();
                auto transition = this->template createTransitionImpl<SrcState, EventType, StateType, action, guard, DstState>();
                StateBase::addTransition(state, transition);
            }
            catch (...)
            {
                // State's error job
                Derived()->onError(std::current_exception());
            }
        }

        /**
         * @brief       addTransition
         * param[in]    action: function object that is called prior to the transition
         * @details     Adds an external transition to the provided DstState with the provided action
         */
        template <typename SrcState, typename EventType, typename StateType, TAction<StateType, EventType> action, typename DstState>
        void addTransition()
        {
            static_assert(!std::is_same_v<SrcState, DstState>);

            if (this->started()) return;
            if (nullptr == action) return;

            try
            {
                auto state = this->template getStateImpl<SrcState>();
                auto transition = this->template createTransitionImpl<SrcState, EventType, StateType, action, nullptr, DstState>();
                StateBase::addTransition(state, transition);
            }
            catch (...)
            {
                // State's error job
                Derived()->onError(std::current_exception());
            }
        }

        /**
         * @brief       addTransition
         * param[in]    action: function object that is called prior to the transition
         * @details     Adds an internal transition with the provided action
         */
        template <typename SrcState, typename EventType, typename StateType, TAction<StateType, EventType> action>
        void addTransition()
        {
            if (this->started()) return;
            if (nullptr == action) return;

            try
            {
                auto state = this->template getStateImpl<SrcState>();
                auto transition = this->template createTransitionImpl<SrcState, EventType, StateType, action, nullptr, SrcState>();
                StateBase::addTransition(state, transition);
            }
            catch (...)
            {
                // State's error job
                Derived()->onError(std::current_exception());
            }
        }

        /**
         * @brief       addTransition
         * param[in]    guard: function object that may be used to discard the execution of the transition
         * @details     Adds an external transition to the provided DstState with the provided guard
         */
        template <typename SrcState, typename EventType, typename StateType, TGuard<StateType, EventType> guard, typename DstState>
        void addTransition()
        {
            static_assert(!std::is_same_v<SrcState, DstState>);

            if (this->started()) return;
            if (nullptr == guard) return;

            try
            {
                auto state = this->template getStateImpl<SrcState>();
                auto transition = this->template createTransitionImpl<SrcState, EventType, StateType, nullptr, guard, DstState>();
                StateBase::addTransition(state, transition);
            }
            catch (...)
            {
                // State's error job
                Derived()->onError(std::current_exception());
            }
        }

        /**
         * @brief       addTransition
         * @details     Adds an external transition to the provided DstState
         */
        template <typename SrcState, typename EventType, typename DstState>
        void addTransition()
        {
            static_assert(!std::is_same_v<SrcState, DstState>);

            if (this->started()) return;

            try
            {
                auto state = this->template getStateImpl<SrcState>();
                auto transition = this->template createTransitionImpl<SrcState, EventType, SrcState, nullptr, nullptr, DstState>();
                StateBase::addTransition(state, transition);
            }
            catch (...)
            {
                // State's error job
                Derived()->onError(std::current_exception());
            }
        }

        /**
         * @brief   processEvent
         * @details Processes the provided event. Forwarded to implementation
         */
        template <typename EventType>
        void processEvent(const EventType& evt) const
        {
            if (!this->started()) return;

            // Forward to implementation
            this->processEventImpl(evt);

            // Unqueue the posted events and executed the corresponding prepared callbacks
            while (!m_posted.empty())
            {
                m_posted.front()->execPrepared();
                delete m_posted.front();
                m_posted.pop();
            }
        }

        template <typename ...StateTypes, typename = is_empty_pack<StateTypes...>>
        bool checkStates()
        {
            return false;
        }

        template <typename FirstStateType, typename ...OtherStateTypes>
        bool checkStates()
        {
            if constexpr (std::is_same_v<FirstStateType, SmType>)
            {
                return this->template checkStatesImpl<OtherStateTypes...>(this->m_topSm);
            }
            else return this->template checkStatesImpl<FirstStateType, OtherStateTypes...>();
        }
    };
}

#endif
