#include "stdafx.h"
#include "ProcessGPUsage.hpp"
#include <setupapi.h>
#include <LMaccess.h>
#include <d3dkmthk.h>
//#include <winternl.h>
#include <VersionHelpers.h>

// Performance Counter

extern "C"
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryPerformanceCounter(
	_Out_ PLARGE_INTEGER PerformanceCounter,
	_Out_opt_ PLARGE_INTEGER PerformanceFrequency
);

namespace {
	static GUID GUID_DISPLAY_DEVICE_ARRIVAL_I = { 0x1ca05180, 0xa699, 0x450a,{ 0x9a, 0x0c, 0xde, 0x4f, 0xbe, 0x3d, 0xdd, 0x89 } };

	std::wstring QueryDeviceDescription(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData)
	{
		ULONG bufferSize = 128;
		wchar_t *buffer = new wchar_t[bufferSize];
		BOOL result = SetupDiGetDeviceRegistryProperty(deviceInfoSet, deviceInfoData,
			SPDRP_DEVICEDESC, NULL, (PBYTE)buffer, bufferSize, &bufferSize);
		if (!result && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
			delete[] buffer;
			buffer = new wchar_t[bufferSize];
			result = SetupDiGetDeviceRegistryProperty(deviceInfoSet, deviceInfoData,
				SPDRP_DEVICEDESC, NULL, (PBYTE)buffer, bufferSize, &bufferSize);
		}

		std::wstring desc;
		if (result)
			desc.assign(buffer);
		return desc;
	}
}

GPUAdapter::GPUAdapter()
	: sharedLimit(0ull)
	, dedicatedLimit(0ull)
{

}

ProcessGPUsage::ProcessGPUsage()
	: m_hGdi32(NULL)
{
	m_hGdi32 = LoadLibraryA("gdi32.dll");
	D3DKMTQueryStatistics = (PFND3DKMT_QUERYSTATISTICS)GetProcAddress(m_hGdi32, "D3DKMTQueryStatistics");

	initializeD3DStatistics();

	addProcess(0);
}

ProcessGPUsage::~ProcessGPUsage()
{
	if (m_hGdi32 != NULL) {
		FreeLibrary(m_hGdi32);
		m_hGdi32 = NULL;
	}
}

ProcessGPUsage & ProcessGPUsage::getInstance()
{
	static std::auto_ptr<ProcessGPUsage> instance;
	if (instance.get() == nullptr)
		instance.reset(new ProcessGPUsage);
	return *instance;
}

void ProcessGPUsage::addProcess(DWORD pid)
{
	if (m_usages.count(pid) != 0)
		return;

	auto usage = std::make_shared<ProcessGPUsageData>();
	usage->pid = pid;
	for each (auto gpuAdapter in m_adapters) {
		auto gpu = std::make_shared<ProcessSingleGPUsageData>();
		gpu->name = gpuAdapter->description;
		usage->gpus.push_back(gpu);
	}
	m_usages.insert(std::make_pair(pid, usage));
}

void ProcessGPUsage::removeProcess(DWORD pid)
{
	if (pid == 0)
		return;
	m_usages.erase(pid);
}

void ProcessGPUsage::record()
{
	recordSegmentInformation(0);
	recordNodeInformation(0);

	double elapsedTime = (double)EtClockTotalRunningTimeDelta.Delta * 10000000 / EtClockTotalRunningTimeFrequency.QuadPart;

	for each (auto item in m_usages) {
		if (item.first == 0)
			continue;
		recordSegmentInformation(item.first);
		recordNodeInformation(item.first);
	}

	for (size_t i = 0; i < m_adapters.size(); ++i) {
		auto gpuAdapter = m_adapters[i];
		for each (auto item in m_usages) {
			auto gpuUsage = item.second->gpus[i];
			if (gpuUsage->GpuRunningTimeDelta.Delta == 0)
				gpuUsage->usage = 0.0f;
			else
				gpuUsage->usage = (float)(gpuUsage->GpuRunningTimeDelta.Delta / (elapsedTime * gpuUsage->activeNodeCount));
			if (gpuUsage->usage > 1)
				gpuUsage->usage = 1;
		}
	}
}

const std::shared_ptr<ProcessGPUsageData> ProcessGPUsage::getUsage(DWORD pid) const
{
	auto iter = m_usages.find(pid);
	if (iter == m_usages.end())
		return nullptr;
	return iter->second;
}

