#pragma once

#include "Mempp/StateMachine.h"
#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <string>

// For logging state transitions in tests
std::vector<std::string> g_log;

// --- Test State Machine Definition ---

enum class TestState {
    // Super states
    Operational,
    Maintenance,
    // Operational substates
    Idle,
    Running,
    // Running's substates
    Running_Sub1,
    Running_Sub2,
    // Maintenance substates
    SelfCheck,
    FirmwareUpdate
};

enum class TestEvent {
    Start,
    Stop,
    Run,
    Pause,
    GoToMaint,
    MaintFinished,
    UpdateFirmware
};

// For logging onActive calls in tests
struct OnActiveCall {
    TestState calledState;
    std::vector<TestState> subStates;

    bool operator==(const OnActiveCall& other) const {
        return calledState == other.calledState && subStates == other.subStates;
    }
};
std::vector<OnActiveCall> g_on_active_log;

// --- State Implementations ---

class Operational : public IState<TestState> {
public:
    void onEntry() override { g_log.push_back("Enter Operational"); }
    void onActive(TestState* subStates, size_t numSubStates) override { g_on_active_log.push_back({TestState::Operational, {subStates, subStates + numSubStates}}); }
    void onExit() override { g_log.push_back("Exit Operational"); }
};

class Maintenance : public IState<TestState> {
public:
    void onEntry() override { g_log.push_back("Enter Maintenance"); }
    void onActive(TestState* subStates, size_t numSubStates) override { g_on_active_log.push_back({TestState::Maintenance, {subStates, subStates + numSubStates}}); }
    void onExit() override { g_log.push_back("Exit Maintenance"); }
};

class Idle : public IState<TestState> {
public:
    void onEntry() override { g_log.push_back("Enter Idle"); }
    void onActive(TestState* subStates, size_t numSubStates) override { g_on_active_log.push_back({TestState::Idle, {subStates, subStates + numSubStates}}); }
    void onExit() override { g_log.push_back("Exit Idle"); }
};

class Running : public IState<TestState> {
public:
    void onEntry() override { g_log.push_back("Enter Running"); }
    void onActive(TestState* subStates, size_t numSubStates) override { g_on_active_log.push_back({TestState::Running, {subStates, subStates + numSubStates}}); }
    void onExit() override { g_log.push_back("Exit Running"); }
};

class Running_Sub1 : public IState<TestState> {
public:
    void onEntry() override { g_log.push_back("Enter Running_Sub1"); }
    void onActive(TestState* subStates, size_t numSubStates) override { g_on_active_log.push_back({TestState::Running_Sub1, {subStates, subStates + numSubStates}}); }
    void onExit() override { g_log.push_back("Exit Running_Sub1"); }
};

class Running_Sub2 : public IState<TestState> {
public:
    void onEntry() override { g_log.push_back("Enter Running_Sub2"); }
    void onActive(TestState* subStates, size_t numSubStates) override { g_on_active_log.push_back({TestState::Running_Sub2, {subStates, subStates + numSubStates}}); }
    void onExit() override { g_log.push_back("Exit Running_Sub2"); }
};

class SelfCheck : public IState<TestState> {
public:
    void onEntry() override { g_log.push_back("Enter SelfCheck"); }
    void onActive(TestState* subStates, size_t numSubStates) override { g_on_active_log.push_back({TestState::SelfCheck, {subStates, subStates + numSubStates}}); }
    void onExit() override { g_log.push_back("Exit SelfCheck"); }
};

class FirmwareUpdate : public IState<TestState> {
public:
    void onEntry() override { g_log.push_back("Enter FirmwareUpdate"); }
    void onActive(TestState* subStates, size_t numSubStates) override { g_on_active_log.push_back({TestState::FirmwareUpdate, {subStates, subStates + numSubStates}}); }
    void onExit() override { g_log.push_back("Exit FirmwareUpdate"); }
};


// --- Type Aliases ---
using OperationalState    = State<TestState::Operational, Operational>;
using MaintenanceState    = State<TestState::Maintenance, Maintenance>;
using IdleState           = State<TestState::Idle, Idle>;
using RunningState        = State<TestState::Running, Running>;
using Running_Sub1State   = State<TestState::Running_Sub1, Running_Sub1>;
using Running_Sub2State   = State<TestState::Running_Sub2, Running_Sub2>;
using SelfCheckState      = State<TestState::SelfCheck, SelfCheck>;
using FirmwareUpdateState = State<TestState::FirmwareUpdate, FirmwareUpdate>;

// --- States Container ---
using TestHSMStates = States<
    OperationalState,
    MaintenanceState,
    IdleState,
    RunningState,
    Running_Sub1State,
    Running_Sub2State,
    SelfCheckState,
    FirmwareUpdateState
>;

// --- States Container (with substate as default) ---
using TestHSMStates_SubstateDefault = States<
    IdleState, // Default is now IdleState
    OperationalState,
    MaintenanceState,
    RunningState,
    Running_Sub1State,
    Running_Sub2State,
    SelfCheckState,
    FirmwareUpdateState
>;

// --- Transitions ---
using TestHSMTransitions = StateTransitions<
    // Transitions within Operational
    StateTransition<IdleState, TestEvent::Run, Running_Sub1State>,
    StateTransition<RunningState, TestEvent::Pause, IdleState>,
    StateTransition<Running_Sub1State, TestEvent::Stop, Running_Sub2State>,

    // Transitions between super states
    StateTransition<OperationalState, TestEvent::GoToMaint, SelfCheckState>,
    StateTransition<MaintenanceState, TestEvent::MaintFinished, IdleState>,

    // Transitions within Maintenance
    StateTransition<SelfCheckState, TestEvent::UpdateFirmware, FirmwareUpdateState>
