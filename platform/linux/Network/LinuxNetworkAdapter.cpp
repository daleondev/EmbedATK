#if defined(EATK_PLATFORM_LINUX)

#include "pch.h"
#include "LinuxNetworkAdapter.h"

#include "EmbedATK/Core/Assert.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <netpacket/packet.h>
#include <pthread.h>
#include <poll.h>

LinuxNetworkAdapter::LinuxNetworkAdapter(const NetworkAdapterInfo& info)
	: INetworkAdapter(info), m_socket(0)
{
}
LinuxNetworkAdapter::~LinuxNetworkAdapter() = default;

std::expected<void, std::string> LinuxNetworkAdapter::getNetworkAdapters(Adapters& adapters)
{
	struct if_nameindex *ids = if_nameindex();
	if (!ids) {
		return std::unexpected(strerror(errno));
	}

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		if_freenameindex(ids);
		return std::unexpected(strerror(errno));
	}

	for (int i = 0; ids[i].if_index != 0; ++i) {	
		if (ids[i].if_name) {
			// Copy the interface name into the ifreq structure
			struct ifreq ifr{};
			strncpy(ifr.ifr_name, ids[i].if_name, IFNAMSIZ - 1);
			ifr.ifr_name[IFNAMSIZ - 1] = '\0';

			// Make the ioctl call to get the hardware address
			MAC mac{};
			if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
				// Extract the MAC address
				const uint8_t* mac_ptr = reinterpret_cast<const uint8_t*>(ifr.ifr_hwaddr.sa_data);
				memcpy(mac.data(), mac_ptr, 6);
			}

			adapters.emplace_back(std::string(ids[i].if_name), "", mac);
		}
	}

	close(sock);
	if_freenameindex(ids);
	return {};
}

std::expected<void, std::string> LinuxNetworkAdapter::openSocket(EthType proto)
{
	m_socket = socket(PF_PACKET, SOCK_RAW, htons(std::to_underlying(proto)));
	if (m_socket < 0) {
		return std::unexpected(strerror(errno));
	}

	int broadcast_on = 1;
	if (setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, &broadcast_on, sizeof(broadcast_on)) != 0) {
		close(m_socket);
		return std::unexpected(std::string("Failed to set SO_BROADCAST: ") + strerror(errno));
	}

	// Copy the interface name into the ifreq structure
	struct ifreq ifr{};
	strncpy(ifr.ifr_name, m_info.name.c_str(), IFNAMSIZ - 1);
	ifr.ifr_name[IFNAMSIZ - 1] = '\0';

	// disable route
	int dontroute = 1;
	if (setsockopt(m_socket, SOL_SOCKET, SO_DONTROUTE, &dontroute, sizeof(int)) != 0) {
		return std::unexpected(strerror(errno));
	}

	// connect socket to NIC by name 
	if (ioctl(m_socket, SIOCGIFINDEX, &ifr) != 0) {
		return std::unexpected(strerror(errno));
	}
	auto ifindex = ifr.ifr_ifindex;

	// set flags
	struct packet_mreq mreq{};
	mreq.mr_ifindex = ifindex;
	mreq.mr_type = PACKET_MR_PROMISC;
	if (setsockopt(m_socket, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 0) {
		return std::unexpected(strerror(errno));
	}

	// bind socket to protocol, in this case RAW EtherCAT
	struct sockaddr_ll sll{};
	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ifindex;
	sll.sll_protocol = htons(std::to_underlying(proto));
	if (bind(m_socket, (struct sockaddr *)&sll, sizeof(sll)) != 0) {
		return std::unexpected(strerror(errno));
	}

	m_open = true;
	return {};
}

void LinuxNetworkAdapter::closeSocket()
{
	close(m_socket);
	m_open = false;
}

std::expected<size_t, std::string> LinuxNetworkAdapter::sendFrame(const uint8_t* data, size_t size) const
{
	EATK_ASSERT(m_open, "socket not open");

	auto bytesTx = send(m_socket, data, size, 0);
	if (bytesTx < 0) {
		return std::unexpected(strerror(errno));
	}

	return bytesTx;
}

std::expected<size_t, std::string> LinuxNetworkAdapter::receiveFrame(uint8_t* buff, size_t buffSize) const
{
	EATK_ASSERT(m_open, "socket not open");

	auto bytesRx = recv(m_socket, buff, buffSize, MSG_DONTWAIT);
	if (bytesRx < 0) {
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return 0;
		return std::unexpected(strerror(errno));
	}

	return bytesRx;
}

#endif