#include "stdafx.h"
#include "ProcessInfo.hpp"
#include "CpuUsage.hpp"

ProcessCpuUsage::ProcessCpuUsage(DWORD processId)
	: m_processId(processId)
	, sysTotalTime(0)
	, CpuKernelDelta(new PH_UINT64_DELTA)
	, CpuUserDelta(new PH_UINT64_DELTA)
{
	getUsage();
}

ProcessCpuUsage::~ProcessCpuUsage()
{
	delete CpuKernelDelta;
	delete CpuUserDelta;
}

float ProcessCpuUsage::getUsage()
{
	PhpUpdateCpuInformation(TRUE, &sysTotalTime);

	PVOID processes;
	PSYSTEM_PROCESS_INFORMATION process;
	if (!NT_SUCCESS(PhEnumProcesses(&processes)))
		return 0;

	process = PH_FIRST_PROCESS(processes);
	do {
		if ((DWORD)process->UniqueProcessId == m_processId)
			break;
	} while (process = PH_NEXT_PROCESS(process));

	if (process == NULL)
	{
		PhFree(processes);
		return 0;
	}

	PhUpdateDelta(CpuKernelDelta, process->KernelTime.QuadPart);
	PhUpdateDelta(CpuUserDelta, process->UserTime.QuadPart);

	PhFree(processes);

	FLOAT newCpuUsage;
	FLOAT kernelCpuUsage;
	FLOAT userCpuUsage;
	kernelCpuUsage = (FLOAT)CpuKernelDelta->Delta / sysTotalTime;
	userCpuUsage = (FLOAT)CpuUserDelta->Delta / sysTotalTime;
	newCpuUsage = kernelCpuUsage + userCpuUsage;
	return newCpuUsage * 100.0f;
}
