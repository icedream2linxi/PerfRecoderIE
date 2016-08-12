#include "stdafx.h"
#include "ProcessResourceUsage.hpp"
#include <numeric>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <psapi.h>
#include "CpuUsage.hpp"
#include "PcapSource.h"
#include "PortCache.h"
#include "ProcessGPUsage.hpp"

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <rapidjson/document.h>
#include <rapidjson/reader.h>

typedef websocketpp::client<websocketpp::config::asio_client> wsclient;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

struct FpsInfo
{
	uint16_t port;
};

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

struct ResourceUsageRecordAssist
{
	DWORD pid;
	PH_UINT64_DELTA *CpuKernelDelta;
	PH_UINT64_DELTA *CpuUserDelta;

	std::vector<NetworkTransmittedSize> networkTransmittedSize;
	std::mutex networkMutex;
	std::chrono::high_resolution_clock::time_point prevTime;

	int64_t osgFps;
	int64_t webFps;
	HANDLE hFPSMapFile;
	// 下面两个变量是为避免初始化过早，被监测程序还未FileMapping；如果构造函数中获取FileMapping失败，则等超过1秒后在recordFPS中获取一次。
	bool isFPSInited;
	std::chrono::high_resolution_clock::time_point fpsPrevInitTime;

	wsclient fpsMonitor;
	std::thread monitorThread;

	ResourceUsageRecordAssist(DWORD pid);
	~ResourceUsageRecordAssist();

	void initFPS();
	void onMessage(websocketpp::connection_hdl hdl, message_ptr msg);
};

ResourceUsageRecordAssist::ResourceUsageRecordAssist(DWORD _pid)
	: CpuKernelDelta(new PH_UINT64_DELTA)
	, CpuUserDelta(new PH_UINT64_DELTA)
	, prevTime(std::chrono::high_resolution_clock::now())
	, hFPSMapFile(NULL)
	, isFPSInited(false)
	, pid(_pid)
	, osgFps(0)
	, webFps(0)
{
	initFPS();
	if (hFPSMapFile == NULL)
		fpsPrevInitTime = std::chrono::high_resolution_clock::now();
	else
		isFPSInited = true;
}

ResourceUsageRecordAssist::~ResourceUsageRecordAssist()
{
	fpsMonitor.stop();
	if (monitorThread.joinable())
		monitorThread.join();
	if (CpuKernelDelta != nullptr) {
		delete CpuKernelDelta;
		CpuKernelDelta = nullptr;
	}

	if (CpuUserDelta != nullptr) {
		delete CpuUserDelta;
		CpuUserDelta = nullptr;
	}

	if (hFPSMapFile != NULL) {
		CloseHandle(hFPSMapFile);
		hFPSMapFile = NULL;
	}
}

void ResourceUsageRecordAssist::initFPS()
{
	wchar_t uniqueName[64];
	swprintf_s(uniqueName, L"FTK%d", pid);
	hFPSMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, uniqueName);
	if (hFPSMapFile != NULL)
	{
		auto fpsInfo = (FpsInfo*)MapViewOfFile(hFPSMapFile, FILE_MAP_READ, 0, 0, sizeof(FpsInfo));
		auto port = fpsInfo->port;
		UnmapViewOfFile(fpsInfo);

		std::string uri = "ws://localhost:";
		uri += boost::lexical_cast<std::string>(port) + "/monitor";

		fpsMonitor.set_access_channels(websocketpp::log::alevel::none);
		fpsMonitor.clear_access_channels(websocketpp::log::alevel::none);

		fpsMonitor.init_asio();

		fpsMonitor.set_message_handler(std::bind(&ResourceUsageRecordAssist::onMessage, this, std::placeholders::_1, std::placeholders::_2));

		websocketpp::lib::error_code ec;
		auto con = fpsMonitor.get_connection(uri, ec);
		fpsMonitor.connect(con);

		monitorThread = std::thread(&wsclient::run, &fpsMonitor);
		monitorThread.detach();
	}
}

void ResourceUsageRecordAssist::onMessage(websocketpp::connection_hdl hdl, message_ptr msg)
{
	rapidjson::Document document;
	document.Parse(msg->get_payload().c_str());
	auto osgIter = document.FindMember("osgFps");
	if (osgIter != document.MemberEnd())
	{
		int64_t fps = osgIter->value.GetInt64();
		InterlockedAdd64(&osgFps, fps);
	}

	auto webIter = document.FindMember("webFps");
	if (webIter != document.MemberEnd())
	{
		int64_t fps = webIter->value.GetInt64();
		InterlockedAdd64(&webFps, fps);
	}
}

ProcessResourceUsage::ProcessResourceUsage()
{
	init();

	record();
}

ProcessResourceUsage::~ProcessResourceUsage()
{
	m_networkThreadStop = true;
	if (m_networkThread.joinable())
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
	recordCpuAndMemoryUsage();
	recordNetworkUsage();

	ProcessGPUsage::getInstance().record();
	for each (auto item in m_resourceUsages) {
		auto resourceUsage = item.second.first;
		auto gpuUsage = ProcessGPUsage::getInstance().getUsage(resourceUsage->processId);

		for (size_t i = 0; i < gpuUsage->gpus.size(); ++i) {
			auto gpu = gpuUsage->gpus[i];
			decltype(resourceUsage->gpuUsages)::value_type tempGPUsage;
			if (resourceUsage->gpuUsages.size() <= i) {
				tempGPUsage = std::make_shared<GPUsage>();
				resourceUsage->gpuUsages.push_back(tempGPUsage);
			}
			else
				tempGPUsage = resourceUsage->gpuUsages[i];

			tempGPUsage->adapterName = gpu->name;
			tempGPUsage->usage = gpu->usage;
			tempGPUsage->dedicatedUsage = gpu->dedicatedUsage;
			tempGPUsage->sharedUsage = gpu->sharedUsage;
		}
	}

	recordFps();
}

