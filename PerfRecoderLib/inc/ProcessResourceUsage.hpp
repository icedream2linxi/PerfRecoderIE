#pragma once
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <ratio>

struct PH_UINT64_DELTA;
class PcapSource;
class PortCache;

struct NetworkTransmittedSize
{
	uint64_t recv;
	uint64_t send;
	NetworkTransmittedSize() : recv(0), send(0) {}
	NetworkTransmittedSize &operator+=(const NetworkTransmittedSize &oth)
	{
		recv += oth.recv;
		send += oth.send;
		return *this;
	}
	NetworkTransmittedSize operator+(const NetworkTransmittedSize &oth)
	{
		NetworkTransmittedSize nt;
		nt.recv = recv + oth.recv;
		nt.send = send + oth.send;
		return nt;
	}
};

struct ResourceUsage
{
	DWORD processId;
	float cpuUsage;
	uint64_t recvSpeed;
	uint64_t sendSpeed;
	uint64_t workingSetSize;
	uint64_t pagefileUsage;

	PH_UINT64_DELTA *CpuKernelDelta;
	PH_UINT64_DELTA *CpuUserDelta;

	std::vector<NetworkTransmittedSize> networkTransmittedSize;
	std::mutex networkMutex;
	std::chrono::high_resolution_clock::time_point prevTime;

	ResourceUsage();
	~ResourceUsage();
};

class ProcessResourceUsage
{
	ProcessResourceUsage();
public:
	~ProcessResourceUsage();
	static ProcessResourceUsage &getInstance();
	void record();
	void addProcess(DWORD processId);
	const std::vector<std::wstring> &getNetworkInterfaceNames() const;
	void selectNetworkInterface(int index);

private:
	void init();
	void recordCpuUsage();
	void recordNetworkUsage();
	void recordMemoryUsage();
	void networkThreadRun();

private:
	ULONG64 m_sysTotalTime;

	bool m_networkThreadStop;
	std::thread m_networkThread;
	std::shared_ptr<PcapSource> m_pcap;
	std::shared_ptr<PortCache> m_portCache;
	std::vector<std::wstring> m_networkInterfaceNames;
	long m_networkInterfaceIndex;
	bool m_reconnect;

	std::map<DWORD, std::shared_ptr<ResourceUsage>> m_resourceUsages;
	std::mutex m_resourceUsagesMutex;
};
