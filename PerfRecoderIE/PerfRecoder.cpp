// PerfRecoder.cpp : Implementation of CPerfRecoder

#include "stdafx.h"
#include "PerfRecoder.h"
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <comutil.h>
#include <shlobj.h>
#include <TlHelp32.h>
#include <psapi.h>
#include <boost/algorithm/string.hpp>
#include <boost/scope_exit.hpp>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
using namespace std;
using namespace RAPIDJSON_NAMESPACE;

typedef void* (*nvapi_QueryInterface_t)(unsigned int offset);
typedef NvAPI_Status(*NvAPI_GPU_GetUsages_t)(NvPhysicalGpuHandle hPhysicalGpu, unsigned int *usages);

nvapi_QueryInterface_t nvapi_QueryInterface = NULL;
NvAPI_GPU_GetUsages_t NvAPI_GPU_GetUsages = NULL;

#define NVAPI_MAX_USAGES_PER_GPU  34

#define TOULL(ft) (*(ULONGLONG*)(void*)&ft)

CPerfRecoder::CPerfRecoder()
	: m_physicalGpuCount(0)
	, m_stop(false)
	, m_processId(0)
	, m_sysNoIdleTime(0)
	, m_processNoIdleTime(0)
	, m_cpuUsage(0)
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

	PhpInit();

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

wchar_t *GetCurrTime()
{
	auto currTime = time(NULL);
	tm currTm;
	localtime_s(&currTm, &currTime);

	static wchar_t buff[128] = { 0 };
	wcsftime(buff, 128, L"%Y-%m-%d %H-%M-%S", &currTm);
	return buff;
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
	wcscat_s(filepath, L"\\perf ");
	wcscat_s(filepath, GetCurrTime());
	wcscat_s(filepath, L".csv");
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
	fout << "CPU,WorkingSetSize,PagefileUsage" << endl;

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
			LONG gpuUsage = 0;
			getGPUUsage(i, &gpuUsage);
			NV_DISPLAY_DRIVER_MEMORY_INFO memoryInfo = { 0 };
			res = getGPUMemoryInfo(hPhysicalHandle, memoryInfo);

			fout << gpuUsage << ","
				<< memoryInfo.dedicatedVideoMemory << ","
				<< memoryInfo.availableDedicatedVideoMemory << ","
				<< memoryInfo.systemVideoMemory << ","
				<< memoryInfo.sharedSystemMemory << ","
				<< memoryInfo.curAvailableDedicatedVideoMemory << ","
				<< memoryInfo.dedicatedVideoMemoryEvictionsSize << ","
				<< memoryInfo.dedicatedVideoMemoryEvictionCount << ",";
		}

		if (m_processId == 0)
			fout << 0 << "," << 0 << "," << 0 << endl;
		else
		{
			//FLOAT cpuUsage = getProcessCPUUsage();
			if (m_cu != nullptr)
				m_cpuUsage = m_cu->getUsage();
			MemoryInfo info = getProcessMemoryInfo();
			fout << m_cpuUsage << ","
				<< info.workingSetSize << ","
				<< info.pagefileUsage << endl;
		}
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


STDMETHODIMP CPerfRecoder::getGPUUsage(LONG id, LONG* usages)
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


