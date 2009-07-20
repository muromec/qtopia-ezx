#if !defined(HX_SOCKUTIL_H__)
#define HX_SOCKUTIL_H__

// helpers for working with API defined in hxnet.h
#include "hxnet.h"

class CHXSimpleList;

struct HXSockUtil // namespace
{
    static HX_RESULT SetAddr(IHXSockAddr* pAddr /*modified*/, 
                             const char* pAddrString, 
                             UINT16 port = 0);

    static HX_RESULT SetAddrNetOrder(IHXSockAddr* pAddr /*modified*/,
                             short family, 
                             const void* const pAddrData /*net order*/, 
                             UINT16 port /*net order*/ = 0);

    static HX_RESULT CreateAddr(IHXNetServices* pServices, HXSockFamily family,
                                 const char* pszAddr /*NULL = INANY*/, UINT16 port, 
                                 IHXSockAddr*& pAddrOut);

    static HX_RESULT CreateAddrIN4(IHXNetServices* pNetServices,
                     UINT32 addr, 
                     UINT16 port,
                     IHXSockAddr*& pAddrOut);

    static HX_RESULT ConvertAddr(IHXNetServices* pNetServices, 
                                    HXSockFamily familyOut, 
                                    IHXSockAddr* pOld, 
                                    IHXSockAddr*& pAddrOut);

    static HX_RESULT Convert6to4(IHXSockAddr* pAddrIn, IHXSockAddr*& pAddrOut);
    static HX_RESULT Convert4to6(IHXNetServices* pNetServices, IHXSockAddr* pAddrIn, IHXSockAddr*& pAddrOut);

    static HX_RESULT AllocAddrVec(IHXSockAddr** ppAddrVec, 
                                    UINT32& nVecLen /*in/out*/, 
                                    IHXSockAddr**& ppAddrVecOut,
                                    HXSockFamily familyOut = HX_SOCK_FAMILY_INANY,
                                    bool convert = false,
                                    IHXNetServices* pNetServices = 0);

    static void FreeAddrVec(IHXSockAddr**& ppAddrVec, UINT32& nVecLen);

    static bool IsAnyAddr(IHXSockAddr* pAddr);
    static bool IsMulticastAddr(IHXSockAddr* pAddr);
    static bool IsAnyAddrIN6(IHXSockAddrIN6* pAddr);
    static bool IsMulticastAddrIN6(IHXSockAddrIN6* pAddr);

    static HX_RESULT CreateSocket(IHXNetServices* pNetSvc, IHXSocketResponse* pResp, HXSockFamily f,
                                          HXSockType t, HXSockProtocol p,
                                          IHXSocket*& pSock);

    static HX_RESULT GetIN4Address(IHXSockAddr* pAddr, REF(UINT32) rulAddr);
    static HX_RESULT GetIN4Port(IHXSockAddr* pAddr, REF(UINT16) rusPort);
    
    static HX_RESULT CreateAddrIN4(IHXNetServices* pNetServices,
                     const struct sockaddr_in* psa, IHXSockAddr*& pAddrOut /*out*/);

    static HX_RESULT CreateAddrIN6(IHXNetServices* pNetServices,
                     const struct sockaddr_in6* psa,IHXSockAddr*& pAddrOut /*out*/);
};
#endif //HX_SOCKUTIL_H__


