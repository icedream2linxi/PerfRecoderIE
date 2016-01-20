// PerfRecoder.cpp : Implementation of CPerfRecoder

#include "stdafx.h"
#include "PerfRecoder.h"
#include <fstream>
#include <comutil.h>
#include <shlobj.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
using namespace std;
using namespace RAPIDJSON_NAMESPACE;

typedef void* (*nvapi_QueryInterface_t)(unsigned int offset);
typedef NvAPI_Status(*NvAPI_GPU_GetUsages_t)(NvPhysicalGpuHandle hPhysicalGpu, unsigned int *usages);

nvapi_QueryInterface_t nvapi_QueryInterface = NULL;
NvAPI_GPU_GetUsages_t NvAPI_GPU_GetUsages = NULL;

#define NVAPI_MAX_USAGES_PER_GPU  34

CPerfRecoder::CPerfRecoder()
	: m_physicalGpuCount(0)
	, m_stop(false)
{
	memset(m_hPhysicalGpu, 0, sizeof(m_hPhysicalGpu));
}

// CPerfRecoder

STDMETHODIMP CPerfRecoder::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_IPerfRecoder
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


HRESULT CPerfRecoder::FinalConstruct()
{
	NvAPI_Status res = NvAPI_Initialize();
	if (res != NVAPI_OK)
	{
		SetNvErr(res);
		return E_FAIL;
	}

	HMODULE hDll = LoadLibrary(
#ifdef _WIN64
		L"nvapi64.dll"
#else
		L"nvapi.dll"
#endif
		);
	nvapi_QueryInterface = (nvapi_QueryInterface_t)GetProcAddress(hDll, "nvapi_QueryInterface");
	NvAPI_GPU_GetUsages = (NvAPI_GPU_GetUsages_t)nvapi_QueryInterface(0x189A1FDF);

	m_thread = std::thread(&CPerfRecoder::run, this);

	return S_OK;
}

void CPerfRecoder::FinalRelease()
{
	m_stop = true;
	m_thread.join();
}

void CPerfRecoder::SetErrMsg(BSTR errMsg)
{
	CComPtr<ICreateErrorInfo> spCreateErrInfo;
	CreateErrorInfo(&spCreateErrInfo);
	spCreateErrInfo->SetDescription(errMsg);
	CComPtr<IErrorInfo> spErrInfo;
	spCreateErrInfo->QueryInterface(IID_IErrorInfo, (void**)&spErrInfo);
	SetErrorInfo(0, spErrInfo);
}

void CPerfRecoder::SetNvErr(NvAPI_Status status)
{
	NvAPI_ShortString szErrMsg;
	NvAPI_GetErrorMessage(status, szErrMsg);
	SetErrMsg(_com_util::ConvertStringToBSTR(szErrMsg));
}

void CPerfRecoder::run()
{
	if (m_physicalGpuCount == 0)
	{
		NvAPI_Status res = NvAPI_EnumPhysicalGPUs(m_hPhysicalGpu, &m_physicalGpuCount);
		if (res != NVAPI_OK)
		{
			SetNvErr(res);
			return ;
		}
	}

	wchar_t filepath[MAX_PATH];
	SHGetSpecialFolderPath(0, filepath, CSIDL_DESKTOPDIRECTORY, 0);
	wcscat_s(filepath, L"\\perf.csv");
	ofstream fout(filepath);

	for (NvU32 i = 0; i < m_physicalGpuCount; ++i)
	{
		NvAPI_ShortString szName;
		NvAPI_GPU_GetFullName(m_hPhysicalGpu[i], szName);
		fout << "GPU #" << i << " : " << szName << endl;
	}

	fout << "time,";
	for (NvU32 i = 0; i < m_physicalGpuCount; ++i)
	{
		fout << "GPU #" << i << " Usages,"
			<< "GPU #" << i << " DedicatedVideoMemory,"
			<< "GPU #" << i << " AvailableDedicatedVideoMemory,"
			<< "GPU #" << i << " SystemVideoMemory,"
			<< "GPU #" << i << " SharedSystemMemory,"
			<< "GPU #" << i << " CurAvailableDedicatedVideoMemory,"
			<< "GPU #" << i << " DedicatedVideoMemoryEvictionsSize,"
			<< "GPU #" << i << " DedicatedVideoMemoryEvictionCount,";
	}
	fout << "CPU,Memory" << endl;

	clock_t prevClock = clock();
	while (!m_stop)
	{
		clock_t curClock = clock();
		std::this_thread::sleep_for(std::chrono::milliseconds(500 - (curClock - prevClock)));

		prevClock = clock();
		fout << prevClock << ",";

		NvAPI_Status res = NVAPI_OK;
		for (NvU32 i = 0; i < m_physicalGpuCount; ++i)
		{
			NvPhysicalGpuHandle hPhysicalHandle = m_hPhysicalGpu[i];
			LONG usage = 0;
			getGPUUsages(i, &usage);
			NV_DISPLAY_DRIVER_MEMORY_INFO memoryInfo = { 0 };
			res = getGPUMemoryInfo(hPhysicalHandle, memoryInfo);

			fout << usage << ","
				<< memoryInfo.dedicatedVideoMemory << ","
				<< memoryInfo.availableDedicatedVideoMemory << ","
				<< memoryInfo.systemVideoMemory << ","
				<< memoryInfo.sharedSystemMemory << ","
				<< memoryInfo.curAvailableDedicatedVideoMemory << ","
				<< memoryInfo.dedicatedVideoMemoryEvictionsSize << ","
				<< memoryInfo.dedicatedVideoMemoryEvictionCount << ",";
		}
		fout << 0 << "," << 0 << endl;
	}
}

