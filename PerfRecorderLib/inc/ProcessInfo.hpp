#pragma once

void PhpInit();

struct PH_UINT64_DELTA;
class ProcessCpuUsage
{
public:
	ProcessCpuUsage(DWORD processId);
	~ProcessCpuUsage();
	float getUsage();
private:
	ULONG64 sysTotalTime;
	PH_UINT64_DELTA *CpuKernelDelta;
	PH_UINT64_DELTA *CpuUserDelta;

	DWORD m_processId;
};
