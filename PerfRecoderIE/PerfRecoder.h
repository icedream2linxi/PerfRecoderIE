// PerfRecoder.h : Declaration of the CPerfRecoder

#pragma once
#include "resource.h"       // main symbols

#include <thread>
#include <nvapi.h>
#include <memory>

#include "PerfRecoderIE_i.h"
#include "_IPerfRecoderEvents_CP.h"
#include "ProcessInfo.hpp"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


// CPerfRecoder

class ATL_NO_VTABLE CPerfRecoder :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPerfRecoder, &CLSID_PerfRecoder>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CPerfRecoder>,
	public CProxy_IPerfRecoderEvents<CPerfRecoder>,
	public IObjectWithSiteImpl<CPerfRecoder>,
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
public:
	CPerfRecoder();

DECLARE_REGISTRY_RESOURCEID(IDR_PERFRECODER)


BEGIN_COM_MAP(CPerfRecoder)
	COM_INTERFACE_ENTRY(IPerfRecoder)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IObjectWithSite)
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

public:



	STDMETHOD(getPhysicalGPUIds)(BSTR* ids);
	STDMETHOD(getGPUFullName)(LONG id, BSTR* name);
	STDMETHOD(getGPUUsage)(LONG id, LONG* usages);
	STDMETHOD(getGPUMemoryInfo)(LONG id, BSTR* info);
	STDMETHOD(getAllProcessInfo)(BSTR* info);
	STDMETHOD(monitoringProcess)(ULONG pid);
	STDMETHOD(getProcessCPUUsage)(FLOAT* usage);
	STDMETHOD(getProcessMemoryInfo)(ULONG* usage);
};

OBJECT_ENTRY_AUTO(__uuidof(PerfRecoder), CPerfRecoder)
