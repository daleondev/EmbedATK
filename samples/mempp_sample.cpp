#include "Mempp/Mempp.h"

#include <iostream>

class Idle : public IState
{
public:
    virtual void onEntry() override {}
    virtual void onActive() override {}
    virtual void onExit() override {}
};

class Active : public IState
{
public:
    virtual void onEntry() override {}
    virtual void onActive() override {}
    virtual void onExit() override {}
};

void cb()
{

}

int main()
{
    constexpr auto myLambda = []() {  };
    enum class Defs { Idle, Active };
    enum class Events { Btn1, Btn2, Btn3 };
    using Impls = States<Idle, Active>;
    using Transitions = StateTransitions<
        StateTransition<Defs::Idle, Events::Btn1, Defs::Active>,
        StateTransition<Defs::Active, Events::Btn2, Defs::Idle>
    >;

    // StateMachine<Defs, Impls, Events, Transitions> statemachine;

    

    return 0;
}