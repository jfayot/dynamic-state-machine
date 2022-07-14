#ifndef DYNAMIC_STATE_MACHINE_INCLUDED_
#define DYNAMIC_STATE_MACHINE_INCLUDED_

#include "log.hpp"

#include <string>
#include <sstream>
#include <ostream>
#include <functional>
#include <typeindex>
#include <map>
#include <queue>
#include <deque>
#include <vector>
#include <optional>
#include <algorithm>
#include <type_traits>

/**
 * Test for non-portable GNU extension compatibility.
 */
#ifdef __GNUC__
#define HAS_GNU_EXTENSIONS_
#endif

/**
 * cxxabi.h is a non-portable GNU extension.  Only include it if we're compiling against a
 * GNU library.
 */
#ifdef HAS_GNU_EXTENSIONS_
#include <cxxabi.h>
#endif

#ifndef DSM_LOGMODULE
#define DSM_LOGMODULE "dsm"
#endif

#ifndef DSM_LOGGER
#define DSM_LOGGER Log::EmptyLogger
#endif

#define LOG_DSM(severity, error)  Log::Logger<DSM_LOGGER>::GetInstance()->writeLog(DSM_LOGMODULE, severity, error)

#define LOG_DEBUG_DSM(...) { std::stringstream ss; ss << __VA_ARGS__; \
Log::Logger<DSM_LOGGER>::GetInstance()->writeLog(DSM_LOGMODULE, Log::eDebug, ss.str()); }

#define LOG_INFO_DSM(...)  { std::stringstream ss; ss << __VA_ARGS__; \
Log::Logger<DSM_LOGGER>::GetInstance()->writeLog(DSM_LOGMODULE, Log::eInfo, ss.str()); }

#define LOG_WARNING_DSM(...) { std::stringstream ss; ss << __VA_ARGS__; \
Log::Logger<DSM_LOGGER>::GetInstance()->writeLog(DSM_LOGMODULE, Log::eWarning, ss.str()); }

#define LOG_ERROR_DSM(...) { std::stringstream ss; ss << __VA_ARGS__; \
Log::Logger<DSM_LOGGER>::GetInstance()->writeLog(DSM_LOGMODULE, Log::eError, ss.str()); }

#define LOG_FATAL_DSM(...) { std::stringstream ss; ss << __VA_ARGS__; \
Log::Logger<DSM_LOGGER>::GetInstance()->writeLog(DSM_LOGMODULE, Log::eFatal, ss.str()); }

namespace dsm
{
    // Forwarded declarations
    enum class History;
    struct Entry;
    struct NoEntry;
    class EventBase;
    template <typename DerivedType>
    class Event;
    class StateBase;
    template <typename DerivedType, typename SmType>
    class State;
    template <typename SmType, typename StoreType>
    class StateMachine;
    namespace details
    {
        class TransitionBase;
        template <typename EventType>
        struct Transition;
        struct PostedTransition;
    }

    // Types aliases
    using TStates = std::vector<StateBase*>;
    using TTransitions = std::vector<details::TransitionBase*>;
    using THistory = std::optional<History>;
    template <typename StateType, typename EventType>
    using TGuard = bool(StateType::*)(const EventType&);
    template <typename StateType, typename EventType>
    using TAction = void(StateType::*)(const EventType&);

    inline std::string what(std::exception_ptr eptr)
    {
        if (false == bool(eptr)) throw std::bad_exception();

        try
        {
            std::rethrow_exception(eptr);
        }
        catch (const std::exception& ex)
        {
            return ex.what();
        }
        catch (const std::string& ex)
        {
            return ex;
        }
        catch (const char* ex)
        {
            return ex;
        }
        catch (...)
        {
            return "Unknown exception";
        }
    }

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
    enum class History
    {
        Shallow = 0,
        Deep = 1
    };

    struct Entry : std::true_type {};
    struct NoEntry : std::false_type {};

    /**
     * @brief   EventBase
     * @details Base class for all Events. Holds the type index of the final subclass
     */
    class EventBase
    {
    private:
        friend class StateBase;

        template <typename DerivedType, typename SmType>
        friend class State;

        template <typename DerivedType>
        friend class Event;

        friend struct details::PostedTransition;

        /**
         * @brief   m_index
         * @details Event's type index
         */
        std::type_index m_index;

    protected:
        /**
         * @brief   m_name
         * @details Event's name
         */
        std::string m_name;

        EventBase(const std::type_index& index)
            : m_index{ index }
        {}

    public:
        virtual ~EventBase() {}

    private:
        virtual bool apply(details::TransitionBase* pTransition) const = 0;

        virtual EventBase* clone() const = 0;
    };

    // Implementation details (not supposed to be used by client code)
    namespace details
    {
        // Concepts and constraints
        template <typename ...Ts>
        using is_empty_pack = std::enable_if_t<0 == sizeof ...(Ts)>;

        template <template<typename...> class BaseType, typename DerivedType>
        struct is_base_of_template
        {
            template <typename ...Ts>
            static constexpr std::true_type test(const BaseType<Ts...>*);
            static constexpr std::false_type test(...);
            using type = decltype(test(std::declval<DerivedType*>()));
        };

        template <template<typename...> class BaseType, typename DerivedType>
        using is_base_of_template_t = typename is_base_of_template<BaseType, DerivedType>::type;

        template <typename DerivedType>
        using is_event_t = is_base_of_template_t<dsm::Event, DerivedType>;

        template <typename DerivedType>
        inline constexpr bool is_event_v = is_event_t<DerivedType>::value;

        template <typename DerivedType>
        using is_state_t = is_base_of_template_t<dsm::State, DerivedType>;

        template <typename DerivedType>
        inline constexpr bool is_state_v = is_state_t<DerivedType>::value;

        template <typename DerivedType>
        using is_state_machine_t = is_base_of_template_t<dsm::StateMachine, DerivedType>;

        template <typename DerivedType>
        inline constexpr bool is_state_machine_v = is_state_machine_t<DerivedType>::value;

        template <typename EntryType>
        inline constexpr bool is_entry_v = std::is_same_v<dsm::Entry, EntryType> || std::is_same_v<dsm::NoEntry, EntryType>;

        template <typename StateType, typename EntryType = dsm::NoEntry>
        using is_state_def_v = std::enable_if_t<is_state_v<StateType> && is_entry_v<EntryType>, bool>;

        template <typename FirstStateType, typename SecondStateType>
        struct is_same_state_t
        {
            static_assert(is_state_v<FirstStateType>, "FirstStateType must inherit from State");
            static_assert(is_state_v<SecondStateType>, "SecondStateType must inherit from State");
            static constexpr bool value = std::is_same_v<typename FirstStateType::Derived, typename SecondStateType::Derived>;
        };

        template <typename FirstStateType, typename SecondStateType>
        inline constexpr bool is_same_state_v = is_same_state_t<FirstStateType, SecondStateType>::value;

        // Helpers

        std::string demangle(const char* name)
        {
            std::string res;
            int status{ 0 };

            // Demangle the type name if using non-portable GNU extensions.
#ifdef HAS_GNU_EXTENSIONS_
            char* ret = abi::__cxa_demangle(name, nullptr, nullptr, &status);
            if (ret != nullptr)
            {
                res = ret;
                free(ret);
            }
#else
            res = name;
#endif

            return res;
        }

        /**
         * @brief   Index
         * @details Transforms Type into type_index for associative containers
         * @return  Returns the Types's type_index
         */
        template <typename Type>
        static const std::type_index Index()
        {
            static_assert(details::is_event_v<Type> || is_state_v<Type>, "Type must inherit either from Event or State");
            return std::type_index(typeid(typename Type::Derived));
        }

        static const std::type_index Index()
        {
            return std::type_index(typeid(void));
        }

        /**
         * @brief   Name
         * @details Retrieves Type's name
         * @return  Returns the Types's name
         */
        template <typename Type>
        static const std::string Name()
        {
            static_assert(details::is_event_v<Type> || is_state_v<Type>, "Type must inherit either from Event or State");
            std::string name = demangle(typeid(Type).name());
            // Remove 'class' or 'struct' specifiers
            if (name.substr(0, 6) == "struct") name = name.substr(7);
            else if (name.substr(0, 5) == "class") name = name.substr(6);
            return name;
        }

        /**
         * @brief   CheckIndex
         * @details Checks whether Type and provided type_index are matching
         * @return  True if there's a match, false otherwise
         */
        template <typename Type>
        static constexpr bool CheckIndex(const std::type_index& index)
        {
            static_assert(details::is_event_v<Type> || is_state_v<Type>, "Type must inherit either from Event or State");
            return index == Index<Type>();
        }

        // Transitions

        /**
         * @brief   TransitionBase
         * @details Base class for all transitions. Holds the type index of the final subclass
         */
        class TransitionBase
        {
        private:
            friend class dsm::StateBase;

