#ifndef _HXSHUTDOWN_H_
#define _HXSHUTDOWN_H_

/***********************************************************************
 * IHXServerShutdownResponse
 ************************************************************************/
DEFINE_GUID_ENUM(IID_IHXServerShutdownResponse, 0xc1fc707d, 0xd62d, 0x4d47, 
                 0x83, 0x95, 0xe4, 0x280, 0xf2, 0x14, 0xdb, 0x10); 

#define CLSID_IHXServerShutdownResponse IID_IHXServerShutdownResponse

#undef  INTERFACE
#define INTERFACE IHXServerShutdownResponse

DECLARE_INTERFACE_(IHXServerShutdownResponse, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXServerShutdownResponse methods

    /*
    * IHXServerShutdownResponse
    */
    STDMETHOD(OnShutDownStart)           (THIS_
                                          BOOL bLogPlayerTermination) PURE;

    STDMETHOD(OnShutDownEnd)             (THIS) PURE;

     
}; 

#endif // _HXSHUTDOWN_H_
