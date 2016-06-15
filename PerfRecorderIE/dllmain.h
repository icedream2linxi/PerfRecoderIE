// dllmain.h : Declaration of module class.

class CPerfRecorderIEModule : public ATL::CAtlDllModuleT< CPerfRecorderIEModule >
{
public :
	DECLARE_LIBID(LIBID_PerfRecorderIELib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_PERFRECORDERIE, "{D6CDFA11-CE11-48BE-9371-FD6899D7A1A1}")
};

extern class CPerfRecorderIEModule _AtlModule;