            template <typename DerivedType, typename SmType>
            friend class dsm::State;

            template <typename SmType, typename StoreType>
            friend class dsm::StateMachine;

            friend struct PostedTransition;

            /**
                * @brief   m_srcState
                * @details The transition's source state
                */
            dsm::StateBase* m_srcState = nullptr;

            /**
                * @brief   m_index
                * @details Transition's type index
                */
            std::type_index m_index;

        protected:
            explicit TransitionBase(const std::type_index& index)
                : m_index{ index }
            {}

        public:
            virtual ~TransitionBase() {}
        };

        /**
         * @brief   Transition
         * @details CRTP base class. Allows type index initialization
         */
        template <typename EventType>
        struct Transition : public TransitionBase
        {
            using TCallbackFunc = std::function<bool(const EventType&)>;

            /**
             * @brief   m_cb
             * @details The stored function object
             */
            TCallbackFunc m_cb;

            explicit Transition(const TCallbackFunc& cb)
                : TransitionBase{ Index<EventType>() }
                , m_cb{ cb }
            {
                static_assert(details::is_event_v<EventType>, "EventType must inherit from Event");
            }

            virtual ~Transition() {}

            /**
             * @brief   exec
             * @details Executes the transition with the provided event.
             */
            bool exec(const EventType& evt)
            {
                return m_cb(evt);
            }
        };

        /**
         * @brief   Transition
         * @details CRTP base class. Allows type index initialization
         */
        template <>
        struct Transition<void> : public TransitionBase
        {
            using TCallbackFunc = std::function<bool()>;

            /**
             * @brief   m_cb
             * @details The stored function object
             */
            TCallbackFunc m_cb;

            explicit Transition(const TCallbackFunc& cb)
                : TransitionBase{ Index() }
                , m_cb{ cb }
            {}

            virtual ~Transition() {}

            /**
             * @brief   exec
             * @details Executes the transition.
             */
            bool exec()
            {
                return m_cb();
            }
        };

        struct PostedTransition
        {
            explicit PostedTransition(dsm::EventBase* evt, bool deferred = false)
                : m_evt{ evt }
                , m_deferred{ deferred }
            {}

            explicit PostedTransition(TransitionBase* transition, bool deferred = false)
                : m_transition{ transition }
                , m_deferred{ deferred }
            {}

            virtual ~PostedTransition()
            {
                delete m_evt;
                m_evt = nullptr;
                delete m_transition;
                m_transition = nullptr;
            }

            PostedTransition(const PostedTransition&) = delete;
            PostedTransition& operator=(const PostedTransition&) = delete;

            PostedTransition(PostedTransition&& other)
            {
                swap(std::move(other));
            }

            PostedTransition& operator=(PostedTransition&& other)
            {
                swap(std::move(other));
                return *this;
            }

            void swap(PostedTransition&& other)
            {
                std::swap(m_evt, other.m_evt);
                std::swap(m_transition, other.m_transition);
                std::swap(m_deferred, other.m_deferred);
            }

            dsm::EventBase* m_evt = nullptr;

            TransitionBase* m_transition = nullptr;

            bool m_deferred = false;
        };

        // Exceptions

        /**
        * @brief   SmError
        * @details StateMachine's exception
        */
        struct SmError : public std::exception
        {
            /**
            * @brief   m_msg
            * @details The exception's message
            */
            mutable std::string m_msg;
            std::stringstream m_sstr;

            SmError() = default;
            virtual ~SmError() = default;
            SmError(SmError&&) = default;
            SmError& operator=(SmError&&) = default;

            SmError(const SmError& other)
            {
                m_sstr << other.m_sstr.rdbuf();
            }

            SmError& operator=(const SmError& other)
            {
                m_sstr << other.m_sstr.rdbuf();
                return *this;
            }

            SmError& operator<<(const char* msg)
            {
                m_sstr << msg;
                return *this;
            }

            SmError& operator<<(const std::string& msg)
            {
                return *this << msg.c_str();
            }

            /**
            * @brief   what
            * @details Retrieves the exception's message
            */
            const char* what() const noexcept override
            {
                // Storing msg is needed because std::stringstream::str returns a temporary object
                m_msg = m_sstr.str();
                return m_msg.c_str();
            }
        };
    }

    /**
     * @brief   Event
     * @details CRTP base class for Events. Allows type index initialization
     */
    template <typename DerivedType>
    class Event : public EventBase
    {
    private:
        template <typename OtherDerivedType, typename SmType>
        friend class State;

    public:
        using Derived = DerivedType;

    protected:
        Event() :
            EventBase{ details::Index<DerivedType>() }
        {
            static_assert(std::is_base_of_v<Event<DerivedType>, DerivedType>, "DerivedType must inherit from Event");
            static_assert(std::is_copy_constructible_v<DerivedType>, "DerivedType must be copy constructible");
            this->m_name = details::Name<DerivedType>();
        }

        virtual ~Event() {}

    private:
        DerivedType& derived()
        {
            return static_cast<DerivedType&>(*this);
        }

        const DerivedType& derived() const
        {
            return static_cast<const DerivedType&>(*this);
        }

        EventBase* clone() const override
        {
            return new DerivedType(derived());
        }

        bool apply(details::TransitionBase* pTransition) const override
        {
            return static_cast<details::Transition<DerivedType>*>(pTransition)->exec(derived());
        }
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
         * @brief   TransitionData
         * @details Data associated to a transition
         */
        struct TransitionData
        {
            /**
             * @brief   commonAncestor
             * @details The common ancestor of both source state and destination state. In the simplest case, this corresponds to the source and destination's parent
             */
            StateBase* commonAncestor = nullptr;

            /**
             * @brief   srcOutermost
             * @details The outermost state containing the source state. In the simplest case, this corresponds to the source state itself
             */
            StateBase* srcOutermost = nullptr;

            /**
             * @brief   dstOutermost
             * @details The outermost state containing the destination state. In the simplest case, this corresponds to the destination state itself
             */
            StateBase* dstOutermost = nullptr;

            /**
             * @brief   src
             * @details The source state
             */
            StateBase* src = nullptr;

            /**
             * @brief   dst
             * @details The destination state
             */
            StateBase* dst = nullptr;
        };

        /**
         * @brief   Region
         * @details Region inside a state. Holds pointers to the entry point state, the current state and the last visited state
         */
        struct Region
        {
            /**
             * @brief   m_index
             * @details The region's index
             */
            int m_index;

            /**
             * @brief   m_parentState
             * @details The region's parent state
             */
            StateBase* m_parentState = nullptr;

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
             * @brief   m_lastVisitedState
             * @details The last visited sub-state
             */
            StateBase* m_lastVisitedState = nullptr;

            /**
             * @brief   m_history
             * @details The state's history type
             */
            THistory m_history = std::nullopt;

            /**
             * @brief   m_children
             * @details The sub-states mapped with their type index
             */
            std::map<std::type_index, StateBase*> m_children = {};

            /**
             * @brief   getDescendant
             * @details Recursively searches for the state with the specified type StateType
             * @return  Pointer to the found state if any, nullptr otherwise
             */
            template <typename StateType>
            StateType* getDescendant()
            {
                // Loop over sub-states
                for (const auto&[_, child] : m_children)
                {
                    auto state = child->template getDescendantImpl<StateType>();
                    if (state != nullptr) return state;
                }

                return nullptr;
            }

            /**
             * @brief   contains
             * @details Recursively searches for the provided state
             * @return  true if state found, false otherwise
             */
            bool contains(const StateBase* state) const
            {
                for (const auto&[_, child] : m_children)
                {
                    if (true == child->contains(state)) return true;
                }

                return false;
            }

            /**
             * @brief   getOutermost
             * @details Searches into this region the outermost state containg a given inner state, skipping the specified state
             * @return  Pointer to the outermost state if found, nullptr otherwise
             */
            StateBase* getOutermost(const StateBase* innerState, const StateBase* skippedState)
            {
                for (const auto&[_, child] : m_children)
                {
                    if (child == skippedState) continue;
                    if (true == child->contains(innerState)) return child;
                }

                return nullptr;
            }

            /**
             * @brief   start
             * @details Starts this region with the provided event. A specific state to start may be provided in order to bypass the entry point or last visited
             */
            void start(const EventBase* evt, bool propagateHistory, StateBase* stateToStart = nullptr)
            {
                // Check if we are provided with a particular state to start
                if (nullptr == stateToStart)
                {
                    // If state already visited then restore the last visited state depending on current history type or history propagation 
                    if (m_lastVisitedState != nullptr && (m_history != std::nullopt || true == propagateHistory)) m_currentState = m_lastVisitedState;
                    // Otherwise use the entry state
                    else m_currentState = m_entryState;
                }
                else
                {
                    // Check if the provided state to start belongs to this region
                    if (m_children.end() != m_children.find(stateToStart->m_index)) m_currentState = stateToStart;
                    else m_currentState = nullptr;
                }

                // Start current sub-state if any
                if (m_currentState != nullptr) m_currentState->startImpl(evt, Propagate(propagateHistory, this));
            }