STDMETHODIMP CPerfRecoder::getPhysicalGPUIds(BSTR* ids)
{
	if (m_physicalGpuCount == 0)
	{
		NvAPI_Status res = NvAPI_EnumPhysicalGPUs(m_hPhysicalGpu, &m_physicalGpuCount);
		if (res != NVAPI_OK)
		{
			SetNvErr(res);
			return E_FAIL;
		}
	}

	StringBuffer sb;
	Writer<StringBuffer> writer(sb);
	writer.StartArray();
	for (NvU32 i = 0; i < m_physicalGpuCount; ++i)
		writer.Uint(i);
	writer.EndArray();
	*ids = _com_util::ConvertStringToBSTR(sb.GetString());
	return S_OK;
}


STDMETHODIMP CPerfRecoder::getGPUFullName(LONG id, BSTR* name)
{
	NvAPI_ShortString szName;
	NvAPI_Status res = NvAPI_GPU_GetFullName(m_hPhysicalGpu[id], szName);
	if (res != NVAPI_OK)
	{
		SetNvErr(res);
		return E_FAIL;
	}
	*name = _com_util::ConvertStringToBSTR(szName);
	return S_OK;
}


STDMETHODIMP CPerfRecoder::getGPUUsages(LONG id, LONG* usages)
{
	unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };
	gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;
	NvAPI_GPU_GetUsages(m_hPhysicalGpu[id], gpuUsages);
	*usages = gpuUsages[3];
	return S_OK;
}


STDMETHODIMP CPerfRecoder::getGPUMemoryInfo(LONG id, BSTR* info)
{
	NvAPI_Status res = NVAPI_OK;
	NvPhysicalGpuHandle hPhysicalHandle = m_hPhysicalGpu[id];
	NV_DISPLAY_DRIVER_MEMORY_INFO memoryInfo = { 0 };
	res = getGPUMemoryInfo(hPhysicalHandle, memoryInfo);

	if (res != NVAPI_OK)
	{
		SetNvErr(res);
		return E_FAIL;
	}

	StringBuffer sb;
	Writer<StringBuffer> writer(sb);
	writer.StartObject();
	switch (memoryInfo.version)
	{
	case NV_DISPLAY_DRIVER_MEMORY_INFO_VER_3:
		writer.Key("dedicatedVideoMemoryEvictionsSize");
		writer.Uint(memoryInfo.dedicatedVideoMemoryEvictionsSize);

		writer.Key("dedicatedVideoMemoryEvictionCount");
		writer.Uint(memoryInfo.dedicatedVideoMemoryEvictionCount);

	case NV_DISPLAY_DRIVER_MEMORY_INFO_VER_2:
		writer.Key("curAvailableDedicatedVideoMemory");
		writer.Uint(memoryInfo.curAvailableDedicatedVideoMemory);

	case NV_DISPLAY_DRIVER_MEMORY_INFO_VER_1:
		writer.Key("dedicatedVideoMemory");
		writer.Uint(memoryInfo.dedicatedVideoMemory);
		
		writer.Key("availableDedicatedVideoMemory");
		writer.Uint(memoryInfo.availableDedicatedVideoMemory);

		writer.Key("systemVideoMemory");
		writer.Uint(memoryInfo.systemVideoMemory);

		writer.Key("sharedSystemMemory");
		writer.Uint(memoryInfo.sharedSystemMemory);

	default:
		break;
	}
	writer.EndObject();
	*info = _com_util::ConvertStringToBSTR(sb.GetString());

	return S_OK;
}

NvAPI_Status CPerfRecoder::getGPUMemoryInfo(NvPhysicalGpuHandle hPhysicalHandle, NV_DISPLAY_DRIVER_MEMORY_INFO &memoryInfo)
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
