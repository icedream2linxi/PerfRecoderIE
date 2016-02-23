

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Tue Feb 23 15:41:16 2016
 */
/* Compiler settings for PerfRecoderIE.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.00.0603 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __PerfRecoderIE_i_h__
#define __PerfRecoderIE_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IPerfRecoder_FWD_DEFINED__
#define __IPerfRecoder_FWD_DEFINED__
typedef interface IPerfRecoder IPerfRecoder;

#endif 	/* __IPerfRecoder_FWD_DEFINED__ */


#ifndef ___IPerfRecoderEvents_FWD_DEFINED__
#define ___IPerfRecoderEvents_FWD_DEFINED__
typedef interface _IPerfRecoderEvents _IPerfRecoderEvents;

#endif 	/* ___IPerfRecoderEvents_FWD_DEFINED__ */


#ifndef __PerfRecoder_FWD_DEFINED__
#define __PerfRecoder_FWD_DEFINED__

#ifdef __cplusplus
typedef class PerfRecoder PerfRecoder;
#else
typedef struct PerfRecoder PerfRecoder;
#endif /* __cplusplus */

#endif 	/* __PerfRecoder_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IPerfRecoder_INTERFACE_DEFINED__
#define __IPerfRecoder_INTERFACE_DEFINED__

/* interface IPerfRecoder */
/* [unique][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IPerfRecoder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E4C5E4FF-AE5E-40A9-BF78-0A6776BFB67F")
    IPerfRecoder : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE getPhysicalGPUIds( 
            /* [retval][out] */ BSTR *ids) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE getGPUFullName( 
            /* [in] */ LONG id,
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE getGPUUsage( 
            /* [in] */ LONG id,
            /* [retval][out] */ LONG *usages) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE getGPUMemoryInfo( 
            /* [in] */ LONG id,
            /* [retval][out] */ BSTR *info) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE getAllProcessInfo( 
            /* [retval][out] */ BSTR *info) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE monitoringProcess( 
            /* [in] */ ULONG pid) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE getProcessCPUUsage( 
            /* [retval][out] */ FLOAT *usage) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE getProcessMemoryInfo( 
            /* [retval][out] */ BSTR *usage) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE getIfAdapters( 
            /* [retval][out] */ BSTR *adapters) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE selectIfAdapter( 
            /* [in] */ SHORT idx) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE getNetTransSpeed( 
            /* [retval][out] */ BSTR *netTransSpeed) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IPerfRecoderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPerfRecoder * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPerfRecoder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPerfRecoder * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPerfRecoder * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPerfRecoder * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPerfRecoder * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPerfRecoder * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *getPhysicalGPUIds )( 
            IPerfRecoder * This,
            /* [retval][out] */ BSTR *ids);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *getGPUFullName )( 
            IPerfRecoder * This,
            /* [in] */ LONG id,
            /* [retval][out] */ BSTR *name);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *getGPUUsage )( 
            IPerfRecoder * This,
            /* [in] */ LONG id,
            /* [retval][out] */ LONG *usages);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *getGPUMemoryInfo )( 
            IPerfRecoder * This,
            /* [in] */ LONG id,
            /* [retval][out] */ BSTR *info);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *getAllProcessInfo )( 
            IPerfRecoder * This,
            /* [retval][out] */ BSTR *info);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *monitoringProcess )( 
            IPerfRecoder * This,
            /* [in] */ ULONG pid);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *getProcessCPUUsage )( 
            IPerfRecoder * This,
            /* [retval][out] */ FLOAT *usage);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *getProcessMemoryInfo )( 
            IPerfRecoder * This,
            /* [retval][out] */ BSTR *usage);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *getIfAdapters )( 
            IPerfRecoder * This,
            /* [retval][out] */ BSTR *adapters);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *selectIfAdapter )( 
            IPerfRecoder * This,
            /* [in] */ SHORT idx);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *getNetTransSpeed )( 
            IPerfRecoder * This,
            /* [retval][out] */ BSTR *netTransSpeed);
        
        END_INTERFACE
    } IPerfRecoderVtbl;

    interface IPerfRecoder
    {
        CONST_VTBL struct IPerfRecoderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPerfRecoder_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPerfRecoder_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPerfRecoder_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPerfRecoder_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPerfRecoder_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPerfRecoder_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPerfRecoder_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IPerfRecoder_getPhysicalGPUIds(This,ids)	\
    ( (This)->lpVtbl -> getPhysicalGPUIds(This,ids) ) 

#define IPerfRecoder_getGPUFullName(This,id,name)	\
    ( (This)->lpVtbl -> getGPUFullName(This,id,name) ) 

#define IPerfRecoder_getGPUUsage(This,id,usages)	\
    ( (This)->lpVtbl -> getGPUUsage(This,id,usages) ) 

#define IPerfRecoder_getGPUMemoryInfo(This,id,info)	\
    ( (This)->lpVtbl -> getGPUMemoryInfo(This,id,info) ) 

#define IPerfRecoder_getAllProcessInfo(This,info)	\
    ( (This)->lpVtbl -> getAllProcessInfo(This,info) ) 

#define IPerfRecoder_monitoringProcess(This,pid)	\
    ( (This)->lpVtbl -> monitoringProcess(This,pid) ) 

#define IPerfRecoder_getProcessCPUUsage(This,usage)	\
    ( (This)->lpVtbl -> getProcessCPUUsage(This,usage) ) 

#define IPerfRecoder_getProcessMemoryInfo(This,usage)	\
    ( (This)->lpVtbl -> getProcessMemoryInfo(This,usage) ) 

#define IPerfRecoder_getIfAdapters(This,adapters)	\
    ( (This)->lpVtbl -> getIfAdapters(This,adapters) ) 

#define IPerfRecoder_selectIfAdapter(This,idx)	\
    ( (This)->lpVtbl -> selectIfAdapter(This,idx) ) 

#define IPerfRecoder_getNetTransSpeed(This,netTransSpeed)	\
    ( (This)->lpVtbl -> getNetTransSpeed(This,netTransSpeed) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPerfRecoder_INTERFACE_DEFINED__ */



#ifndef __PerfRecoderIELib_LIBRARY_DEFINED__
#define __PerfRecoderIELib_LIBRARY_DEFINED__

/* library PerfRecoderIELib */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_PerfRecoderIELib;

#ifndef ___IPerfRecoderEvents_DISPINTERFACE_DEFINED__
#define ___IPerfRecoderEvents_DISPINTERFACE_DEFINED__

/* dispinterface _IPerfRecoderEvents */
/* [uuid] */ 


EXTERN_C const IID DIID__IPerfRecoderEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("E2745D8D-7BC1-4D7C-A721-0DBD507A1D43")
    _IPerfRecoderEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IPerfRecoderEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _IPerfRecoderEvents * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _IPerfRecoderEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _IPerfRecoderEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _IPerfRecoderEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _IPerfRecoderEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _IPerfRecoderEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _IPerfRecoderEvents * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        END_INTERFACE
    } _IPerfRecoderEventsVtbl;

    interface _IPerfRecoderEvents
    {
        CONST_VTBL struct _IPerfRecoderEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IPerfRecoderEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _IPerfRecoderEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _IPerfRecoderEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _IPerfRecoderEvents_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _IPerfRecoderEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _IPerfRecoderEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _IPerfRecoderEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IPerfRecoderEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_PerfRecoder;

#ifdef __cplusplus

class DECLSPEC_UUID("8F1B6710-FCCD-4DC8-B24F-26294818D793")
PerfRecoder;
#endif
#endif /* __PerfRecoderIELib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