            /**
             * @brief   stop
             * @details Stops this region with the provided event
             */
            void stop(const EventBase* evt)
            {
                // Stop current sub-state if any
                if (m_currentState != nullptr) m_currentState->stopImpl(evt);

                // Backup last visited sub-state
                m_lastVisitedState = m_currentState;

                // Reset current sub-state
                m_currentState = nullptr;
            }

            /**
             * @brief   setHistory
             * @details Sets the history for this region
             */
            void setHistory(THistory history)
            {
                if (History::Deep == history)
                {
                    // Ensure that there is no ancestor nor descendant with deep history
                    auto res = getDeepAncestorOrDescendant();
                    if (res != std::nullopt)
                    {
                        LOG_ERROR_DSM("Failed to set Deep history on state '<" << m_parentState->name() << ", " << m_index << ">'."
                            " Deep history already defined in " << std::get<0>(*res) << " state '<" << std::get<1>(*res)->name() << ", " << std::get<2>(*res) << ">'");
                        return;
                    }
                }

                if (History::Shallow == history)
                {
                    // Ensure that there is no ancestor with deep history
                    auto res = getDeepAncestor();
                    if (res != std::nullopt)
                    {
                        LOG_ERROR_DSM("Failed to set Shallow history on state '<" << m_parentState->name() << ", " << m_index << ">'."
                            " Deep history already defined in ancestor state '<" << res->first->name() << ", " << res->second << ">'");
                        return;
                    }
                }

                // Store history type
                m_history = history;

                // Reset the last visited state when changing history settings
                m_lastVisitedState = nullptr;
            }

            void clearHistory(bool recursive)
            {
                m_lastVisitedState = nullptr;

                if (true == recursive)
                {
                    for (const auto&[_, child] : m_children)
                    {
                        for (const auto&[_, region] : child->m_regions)
                        {
                            region->clearHistory(recursive);
                        }
                    }
                }
            }

            /**
             * @brief   resetHistory
             * @details Resets the history for this region and it's children
             */
            void resetHistory(bool recursive)
            {
                // Reset history
                m_history = std::nullopt;

                // Reset the last visited state when changing history settings
                m_lastVisitedState = nullptr;

                if (true == recursive)
                {
                    for (const auto&[_, child] : m_children)
                    {
                        for (const auto&[_, region] : child->m_regions)
                        {
                            region->resetHistory(recursive);
                        }
                    }
                }
            }

            std::optional<std::tuple<std::string, StateBase*, int>> getDeepAncestorOrDescendant()
            {
                if (History::Deep == m_history) return { { "this", m_parentState, m_index } };
                auto res = getDeepAncestor();
                if (res != std::nullopt) return { { "ancestor", res->first, res->second } };
                res = getDeepDescendant();
                if (res != std::nullopt) return { { "descendant", res->first, res->second } };
                return std::nullopt;
            }

            /**
             * @brief   hasAncestorDeep
             * @details Checks if any ancestor of this region has a deep history
             * @return  true if ancestor with deep history found, false otherwise
             */
            std::optional<std::pair<StateBase*, int>> getDeepAncestor() const
            {
                if (History::Deep == m_history) return { { m_parentState, m_index } };
                if (m_parentState != nullptr && m_parentState->m_parentRegion != nullptr) return m_parentState->m_parentRegion->getDeepAncestor();
                return {};
            }

            /**
             * @brief   hasDescendantDeep
             * @details Checks if any descendant of this region has a deep history
             * @return  true if descendant with deep history found, false otherwise
             */
            std::optional<std::pair<StateBase*, int>> getDeepDescendant() const
            {
                if (History::Deep == m_history) return { { m_parentState, m_index } };

                for (const auto&[_, child] : m_children)
                {
                    for (const auto&[_, region] : child->m_regions)
                    {
                        auto res = region->getDeepDescendant();
                        if (res != std::nullopt) return res;
                    }
                }

                return {};
            }
        };

        /**
         * @brief   m_name
         * @details The state's name
         */
        std::string m_name = {};

        /**
         * @brief   m_index
         * @details The state's type index
         */
        std::type_index m_index;

        /**
         * @brief   m_regions
         * @details The state's regions container
         */
        std::map<int, Region*> m_regions = {};

        /**
         * @brief   m_entry
         * @details The state's entry flag
         */
        bool m_entry = false;

        /**
         * @brief   m_started
         * @details Flag indicating whether the state machine is started or not.
         *          - A stopped state machine won't process any event
         *          - A started state machine won't accept any new state nor transition to be added
         */
        bool m_started = false;

        /**
         * @brief   m_regionIndex
         * @details The state's region index
         */
        int m_regionIndex = 0;

        /**
         * @brief   m_parentRegion
         * @details The state's parent region
         */
        Region* m_parentRegion = nullptr;

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
         * @brief   m_transitions
         * @details The state's allowed transitions mapped with their event's type index
         */
        std::map<std::type_index, details::TransitionBase*> m_transitions = {};

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
         * @brief   contains
         * @details Recursively searches for the provided state
         * @return  true if state found, false otherwise
         */
        bool contains(const StateBase* state) const
        {
            // Check self state
            if (this == state) return true;

            // Loop over orthogonal regions
            for (const auto&[_, region] : m_regions)
            {
                if (true == region->contains(state)) return true;
            }

            return false;
        }

        /**
         * @brief           dump
         * @param[in,out]   stream: output stream
         * @details         Outputs this state to the provided stream
         * @return          Returns the output stream
         */
        std::ostream& dump(std::ostream& stream) const
        {
            stream << m_name;
            if (m_regions.size() > 1) stream << "[";
            for (std::map<int, Region*>::const_iterator it = m_regions.cbegin(); it != m_regions.cend(); ++it)
            {
                if (it->second->m_currentState != nullptr) stream << "->" << *(it->second->m_currentState);
                if (std::next(it) != m_regions.end()) stream << "|";
            }
            if (m_regions.size() > 1) stream << "]";

            return stream;
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
            return state.dump(stream);
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
            for (const auto&[_, region] : m_regions)
            {
                if (region->m_currentState != nullptr) region->m_currentState->visitImpl(visitor);
            }
        }

        /**
         * @brief       StateBase
         * @param[in]   index: sub-class type index
         * @details     StateBase ctor
         */
        explicit StateBase(const std::type_index& index)
            : m_index{ index }
        {}

        void setupImpl()
        {
            // Order matters
            this->setupStatesImpl();
            this->setupTransitionsImpl();
            this->setupHistoryImpl();
        }

        /**
         * @brief   clearImpl
         * @details Recursively destro this state and its children
         */
        void clearImpl()
        {
            for (auto&[_, transition] : m_transitions)
            {
                delete transition;
                transition = nullptr;
            }

            m_transitions.clear();

            for (auto&[_, region] : m_regions)
            {
                for (auto&[_, child] : region->m_children)
                {
                    delete child;
                    child = nullptr;
                }

                region->m_children.clear();

                delete region;
                region = nullptr;
            }

            m_regions.clear();
        }

