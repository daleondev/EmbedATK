#pragma once

#include "EmbedATK/Container/Vector.h"

enum class EthType : uint16_t
{
   IP = 0x0800,
   VLAN = 0x00,// Todo
   Ecat = 0x88A4
};

using MAC = std::array<uint8_t, 6>;

struct NetworkAdapterInfo
{
	std::string name;
	std::string desc;
	MAC mac;
};

class INetworkAdapter
{
public:
	static constexpr size_t NETWORK_ADAPTERS_MAX = 16;
	
	using Adapters = StaticVector<NetworkAdapterInfo, NETWORK_ADAPTERS_MAX>;
	using AdaptersConstRef = std::reference_wrapper<const Adapters>;
	static std::expected<AdaptersConstRef, std::string> getNetworkAdapters();

	static void create(IPolymorphic<INetworkAdapter>& adapter, const NetworkAdapterInfo& info);

	virtual ~INetworkAdapter() = default;

	virtual std::expected<void, std::string> openSocket(EthType proto) = 0;
	virtual void closeSocket() = 0;

	virtual std::expected<size_t, std::string> sendFrame(const uint8_t* data, size_t size) const = 0;
	virtual std::expected<size_t, std::string> receiveFrame(uint8_t* buff, size_t buffSize) const = 0;

	const NetworkAdapterInfo& getInfo() const { return m_info; }
	bool isSocketOpen() const{ return m_open; }

	struct StaticImpl;
    struct DynamicImpl
    {
        using Type = DynamicPolymorphic<INetworkAdapter>;
    };

protected:
	INetworkAdapter(const NetworkAdapterInfo& info)
		: m_info(info) {}

	// inline static Adapters s_adapters;

	NetworkAdapterInfo m_info;
	bool m_open = false;
};