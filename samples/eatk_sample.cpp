#include "EmbedATK/EmbedATK.h"

struct GlobalData
{
    int i;
    float f;
} g_data;

enum class TestEvent {
    Start,
    Stop
};

enum class TestState {
    Idle,
    Running
};

class Idle : public IState<TestState::Idle, GlobalData> {
public:
    void onEntry() override { std::cout << "Idle onEntry" << std::endl; }
    void onActive(IdType*, size_t) override 
    { 
        std::cout << "Idle onActive" << std::endl;
        auto& data = std::get<GlobalData>(getData());
        std::cout << data.i << " " << data.f << std::endl;
    }
    void onExit() override { std::cout << "Idle onExit" << std::endl;  }
};

class Running : public IState<TestState::Running, GlobalData> {
public:
    void onEntry() override { std::cout << "Running onEntry" << std::endl; }
    void onActive(IdType*, size_t) override { std::cout << "Running onActive" << std::endl; }
    void onExit() override { std::cout << "Running onExit" << std::endl; }
};

using TestStates = States<
    Idle,
    Running
>;

using TestTransitions = StateTransitions<
    StateTransition<Idle, TestEvent::Start, Running>,
    StateTransition<Running, TestEvent::Stop, Idle>
>;

int main()
{
    StateMachine<TestStates, TestEvent, TestTransitions> stateMachine;
    g_data.i = 1224.34;
    g_data.f = -346.4235;

    stateMachine.forEachState([](auto& state) {
        state.setData(g_data);
    });

    bool running = true;
    std::thread t([&]() {
        while (running) {
            stateMachine.update();
            OSAL::sleep(500000);
        }
    });

    OSAL::sleep(1000000);
    stateMachine.sendEvent(TestEvent::Start);
    OSAL::sleep(1000000);
    stateMachine.sendEvent(TestEvent::Stop);
    OSAL::sleep(1000000);
    running = false;


    t.join();
    return 0;

    EATK_INIT_LOG(10);

    // auto adapters = INetworkAdapter::getNetworkAdapters();
    // if (!adapters) {
    //     EATK_ERROR("Failed to get network adapters: {}", adapters.error());
    // }

    // NetworkAdapterInfo info;
    // for (auto& adpt : adapters->get()) {
    //     EATK_INFO("{} - {}", adpt.name, adpt.desc);
    //     #if defined(EATK_PLATFORM_WINDOWS)
	// 	    if (adpt.desc.contains("Intel(R) Ethernet Connection"))
    //             info = adpt;
    //     #else
    //         if (adpt.name.contains("enp0s31f6"))
    //             info = adpt;
    //     #endif
    // }

    // INetworkAdapter::StaticImpl::Type adapter;
    // INetworkAdapter::create(adapter, info);
    
    // EATK_SHUTDOWN_LOG();

    Utils::StaticThread<
        OSAL::StaticImpl::CyclicThread,
        "TestThread",
        16384,
        4,
        [](){
            EATK_INFO("THREAD");
            std::cout << "Thread" << std::endl;
        },
        1000000
    > thread;

    Utils::setupStaticThread(thread, true);

    OSAL::sleep(10000000);

    Utils::shutdownStaticThread(thread);

    EATK_SHUTDOWN_LOG();

    return 0;
}