void ProcessResourceUsage::addProcess(DWORD pid)
{
	std::lock_guard<std::mutex> lock(m_resourceUsagesMutex);
	if (m_resourceUsages.count(pid) != 0)
		return;

	auto usage = std::make_shared<ResourceUsage>();
	usage->processId = pid;
	auto assist = std::make_shared<ResourceUsageRecordAssist>(pid);
	m_resourceUsages.insert(std::make_pair(pid, std::make_pair(usage, assist)));

	ProcessGPUsage::getInstance().addProcess(pid);
}

void ProcessResourceUsage::removeProcess(DWORD pid)
{
	std::lock_guard<std::mutex> lock(m_resourceUsagesMutex);
	m_resourceUsages.erase(pid);

	ProcessGPUsage::getInstance().removeProcess(pid);
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

std::vector<std::shared_ptr<ResourceUsage>> ProcessResourceUsage::getUsages() const
{
	std::lock_guard<std::mutex> lock(m_resourceUsagesMutex);
	std::vector<std::shared_ptr<ResourceUsage>> usages;
	for each (auto item in m_resourceUsages)
		usages.push_back(item.second.first);
	return usages;
}

std::vector<std::shared_ptr<TotalGPUsageData>> ProcessResourceUsage::getTotalUsage()
{
	return ProcessGPUsage::getInstance().getTotalUsage();
}

std::wstring ProcessResourceUsage::detectEnvironment()
{
	std::wstring msg;
	auto hDll = ::LoadLibraryA("wpcap.dll");
	if (hDll != NULL) {
		FreeLibrary(hDll);
		hDll = NULL;
	}
	else {
		msg = L"请先安装WinPcap!";
	}

	return msg;
}

bool ProcessResourceUsage::init()
{
	PhpInit();

	m_pcap.reset(new PcapSource);
	m_portCache.reset(new PortCache);

	bool ret = m_pcap->Initialize();
	if (!ret)
		return ret;

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
	return true;
}

void ProcessResourceUsage::recordCpuAndMemoryUsage()
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
			auto usage = iter->second.first;
			auto assist = iter->second.second;
			lock.unlock();

			PhUpdateDelta(assist->CpuKernelDelta, process->KernelTime.QuadPart);
			PhUpdateDelta(assist->CpuUserDelta, process->UserTime.QuadPart);

			FLOAT newCpuUsage;
			FLOAT kernelCpuUsage;
			FLOAT userCpuUsage;
			kernelCpuUsage = (FLOAT)assist->CpuKernelDelta->Delta / m_sysTotalTime;
			userCpuUsage = (FLOAT)assist->CpuUserDelta->Delta / m_sysTotalTime;
			newCpuUsage = kernelCpuUsage + userCpuUsage;
			usage->cpuUsage = newCpuUsage;

			InterlockedExchange(&usage->workingSetSize, process->WorkingSetSize);
			InterlockedExchange(&usage->pagefileUsage, process->PagefileUsage);
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

		auto resource = iter->second.first;
		auto assist = iter->second.second;
		lock.unlock();

		auto nowTime = std::chrono::high_resolution_clock::now();
		decltype(nowTime) prevTime;

		std::vector<NetworkTransmittedSize> sizes;
		{
			std::lock_guard<std::mutex> sizesLock(assist->networkMutex);
			sizes.swap(assist->networkTransmittedSize);
			prevTime = assist->prevTime;
			assist->prevTime = nowTime;
		}

		std::chrono::duration<double> duration = std::chrono::duration_cast<decltype(duration)>(nowTime - prevTime);

		NetworkTransmittedSize total;
		total = std::accumulate(sizes.begin(), sizes.end(), total);
		InterlockedExchange(&resource->recvSpeed, total.recv / duration.count());
		InterlockedExchange(&resource->sendSpeed, total.send / duration.count());
	}
}

void ProcessResourceUsage::recordFps()
{
	for (auto item : m_resourceUsages) {
		auto usage = item.second.first;
		auto assist = item.second.second;

		if (assist->hFPSMapFile == NULL) {
			if (assist->isFPSInited)
				continue;

			auto now = std::chrono::high_resolution_clock::now();
			if (now - assist->fpsPrevInitTime < std::chrono::seconds(5))
				continue;
			assist->initFPS();
			assist->isFPSInited = true;
			if (assist->hFPSMapFile == NULL)
				continue;
		}

		usage->osgFps = InterlockedExchange64(&assist->osgFps, 0);
		usage->webFps = InterlockedExchange64(&assist->webFps, 0);
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
		auto assist = iter->second.second;
		resourceLock.unlock();

		NetworkTransmittedSize size;
		if (packetInfo.dir == DIR_UP)
			size.send = packetInfo.size;
		else if (packetInfo.dir == DIR_DOWN)
			size.recv = packetInfo.size;
		std::lock_guard<std::mutex> sizeLock(assist->networkMutex);
		assist->networkTransmittedSize.push_back(size);
	}
}

ResourceUsage::ResourceUsage()
	: processId(0)
	, cpuUsage(0.0f)
	, recvSpeed(0)
	, sendSpeed(0)
	, workingSetSize(0)
	, pagefileUsage(0)
	, osgFps(INVALID_FPS)
	, webFps(INVALID_FPS)
{

}
