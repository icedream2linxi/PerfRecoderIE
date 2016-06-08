#include "stdafx.h"
#include "ProcessResourceUsage.hpp"
#include <memory>
#include "CpuUsage.hpp"

ResourceUsage::ResourceUsage()
	: CpuKernelDelta(new PH_UINT64_DELTA)
	, CpuUserDelta(new PH_UINT64_DELTA)
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
	PhpInit();
}

ProcessResourceUsage::~ProcessResourceUsage()
{
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
}

void ProcessResourceUsage::addProcess(DWORD processId)
{
	std::shared_ptr<ResourceUsage> usage(new ResourceUsage);
	usage->processId = processId;
	m_resourceUsages.insert(std::make_pair(processId, usage));
}

void ProcessResourceUsage::recordCpuUsage()
{
	PhpUpdateCpuInformation(TRUE, &sysTotalTime);

	PVOID processes;
	PSYSTEM_PROCESS_INFORMATION process;
	if (!NT_SUCCESS(PhEnumProcesses(&processes)))
		return;

	process = PH_FIRST_PROCESS(processes);
	do {
		DWORD processId = (DWORD)process->UniqueProcessId;
		auto iter = m_resourceUsages.find(processId);
		if (iter != m_resourceUsages.end()) {
			auto &usage = *iter->second;

			PhUpdateDelta(usage.CpuKernelDelta, process->KernelTime.QuadPart);
			PhUpdateDelta(usage.CpuUserDelta, process->UserTime.QuadPart);

			FLOAT newCpuUsage;
			FLOAT kernelCpuUsage;
			FLOAT userCpuUsage;
			kernelCpuUsage = (FLOAT)usage.CpuKernelDelta->Delta / sysTotalTime;
			userCpuUsage = (FLOAT)usage.CpuUserDelta->Delta / sysTotalTime;
			newCpuUsage = kernelCpuUsage + userCpuUsage;
			usage.cpuUsage = newCpuUsage * 100.0f;
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

}
