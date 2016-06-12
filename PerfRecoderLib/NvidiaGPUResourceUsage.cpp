#include "stdafx.h"
#include "NvidiaGPUResourceUsage.hpp"

constexpr NvU32 NVAPI_MAX_USAGES_PER_GPU = 34;

NvidiaGPUResourceUsage::NvidiaGPUResourceUsage()
	: m_hNvapi(NULL)
	, m_physicalGpuCount(0)
{
	memset(m_hPhysicalGpu, 0, sizeof(m_hPhysicalGpu));

	m_hNvapi = LoadLibrary(
#ifdef _WIN64
		L"nvapi64.dll"
#else
		L"nvapi.dll"
#endif
	);

	if (m_hNvapi == NULL)
		return;

	NvAPI_Status res = NvAPI_Initialize();

	NvAPI_QueryInterface = (NvAPI_QueryInterface_t)GetProcAddress(m_hNvapi, "nvapi_QueryInterface");
	NvAPI_GPU_GetUsages = (NvAPI_GPU_GetUsages_t)NvAPI_QueryInterface(0x189A1FDF);

	res = NvAPI_EnumPhysicalGPUs(m_hPhysicalGpu, &m_physicalGpuCount);
	if (res != NVAPI_OK)
		return;

	for (NvU32 i = 0; i < m_physicalGpuCount; ++i) {
		NvAPI_ShortString szName;
		NvAPI_GPU_GetFullName(m_hPhysicalGpu[i], szName);

		auto gpuUsage = std::make_shared<GPUsage>();
		gpuUsage->name = szName;
		m_usages.push_back(gpuUsage);
	}
}

NvidiaGPUResourceUsage::~NvidiaGPUResourceUsage()
{
	if (m_hNvapi != NULL) {
		FreeLibrary(m_hNvapi);
		m_hNvapi = NULL;
	}

	NvAPI_Unload();
}

std::vector<std::shared_ptr<GPUsage>> NvidiaGPUResourceUsage::getUsages()
{
	if (m_hNvapi == NULL)
		return m_usages;

	for (NvU32 i = 0; i < m_physicalGpuCount; ++i) {
		unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };
		gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;
		NvAPI_GPU_GetUsages(m_hPhysicalGpu[i], gpuUsages);
		m_usages[i]->gpuUsage = gpuUsages[3];

		NV_DISPLAY_DRIVER_MEMORY_INFO memoryInfo = { 0 };
		NvAPI_Status res = getMemoryInfo(m_hPhysicalGpu[i], memoryInfo);
		if (res != NVAPI_OK)
			continue;
		m_usages[i]->totalMemory = memoryInfo.dedicatedVideoMemory * 1024;
		m_usages[i]->usedMemory = memoryInfo.version == 1 ?
			(memoryInfo.dedicatedVideoMemory - memoryInfo.availableDedicatedVideoMemory) :
			(memoryInfo.dedicatedVideoMemory - memoryInfo.curAvailableDedicatedVideoMemory);
		m_usages[i]->usedMemory *= 1024;
	}

	return m_usages;
}

NvAPI_Status NvidiaGPUResourceUsage::getMemoryInfo(NvPhysicalGpuHandle hPhysicalHandle, NV_DISPLAY_DRIVER_MEMORY_INFO &memoryInfo)
{
	memoryInfo.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER_3;
	NvAPI_Status res = NvAPI_GPU_GetMemoryInfo(hPhysicalHandle, &memoryInfo);
	if (res != NVAPI_OK)
	{
		if (res != NVAPI_INCOMPATIBLE_STRUCT_VERSION)
		{
			return res;
		}

		memoryInfo.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER_2;
		res = NvAPI_GPU_GetMemoryInfo(hPhysicalHandle, &memoryInfo);
	}

	if (res != NVAPI_OK)
	{
		if (res != NVAPI_INCOMPATIBLE_STRUCT_VERSION)
		{
			return res;
		}

		memoryInfo.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER_1;
		res = NvAPI_GPU_GetMemoryInfo(hPhysicalHandle, &memoryInfo);
	}

	return res;
}