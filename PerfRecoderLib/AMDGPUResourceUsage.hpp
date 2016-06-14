#pragma once
#include <ADL/adl_sdk.h>
#include "GPUResourceUsage.hpp"

class AMDGPUResourceUsage : public GPUResourceUsageBase
{
public:
	AMDGPUResourceUsage();
	~AMDGPUResourceUsage();
	virtual std::vector<std::shared_ptr<GPUResourceUsageData>> getUsages() override;

private:
	HMODULE m_hAdl;
	int m_numberAdapters;
	std::vector<std::shared_ptr<GPUResourceUsageData>> m_usages;

	typedef int(*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);
	typedef int(*ADL_MAIN_CONTROL_DESTROY)();
	typedef int(*ADL_ADAPTER_NUMBEROFADAPTERS_GET) (int*);
	typedef int(*ADL_ADAPTER_ADAPTERINFO_GET) (LPAdapterInfo, int);
	typedef int(*ADL_ADAPTER_MEMORYINFO_GET)(int iAdapterIndex, ADLMemoryInfo *lpMemoryInfo);
	typedef int(*ADL_OVERDRIVE5_CURRENTACTIVITY_GET)(int, ADLPMActivity*);

	ADL_MAIN_CONTROL_CREATE				ADL_Main_Control_Create;
	ADL_MAIN_CONTROL_DESTROY			ADL_Main_Control_Destroy;
	ADL_ADAPTER_NUMBEROFADAPTERS_GET	ADL_Adapter_NumberOfAdapters_Get;
	ADL_ADAPTER_ADAPTERINFO_GET			ADL_Adapter_AdapterInfo_Get;
	ADL_ADAPTER_MEMORYINFO_GET			ADL_Adapter_MemoryInfo_Get;
	ADL_OVERDRIVE5_CURRENTACTIVITY_GET	ADL_Overdrive5_CurrentActivity_Get;
};
