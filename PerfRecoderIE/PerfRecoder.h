// PerfRecoder.h : Declaration of the CPerfRecoder

#pragma once
#include "resource.h"       // main symbols

#include <thread>
#include <mutex>
#include <nvapi.h>
#include <memory>
#include <vector>

#include "PerfRecoderIE_i.h"
#include "_IPerfRecoderEvents_CP.h"
#include "ProcessInfo.hpp"
#include "PcapSource.h"
#include "PortCache.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;

struct NetTrans
{
	uint64_t recv;
	uint64_t send;
	NetTrans() : recv(0), send(0) {}
	NetTrans &operator+=(const NetTrans &oth)
	{
		recv += oth.recv;
		send += oth.send;
		return *this;
	}
	NetTrans operator+(const NetTrans &oth)
	{
		NetTrans nt;
		nt.recv = recv + oth.recv;
		nt.send = send + oth.send;
		return nt;
	}
};

// CPerfRecoder

class ATL_NO_VTABLE CPerfRecoder :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPerfRecoder, &CLSID_PerfRecoder>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CPerfRecoder>,
	public CProxy_IPerfRecoderEvents<CPerfRecoder>,
	public IObjectWithSiteImpl<CPerfRecoder>,
	public IObjectSafetyImpl<CPerfRecoder, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>,
	public IDispatchImpl<IPerfRecoder, &IID_IPerfRecoder, &LIBID_PerfRecoderIELib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
private:
	NvPhysicalGpuHandle m_hPhysicalGpu[NVAPI_MAX_PHYSICAL_GPUS];
	NvU32 m_physicalGpuCount;
	std::thread m_thread;
	bool m_stop;
	DWORD m_processId;
	ULONGLONG m_sysNoIdleTime;
	ULONGLONG m_processNoIdleTime;
	float m_cpuUsage;
	std::shared_ptr<ProcessCpuUsage> m_cu;

	PcapSource m_pcap;
	PortCache m_pc;
	std::vector<std::wstring> m_ifNames;
	long m_ifIdx;
	bool m_reconnect;
	std::thread m_netTransThread;
	std::vector<NetTrans> m_nts;
	std::mutex m_ntsMutex;
	uint64_t m_recvSpeed;
	uint64_t m_sendSpeed;
	double m_recvPrevTime;
	double m_sendPrevTime;

public:
	CPerfRecoder();

DECLARE_REGISTRY_RESOURCEID(IDR_PERFRECODER)


BEGIN_COM_MAP(CPerfRecoder)
	COM_INTERFACE_ENTRY(IPerfRecoder)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafety)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CPerfRecoder)
	CONNECTION_POINT_ENTRY(__uuidof(_IPerfRecoderEvents))
END_CONNECTION_POINT_MAP()
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct();

	void FinalRelease();
	void SetErrMsg(BSTR errMsg);
	void SetNvErr(NvAPI_Status status);
	void run();
	NvAPI_Status getGPUMemoryInfo(NvPhysicalGpuHandle hPhysicalHandle, NV_DISPLAY_DRIVER_MEMORY_INFO &memoryInfo);
	float getProcessCPUUsage();
	struct MemoryInfo
	{
		uint64_t workingSetSize;
		uint64_t pagefileUsage;
	};
	MemoryInfo getProcessMemoryInfo();

public:



	STDMETHOD(getPhysicalGPUIds)(BSTR* ids);
	STDMETHOD(getGPUFullName)(LONG id, BSTR* name);
	STDMETHOD(getGPUUsage)(LONG id, LONG* usages);
	STDMETHOD(getGPUMemoryInfo)(LONG id, BSTR* info);
	STDMETHOD(getAllProcessInfo)(BSTR* info);
	STDMETHOD(monitoringProcess)(ULONG pid);
	STDMETHOD(getProcessCPUUsage)(FLOAT* usage);
	STDMETHOD(getProcessMemoryInfo)(BSTR* usage);
	STDMETHOD(getIfAdapters)(BSTR* adapters);
	STDMETHOD(selectIfAdapter)(SHORT idx);
	STDMETHOD(getNetTransSpeed)(BSTR* netTransSpeed);
};

OBJECT_ENTRY_AUTO(__uuidof(PerfRecoder), CPerfRecoder)