        /**
         * @brief   ~StateBase
         * @details StateBase dtor
         */
        virtual ~StateBase()
        {
            clearImpl();
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
        virtual THistory getHistory(int regionIndex) { return {}; }

        /**
         * @brief   onEntry
         * @details State's entry job
         */
        virtual void onEntry()
        {
            LOG_DEBUG_DSM("Entering state " << m_name << " through event " << (m_trigEvent != nullptr ? m_trigEvent->m_name : "anonymous"));
        }

        /**
         * @brief   onExit
         * @details State's exit job
         */
        virtual void onExit()
        {
            LOG_DEBUG_DSM("Leaving state " << m_name << " through event " << (m_trigEvent != nullptr ? m_trigEvent->m_name : "anonymous"));
        }

        /**
         * @brief       onError
         * @param[in]   ex: exception causing this call
         * @details     State's error job
         */
        virtual void onError(std::exception_ptr eptr)
        {
            LOG_ERROR_DSM(what(eptr));
        }

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
        void startImpl(const EventBase* evt, bool propagateHistory, bool recurse = true)
        {
            m_started = true;

            // Backup triggering event
            m_trigEvent = evt;

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

            if (true == recurse)
            {
                // Loop over orthogonal regions
                for (auto&[_, region] : m_regions)
                {
                    region->start(evt, propagateHistory);
                }
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
        void stopImpl(const EventBase* evt)
        {
            // Backup triggering event
            m_trigEvent = evt;

            // Loop over orthogonal regions
            for (auto&[_, region] : m_regions)
            {
                region->stop(evt);
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
         * @param[in]   parent: the parent state where this state shall be added
         * @details     Tries to add this state as a child state to the provided parent state in parameter
         */
        void addState(StateBase* parent)
        {
            // parent cannot be null by design

            Region* region{ nullptr };
            auto itRegion = parent->m_regions.find(m_regionIndex);
            if (itRegion != parent->m_regions.end())
            {
                region = itRegion->second;

                // No more than one entry point allowed in a single region
                if (true == m_entry)
                {
                    for (const auto&[_, child] : region->m_children)
                    {
                        if (true == child->m_entry)
                        {
                            throw details::SmError() << "Failed to add entry point state '" << m_name << "'. The parent's region already has an entry point which is state '" << child->m_name << "'";
                        }
                    }
                }
            }
            else
            {
                region = new Region();

                if (nullptr == region)
                {
                    throw details::SmError() << "Failed to create region";
                }

                parent->m_regions[m_regionIndex] = region;

                // Set the region's index
                region->m_index = m_regionIndex;
            }

            // Set the child state's parent region
            m_parentRegion = region;

            // Set the region's parent state
            region->m_parentState = parent;

            // Add the newly created state to the corresponding region
            region->m_children.emplace(m_index, this);

            // Define the entry state if needed
            if (true == m_entry) region->m_entryState = this;
        }

        /**
         * @brief       addTransition
         * @param[in]   transition: transition to add
         * @details     Adds the provided transition to its source state
         */
        void addTransition(details::TransitionBase* transition)
        {
            // transition cannot be null by design
            // source state cannot be null by design
            StateBase* srcState = transition->m_srcState;

            // Add the newly created Transition to the transitions container
            auto res = srcState->m_transitions.insert({ transition->m_index, transition });

            if (false == res.second)
            {
                throw details::SmError() << "Trying to insert an already existing transition";
            }
        }

        /**
         * @brief       setupStatesImpl
         * @details     Recursively adds the user provided states (through getStates overrides) to the state machine
         */
        void setupStatesImpl()
        {
            for (auto& state : getStates())
            {
                try
                {
                    if (state != nullptr) state->addState(this);
                }
                catch (...)
                {
                    delete state;
                    state = nullptr;

                    // State's error job
                    onError(std::current_exception());
                }

            }

            for (const auto&[_, region] : m_regions)
            {
                for (const auto&[_, child] : region->m_children)
                {
                    child->setupStatesImpl();
                }
            }
        }

        /**
         * @brief       setupTransitionsImpl
         * @details     Recursively adds the user provided transitions (through getTransitions overrides) to the state machine
         */
        void setupTransitionsImpl()
        {
            for (auto& transition : getTransitions())
            {
                try
                {
                    if (transition != nullptr) addTransition(transition);
                }
                catch (...)
                {
                    delete transition;
                    transition = nullptr;

                    // State's error job
                    onError(std::current_exception());
                }
            }

            for (const auto&[_, region] : m_regions)
            {
                for (const auto&[_, child] : region->m_children)
                {
                    child->setupTransitionsImpl();
                }
            }
        }

        /**
         * @brief       setupHistoryImpl
         * @details     Recursively sets the user provided histories (through getHistory overrides) to the state machine
         */
        void setupHistoryImpl()
        {
            for (auto&[index, region] : m_regions)
            {
                region->setHistory(getHistory(index));

                for (const auto&[_, child] : region->m_children)
                {
                    child->setupHistoryImpl();
                }
            }
        }

        /**
         * @brief   getDescendantImpl
         * @details Recursively searches downwards in descendants for the state with the specified type StateType
         * @return  Pointer to the found state if any, nullptr otherwise
         */
        template <typename StateType>
        StateType* getDescendantImpl()
        {
            static_assert(details::is_state_v<StateType>, "StateType must inherit from State");

            // Check self state index
            if (true == details::CheckIndex<StateType>(m_index)) return static_cast<StateType*>(this);

            // Loop over orthogonal regions
            for (const auto&[_, region] : m_regions)
            {
                auto state = region->template getDescendant<StateType>();
                if (state != nullptr) return state;
            }

            return nullptr;
        }

        /**
         * @brief   getAncestorImpl
         * @details Recursively searches upwards in ancestors for the state with the specified type StateType
         * @return  Pointer to the found state if any, nullptr otherwise
         */
        template <typename StateType>
        StateType* getAncestorImpl()
        {
            static_assert(details::is_state_v<StateType>, "StateType must inherit from State");

            // Check self state index
            if (true == details::CheckIndex<StateType>(m_index)) return static_cast<StateType*>(this);

            // Recurse stop condition
            if (nullptr == m_parentState) return nullptr;

            // Recurse call
            return m_parentState->template getAncestorImpl<StateType>();
        }

        /**
         * @brief   Propagate
         * @details Helper function that computes the history propagation flag according to previous propagation flag and provided region's history
         * @return  true if previous propagation flag is true or if provided region has deep history, false otherwise
         */
        static bool Propagate(bool propagateHistory, const Region* region)
        {
            return true == propagateHistory || History::Deep == region->m_history;
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
        bool transitImpl(const EventBase* evt, const TransitionData& data, bool propagateHistory)
        {
            if (this == data.commonAncestor)
            {
                if (data.srcOutermost != nullptr && true == data.srcOutermost->m_started) data.srcOutermost->stopImpl(evt);
                bool propagate = Propagate(propagateHistory, data.dstOutermost->m_parentRegion);
                data.dst->startAncestors(evt, data, this, propagate);
                return true;
            }

            for (const auto&[_, region] : m_regions)
            {
                if (region->m_currentState != nullptr)
                {
                    if (true == region->m_currentState->transitImpl(evt, data, Propagate(propagateHistory, region)))
                        return true;
                }
            }

            return false;
        }

        /**
         * @brief           startAncestors
         * param[in]        evt: transition's triggering event
         * param[in]        data: transition's data
         * param[in]        previousState: previous calling state
         * param[in,out]    propagateHistory: history propagation flag
         * @details         Starts the destination state and all its ancestors up to the outermost destination.
         *                  The starting order is descending from outermost to innermost and takes into account the history propagation flag
         */
        void startAncestors(const EventBase* evt, const TransitionData& data, StateBase* previousState, bool& propagateHistory)
        {
            // If we've reached the common ancestor, just leave out
            if (this == data.commonAncestor) return;

            // Move upwards until outermost destination is reached
            // Don't mutate the history propagation since this is the one from common ancestor
            this->m_parentState->startAncestors(evt, data, this, propagateHistory);

            bool propagate = Propagate(propagateHistory, m_parentRegion);

            // We've reached the outermost destination. If it's also the destination, then start it
            if (this == data.dst)
            {
                this->m_parentRegion->start(evt, propagate, this);
                return;
            }

            this->m_parentRegion->m_currentState = this;

            this->startImpl(evt, false, false);

            for (const auto&[_, region] : this->m_regions)
            {
                if (region != previousState->m_parentRegion) region->start(evt, propagate);
            }

            propagateHistory = propagate;
        }

        /**
         * @brief       getTransitionData
         * param[in]    dst: the destination state
         * @details     Calculates the transition data for transitions performed from the top state machine.
         *              In this case, the common ancestor is the first started state containing the destination,
         *              the source outermost is the current state of the common ancestor,
         *              and the destination outermost is the common ancestor's child
         * @return      A non null transition data structure if it could be computed, nullopt otherwise
         */
        std::optional<TransitionData> getTransitionData(StateBase* dst)
        {
            if (nullptr == this->m_parentState) return std::nullopt;
            if (false == this->m_parentState->m_started) return this->m_parentState->getTransitionData(dst);
            auto srcOutermost = this->m_parentRegion->m_currentState;
            return TransitionData{ this->m_parentState, srcOutermost, this, srcOutermost, dst };
        }

        /**
         * @brief       getTransitionData
         * param[in]    src: the source state
         * param[in]    dst: the destination state
         * @details     In this case, the common ancestor is the first state containing both source and destination states
         * @return      A non null transition data structure if it could be computed, nullopt otherwise
         */
        std::optional<TransitionData> getTransitionData(StateBase* src, StateBase* dst)
        {
            if (nullptr == this->m_parentState) return std::nullopt;
            auto srcOutermost = this->m_parentRegion->getOutermost(src, this);
            if (nullptr == srcOutermost) return this->m_parentState->getTransitionData(src, dst);
            return TransitionData{ this->m_parentState, srcOutermost, this, src, dst };
        }

        /**
         * @brief       processEventImpl
         * param[in]    evt: Event to process
         * @details     Processes the provided event:
         *              - Searches for a transition with index corresponding with the provided Event's type index
         *              - If such transition found, execute its callback using the provided event as parameter
         *              - If no transition found, recursively search inside sub-states
         */
        bool processEventImpl(const EventBase& evt, bool propagateHistory) const
        {
            // Loop over transitions
            auto it = m_transitions.find(evt.m_index);
            if (it != m_transitions.end())
            {
                // Invoke the corresponding callback and break recursive chain if successful
                if (true == evt.apply(it->second)) return true;
            }

            bool result{ false };

            // Recursive calls over sub-states if no transition found yet
            for (const auto&[_, region] : m_regions)
            {
                if (region->m_currentState != nullptr)
                {
                    result |= region->m_currentState->processEventImpl(evt, Propagate(propagateHistory, region));
                }
            }

            return result;
        }

        /**
         * @brief   checkStatesImpl
         * @details Terminating recursive variadic call
         */
        template <typename ...StateTypes, typename = details::is_empty_pack<StateTypes...>>
        bool checkStatesImpl(StateBase* previousState = nullptr)
        {
            return m_started;
        }

        /**
         * @brief   checkStatesImpl
         * @details Checks wether the provided state in template parameters are active (started)
         *          Order in which states are provided matters, and there cannot be "holes"
         */
        template <typename FirstStateType, typename ...OtherStateTypes>
        bool checkStatesImpl(StateBase* previousState = nullptr)
        {
            static_assert(details::is_state_v<FirstStateType>, "FirstStateType must inherit from State");

            auto state = this->template getDescendantImpl<FirstStateType>();
            if (nullptr == state) return false;
            if (state == previousState) return false;
            if (!state->m_started) return false;
            if (previousState != nullptr && state->m_parentState != previousState) return false;
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
        template <typename OtherDerivedType, typename OtherSmType>
        friend class State;

        template <typename OtherSmType, typename StoreType>
        friend class StateMachine;

    public:
        using Derived = DerivedType;

    protected:
        State()
            : StateBase{ details::Index<DerivedType>() }
        {
            static_assert(std::is_default_constructible_v<DerivedType>, "DerivedType must be default constructible");
            static_assert(std::is_base_of_v<State<DerivedType, SmType>, DerivedType>, "DerivedType must inherit from State");
            static_assert(details::is_state_machine_v<SmType>, "SmType must inherit from StateMachine");
            if constexpr (IsTopSm()) m_topSm = this;
        }

        virtual ~State() = default;

    private:
        /**
         * @brief   IsTopSm
         * @details Check whether the current state corresponds to top-sm or not
         * @return  true if current state is the top-sm, false otherwise
         */
        static constexpr bool IsTopSm()
        {
            if constexpr (details::is_same_state_v<DerivedType, SmType>) return true;
            else return false;
        }

        bool isProcessing() const
        {
            if (nullptr == m_topSm) return false;

            return topSm()->m_processing;
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
         * @brief       createStateImpl
         * param[in]    name: Child state's name
         * param[in]    entry: Flag indicating if the added child state shall be the entry point or not
         * @details     Creates a child state of type ChildState in the provided RegionIndex
         * @return      true if child state added, false otherwise
         */
        template <typename ChildState, int RegionIndex, typename EntryType>
        StateBase* createStateImpl(const std::string& name)
        {
            static_assert(details::is_entry_v<EntryType>, "EntryType must be either Entry or NoEntry");
            static_assert(details::is_state_v<ChildState>, "ChildState must inherit from State");

            // Don't allow same states at different places in a statemachine
            auto existingState = topSm()->template getDescendantImpl<ChildState>();
            if (existingState != nullptr)
            {
                throw details::SmError() << "Failed to create state '" << name << "'. It already exists as a child of '" << existingState->m_parentState->name() << "'";
            }

            // Create the new child state and initialize it
            auto child = new ChildState();
            if (child != nullptr)
            {
                child->m_name = name;
                child->m_regionIndex = RegionIndex;
                child->m_entry = EntryType::value;
                child->m_parentState = this;
                child->m_topSm = m_topSm;
            }
            else
            {
                throw details::SmError() << "Failed to create state '" << details::Name<ChildState>() << "'";
            }

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
        details::TransitionBase* createTransitionImpl()
        {
            static_assert(details::is_state_v<DstState>, "DstState must inherit from State");
            static_assert(details::is_state_v<SrcState>, "SrcState must inherit from State");
            static_assert(details::is_event_v<EventType>, "EventType must inherit from Event");

            // Retrieve and check the source state
            auto srcState = topSm()->template getDescendantImpl<SrcState>();
            if (nullptr == srcState)
            {
                throw details::SmError() << ErrorMessage<SrcState, EventType, DstState>() << "Source state '" << details::Name<SrcState>() << "' not found";
            }

            // Retrieve and check the destination state
            auto dstState = topSm()->template getDescendantImpl<DstState>();
            if (nullptr == dstState)
            {
                throw details::SmError() << ErrorMessage<SrcState, EventType, DstState>() << "Destination state '" << details::Name<DstState>() << "' not found";
            }

            // Retrieve and check the state where the action and guard shall take place
            auto actionState = topSm()->template getDescendantImpl<StateType>();
            if (nullptr == actionState)
            {
                throw details::SmError() << ErrorMessage<SrcState, EventType, DstState>() << "Action state '" << details::Name<StateType>() << "' not found";
            }

            // Check that the action state is a parent of source state, or source state itself
            if (false == actionState->contains(srcState))
            {
                throw details::SmError() << ErrorMessage<SrcState, EventType, DstState>() << "Action state '" << details::Name<StateType>() << "' is not an ancestor of source state '" << details::Name<SrcState>() << "' nor source state itself";
            }

            details::Transition<EventType>* transition{ nullptr };

            if constexpr (false == details::is_same_state_v<SrcState, DstState>)
            {
                auto transitionData = dstState->getTransitionData(srcState, dstState);
                if (std::nullopt == transitionData)
                {
                    throw details::SmError() << ErrorMessage<SrcState, EventType, DstState>() << "Transition impossible. Either crossing regions or source and destination are nested";
                }

                auto cb = [topSm = m_topSm, data = transitionData.value(), actionState, actionMember = action, guardMember = guard](const EventType& evt)
                {
                    try
                    {
                        if (guardMember != nullptr && false == std::invoke(guardMember, actionState, evt)) return false;
                        if (actionMember != nullptr) std::invoke(actionMember, actionState, evt);

                        return topSm->transitImpl(&evt, data, false);
                    }
                    catch (...)
                    {
                        // Transitions's error job
                        data.src->onError(std::current_exception());
                        return false;
                    }
                };

                transition = new details::Transition<EventType>{ cb };
            }
            else
            {
                auto cb = [srcState, actionState, actionMember = action, guardMember = guard](const EventType& evt)
                {
                    try
                    {
                        if (guardMember != nullptr && false == std::invoke(guardMember, actionState, evt)) return false;
                        if (actionMember != nullptr) std::invoke(actionMember, actionState, evt);

                        return true;
                    }
                    catch (...)
                    {
                        // Transitions's error job
                        srcState->onError(std::current_exception());
                        return false;
                    }
                };

                transition = new details::Transition<EventType>{ cb };
            }

            transition->m_srcState = srcState;

            return transition;
        }

        /**
         * @brief       ErrorMessage
         * @details     Helper to create an error message related to an impossible transition
         * @return      The error message
         */
        template <typename SrcState, typename EventType, typename DstState>
        static std::string ErrorMessage()
        {
            std::stringstream sstr;
            sstr << "Failed to create transition '" << details::Name<SrcState>() << "+" << details::Name<EventType>() << "=" << details::Name<DstState>() << "'. ";
            return sstr.str();
        }

    public:
        /**
         * @brief   trigEvent
         * @details Provides the triggering event if any
         * @return  Pointer to triggering event, or nullptr if none (start case)
         */
        template <typename EventType>
        const EventType* trigEvent() const
        {
            static_assert(details::is_event_v<EventType>, "EventType must inherit from Event");

            // Check triggering event pointer validity and type index match the provided EventType
            if (m_trigEvent != nullptr && true == details::CheckIndex<EventType>(m_trigEvent->m_index))
            {
                // Return the upcasted event
                return static_cast<const EventType*>(m_trigEvent);
            }

            return nullptr;
        }

        /**
         * @brief   clearHistory
         * @details Clears the last visited state history information for the provided state
         */
        template <typename StateType>
        void clearHistory(bool recursive = false)
        {
            auto state = this->template getDescendantImpl<StateType>();
            if (state != nullptr)
            {
                for (auto&[_, region] : state->m_regions)
                {
                    region->clearHistory(recursive);
                }
            }
        }

        /**
         * @brief   clearHistory
         * @details Clears the last visited state history information for the provided state an region
         */
        template <typename StateType, int RegionIndex>
        void clearHistory(bool recursive = false)
        {
            auto state = this->template getDescendantImpl<StateType>();
            if (state != nullptr)
            {
                auto itRegion = state->m_regions.find(RegionIndex);
                if (itRegion != state->m_regions.end())
                {
                    // Clear the last visited state
                    itRegion->second->clearHistory(recursive);
                }
                else
                {
                    LOG_ERROR_DSM("Failed to clear history on state '" << state->name() << "' and region " << RegionIndex << ". Region not found");
                }
            }
        }

        /**
         * @brief   checkStatesImpl
         * @details Terminating recursive variadic call
         */
        template <typename ...StateTypes, typename = details::is_empty_pack<StateTypes...>>
        bool checkStates()
        {
            return false;
        }

        /**
         * @brief   checkStatesImpl
         * @details Checks wether the provided state in template parameters are active (started)
         *          Order in which states are provided matters, and there cannot be "holes"
         */
        template <typename FirstStateType, typename ...OtherStateTypes>
        bool checkStates()
        {
            if (nullptr == m_topSm) return false;
            if constexpr (details::is_same_state_v<FirstStateType, SmType>)
                return m_topSm->template checkStatesImpl<OtherStateTypes...>(this->m_topSm);
            else
                return m_topSm->template checkStatesImpl<FirstStateType, OtherStateTypes...>();
        }

        /**
         * @brief   getAncestor
         * @details Recursively searches for the state or sub-state with the specified type StateType
         * @return  Pointer to the found state if any, nullptr otherwise
         */
        template <typename StateType>
        StateType* getAncestor()
        {
            return this->template getAncestorImpl<StateType>();
        }

        /**
         * @brief   store
         * @details Provides the state machine's local storage
         * @return  Reference to state machine's local storage
         */
        auto store()
        {
            return topSm() != nullptr ? topSm()->store() : nullptr;
        }

        /**
         * @brief   store
         * @details Provides the state machine's local storage
         * @return  Const reference to state machine's local storage
         */
        const auto store() const
        {
            return topSm() != nullptr ? topSm()->store() : nullptr;
        }

        /**
         * @brief       createState
         * param[in]    name: Child state's name
         * param[in]    entry: Flag indicating if the added child state shall be the entry point or not
         * @details     Creates a child state of type ChildType to the parent state in the provided RegionIndex. Forwarded to implementation
         * @return      Pointer to the created child state
         */
        template <typename ChildState, int RegionIndex, typename EntryType, details::is_state_def_v<ChildState, EntryType> = true>
        StateBase* createState(const std::string& name = details::Name<ChildState>())
        {
            try
            {
                // Forward to implementation
                return this->template createStateImpl<ChildState, RegionIndex, EntryType>(name);
            }
            catch (...)
            {
                // State's error job
                onError(std::current_exception());
            }

            return nullptr;
        }

        /**
         * @brief       createState
         * param[in]    name: Child state's name
         * param[in]    entry: Flag indicating if the added child state shall be the entry point or not
         * @details     Creates a child state of type ChildType to the parent state in the provided RegionIndex. Forwarded to implementation
         * @return      Pointer to the created child state
         */
        template <typename ChildState, typename EntryType, details::is_state_def_v<ChildState, EntryType> = true>
        StateBase* createState(const std::string& name = details::Name<ChildState>())
        {
            try
            {
                // Forward to implementation
                return this->template createStateImpl<ChildState, 0, EntryType>(name);
            }
            catch (...)
            {
                // State's error job
                onError(std::current_exception());
            }

            return nullptr;
        }

        /**
         * @brief       createState
         * param[in]    name: Child state's name
         * param[in]    entry: Flag indicating if the added child state shall be the entry point or not
         * @details     Creates a child state of type ChildType to the parent state in the provided RegionIndex. Forwarded to implementation
         * @return      Pointer to the created child state
         */
        template <typename ChildState, int RegionIndex = 0, details::is_state_def_v<ChildState> = true>
        StateBase* createState(const std::string& name = details::Name<ChildState>())
        {
            try
            {
                // Forward to implementation
                return this->template createStateImpl<ChildState, RegionIndex, NoEntry>(name);
            }
            catch (...)
            {
                // State's error job
                onError(std::current_exception());
            }

            return nullptr;
        }

        /**
         * @brief       createTransition
         * param[in]    guard: function object that may be used to discard the execution of the transition
         * param[in]    action: function object that is called prior to the transition
         * @details     Creates a transition to the provided DstState with the provided guard and action. Forwarded to implementation
         * @return      Pointer to the created transition
         */
        template <typename EventType, typename StateType, TAction<StateType, EventType> action, TGuard<StateType, EventType> guard, typename DstState = DerivedType>
        details::TransitionBase* createTransition()
        {
            static_assert(action != nullptr && guard != nullptr, "Action and guard cannot be null. Use another template overload");

            try
            {
                // Forward to implementation
                return createTransitionImpl<DerivedType, EventType, StateType, action, guard, DstState>();
            }
            catch (...)
            {
                // State's error job
                onError(std::current_exception());
            }

            return nullptr;
        }

        /**
         * @brief       createTransition
         * param[in]    action: function object that is called prior to the transition
         * @details     Creates an external transition to the provided DstState with the provided action. Forwarded to implementation
         * @return      Pointer to the created transition
         */
        template <typename EventType, typename StateType, TAction<StateType, EventType> action, typename DstState>
        details::TransitionBase* createTransition()
        {
            static_assert(action != nullptr, "Action cannot be null. Use another template overload");
            static_assert(!details::is_same_state_v<DerivedType, DstState>, "Source and Destination cannot be identical. Use another template overload");

            try
            {
                // Forward to implementation
                return createTransitionImpl<DerivedType, EventType, StateType, action, nullptr, DstState>();
            }
            catch (...)
            {
                // State's error job
                onError(std::current_exception());
            }

            return nullptr;
        }

        /**
         * @brief       createTransition
         * param[in]    action: function object that is called prior to the transition
         * @details     Creates an inner transition with the provided action. Forwarded to implementation
         * @return      Pointer to the created transition
         */
        template <typename EventType, typename StateType, TAction<StateType, EventType> action>
        details::TransitionBase* createTransition()
        {
            static_assert(action != nullptr, "Action cannot be null. Use another template overload");

            try
            {
                // Forward to implementation
                return createTransitionImpl<DerivedType, EventType, StateType, action, nullptr, DerivedType>();
            }
            catch (...)
            {
                // State's error job
                onError(std::current_exception());
            }

            return nullptr;
        }

        /**
         * @brief       createTransition
         * param[in]    guard: function object that may be used to discard the execution of the transition
         * @details     Creates an external transition to the provided DstState with the provided guard. Forwarded to implementation
         * @return      Pointer to the created transition
         */
        template <typename EventType, typename StateType, TGuard<StateType, EventType> guard, typename DstState>
        details::TransitionBase* createTransition()
        {
            static_assert(guard != nullptr, "Guard cannot be null. Use another template overload");
            static_assert(!details::is_same_state_v<DerivedType, DstState>, "Source and Destination cannot be identical. Use another template overload");

            try
            {
                // Forward to implementation
                return createTransitionImpl<DerivedType, EventType, StateType, nullptr, guard, DstState>();
            }
            catch (...)
            {
                // State's error job
                onError(std::current_exception());
            }

            return nullptr;
        }

        /**
         * @brief       createTransition
         * @details     Creates an external transition to the provided DstState. Forwarded to implementation
         * @return      Pointer to the created transition
         */
        template <typename EventType, typename DstState>
        details::TransitionBase* createTransition()
        {
            static_assert(!details::is_same_state_v<DerivedType, DstState>, "Source and Destination cannot be identical. Use another template overload");

            try
            {
                // Forward to implementation
                return createTransitionImpl<DerivedType, EventType, DerivedType, nullptr, nullptr, DstState>();
            }
            catch (...)
            {
                // State's error job
                onError(std::current_exception());
            }

            return nullptr;
        }

        void preProcess() const
        {
            if (nullptr == m_topSm) return;
            topSm()->m_processing = true;
        }

        void postProcess() const
        {
            if (nullptr == m_topSm) return;
            topSm()->m_processing = false;
        }

        /**
         * @brief   transit
         * @details Performs the anonymous transition to the provided DstState
         */
        template <typename DstState>
        void transit()
        {
            static_assert(details::is_state_v<DstState>, "DstState must inherit from State");

            if (nullptr == m_topSm) return;
            if (false == topSm()->started()) return;

            // Retrieve and check the destination state
            auto dstState = topSm()->template getDescendantImpl<DstState>();
            if (nullptr == dstState) return;
            if (true == dstState->m_started) return;

            // Retrieve and check the transition data
            auto transitionData = (this == m_topSm) ? dstState->getTransitionData(dstState) : dstState->getTransitionData(this, dstState);
            if (std::nullopt == transitionData) return;

            auto cb = [topSm = m_topSm, data = transitionData.value()]() -> bool
            {
                return topSm->transitImpl(nullptr, data, false);
            };

            if (false == isProcessing())
            {
                preProcess();
                cb();
                postProcess();
            }
            else
            {
                topSm()->m_postedTransitions.emplace(new details::Transition<void>(cb));
            }
        }

        /**
         * @brief   transit
         * @details Performs the event triggered transition to the provided DstState
         */
        template <typename DstState>
        void transit(const EventBase& evt)
        {
            static_assert(details::is_state_v<DstState>, "DstState must inherit from State");

            if (nullptr == m_topSm) return;
            if (false == topSm()->started()) return;

            // Retrieve and check the destination state
            auto dstState = topSm()->template getDescendantImpl<DstState>();
            if (nullptr == dstState) return;
            if (true == dstState->m_started) return;

            // Retrieve and check the transition data
            auto transitionData = (this == m_topSm) ? dstState->getTransitionData(dstState) : dstState->getTransitionData(this, dstState);
            if (std::nullopt == transitionData) return;

            auto cb = [evt = evt.clone(), topSm = m_topSm, data = transitionData.value()]() -> bool
            {
                return topSm->transitImpl(evt, data, false);
            };

            if (false == isProcessing())
            {
                preProcess();
                cb();
                postProcess();
            }
            else
            {
                topSm()->m_postedTransitions.emplace(new details::Transition<void>(cb));
            }
        }

        /**
         * @brief       deferEvent
         * param[in]    evt: Event to defer
         * @details     Defers the provided event. The event shall be processed as soon as the state machine enters back into the state that handles this kind of event
         */
        template <typename EventType>
        void deferEvent(const EventType& evt) const
        {
            static_assert(details::is_event_v<EventType>, "EventType must inherit from Event");

            if (nullptr == m_topSm) return;
            if (false == topSm()->started()) return;

            if (false == isProcessing())
            {
                preProcess();
                if (false == processEventImpl(evt, false)) topSm()->m_postedTransitions.emplace(evt.clone(), true);
                postProcess();
            }
            else
            {
                topSm()->m_postedTransitions.emplace(evt.clone(), true);
            }
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
            static_assert(details::is_event_v<EventType>, "EventType must inherit from Event");

            if (nullptr == m_topSm) return;
            if (false == topSm()->started()) return;

            // If post from top-sm, it is equivalent to processEvent
            if (false == isProcessing())
            {
                preProcess();
                processEventImpl(evt, false);
                postProcess();
            }
            else
            {
                topSm()->m_postedTransitions.emplace(evt.clone());
            }
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
        static_assert(std::is_default_constructible_v<StoreType>, "StoreType must be default constructible");

        template <typename DerivedType, typename OtherSmType>
        friend class State;

        /**
         * @brief   m_store
         * @details State machine's local storage that is shared among all sub-states to access common data
         */
        StoreType* m_store = nullptr;

        mutable bool m_processing = false;

        mutable std::deque<details::PostedTransition> m_pendingTransitions;
        mutable std::queue<details::PostedTransition> m_postedTransitions;

        /**
         * @brief       Derived
         * @details     Returns the state machine's derived pointer type
         */
        SmType& derived()
        {
            return static_cast<SmType&>(*this);
        }

        /**
         * @brief       Derived
         * @details     Returns the state machine's derived pointer type
         */
        const SmType& derived() const
        {
            return static_cast<const SmType&>(*this);
        }

    protected:
        StateMachine(const std::string& name = details::Name<SmType>())
            : State<SmType, SmType>{}
            , m_store{ new StoreType() }
        {
            this->m_name = name;
        }

        virtual ~StateMachine()
        {
            clear();
            stop();

            m_pendingTransitions.clear();
            while (false == m_postedTransitions.empty())
            {
                m_postedTransitions.pop();
            }

            delete m_store;
            m_store = nullptr;
        }

    public:
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

        /**
         * @brief           visit
         * @param[in,out]   visitor: applied visitor
         * @details         Visits the current state and sub-states
         */
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

                this->setupImpl();
            }
            catch (...)
            {
                // State's error job
                derived().onError(std::current_exception());
            }
        }

        /**
         * @brief   tearDown
         * @details Clears the statemachine: removes all states and transitions
         */
        void clear()
        {
            if (this->started()) return;

            this->clearImpl();
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
                this->startImpl(nullptr, false);
            }
            catch (...)
            {
                // State's error job
                derived().onError(std::current_exception());
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
         * @details     Adds a child state of type ChildState to the provided ParentState in the provided RegionIndex
         */
        template <typename ChildState, int RegionIndex, typename EntryType, details::is_state_def_v<ChildState, EntryType> = true>
        void addState(const std::string& name = details::Name<ChildState>())
        {
            static_assert(details::is_state_v<ChildState>, "ChildState must inherit from State");
            static_assert(details::is_entry_v<EntryType>, "EntryType must be either Entry or NoEntry");

            if (this->started()) return;

            StateBase* childState{ nullptr };

            try
            {
                childState = this->template createStateImpl<ChildState, RegionIndex, EntryType>(name);
                childState->addState(this);
            }
            catch (...)
            {
                delete childState;
                childState = nullptr;

                // State's error job
                derived().onError(std::current_exception());
            }
        }

        /**
         * @brief       addState
         * param[in]    name: Child state's name
         * param[in]    entry: Flag indicating if the added child state shall be the entry point or not
         * @details     Adds a child state of type ChildState to the provided ParentState in the provided RegionIndex
         */
        template <typename ChildState, typename EntryType, details::is_state_def_v<ChildState, EntryType> = true>
        void addState(const std::string& name = details::Name<ChildState>())
        {
            static_assert(details::is_state_v<ChildState>, "ChildState must inherit from State");
            static_assert(details::is_entry_v<EntryType>, "EntryType must be either Entry or NoEntry");

            if (this->started()) return;

            StateBase* childState{ nullptr };

            try
            {
                childState = this->template createStateImpl<ChildState, 0, EntryType>(name);
                childState->addState(this);
            }
            catch (...)
            {
                delete childState;
                childState = nullptr;

                // State's error job
                derived().onError(std::current_exception());
            }
        }

        /**
         * @brief       addState
         * param[in]    name: Child state's name
         * param[in]    entry: Flag indicating if the added child state shall be the entry point or not
         * @details     Adds a child state of type ChildState to the provided ParentState in the provided RegionIndex
         */
        template <typename ChildState, int RegionIndex = 0, details::is_state_def_v<ChildState> = true>
        void addState(const std::string& name = details::Name<ChildState>())
        {
            static_assert(details::is_state_v<ChildState>, "ChildState must inherit from State");

            if (this->started()) return;

            StateBase* childState{ nullptr };

            try
            {
                childState = this->template createStateImpl<ChildState, RegionIndex, NoEntry>(name);
                childState->addState(this);
            }
            catch (...)
            {
                delete childState;
                childState = nullptr;

                // State's error job
                derived().onError(std::current_exception());
            }
        }

        /**
         * @brief       addState
         * param[in]    name: Child state's name
         * param[in]    entry: Flag indicating if the added child state shall be the entry point or not
         * @details     Adds a child state of type ChildState to the provided ParentState in the provided RegionIndex
         */
        template <typename ParentState, typename ChildState, int RegionIndex, typename EntryType, details::is_state_def_v<ChildState, EntryType> = true>
        void addState(const std::string& name = details::Name<ChildState>())
        {
            static_assert(details::is_state_v<ParentState>, "ParentState must inherit from State");
            static_assert(details::is_state_v<ChildState>, "ChildState must inherit from State");
            static_assert(details::is_entry_v<EntryType>, "EntryType must be either Entry or NoEntry");

            if (this->started()) return;

            StateBase* childState{ nullptr };

            try
            {
                auto parentState = this->template getDescendantImpl<ParentState>();
                if (nullptr == parentState)
                {
                    throw details::SmError() << "Failed to add state '" << details::Name<ChildState>() << "'. Parent state '" << details::Name<ParentState>() << "' not found";
                }

                childState = parentState->template createStateImpl<ChildState, RegionIndex, EntryType>(name);
                childState->addState(parentState);
            }
            catch (...)
            {
                delete childState;
                childState = nullptr;

                // State's error job
                derived().onError(std::current_exception());
            }
        }

        /**
         * @brief       addState
         * param[in]    name: Child state's name
         * param[in]    entry: Flag indicating if the added child state shall be the entry point or not
         * @details     Adds a child state of type ChildState to the provided ParentState in the provided RegionIndex
         */
        template <typename ParentState, typename ChildState, typename EntryType, details::is_state_def_v<ChildState, EntryType> = true>
        void addState(const std::string& name = details::Name<ChildState>())
        {
            static_assert(details::is_state_v<ParentState>, "ParentState must inherit from State");
            static_assert(details::is_state_v<ChildState>, "ChildState must inherit from State");
            static_assert(details::is_entry_v<EntryType>, "EntryType must be either Entry or NoEntry");

            if (this->started()) return;

            StateBase* childState{ nullptr };

            try
            {
                auto parentState = this->template getDescendantImpl<ParentState>();
                if (nullptr == parentState)
                {
                    throw details::SmError() << "Failed to add state '" << details::Name<ChildState>() << "'. Parent state '" << details::Name<ParentState>() << "' not found";
                }

                childState = parentState->template createStateImpl<ChildState, 0, EntryType>(name);
                childState->addState(parentState);
            }
            catch (...)
            {
                delete childState;
                childState = nullptr;

                // State's error job
                derived().onError(std::current_exception());
            }
        }

        /**
         * @brief       addState
         * param[in]    name: Child state's name
         * param[in]    entry: Flag indicating if the added child state shall be the entry point or not
         * @details     Adds a child state of type ChildState to the provided ParentState in the provided RegionIndex
         */
        template <typename ParentState, typename ChildState, int RegionIndex = 0, details::is_state_def_v<ChildState> = true>
        void addState(const std::string& name = details::Name<ChildState>())
        {
            static_assert(details::is_state_v<ParentState>, "ParentState must inherit from State");
            static_assert(details::is_state_v<ChildState>, "ChildState must inherit from State");

            if (this->started()) return;

            StateBase* childState{ nullptr };

            try
            {
                auto parentState = this->template getDescendantImpl<ParentState>();
                if (nullptr == parentState)
                {
                    throw details::SmError() << "Failed to add state '" << details::Name<ChildState>() << "'. Parent state '" << details::Name<ParentState>() << "' not found";
                }

                childState = parentState->template createStateImpl<ChildState, RegionIndex, NoEntry>(name);
                childState->addState(parentState);
            }
            catch (...)
            {
                delete childState;
                childState = nullptr;

                // State's error job
                derived().onError(std::current_exception());
            }
        }

        /**
         * @brief       setHistory
         * param[in]    history: history type
         * @details     Sets the history to the provided StateType
         */
        template <typename StateType>
        void setHistory(History history)
        {
            if (this->started()) return;

            auto state = this->template getDescendantImpl<StateType>();
            if (nullptr == state) return;

            for (auto&[_, region] : state->m_regions)
            {
                region->setHistory(history);
            }
        }

        /**
         * @brief       setHistory
         * param[in]    history: history type
         * @details     Sets the history to the provided StateType and region
         */
        template <typename StateType, int RegionIndex>
        void setHistory(History history)
        {
            if (this->started()) return;

            auto state = this->template getDescendantImpl<StateType>();
            if (nullptr == state) return;

            auto itRegion = state->m_regions.find(RegionIndex);
            if (itRegion != state->m_regions.end())
            {
                itRegion->second->setHistory(history);
            }
            else
            {
                LOG_ERROR_DSM("Failed to set history on state '" << state->name() << "' and region " << RegionIndex << ". Region not found");
            }
        }

        /**
         * @brief     getHistory
         * @details   Gets the history to the provided StateType and region
         */
        template <typename StateType, int RegionIndex>
        THistory getHistory()
        {
            auto state = this->template getDescendantImpl<StateType>();
            if (nullptr == state) return std::nullopt;

            auto itRegion = state->m_regions.find(RegionIndex);
            if (itRegion != state->m_regions.end())
            {
                return itRegion->second->m_history;
            }
            else
            {
                return std::nullopt;
            }
        }

        /**
         * @brief       resetHistory
         * @details     Resets the history of the provided StateType
         */
        template <typename StateType>
        void resetHistory(bool recursive = false)
        {
            if (this->started()) return;

            auto state = this->template getDescendantImpl<StateType>();
            if (nullptr == state) return;

            for (auto&[_, region] : state->m_regions)
            {
                region->resetHistory(recursive);
            }
        }

        /**
         * @brief       resetHistory
         * @details     Resets the history of the provided StateType and region
         */
        template <typename StateType, int RegionIndex>
        void resetHistory(bool recursive = false)
        {
            if (this->started()) return;

            auto state = this->template getDescendantImpl<StateType>();
            if (nullptr == state) return;

            auto itRegion = state->m_regions.find(RegionIndex);
            if (itRegion != state->m_regions.end())
            {
                itRegion->second->resetHistory(recursive);
            }
            else
            {
                LOG_ERROR_DSM("Failed to reset history on state '" << state->name() << "' and region " << RegionIndex << ". Region not found");
            }
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
            static_assert(action != nullptr && guard != nullptr, "Action and guard cannot be null. Use another template overload");
            if (this->started()) return;

            try
            {
                auto transition = this->template createTransitionImpl<SrcState, EventType, StateType, action, guard, DstState>();
                StateBase::addTransition(transition);
            }
            catch (...)
            {
                // State's error job
                derived().onError(std::current_exception());
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
            static_assert(action != nullptr, "Action cannot be null. Use another template overload");
            static_assert(!details::is_same_state_v<SrcState, DstState>, "Source and Destination cannot be identical. Use another template overload");

            if (this->started()) return;

            try
            {
                auto transition = this->template createTransitionImpl<SrcState, EventType, StateType, action, nullptr, DstState>();
                StateBase::addTransition(transition);
            }
            catch (...)
            {
                // State's error job
                derived().onError(std::current_exception());
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
            static_assert(action != nullptr, "Action cannot be null. Use another template overload");

            if (this->started()) return;

            try
            {
                auto transition = this->template createTransitionImpl<SrcState, EventType, StateType, action, nullptr, SrcState>();
                StateBase::addTransition(transition);
            }
            catch (...)
            {
                // State's error job
                derived().onError(std::current_exception());
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
            static_assert(guard != nullptr, "Guard cannot be null. Use another template overload");
            static_assert(!details::is_same_state_v<SrcState, DstState>, "Source and Destination cannot be identical. Use another template overload");

            if (this->started()) return;

            try
            {
                auto transition = this->template createTransitionImpl<SrcState, EventType, StateType, nullptr, guard, DstState>();
                StateBase::addTransition(transition);
            }
            catch (...)
            {
                // State's error job
                derived().onError(std::current_exception());
            }
        }

        /**
         * @brief       addTransition
         * @details     Adds an external transition to the provided DstState
         */
        template <typename SrcState, typename EventType, typename DstState>
        void addTransition()
        {
            static_assert(!details::is_same_state_v<SrcState, DstState>, "Source and Destination cannot be identical. Use another template overload");

            if (this->started()) return;

            try
            {
                auto transition = this->template createTransitionImpl<SrcState, EventType, SrcState, nullptr, nullptr, DstState>();
                StateBase::addTransition(transition);
            }
            catch (...)
            {
                // State's error job
                derived().onError(std::current_exception());
            }
        }

        /**
         * @brief   processEvent
         * @details Processes the provided event. Forwarded to implementation
         */
        template <typename EventType>
        void processEvent(const EventType& evt) const
        {
            static_assert(details::is_event_v<EventType>, "EventType must inherit from Event");

            if (!this->started()) return;

            this->preProcess();

            // Forward to implementation
            this->processEventImpl(evt, false);

            do
            {
                // Transfer elements from posted to pending
                while (false == m_postedTransitions.empty())
                {
                    m_pendingTransitions.push_back(std::move(m_postedTransitions.front()));
                    m_postedTransitions.pop();
                }

                // Loop over pending transitions and executed the corresponding callbacks
                auto it = m_pendingTransitions.begin();
                while (it != m_pendingTransitions.end())
                {
                    details::PostedTransition& pending = *it;
                    if (nullptr == pending.m_transition) // Posted or deferred events
                    {
                        bool result = this->processEventImpl(*pending.m_evt, false);
                        if (false == pending.m_deferred || true == result)
                        {
                            it = m_pendingTransitions.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                    else // User transition
                    {
                        static_cast<details::Transition<void>*>(pending.m_transition)->exec();
                        it = m_pendingTransitions.erase(it);
                    }
                }
            }
            // Repeat until no more posted transition
            while (false == m_postedTransitions.empty());

            this->postProcess();
        }

        /**
         * @brief   getState
         * @details Recursively searches for the state or sub-state with the specified type StateType
         * @return  Pointer to the found state if any, nullptr otherwise
         */
        template <typename StateType>
        StateType* getState()
        {
            return this->template getDescendantImpl<StateType>();
        }
    };
}

#endif
