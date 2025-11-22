#if defined(EATK_PLATFORM_ARM)

#include "../../../src/pch.h"
#include "ArmNetworkAdapter.h"

#include "nx_api.h"
#include "nxd_bsd.h"

extern "C" {
	extern NX_IP NetXDuoEthIpInstance;
}

ArmNetworkAdapter::ArmNetworkAdapter(const NetworkAdapterInfo& info)
	: INetworkAdapter(info), m_socket(0)
{
}
ArmNetworkAdapter::~ArmNetworkAdapter() = default;

std::expected<void, std::string> ArmNetworkAdapter::getNetworkAdapters(Adapters& adapters)
{
	for (size_t i = 0; i < NX_MAX_PHYSICAL_INTERFACES; ++i) {
		if (NetXDuoEthIpInstance.nx_ip_interface[i].nx_interface_valid == NX_TRUE) {
			NX_INTERFACE *itf = &NetXDuoEthIpInstance.nx_ip_interface[i];

			const MAC mac {
				static_cast<uint8_t>(itf->nx_interface_physical_address_msw >> 8),
				static_cast<uint8_t>(itf->nx_interface_physical_address_msw & 0xFF),
				static_cast<uint8_t>(itf->nx_interface_physical_address_lsw >> 24),
				static_cast<uint8_t>((itf->nx_interface_physical_address_lsw >> 16) & 0xFF),
				static_cast<uint8_t>((itf->nx_interface_physical_address_lsw >> 8) & 0xFF),
				static_cast<uint8_t>(itf->nx_interface_physical_address_lsw & 0xF)
			};

			adapters.emplace_back(std::string(itf->nx_interface_name), "", mac);
		}
	}

	return {};
}

std::expected<void, std::string> ArmNetworkAdapter::openSocket(EthType proto)
{
	m_socket = nx_bsd_socket(PF_PACKET, SOCK_RAW, htons(std::to_underlying(proto)));
	if (m_socket == NX_SOC_ERROR) {
		return std::unexpected(strerror(_nxd_get_errno()));
	}

	int broadcast_on = 1;
	if (nx_bsd_setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, &broadcast_on, sizeof(broadcast_on)) != NX_SOC_OK) {
		nx_bsd_soc_close(m_socket);
		return std::unexpected(std::string("Failed to set SO_BROADCAST: ") + strerror(_nxd_get_errno()));
	}

	// Copy the interface name into the ifreq structure
	struct nx_bsd_ifreq ifr{};
	strncpy(ifr.ifr_name, m_info.name.c_str(), NX_BSD_IFNAMSIZE - 1);
	ifr.ifr_name[NX_BSD_IFNAMSIZE - 1] = '\0';

	// // disable route
	// int dontroute = 1;
	// if (nx_bsd_setsockopt(m_socket, SOL_SOCKET, SO_DONTROUTE, &dontroute, sizeof(int)) != NX_SOC_OK) {
	// 	return std::unexpected(strerror(_nxd_get_errno()));
	// }

	// connect socket to NIC by name 
	if (nx_bsd_ioctl(m_socket, SIOCGIFINDEX, (INT*)&ifr) != 0) {
		return std::unexpected(strerror(_nxd_get_errno()));
	}
	auto ifindex = ifr.ifr_ifindex;

	// // set flags
	// struct nx_bsd_packet_mreq mreq{};
	// mreq.mr_ifindex = ifindex;
	// mreq.mr_type = PACKET_MR_PROMISC;
	// if (nx_bsd_setsockopt(m_socket, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 0) {
	// 	return std::unexpected(strerror(_nxd_get_errno()));
	// }

	// bind socket to protocol, in this case RAW EtherCAT
	struct nx_bsd_sockaddr_ll sll{};
	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ifindex;
	sll.sll_protocol = htons(std::to_underlying(proto));
	if (nx_bsd_bind(m_socket, (struct nx_bsd_sockaddr *)&sll, sizeof(sll)) != 0) {
		return std::unexpected(strerror(_nxd_get_errno()));
	}

	m_open = true;
	return {};
}

void ArmNetworkAdapter::closeSocket()
{
	nx_bsd_soc_close(m_socket);
	m_open = false;
}

std::expected<size_t, std::string> ArmNetworkAdapter::sendFrame(const uint8_t* data, size_t size) const
{
	auto bytesTx = nx_bsd_send(m_socket, reinterpret_cast<const CHAR*>(data), size, 0);
	if (bytesTx < 0) {
		return std::unexpected(strerror(_nxd_get_errno()));
	}

	return bytesTx;
}

std::expected<size_t, std::string> ArmNetworkAdapter::receiveFrame(uint8_t* buff, size_t buffSize) const
{
	auto bytesRx = nx_bsd_recv(m_socket, buff, buffSize, MSG_DONTWAIT);
	if (bytesRx < 0) {
		if (_nxd_get_errno() == EWOULDBLOCK || _nxd_get_errno() == EAGAIN)
			return 0;
		return std::unexpected(strerror(_nxd_get_errno()));
	}

	return bytesRx;
}

#endif
