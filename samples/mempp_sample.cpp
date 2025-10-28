#include "Mempp/Mempp.h"

#include <iostream>

class Idle : public IState
{
public:
    virtual void onEntry() override { std::cout << "entry Idle" << std::endl; }
    virtual void onActive() override { std::cout << "active Idle" << std::endl; }
    virtual void onExit() override { std::cout << "exit Idle" << std::endl; }
};

class Active : public IState
{
public:
    virtual void onEntry() override { std::cout << "entry Active" << std::endl; }
    virtual void onActive() override { std::cout << "active Active" << std::endl; }
    virtual void onExit() override { std::cout << "exit Active" << std::endl; }
};

void cb()
{

}

int main()
{
    enum class Defs { Idle, Active };
    enum class Events { Btn1, Btn2, Btn3 };
    using Impls = States<Idle, Active>;

    constexpr auto traceTransition = [](Defs from, Events trig, Defs to) 
    { 
        std::cout << "transition from: " << magic_enum::enum_name(from);
        std::cout << " to: " << magic_enum::enum_name(to);
        std::cout << ", trigger: " << magic_enum::enum_name(trig) << std::endl;
    };
    using Transitions = StateTransitions<
        StateTransition<Defs::Idle, Events::Btn1, Defs::Active, traceTransition>,
        StateTransition<Defs::Active, Events::Btn2, Defs::Idle, traceTransition>
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