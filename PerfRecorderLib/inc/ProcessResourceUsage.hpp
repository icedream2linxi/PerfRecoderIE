#pragma once
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <ratio>
#include <limits>
#include "TypeDefine.hpp"

class PcapSource;
class PortCache;
struct ResourceUsageRecordAssist;

struct GPUsage
{
	std::wstring adapterName;
	float usage;
	uint64_t dedicatedUsage;
	uint64_t sharedUsage;
};

constexpr uint32_t INVALID_FPS = std::numeric_limits<uint32_t>::max();
struct ResourceUsage
{
	DWORD processId;
	float cpuUsage;
	uint64_t recvSpeed;
	uint64_t sendSpeed;
	uint64_t workingSetSize;
	uint64_t pagefileUsage;
	uint32_t osgFps;
	uint32_t webFps;

	std::vector<std::shared_ptr<GPUsage>> gpuUsages;

	ResourceUsage();
};

class ProcessResourceUsage
{
	ProcessResourceUsage();
public:
	~ProcessResourceUsage();
	static ProcessResourceUsage &getInstance();
	void record();
	void addProcess(DWORD pid);
	void removeProcess(DWORD pid);
	const std::vector<std::wstring> &getNetworkInterfaceNames() const;
	void selectNetworkInterface(int index);
	std::vector<std::shared_ptr<ResourceUsage>> getUsages() const;

	static std::vector<std::shared_ptr<TotalGPUsageData>> getTotalUsage();
	static std::wstring detectEnvironment();

private:
	bool init();
	void recordCpuAndMemoryUsage();
	void recordNetworkUsage();
	void recordFps();
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

	std::map<DWORD, std::pair<std::shared_ptr<ResourceUsage>, std::shared_ptr<ResourceUsageRecordAssist>>> m_resourceUsages;
	mutable std::mutex m_resourceUsagesMutex;
};
