#include "Mempp/Mempp.h"

#include <iostream>
#include <thread>

enum class RobotState {
    // Top Level/Super States (Parents)
    OperationalMode,
    EmergencyStop,

    // Sub States (Children of OperationalMode)
    IdleWaiting,
    AssemblyProcess,

    // Sub States (Children of AssemblyProcess)
    LoadComponent,
    WeldJoint,
    InspectQuality,
    UnloadProduct,
    
    // Sub States (Children of EmergencyStop)
    ErrorLogged,
    ManualOverride
};

enum class RobotEvent {
    Start_Cycle,
    Component_Loaded,
    Weld_Complete,
    Inspection_Pass,
    Inspection_Fail,
    Product_Unloaded,
    Trigger_EStop,
    Reset_Command
};

class OperationalMode : public IState<RobotState>
{
public:
    virtual void onEntry() override { std::cout << "entry OperationalMode: Robot is ready for work." << std::endl; }
    virtual void onActive() override { /* Monitor system health */ }
    virtual void onExit() override { std::cout << "exit OperationalMode: System Shutting Down or Emergency." << std::endl; }
};

class EmergencyStop : public IState<RobotState>
{
public:
    virtual void onEntry() override { std::cout << "entry EmergencyStop: All motion IMMEDIATELY ceased." << std::endl; }
    virtual void onActive() override { /* Flash error light/sound alarm */ }
    virtual void onExit() override { std::cout << "exit EmergencyStop: Safety checks completed." << std::endl; }
};

class IdleWaiting : public IState<RobotState>
{
public:
    virtual void onEntry() override { std::cout << "entry IdleWaiting." << std::endl; }
    virtual void onActive() override { /* Flash error light/sound alarm */ }
    virtual void onExit() override { std::cout << "exit IdleWaiting." << std::endl; }
};

class AssemblyProcess : public IState<RobotState>
{
public:
    virtual void onEntry() override { std::cout << "entry AssemblyProcess: Starting new build sequence." << std::endl; }
    virtual void onActive() override { /* Monitor cycle time */ }
    virtual void onExit() override { std::cout << "exit AssemblyProcess: Build sequence complete/aborted." << std::endl; }
};

class LoadComponent : public IState<RobotState>
{
public:
    virtual void onEntry() override { std::cout << "entry LoadComponent: Moving arm to pick-up position." << std::endl; }
    virtual void onActive() override { /* Check sensor for component presence */ }
    virtual void onExit() override { std::cout << "exit LoadComponent: Component secured." << std::endl; }
};

class WeldJoint : public IState<RobotState>
{
public:
    virtual void onEntry() override { std::cout << "entry WeldJoint." << std::endl; }
    virtual void onActive() override { /* Check sensor for component presence */ }
    virtual void onExit() override { std::cout << "exit WeldJoint." << std::endl; }
};

class InspectQuality : public IState<RobotState>
{
public:
    virtual void onEntry() override { std::cout << "entry InspectQuality: Running vision system check." << std::endl; }
    virtual void onActive() override { /* Process image data */ }
    virtual void onExit() override { std::cout << "exit InspectQuality" << std::endl; }
};

class UnloadProduct : public IState<RobotState>
{
public:
    virtual void onEntry() override { std::cout << "entry UnloadProduct." << std::endl; }
    virtual void onActive() override { /* Check sensor for component presence */ }
    virtual void onExit() override { std::cout << "exit UnloadProduct." << std::endl; }
};

class ErrorLogged : public IState<RobotState>
{
public:
    virtual void onEntry() override { std::cout << "entry ErrorLogged." << std::endl; }
    virtual void onActive() override { /* Check sensor for component presence */ }
    virtual void onExit() override { std::cout << "exit ErrorLogged." << std::endl; }
};

class ManualOverride : public IState<RobotState>
{
public:
    virtual void onEntry() override { std::cout << "entry ManualOverride." << std::endl; }
    virtual void onActive() override { /* Check sensor for component presence */ }
    virtual void onExit() override { std::cout << "exit ManualOverride." << std::endl; }
};

// --- Type Aliases ---
using OperationalModeState  = State<RobotState::OperationalMode, OperationalMode>;
using EmergencyStopState    = State<RobotState::EmergencyStop, EmergencyStop>;
using IdleWaitingState      = State<RobotState::IdleWaiting, IdleWaiting>;
using AssemblyProcessState  = State<RobotState::AssemblyProcess, AssemblyProcess>;
using LoadComponentState    = State<RobotState::LoadComponent, LoadComponent>;
using WeldJointState        = State<RobotState::WeldJoint, WeldJoint>;
using InspectQualityState   = State<RobotState::InspectQuality, InspectQuality>;
using UnloadProductState    = State<RobotState::UnloadProduct, UnloadProduct>;
using ErrorLoggedState      = State<RobotState::ErrorLogged, ErrorLogged>;
using ManualOverrideState   = State<RobotState::ManualOverride, ManualOverride>;

// --- States Container ---
using RobotStates = States<
    OperationalModeState,
    EmergencyStopState,
    IdleWaitingState,
    AssemblyProcessState,
    LoadComponentState,
    WeldJointState,
    InspectQualityState,
    UnloadProductState,
    ErrorLoggedState,
    ManualOverrideState
>;

