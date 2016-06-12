#include "stdafx.h"
#include "ProcessResourceUsage.hpp"
#include <numeric>
#include <boost/algorithm/string.hpp>
#include <psapi.h>
#include "CpuUsage.hpp"
#include "PcapSource.h"
#include "PortCache.h"

ResourceUsage::ResourceUsage()
	: CpuKernelDelta(new PH_UINT64_DELTA)
	, CpuUserDelta(new PH_UINT64_DELTA)
	, prevTime(std::chrono::high_resolution_clock::now())
{

}

ResourceUsage::~ResourceUsage()
{
	if (CpuKernelDelta != nullptr) {
		delete CpuKernelDelta;
		CpuKernelDelta = nullptr;
	}

	if (CpuUserDelta != nullptr) {
		delete CpuUserDelta;
		CpuUserDelta = nullptr;
	}
}

ProcessResourceUsage::ProcessResourceUsage()
{
	init();
}

ProcessResourceUsage::~ProcessResourceUsage()
{
	m_networkThreadStop = true;
	m_networkThread.join();
}

ProcessResourceUsage & ProcessResourceUsage::getInstance()
{
	static std::auto_ptr<ProcessResourceUsage> instance;
	if (instance.get() == nullptr)
		instance.reset(new ProcessResourceUsage());
	return *instance;
}

void ProcessResourceUsage::record()
{
	recordCpuUsage();
	recordNetworkUsage();
	recordMemoryUsage();
}

void ProcessResourceUsage::addProcess(DWORD processId)
{
	std::shared_ptr<ResourceUsage> usage(new ResourceUsage);
	usage->processId = processId;
	std::lock_guard<std::mutex> lock(m_resourceUsagesMutex);
	m_resourceUsages.insert(std::make_pair(processId, usage));
}

const std::vector<std::wstring> & ProcessResourceUsage::getNetworkInterfaceNames() const
{
	return m_networkInterfaceNames;
}

void ProcessResourceUsage::selectNetworkInterface(int index)
{
	if (index < 0)
		return;
	m_networkInterfaceIndex = index;
	m_reconnect = true;
}

void ProcessResourceUsage::init()
{
	PhpInit();

	m_pcap.reset(new PcapSource);
	m_portCache.reset(new PortCache);

	m_pcap->Initialize();
	auto count = m_pcap->EnumDevices();
	bool isSelected = false;
	for (decltype(count) i = 0; i < count; ++i) {
		m_networkInterfaceNames.push_back(m_pcap->GetDeviceName(i));
		if (!isSelected && !boost::contains(m_networkInterfaceNames.back(), L"Loop")) {
			m_pcap->SelectDevice(i);
			m_networkInterfaceIndex = i;
			isSelected = true;
		}
	}
	m_reconnect = false;

	m_networkThreadStop = false;
	m_networkThread = std::thread(&ProcessResourceUsage::networkThreadRun, this);
}

void ProcessResourceUsage::recordCpuUsage()
{
	PhpUpdateCpuInformation(TRUE, &m_sysTotalTime);

	PVOID processes;
	PSYSTEM_PROCESS_INFORMATION process;
	if (!NT_SUCCESS(PhEnumProcesses(&processes)))
		return;

	process = PH_FIRST_PROCESS(processes);
	do {
		DWORD processId = (DWORD)process->UniqueProcessId;

		std::unique_lock<std::mutex> lock(m_resourceUsagesMutex);
		auto iter = m_resourceUsages.find(processId);
		if (iter != m_resourceUsages.end()) {
			auto usage = iter->second;
			lock.unlock();

			PhUpdateDelta(usage->CpuKernelDelta, process->KernelTime.QuadPart);
			PhUpdateDelta(usage->CpuUserDelta, process->UserTime.QuadPart);

			FLOAT newCpuUsage;
			FLOAT kernelCpuUsage;
			FLOAT userCpuUsage;
			kernelCpuUsage = (FLOAT)usage->CpuKernelDelta->Delta / m_sysTotalTime;
			userCpuUsage = (FLOAT)usage->CpuUserDelta->Delta / m_sysTotalTime;
			newCpuUsage = kernelCpuUsage + userCpuUsage;
			usage->cpuUsage = newCpuUsage * 100.0f;
		}
	} while (process = PH_NEXT_PROCESS(process));

	if (process == NULL)
	{
		PhFree(processes);
		return;
	}

	PhFree(processes);
}

