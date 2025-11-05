#include "EmbedATK/EmbedATK.h"

int main()
{
    EATK_INIT_LOG();

    auto adapters = INetworkAdapter::getNetworkAdapters();
    if (!adapters) {
        EATK_ERROR("Failed to get network adapters: {}", adapters.error());
    }

    NetworkAdapterInfo info;
    for (auto& adpt : adapters->get()) {
        EATK_INFO("{} - {}", adpt.name, adpt.desc);
        #if defined(EATK_PLATFORM_WINDOWS)
		    if (adpt.desc.contains("Intel(R) Ethernet Connection"))
                info = adpt;
        #else
            if (adpt.name.contains("enp0s31f6"))
                info = adpt;
        #endif
    }

    INetworkAdapter::StaticImpl::Type adapter;
    INetworkAdapter::create(adapter, info);
    
    EATK_SHUTDOWN_LOG();
    return 0;
}