// --- Transitions (The Logic) ---
using RobotTransitions = StateTransitions<
    // 1. Operational Cycle Start/Stop (Intra-OperationalMode)
    StateTransition<IdleWaitingState, RobotEvent::Start_Cycle, LoadComponentState>, // Note: Jumps into sub-state of AssemblyProcess
    
    // 2. Assembly Sequence (Intra-AssemblyProcess)
    StateTransition<LoadComponentState, RobotEvent::Component_Loaded, WeldJointState>,
    StateTransition<WeldJointState, RobotEvent::Weld_Complete, InspectQualityState>,
    
    // 3. Conditional Branching
    StateTransition<InspectQualityState, RobotEvent::Inspection_Pass, UnloadProductState>,
    StateTransition<InspectQualityState, RobotEvent::Inspection_Fail, IdleWaitingState>, // Fail -> Rejects part, goes back to Idle
    
    // 4. Cycle End
    StateTransition<UnloadProductState, RobotEvent::Product_Unloaded, IdleWaitingState>,
    
    // 5. Emergency Transitions (Super-State Logic)
    // From ANY state in OperationalMode to ErrorLogged (EmergencyStop's default sub-state)
    StateTransition<OperationalModeState, RobotEvent::Trigger_EStop, ErrorLoggedState>,
    
    // 6. Reset/Recovery Transitions (Intra-EmergencyStop/Back to Operational)
    StateTransition<ErrorLoggedState, RobotEvent::Reset_Command, ManualOverrideState>,
    StateTransition<ManualOverrideState, RobotEvent::Reset_Command, IdleWaitingState> // Only transition back to a safe, known state
>;

using RobotHierarchy = std::tuple<
    // Group 1: OperationalMode is the parent of the two main working states
    SubstateGroup<OperationalModeState, IdleWaitingState, AssemblyProcessState>,
    
    // Group 2: AssemblyProcess is the parent of all sequential build steps
    SubstateGroup<AssemblyProcessState, LoadComponentState, WeldJointState, InspectQualityState, UnloadProductState>,
    
    // Group 3: EmergencyStop is the parent of the error/recovery states
    SubstateGroup<EmergencyStopState, ErrorLoggedState, ManualOverrideState>
>;

static StateMachine<RobotStates, RobotEvent, RobotTransitions, RobotHierarchy> robotStateMachine;

int main() {
    std::cout << "--- Industrial Robot Plant HFSM Simulation Start ---" << std::endl;

    bool running = true;
    std::thread t([&]() {
        while(running) {
            robotStateMachine.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // 1. Initialization
    // The state machine typically starts in the first state of the hierarchy, 
    // which, due to the structure, defaults to the default state of the 
    // OperationalMode Super-State, which is IdleWaiting.
    std::cout << "\n[Initial State Check] The robot is initialized and is in IdleWaiting." << std::endl;
    // Assuming a call to start the machine is needed:
    // robotStateMachine.start(); 

    // -------------------------------------------------------------------
    std::cout << "\n--- STARTING NORMAL ASSEMBLY CYCLE ---" << std::endl;

    // 2. Start the Assembly Process: IdleWaiting -> LoadComponent (Jumps into AssemblyProcess H-State)
    std::cout << "\n[Event: Start_Cycle]" << std::endl;
    robotStateMachine.sendEvent(RobotEvent::Start_Cycle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 3. Sequential Assembly Steps
    // LoadComponent -> WeldJoint
    std::cout << "\n[Event: Component_Loaded]" << std::endl;
    robotStateMachine.sendEvent(RobotEvent::Component_Loaded);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // WeldJoint -> InspectQuality
    std::cout << "[Event: Weld_Complete]" << std::endl;
    robotStateMachine.sendEvent(RobotEvent::Weld_Complete);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 4. Conditional Branching (SUCCESS PATH)
    // InspectQuality -> UnloadProduct
    std::cout << "[Event: Inspection_Pass]" << std::endl;
    robotStateMachine.sendEvent(RobotEvent::Inspection_Pass);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // UnloadProduct -> IdleWaiting (Cycle Complete)
    std::cout << "[Event: Product_Unloaded]" << std::endl;
    robotStateMachine.sendEvent(RobotEvent::Product_Unloaded);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // -------------------------------------------------------------------
    std::cout << "\n--- DEMONSTRATING HIERARCHICAL EMERGENCY STOP ---" << std::endl;
    
    // 5. Start a new cycle and stop mid-weld
    std::cout << "\n[Event: Start_Cycle]" << std::endl;
    robotStateMachine.sendEvent(RobotEvent::Start_Cycle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "[Event: Component_Loaded]" << std::endl;
    robotStateMachine.sendEvent(RobotEvent::Component_Loaded);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 6. Emergency Stop (The critical HFSM transition)
    // Current state is WeldJoint (child of AssemblyProcess, child of OperationalMode).
    // This event transitions the machine from the OperationalMode Super-State to 
    // the ErrorLogged Sub-State (which is a child of the EmergencyStop Super-State).
    std::cout << "\n** CRITICAL: Triggering E-Stop from OperationalMode (WeldJoint state) **" << std::endl;
    robotStateMachine.sendEvent(RobotEvent::Trigger_EStop);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // -------------------------------------------------------------------
    std::cout << "\n--- RECOVERY AND RESET SEQUENCE ---" << std::endl;

    // 7. Error Handling
    // ErrorLogged -> ManualOverride
    std::cout << "\n[Event: Reset_Command] (Acknowledging the error)" << std::endl;
    robotStateMachine.sendEvent(RobotEvent::Reset_Command);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 8. Return to Operational Mode
    // ManualOverride -> IdleWaiting (Jumps back across the Super-State boundary)
    std::cout << "[Event: Reset_Command] (Finalizing the reset)" << std::endl;
    robotStateMachine.sendEvent(RobotEvent::Reset_Command);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "\n--- Simulation Complete ---" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    running = false;
    t.join();
    return 0;
}