std::vector<std::shared_ptr<TotalGPUsageData>> ProcessGPUsage::getTotalUsage() const
{
	std::vector<std::shared_ptr<TotalGPUsageData>> usages;
	auto dynamicUsages = m_usages.find(0)->second->gpus;
	for (size_t i = 0; i < m_adapters.size(); ++i) {
		auto usage = std::make_shared<TotalGPUsageData>();

		auto adapter = m_adapters[i];
		usage->name = adapter->description;
		usage->dedicatedLimit = adapter->dedicatedLimit;
		usage->sharedLimit = adapter->sharedLimit;

		auto dynamicUsage = dynamicUsages[i];
		usage->usage = dynamicUsage->usage;
		usage->dedicatedUsage = dynamicUsage->dedicatedUsage;
		usage->sharedUsage = dynamicUsage->sharedUsage;
		
		usages.push_back(usage);
	}
	return usages;
}

bool ProcessGPUsage::initializeD3DStatistics()
{
	HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&GUID_DISPLAY_DEVICE_ARRIVAL_I, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (deviceInfoSet == NULL)
		return false;

	ULONG memberIndex = 0;
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	while (SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &GUID_DISPLAY_DEVICE_ARRIVAL_I, memberIndex, &deviceInterfaceData)) {
		ULONG detailDataSize = 0x100;
		PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(detailDataSize);
		detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		SP_DEVINFO_DATA deviceInfoData;
		deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		BOOL result;
		if (!(result = SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, detailData, detailDataSize, &detailDataSize, &deviceInfoData)) &&
			GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
			free(detailData);
			detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(detailDataSize);

			if (detailDataSize >= sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA))
				detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

			result = SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, detailData, detailDataSize, &detailDataSize, &deviceInfoData);
		}

		if (result) {
			D3DKMT_OPENADAPTERFROMDEVICENAME openAdapterFromDeviceName;
			openAdapterFromDeviceName.pDeviceName = detailData->DevicePath;

			if (NT_SUCCESS(D3DKMTOpenAdapterFromDeviceName(&openAdapterFromDeviceName))) {
				D3DKMT_QUERYSTATISTICS queryStatistics;
				memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
				queryStatistics.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
				queryStatistics.AdapterLuid = openAdapterFromDeviceName.AdapterLuid;

				if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
				{
					auto gpuAdapter = std::make_shared<GPUAdapter>();
					gpuAdapter->adapterLuid = openAdapterFromDeviceName.AdapterLuid;
					gpuAdapter->description = QueryDeviceDescription(deviceInfoSet, &deviceInfoData);
					gpuAdapter->nodeCount = queryStatistics.QueryResult.AdapterInformation.NodeCount;
					gpuAdapter->segmentCount = queryStatistics.QueryResult.AdapterInformation.NbSegments;
					gpuAdapter->apertureBitSet.resize(gpuAdapter->segmentCount);

					for (uint32_t i = 0; i < gpuAdapter->segmentCount; i++)
					{
						memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
						queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
						queryStatistics.AdapterLuid = gpuAdapter->adapterLuid;
						queryStatistics.QuerySegment.SegmentId = i;

						if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
						{
							ULONG64 commitLimit;
							ULONG aperture;

							if (IsWindows8OrGreater())
							{
								commitLimit = queryStatistics.QueryResult.SegmentInformation.CommitLimit;
								aperture = queryStatistics.QueryResult.SegmentInformation.Aperture;
							}
							else
							{
								commitLimit = queryStatistics.QueryResult.SegmentInformationV1.CommitLimit;
								aperture = queryStatistics.QueryResult.SegmentInformationV1.Aperture;
							}

							if (aperture)
								gpuAdapter->sharedLimit += commitLimit;
							else
								gpuAdapter->dedicatedLimit += commitLimit;

							if (aperture)
								gpuAdapter->apertureBitSet[i] = true;
						}
					}

					if (gpuAdapter->description != L"Microsoft Basic Render Driver")
						m_adapters.push_back(gpuAdapter);
				}
			}
		}

		free(detailData);
		memberIndex++;
	}
	
	SetupDiDestroyDeviceInfoList(deviceInfoSet);
	return true;
}

