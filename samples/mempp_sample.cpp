#include "Mempp/Mempp.h"

#include <iostream>

enum class TestState { TestIdle, TestActive, TestRunning, TestPaused };
enum class TestEvent { Btn1, Btn2 };

class TestIdle : public IState<TestState>
{
public:
    virtual void onEntry() override { std::cout << "entry Idle" << std::endl; }
    virtual void onActive() override { std::cout << "active Idle" << std::endl; }
    virtual void onExit() override { std::cout << "exit Idle" << std::endl; }
};

class TestActive : public IState<TestState>
{
public:
    virtual void onEntry() override { std::cout << "entry Active" << std::endl; }
    virtual void onActive() override { std::cout << "active Active" << std::endl; }
    virtual void onExit() override { std::cout << "exit Active" << std::endl; }
};

class TestRunning : public IState<TestState>
{
public:
    virtual void onEntry() override { std::cout << "entry Running" << std::endl; }
    virtual void onActive() override { std::cout << "active Running" << std::endl; }
    virtual void onExit() override { std::cout << "exit Running" << std::endl; }
};

class TestPaused : public IState<TestState>
{
public:
    virtual void onEntry() override { std::cout << "entry Paused" << std::endl; }
    virtual void onActive() override { std::cout << "active Paused" << std::endl; }
    virtual void onExit() override { std::cout << "exit Paused" << std::endl; }
};

int main()
{
    constexpr auto traceTransition = [](TestState from, TestEvent trig, TestState to) 
    { 
        std::cout << "transition from: " << magic_enum::enum_name(from);
        std::cout << " to: " << magic_enum::enum_name(to);
        std::cout << ", trigger: " << magic_enum::enum_name(trig) << std::endl;
    };

    using TestIdleState = State<TestState::TestIdle, TestIdle>;
    using TestActiveState = State<TestState::TestActive, TestActive>;
    using TestRunningState = State<TestState::TestRunning, TestRunning>;
    using TestPausedState = State<TestState::TestPaused, TestPaused>;

    using TestStates = States<
        TestIdleState,
        TestActiveState,
        TestRunningState,
        TestPausedState
    >;

    using TestTransitions = StateTransitions<
        StateTransition<TestIdleState, TestEvent::Btn1, TestActiveState, traceTransition>,
        StateTransition<TestActiveState, TestEvent::Btn2, TestIdleState, traceTransition>
    >;

    using StateHierarchy = std::tuple<
         SubstateGroup<TestActiveState, TestRunningState, TestPausedState>
    >;

    StateMachine<TestStates, TestEvent, TestTransitions, StateHierarchy> testStateMachine;

    size_t i = 0;
    while(true) {
        testStateMachine.update();
        if (i++ == 10) {
            testStateMachine.sendEvent(TestEvent::Btn1);
        }
        if (i == 20){
            testStateMachine.sendEvent(TestEvent::Btn2);
        }
        if (i == 30){
            break;
        }
    }

    return 0;
}