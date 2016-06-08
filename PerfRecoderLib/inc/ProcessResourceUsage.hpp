#pragma once
#include <map>
#include <memory>

struct PH_UINT64_DELTA;

struct ResourceUsage
{
	DWORD processId;
	float cpuUsage;

	PH_UINT64_DELTA *CpuKernelDelta;
	PH_UINT64_DELTA *CpuUserDelta;

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

private:
	void recordCpuUsage();
	void recordNetworkUsage();

private:
	ULONG64 sysTotalTime;

	std::map<DWORD, std::shared_ptr<ResourceUsage>> m_resourceUsages;
};
