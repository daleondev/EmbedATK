#pragma once

#include "EmbedATK/Network/NetworkAdapter.h"

class ArmNetworkAdapter : public INetworkAdapter
{
public:
	ArmNetworkAdapter(const NetworkAdapterInfo& info);
	~ArmNetworkAdapter();

	static std::expected<void, std::string> getNetworkAdapters(Adapters& adapters);

	std::expected<void, std::string> openSocket(EthType proto) override;
	void closeSocket() override;

	std::expected<size_t, std::string> sendFrame(const uint8_t* data, size_t size) const override;
	std::expected<size_t, std::string> receiveFrame(uint8_t* buff, size_t buffSize) const override;

private:
	int m_socket;
};

struct INetworkAdapter::StaticImpl
{
    using Type = StaticPolymorphic<INetworkAdapter, ArmNetworkAdapter>;
};