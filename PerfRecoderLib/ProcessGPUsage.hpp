#pragma once
#include <map>
#include <memory>
#include <cstdint>
#include <string>
#include <vector>
#include "d3dkmt.h"
#include "CpuUsage.hpp"
#include "TypeDefine.hpp"

struct ProcessSingleGPUsageData
{
	std::wstring name;
	float usage;
	uint64_t dedicatedUsage;
	uint64_t sharedUsage;

	uint64_t totalRunningTime;
	PH_UINT64_DELTA GpuRunningTimeDelta;
};

struct ProcessGPUsageData
{
	DWORD pid;
	std::vector<std::shared_ptr<ProcessSingleGPUsageData>> gpus;
};

struct GPUAdapter
{
	LUID adapterLuid;
	std::wstring description;
	uint32_t segmentCount;
	uint32_t nodeCount;
	bool hasActivity;
	std::vector<bool> apertureBitSet;

	uint64_t sharedLimit;
	uint64_t dedicatedLimit;

	GPUAdapter();
};

class ProcessGPUsage
{
	ProcessGPUsage();
public:
	~ProcessGPUsage();
	static ProcessGPUsage &getInstance();
	void addProcess(DWORD pid);
	void removeProcess(DWORD pid);
	void record();
	const std::shared_ptr<ProcessGPUsageData> getUsage(DWORD pid) const;
	std::vector<std::shared_ptr<TotalGPUsageData>> getTotalUsage() const;

private:
	bool initializeD3DStatistics();
	void recordSegmentInformation(DWORD pid);
	void recordNodeInformation(DWORD pid);

private:
	HMODULE m_hGdi32;
	std::vector<std::shared_ptr<GPUAdapter>> m_adapters;
	std::map<DWORD, std::shared_ptr<ProcessGPUsageData>> m_usages;

	PH_UINT64_DELTA EtClockTotalRunningTimeDelta;
	LARGE_INTEGER EtClockTotalRunningTimeFrequency;

	PFND3DKMT_QUERYSTATISTICS D3DKMTQueryStatistics;
};
