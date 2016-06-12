#pragma once
#include <nvapi.h>
#include "GPUResourceUsage.hpp"

class NvidiaGPUResourceUsage : public GPUResourceUsageBase
{
public:
	NvidiaGPUResourceUsage();
	~NvidiaGPUResourceUsage();
	virtual std::vector<std::shared_ptr<GPUsage>> getUsages() override;

private:
	NvAPI_Status getMemoryInfo(NvPhysicalGpuHandle hPhysicalHandle, NV_DISPLAY_DRIVER_MEMORY_INFO &memoryInfo);

private:
	HMODULE m_hNvapi;
	NvPhysicalGpuHandle m_hPhysicalGpu[NVAPI_MAX_PHYSICAL_GPUS];
	NvU32 m_physicalGpuCount;
	std::vector<std::shared_ptr<GPUsage>> m_usages;

	typedef void* (*NvAPI_QueryInterface_t)(unsigned int offset);
	typedef NvAPI_Status(*NvAPI_GPU_GetUsages_t)(NvPhysicalGpuHandle hPhysicalGpu, unsigned int *usages);

	NvAPI_QueryInterface_t NvAPI_QueryInterface;
	NvAPI_GPU_GetUsages_t NvAPI_GPU_GetUsages;
};