void ProcessGPUsage::recordSegmentInformation(DWORD pid)
{
	bool recordProcess = pid != 0;
	HANDLE hProcess = NULL;

	if (recordProcess) {
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
		if (hProcess == NULL)
			return;
	}
	
	auto iter = m_usages.find(pid);
	if (iter == m_usages.end())
		return;
	auto usage = iter->second;

	for (size_t i = 0; i < m_adapters.size(); ++i) {
		auto gpuAdapter = m_adapters[i];
		auto gpuUsage = usage->gpus[i];
		gpuUsage->sharedUsage = 0ull;
		gpuUsage->dedicatedUsage = 0ull;

		for (uint32_t j = 0; j < gpuAdapter->segmentCount; ++j) {
			D3DKMT_QUERYSTATISTICS queryStatistics;
			memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));

			if (recordProcess)
				queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT;
			else
				queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;

			queryStatistics.AdapterLuid = gpuAdapter->adapterLuid;

			if (recordProcess)
			{
				queryStatistics.hProcess = hProcess;
				queryStatistics.QueryProcessSegment.SegmentId = j;
			}
			else
			{
				queryStatistics.QuerySegment.SegmentId = j;
			}

			if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
			{
				if (recordProcess)
				{
					ULONG64 bytesCommitted;

					if (IsWindows8OrGreater())
					{
						bytesCommitted = queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
					}
					else
					{
						bytesCommitted = (ULONG)queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
					}

					if (gpuAdapter->apertureBitSet[j])
						gpuUsage->sharedUsage += bytesCommitted;
					else
						gpuUsage->dedicatedUsage += bytesCommitted;
				}
				else
				{
					ULONG64 bytesCommitted;

					if (IsWindows8OrGreater())
					{
						bytesCommitted = queryStatistics.QueryResult.SegmentInformation.BytesCommitted;
					}
					else
					{
						bytesCommitted = queryStatistics.QueryResult.SegmentInformationV1.BytesCommitted;
					}

					if (gpuAdapter->apertureBitSet[j])
						gpuUsage->sharedUsage += bytesCommitted;
					else
						gpuUsage->dedicatedUsage += bytesCommitted;
				}
			}
		}
	}

	if (hProcess != NULL)
		CloseHandle(hProcess);
}

void ProcessGPUsage::recordNodeInformation(DWORD pid)
{
	bool recordProcess = pid != 0;
	HANDLE hProcess = NULL;

	if (recordProcess) {
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
		if (hProcess == NULL)
			return;
	}

	auto iter = m_usages.find(pid);
	if (iter == m_usages.end())
		return;
	auto usage = iter->second;

	for (size_t i = 0; i < m_adapters.size(); ++i) {
		auto gpuAdapter = m_adapters[i];
		auto gpuUsage = usage->gpus[i];
		gpuUsage->totalRunningTime = 0ull;
		gpuUsage->activeNodeCount = 0u;

		for (uint32_t j = 0; j < gpuAdapter->nodeCount; ++j) {
			D3DKMT_QUERYSTATISTICS queryStatistics;
			memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));

			if (recordProcess)
				queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_NODE;
			else
				queryStatistics.Type = D3DKMT_QUERYSTATISTICS_NODE;

			queryStatistics.AdapterLuid = gpuAdapter->adapterLuid;

			if (recordProcess)
			{
				queryStatistics.hProcess = hProcess;
				queryStatistics.QueryProcessNode.NodeId = j;
			}
			else
			{
				queryStatistics.QueryNode.NodeId = j;
			}

			if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
			{
				LONGLONG runningTime = 0ll;
				if (recordProcess)
				{
					runningTime = queryStatistics.QueryResult.ProcessNodeInformation.RunningTime.QuadPart;
				}
				else
				{
					runningTime = queryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
				}

				if (runningTime != 0) {
					gpuUsage->totalRunningTime += runningTime;
					gpuUsage->activeNodeCount++;
				}
			}
		}

		PhUpdateDelta(&gpuUsage->GpuRunningTimeDelta, gpuUsage->totalRunningTime);
	}

	if (!recordProcess) {
		LARGE_INTEGER performanceCounter;

		NtQueryPerformanceCounter(&performanceCounter, &EtClockTotalRunningTimeFrequency);
		PhUpdateDelta(&EtClockTotalRunningTimeDelta, performanceCounter.QuadPart);
	}

	if (hProcess != NULL)
		CloseHandle(hProcess);
}
