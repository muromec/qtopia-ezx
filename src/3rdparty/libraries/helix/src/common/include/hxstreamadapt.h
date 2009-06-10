#ifndef _HX_STREAMADAPT_H
#define _HX_STREAMADAPT_H

#include "hxqos.h"

#define MAX_EXCLUDE_RULES 256

typedef enum _StreamAdaptationSchemeEnum
{   ADAPTATION_ANNEXG,
    ADAPTATION_REL6_PER_STREAM,
    ADAPTATION_HLX_AGGR,
    ADAPTATION_HLX_PER_STREAM,
    ADAPTATION_NONE
} StreamAdaptationSchemeEnum;


#ifndef MDP_STREAM_ADAPT_HELPER_FUNC
#define MDP_STREAM_ADAPT_HELPER_FUNC
inline HXBOOL IsHelixStreamAdaptScheme(StreamAdaptationSchemeEnum enumStreamAdaptScheme)
{
    switch (enumStreamAdaptScheme)
    {
	case ADAPTATION_HLX_AGGR:
	case ADAPTATION_HLX_PER_STREAM:
	    return TRUE;
	default:
	    return FALSE;
    }
}

inline HXBOOL IsRel6StyleStreamAdaptScheme(StreamAdaptationSchemeEnum enumStreamAdaptScheme)
{
    switch (enumStreamAdaptScheme)
    {
	case ADAPTATION_HLX_AGGR:
	case ADAPTATION_HLX_PER_STREAM:
	case ADAPTATION_REL6_PER_STREAM:
	    return TRUE;
	default:
	    return FALSE;
    }
}
#endif


typedef struct _StreamAdaptationParams
{
    _StreamAdaptationParams() : m_unStreamNum(0xFFFF)
                                , m_uStreamGroupNum(0xFFFF)
                                , m_ulClientBufferSize(0)
                                , m_ulTargetProtectionTime(0)
                                , m_bStreamSwitch(FALSE)
                                , m_bBufferStateAvailable(FALSE)
                                , m_ulNumExcludedRules(0)
                                , m_ulMaxPreRoll(0)
    {
    }

    UINT16                      m_unStreamNum;
    UINT16                      m_uStreamGroupNum;
//  StreamAdaptationSchemeEnum  m_nAdaptationType;
    UINT32                      m_ulClientBufferSize;       //client buffer size in bytes
    UINT32                      m_ulTargetProtectionTime;   //target protection time in ms
    //Helix-Adaptation-Support header specific:
    HXBOOL                        m_bStreamSwitch;            //true to allow adaptation beyond initial subscribe
    HXBOOL                        m_bBufferStateAvailable;    //true indicates client buffer feedback is available
    UINT32                      m_ulNumExcludedRules;       // Number of excluded rules in m_pExcludeRules
    UINT16                      m_pExcludeRules [MAX_EXCLUDE_RULES];  //vector of rules to be excluded from adaptation

    UINT32                      m_ulMaxPreRoll;       // Used for Aggregate Buffer Manangement (Helix-Adapt)
} StreamAdaptationParams;

typedef struct _LinkCharParams
{
    _LinkCharParams() : m_bSessionAggregate(FALSE)
                        , m_unStreamNum(0xFFFF)
                        , m_unStreamGroupNum(0xFFFF)
                        , m_ulGuaranteedBW(0)
                        , m_ulMaxBW(0)
                        , m_ulMaxTransferDelay(0)
    {
    }

    HXBOOL    m_bSessionAggregate;    //set if Link Characteristics defined on a Session level
    UINT16  m_unStreamNum;          //Stream Number, valid only when link char set for individual streams
    UINT16  m_unStreamGroupNum;     //Stream Group Number, valid only when link char set for individual streams
    UINT32  m_ulGuaranteedBW;       //guaranteed BW in bps
    UINT32  m_ulMaxBW;              //Maximum BW in bps
    UINT32  m_ulMaxTransferDelay;   //Transfer Delay in ms
} LinkCharParams;