STDMETHODIMP CPerfRecoder::getAllProcessInfo(BSTR* info)
{
	HANDLE hProcessSnap = NULL;
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return E_FAIL;
	PROCESSENTRY32 pe32 = { 0 };
	pe32.dwSize = sizeof(pe32);
	if (!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);
		return E_FAIL;
	}

	struct Info
	{
		DWORD pid;
		DWORD ppid;
		string name;
		bool operator<(const Info &oth) const {
			//return name < oth.name;
			return _stricmp(name.c_str(), oth.name.c_str()) < 0;
		}
	};
	vector<Info> infos;

	DWORD pid = GetCurrentProcessId();
	wchar_t processName[MAX_PATH];

	do {
		Info info;
		info.pid = pe32.th32ProcessID;
		if (info.pid == pid)
			continue;

		info.name = static_cast<char*>(CW2A(pe32.szExeFile));
		if (!boost::iequals(info.name, "iexplore.exe"))
			continue;

		info.ppid = pe32.th32ParentProcessID;
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, info.ppid);
		if (hProcess == NULL)
			continue;
		BOOST_SCOPE_EXIT((hProcess)) {
			CloseHandle(hProcess);
		} BOOST_SCOPE_EXIT_END;

		DWORD dwSize = MAX_PATH;
		QueryFullProcessImageName(hProcess, 0, processName, &dwSize);
		//if (_wcsicmp(processName, L"iexplore.exe") != 0)
		if (!boost::iends_with(processName, L"\\iexplore.exe"))
			continue;

		infos.push_back(info);
	} while (Process32Next(hProcessSnap, &pe32));
	CloseHandle(hProcessSnap);

	sort(infos.begin(), infos.end());

	StringBuffer sb;
	Writer<StringBuffer> writer(sb);
	writer.StartArray();
	for (size_t i = 0; i < infos.size(); ++i)
	{
		writer.StartObject();

		writer.Key("pid");
		writer.Uint(infos[i].pid);

		writer.Key("ppid");
		writer.Uint(infos[i].ppid);

		writer.Key("name");
		writer.String(infos[i].name.c_str());

		writer.EndObject();
	}
	writer.EndArray();
	*info = _com_util::ConvertStringToBSTR(sb.GetString());
	return S_OK;
}


STDMETHODIMP CPerfRecoder::monitoringProcess(ULONG pid)
{
	m_processId = pid;
	m_sysNoIdleTime = 0;
	m_processNoIdleTime = 0;
	m_cu.reset(new ProcessCpuUsage(pid));
	return S_OK;
}


STDMETHODIMP CPerfRecoder::getProcessCPUUsage(FLOAT* usage)
{
	*usage = m_cpuUsage;
	return S_OK;
}


float CPerfRecoder::getProcessCPUUsage()
{
	FILETIME kernelTime, userTime, idleTime;
	GetSystemTimes(&idleTime, &kernelTime, &userTime);

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, m_processId);
	FILETIME processCreateTime, processExitTime, processKernelTime, processUserTime;
	GetProcessTimes(hProcess, &processCreateTime, &processExitTime,
		&processKernelTime, &processUserTime);
	CloseHandle(hProcess);

	ULONGLONG totalTime = TOULL(kernelTime) + TOULL(userTime) + TOULL(idleTime);
	ULONGLONG processTotalTime = TOULL(processKernelTime) + TOULL(processUserTime);
	if (m_sysNoIdleTime == 0)
	{
		m_sysNoIdleTime = totalTime;
		m_processNoIdleTime = processTotalTime;
		return 0;
	}

	m_cpuUsage = (processTotalTime - m_processNoIdleTime) * 100.0 / (totalTime - m_sysNoIdleTime);
	m_sysNoIdleTime = totalTime;
	m_processNoIdleTime = processTotalTime;
	return m_cpuUsage;
}

STDMETHODIMP CPerfRecoder::getProcessMemoryInfo(BSTR* usage)
{
	MemoryInfo info = getProcessMemoryInfo();

	StringBuffer sb;
	Writer<StringBuffer> writer(sb);
	writer.StartObject();
	
	writer.Key("workingSetSize");
	writer.Uint64(info.workingSetSize);

	writer.Key("pagefileUsage");
	writer.Uint64(info.pagefileUsage);

	writer.EndObject();

	*usage = _com_util::ConvertStringToBSTR(sb.GetString());
	return S_OK;
}

CPerfRecoder::MemoryInfo CPerfRecoder::getProcessMemoryInfo()
{
	PROCESS_MEMORY_COUNTERS_EX mem;

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, m_processId);
	GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&mem, sizeof(PROCESS_MEMORY_COUNTERS_EX));

	MemoryInfo info;
	info.workingSetSize = mem.WorkingSetSize;
	info.pagefileUsage = mem.PagefileUsage;

	CloseHandle(hProcess);
	return info;
}

