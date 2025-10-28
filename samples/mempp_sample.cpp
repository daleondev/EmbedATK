#include "Mempp/Mempp.h"

#include <iostream>

class Idle : public IState
{
public:
    virtual void onEntry() override { std::cout << "entry idle" << std::endl; }
    virtual void onActive() override { std::cout << "active idle" << std::endl; }
    virtual void onExit() override { std::cout << "exit idle" << std::endl; }
};

class Active : public IState
{
public:
    virtual void onEntry() override { std::cout << "entry active" << std::endl; }
    virtual void onActive() override { std::cout << "active active" << std::endl; }
    virtual void onExit() override { std::cout << "exit active" << std::endl; }
};

void cb()
{

}

int main()
{
    // constexpr auto myLambda = [](int i) {  };
    // std::optional<decltype(myLambda)> emptyOpt = std::nullopt;

    enum class Defs { Idle, Active };
    enum class Events { Btn1, Btn2, Btn3 };
    using Impls = States<Idle, Active>;
    using Transitions = StateTransitions<
        StateTransition<Defs::Idle, Events::Btn1, Defs::Active, []() { std::cout << "transition 1" << std::endl; }>,
        StateTransition<Defs::Active, Events::Btn2, Defs::Idle, []() { std::cout << "transition 2" << std::endl; }>
    >;

    StateMachine<Defs, Impls, Events, Transitions, Defs::Idle, false> statemachine;

    size_t i = 0;
    while(true) {
        statemachine.update();
        if (i++ == 10) {
            statemachine.sendEvent(Events::Btn1);
        }
        if (i == 20){
            statemachine.sendEvent(Events::Btn2);
        }
        if (i == 30){
            break;
        }
    }

    return 0;
}