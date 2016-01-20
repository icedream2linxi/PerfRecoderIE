#pragma once

void PhpInit();

typedef struct _PH_UINT64_DELTA
{
	ULONG64 Value;
	ULONG64 Delta;
} PH_UINT64_DELTA, *PPH_UINT64_DELTA;

class ProcessCpuUsage
{
public:
	ProcessCpuUsage(DWORD processId);
	float getUsage();
private:
	ULONG64 sysTotalTime;
	PH_UINT64_DELTA CpuKernelDelta;
	PH_UINT64_DELTA CpuUserDelta;

	DWORD m_processId;
};
