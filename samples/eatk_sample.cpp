#include "EmbedATK/EmbedATK.h"

int main()
{
    EATK_INIT_LOG();

    auto adapters = INetworkAdapter::getNetworkAdapters();
    if (!adapters) {
        EATK_ERROR("Failed to get network adapters: {}", adapters.error());
    }

    for (auto& a : adapters->get()) {
        EATK_INFO("{} - {}", a.name, a.desc);
    }
    
    EATK_SHUTDOWN_LOG();
    return 0;
}