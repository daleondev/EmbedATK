#include "pch.h"

#include "EmbedATK/Network/NetworkAdapter.h"

#if defined(EATK_PLATFORM_WINDOWS)
	#include "../../platform/windows/Network/WindowsNetworkAdapter.h"
#elif defined(EATK_PLATFORM_LINUX)
	#include "../../platform/linux/Network/LinuxNetworkAdapter.h"
#elif defined(EATK_PLATFORM_ARM)
	#include "../../platform/arm/Network/ArmNetworkAdapter.h"
#else
	#error "Unsupported platform"
#endif

std::expected<INetworkAdapter::AdaptersConstRef, std::string> INetworkAdapter::getNetworkAdapters()
{
	s_adapters.clear();

#if defined(EATK_PLATFORM_WINDOWS)
	auto result = WindowsNetworkAdapter::getNetworkAdapters(s_adapters);
#elif defined(EATK_PLATFORM_LINUX)
	auto result = LinuxNetworkAdapter::getNetworkAdapters(s_adapters);
#elif defined(EATK_PLATFORM_ARM)
	auto result = ArmNetworkAdapter::getNetworkAdapters(s_adapters);
#endif

	if (!result)
		return std::unexpected(result.error());

	return std::ref(s_adapters);
}

void INetworkAdapter::create(IPolymorphic<INetworkAdapter>& adapter, const NetworkAdapterInfo& info)
{
#if defined(EATK_PLATFORM_WINDOWS)
	adapter.construct<WindowsNetworkAdapter>(info);
#elif defined(EATK_PLATFORM_LINUX)
	adapter.construct<LinuxNetworkAdapter>(info);
#elif defined(EATK_PLATFORM_ARM)
	adapter.construct<ArmNetworkAdapter>(info);
#endif
}