void ProcessResourceUsage::recordNetworkUsage()
{
	std::vector<decltype(m_resourceUsages)::key_type> pids;
	{
		std::lock_guard<std::mutex> lock(m_resourceUsagesMutex);
		for (auto &item : m_resourceUsages)
			pids.push_back(item.first);
	}

	for (auto pid : pids) {
		std::unique_lock<std::mutex> lock(m_resourceUsagesMutex);
		auto iter = m_resourceUsages.find(pid);
		if (iter == m_resourceUsages.end())
			continue;

		auto resource = iter->second;
		lock.unlock();

		auto nowTime = std::chrono::high_resolution_clock::now();
		decltype(nowTime) prevTime;

		std::vector<NetworkTransmittedSize> sizes;
		{
			std::lock_guard<std::mutex> sizesLock(resource->networkMutex);
			sizes.swap(resource->networkTransmittedSize);
			prevTime = resource->prevTime;
			resource->prevTime = nowTime;
		}

		std::chrono::duration<double> duration = std::chrono::duration_cast<decltype(duration)>(nowTime - prevTime);

		NetworkTransmittedSize total;
		total = std::accumulate(sizes.begin(), sizes.end(), total);
		InterlockedExchange(&resource->recvSpeed, total.recv / duration.count());
		InterlockedExchange(&resource->sendSpeed, total.send / duration.count());
	}
}

void ProcessResourceUsage::recordMemoryUsage()
{
	std::vector<decltype(m_resourceUsages)::key_type> pids;
	{
		std::lock_guard<std::mutex> lock(m_resourceUsagesMutex);
		for (auto &item : m_resourceUsages)
			pids.push_back(item.first);
	}

	for (auto pid : pids) {
		std::unique_lock<std::mutex> lock(m_resourceUsagesMutex);
		auto iter = m_resourceUsages.find(pid);
		if (iter == m_resourceUsages.end())
			continue;

		auto resource = iter->second;
		lock.unlock();

		PROCESS_MEMORY_COUNTERS_EX mem;

		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
		GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&mem, sizeof(PROCESS_MEMORY_COUNTERS_EX));

		InterlockedExchange(&resource->workingSetSize, mem.WorkingSetSize);
		InterlockedExchange(&resource->pagefileUsage, mem.PagefileUsage);

		CloseHandle(hProcess);
	}
}

void ProcessResourceUsage::networkThreadRun()
{
	while (!m_networkThreadStop)
	{
		if (m_reconnect) {
			m_reconnect = false;
			long index = 0;
			InterlockedExchange(&index, m_networkInterfaceIndex);
			if (!m_pcap->SelectDevice(index))
				continue;
		}
	
		bool timeout = false;
		PacketInfo packetInfo;
		if (!m_pcap->Capture(&packetInfo, &timeout)) {
			if (!timeout)
				m_pcap->Reconnect(m_networkInterfaceIndex);
			continue;
		}
	
		int pid = 0;
		if (packetInfo.trasportProtocol == TRA_TCP)
			pid = m_portCache->GetTcpPortPid(packetInfo.local_port);
		else if (packetInfo.trasportProtocol == TRA_UDP)
			pid = m_portCache->GetUdpPortPid(packetInfo.local_port);

		std::unique_lock<std::mutex> resourceLock(m_resourceUsagesMutex);
		auto iter = m_resourceUsages.find(pid);
		if (iter == m_resourceUsages.end())
			continue;
		auto resource = iter->second;
		resourceLock.unlock();

		NetworkTransmittedSize size;
		if (packetInfo.dir = DIR_UP)
			size.send = packetInfo.size;
		else if (packetInfo.dir = DIR_DOWN)
			size.recv = packetInfo.size;
		std::lock_guard<std::mutex> sizeLock(resource->networkMutex);
		resource->networkTransmittedSize.push_back(size);
	}
}
