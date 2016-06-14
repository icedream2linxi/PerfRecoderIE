#include "stdafx.h"
#include "AMDGPUResourceUsage.hpp"

// Memory allocation function
void* __stdcall ADL_Main_Memory_Alloc(int iSize)
{
	void* lpBuffer = malloc(iSize);
	return lpBuffer;
}

// Optional Memory de-allocation function
void __stdcall ADL_Main_Memory_Free(void* lpBuffer)
{
	if (NULL != lpBuffer)
	{
		free(lpBuffer);
		lpBuffer = NULL;
	}
}

AMDGPUResourceUsage::AMDGPUResourceUsage()
{
	m_hAdl = LoadLibraryA("atiadlxx.dll");
	if (m_hAdl == NULL)
		m_hAdl = LoadLibraryA("atiadlxy.dll");
	if (m_hAdl == NULL)
		return;

	ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE)GetProcAddress(m_hAdl, "ADL_Main_Control_Create");
	ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY)GetProcAddress(m_hAdl, "ADL_Main_Control_Destroy");
	ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress(m_hAdl, "ADL_Adapter_NumberOfAdapters_Get");
	ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET)GetProcAddress(m_hAdl, "ADL_Adapter_AdapterInfo_Get");
	ADL_Adapter_MemoryInfo_Get = (ADL_ADAPTER_MEMORYINFO_GET)GetProcAddress(m_hAdl, "ADL_Adapter_MemoryInfo_Get");
	ADL_Overdrive5_CurrentActivity_Get = (ADL_OVERDRIVE5_CURRENTACTIVITY_GET)GetProcAddress(m_hAdl, "ADL_Overdrive5_CurrentActivity_Get");

	if (ADL_Main_Control_Create(&ADL_Main_Memory_Alloc, 0) != ADL_OK)
		return;
	if (ADL_Adapter_NumberOfAdapters_Get(&m_numberAdapters) != ADL_OK)
		return;

	AdapterInfo *lpAdapterInfo = (AdapterInfo*)malloc(sizeof(AdapterInfo) * m_numberAdapters);
	memset(lpAdapterInfo, 0, sizeof(AdapterInfo) * m_numberAdapters);
	ADL_Adapter_AdapterInfo_Get(lpAdapterInfo, sizeof(AdapterInfo) * m_numberAdapters);

	for (int i = 0; i < m_numberAdapters; ++i) {
		auto usage = std::make_shared<GPUResourceUsageData>();
		usage->name = (lpAdapterInfo + i)->strAdapterName;

		ADLMemoryInfo memoryInfo;
		ADL_Adapter_MemoryInfo_Get(i, &memoryInfo);
		usage->totalMemory = memoryInfo.iMemorySize;
	}

	ADL_Main_Memory_Free(lpAdapterInfo);
}

AMDGPUResourceUsage::~AMDGPUResourceUsage()
{
	if (m_hAdl != NULL) {
		ADL_Main_Control_Destroy();
		FreeLibrary(m_hAdl);
		m_hAdl = NULL;
	}
}

std::vector<std::shared_ptr<GPUResourceUsageData>> AMDGPUResourceUsage::getUsages()
{
	if (m_hAdl == NULL)
		return m_usages;
	
	for (int i = 0; i < m_numberAdapters; ++i) {
		auto usage = m_usages[i];

		ADLPMActivity activity;
		ADL_Overdrive5_CurrentActivity_Get(i, &activity);
		usage->gpuUsage = activity.iActivityPercent;
	}
	return m_usages;
}
