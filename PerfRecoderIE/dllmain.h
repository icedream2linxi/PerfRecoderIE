// dllmain.h : Declaration of module class.

class CPerfRecoderIEModule : public ATL::CAtlDllModuleT< CPerfRecoderIEModule >
{
public :
	DECLARE_LIBID(LIBID_PerfRecoderIELib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_PERFRECODERIE, "{D6CDFA11-CE11-48BE-9371-FD6899D7A1A1}")
};

extern class CPerfRecoderIEModule _AtlModule;