>;

// --- Hierarchy ---
using TestHSMHierarchy = StateHierarchy<
    SubstateGroup<OperationalState, IdleState, RunningState>,
    SubstateGroup<RunningState, Running_Sub1State, Running_Sub2State>,
    SubstateGroup<MaintenanceState, SelfCheckState, FirmwareUpdateState>
>;

// --- Test Fixture ---
class StateMachineTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_log.clear();
        g_on_active_log.clear();
    }

    void TearDown() override {
    }

    template<typename T>
    void printLog(const T& log) {
        std::cout << "Log:" << std::endl;
        for(const auto& entry : log) {
            std::cout << "- " << entry << std::endl;
        }
    }
};

// --- Tests ---

TEST_F(StateMachineTest, Initialization) {
    StateMachine<TestHSMStates, TestEvent, TestHSMTransitions, TestHSMHierarchy> sm;
    
    std::vector<std::string> expected_log = {"Enter Operational", "Enter Idle"};
    EXPECT_EQ(g_log, expected_log);
    EXPECT_EQ(sm.currentState(), TestState::Idle);
}

TEST_F(StateMachineTest, InitializationWithSubstateAsDefault) {
    StateMachine<TestHSMStates_SubstateDefault, TestEvent, TestHSMTransitions, TestHSMHierarchy> sm;
    
    std::vector<std::string> expected_log = {"Enter Operational", "Enter Idle"};
    EXPECT_EQ(g_log, expected_log);
    EXPECT_EQ(sm.currentState(), TestState::Idle);
}

TEST_F(StateMachineTest, HierarchicalTransition) {
    StateMachine<TestHSMStates, TestEvent, TestHSMTransitions, TestHSMHierarchy> sm;
    g_log.clear();

    sm.sendEvent(TestEvent::GoToMaint);
    sm.update();

    std::vector<std::string> expected_log = {
        "Exit Idle",
        "Exit Operational",
        "Enter Maintenance",
        "Enter SelfCheck"
    };
    EXPECT_EQ(g_log, expected_log);
    EXPECT_EQ(sm.currentState(), TestState::SelfCheck);
}

TEST_F(StateMachineTest, TransitionToSubstate) {
    StateMachine<TestHSMStates, TestEvent, TestHSMTransitions, TestHSMHierarchy> sm;
    g_log.clear();

    sm.sendEvent(TestEvent::Run);
    sm.update();

    std::vector<std::string> expected_log = {
        "Exit Idle",
        "Enter Running",
        "Enter Running_Sub1"
    };
    EXPECT_EQ(g_log, expected_log);
    EXPECT_EQ(sm.currentState(), TestState::Running_Sub1);
}

TEST_F(StateMachineTest, TransitionFromSuperstate) {
    StateMachine<TestHSMStates, TestEvent, TestHSMTransitions, TestHSMHierarchy> sm;
    
    // Go to Running_Sub1
    sm.sendEvent(TestEvent::Run);
    sm.update();
    EXPECT_EQ(sm.currentState(), TestState::Running_Sub1);
    g_log.clear();

    // Send event that is handled by the 'Running' superstate
    sm.sendEvent(TestEvent::Pause);
    sm.update();

    std::vector<std::string> expected_log = {
        "Exit Running_Sub1",
        "Exit Running",
        "Enter Idle"
    };
    EXPECT_EQ(g_log, expected_log);
    EXPECT_EQ(sm.currentState(), TestState::Idle);
}

TEST_F(StateMachineTest, OriginalBug_HierarchicalExit) {
    StateMachine<TestHSMStates, TestEvent, TestHSMTransitions, TestHSMHierarchy> sm;
    
    // Go to Running_Sub1
    sm.sendEvent(TestEvent::Run);
    sm.update();
    EXPECT_EQ(sm.currentState(), TestState::Running_Sub1);
    g_log.clear();

    // Send event that is handled by the 'Operational' superstate
    sm.sendEvent(TestEvent::GoToMaint);
    sm.update();

    std::vector<std::string> expected_log = {
        "Exit Running_Sub1",
        "Exit Running",
        "Exit Operational",
        "Enter Maintenance",
        "Enter SelfCheck"
    };
    // printLog(g_log); // Keep this for debugging if it fails
    EXPECT_EQ(g_log, expected_log);
    EXPECT_EQ(sm.currentState(), TestState::SelfCheck);
}

TEST_F(StateMachineTest, OnActiveOrderAndParameters) {
    StateMachine<TestHSMStates, TestEvent, TestHSMTransitions, TestHSMHierarchy> sm;
    
    // Go to a nested state: Operational -> Idle -> Running -> Running_Sub1
    sm.sendEvent(TestEvent::Run);
    sm.update();
    ASSERT_EQ(sm.currentState(), TestState::Running_Sub1);

    // Clear logs and call update again to trigger onActive calls
    g_on_active_log.clear();
    sm.update();

    // --- Verification ---
    
    // The active path is [Operational, Running, Running_Sub1].
    // The call order for onActive should be bottom-up: Running_Sub1 -> Running -> Operational.
    // The 'subStates' parameter should contain the children that have already been processed in the same tick.
    std::vector<OnActiveCall> expected_calls = {
        // 1. Called on Running_Sub1. subStates is empty.
        {TestState::Running_Sub1, {}},
        // 2. Called on Running. subStates is [Running_Sub1].
        {TestState::Running, {TestState::Running_Sub1}},
        // 3. Called on Operational. subStates is [Running_Sub1, Running].
        {TestState::Operational, {TestState::Running_Sub1, TestState::Running}},
    };

    ASSERT_EQ(g_on_active_log.size(), expected_calls.size());
    EXPECT_EQ(g_on_active_log, expected_calls);
}