/*
 * IHXStreamAdaptationSetup
 * {0AD4EBF5-F125-42ca-B477-B4AE828AC50F}
 */
 DEFINE_GUID(IID_IHXStreamAdaptationSetup,
			0xad4ebf5, 0xf125, 0x42ca, 0xb4, 0x77, 0xb4, 0xae, 0x82, 0x8a, 0xc5, 0xf);

#undef  INTERFACE
#define INTERFACE   IHXStreamAdaptationSetup
DECLARE_INTERFACE_(IHXStreamAdaptationSetup, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)   ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)  ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release) ( THIS ) PURE;

    // IHXStreamAdaptationSchemeSetup
    STDMETHOD (GetStreamAdaptationScheme)   (THIS_ 
                                 	REF(StreamAdaptationSchemeEnum) /* OUT */ enumAdaptScheme ) PURE;
    STDMETHOD (SetStreamAdaptationScheme)   (THIS_ 
                                 StreamAdaptationSchemeEnum enumAdaptScheme ) PURE;

    STDMETHOD (SetStreamAdaptationParams) (THIS_
				 StreamAdaptationParams* /* IN */ pStreamAdaptParams) PURE;
    STDMETHOD (GetStreamAdaptationParams) (THIS_
				 UINT16 unStreamNum,
				 REF(StreamAdaptationParams) /* OUT */ streamAdaptParams) PURE;

    STDMETHOD (UpdateStreamAdaptationParams) (THIS_
				 StreamAdaptationParams* /* IN */ pStreamAdaptParams) PURE;
    STDMETHOD (UpdateTargetProtectionTime) (THIS_
				 StreamAdaptationParams* /* IN */ pStreamAdaptParams) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXQoSLinkCharSetup
 *
 *  Purpose:
 *
 *      Used to set and get Link characteristics parameters
 *
 *  IID_IHXQoSLinkCharSetup:
 *
 *      {01A668EB-DC9E-414f-A868-C94809A13A6B}
 *
 */
 DEFINE_GUID(IID_IHXQoSLinkCharSetup,
 			0x1a668eb, 0xdc9e, 0x414f, 0xa8, 0x68, 0xc9, 0x48, 0x9, 0xa1, 0x3a, 0x6b);
#undef  INTERFACE
#define INTERFACE IHXQoSLinkCharSetup
DECLARE_INTERFACE_(IHXQoSLinkCharSetup, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)   ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)  ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release) ( THIS ) PURE;

    // IHXQoSLinkCharSetup
    STDMETHOD (SetLinkCharParams) (THIS_
				 LinkCharParams* /* IN */ pLinkCharParams) PURE;
    STDMETHOD (GetLinkCharParams) (THIS_
				 UINT16 unStreamNum,
				 REF(LinkCharParams) /* OUT */ linkCharParams) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXQOSRateMgrClassFactory
 *
 *  Purpose:
 *
 *      HX interface that manages the creation of HX QoS classes.
 *
 *  IID_IHXQoSRateMgrClassFactory:
 *
 *      {CC46E2F4-F985-49b7-9B7F-834472BA00E2}
 *
 */
DEFINE_GUID(IID_IHXQoSRateMgrClassFactory,
			0xcc46e2f4, 0xf985, 0x49b7, 0x9b, 0x7f, 0x83, 0x44, 0x72, 0xba, 0x0, 0xe2);

#undef  INTERFACE
#define INTERFACE   IHXQoSRateMgrClassFactory

DECLARE_INTERFACE_(IHXQoSRateMgrClassFactory, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     *  IHXQOSRateMgrClassFactory methods
     */

    /************************************************************************
     *  Method:
     *      IHXQoSRateMgrClassFactory::CreateInstance
     *  Purpose:
     *      Creates instances of Rate Manager  classes based on the Stream Adaptation
	 *      Scheme passed and the profile for which you are creating the objects.
     */
    STDMETHOD(CreateInstance)           (THIS_
                                         IHXQoSSignalBus* pSignalBus,
                                         StreamAdaptationSchemeEnum enumAdaptScheme,
                                         REFCLSID    /*IN*/  rclsid,
                                         void**     /*OUT*/ ppUnknown) PURE;
};

#endif
