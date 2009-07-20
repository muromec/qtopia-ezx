/* ***** BEGIN LICENSE BLOCK *****  
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
 *       
 * The contents of this file, and the files included with this file, 
 * are subject to the current version of the RealNetworks Public 
 * Source License (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the current version of the RealNetworks Community 
 * Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply. You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *   
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created. 
 *   
 * This file, and the files included with this file, is distributed 
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
 * ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck  
 *  
 * Contributor(s):  
 *   
 * ***** END LICENSE BLOCK ***** */  
#ifndef _ISIFS_H_
#define _ISIFS_H_


_INTERFACE IHXRateDescription;
_INTERFACE IHXRateDescEnumerator;
_INTERFACE IHXRateDescResponse;
_INTERFACE IHXStreamRateDescResponse;
_INTERFACE IHXUberStreamManager;
_INTERFACE IHXSessionStats;
_INTERFACE IHXQoSProfileConfigurator;
_INTERFACE IHXErrorMessages;
_INTERFACE IHXCommonClassFactory;
_INTERFACE IHXASMSource;
_INTERFACE IHXValues;
_INTERFACE IHXRateSelectionInfo;

class Process;
typedef struct _StreamAdaptationParams StreamAdaptationParams;

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXRateDescription
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXRateDescription:
 * 
 *	{4B5B28F6-F6FF-4f60-AD66-AC6DE9E75CDF}
 * 
 */

DEFINE_GUID(IID_IHXRateDescription, 
    0x4b5b28f6, 0xf6ff, 0x4f60, 0xad, 0x66, 0xac, 0x6d, 0xe9, 0xe7, 0x5c, 0xdf);


#undef  INTERFACE
#define INTERFACE   IHXRateDescription

enum HXISRateDescID
{
    HX_IS_RATEDESC_PREDATA,
    HX_IS_RATEDESC_MAXRATE,
    HX_IS_RATEDESC_PREROLL
};

enum EHXSwitchableReason
{
    HX_SWI_INADEQUATE_CLIENT_CAPABILITIES=0, // Pre-decode buffer too small, etc.
    HX_SWI_INACTIVE_SWITCH_GROUP,	// Switch group was not selected
    HX_SWI_RATE_EXCLUDED_BY_CLIENT,	// Client has signalled that rate should be excluded
    HX_SWI_RV_THINNING_STREAM,	// RealVideo thinning (key-frame only) stream is excluded
    HX_SWI_NUM_REASONS			// Must be the last enum
};

enum EHXSelectableReason
{
    HX_SEL_INADEQUATE_CLIENT_CAPABILITIES=0, // Pre-decode buffer too small, etc.
    HX_SEL_RV_THINNING_STREAM,	// RealVideo thinning (key-frame only) stream is excluded
    HX_SEL_NUM_REASONS			// Must be the last enum
};

DECLARE_INTERFACE_(IHXRateDescription, IUnknown)
{
    /*
     * IUnknown Methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRateDescription::GetAvgRate
     *	Purpose:
     */
    STDMETHOD(GetAvgRate)   (THIS_ REF(UINT32)ulRate) PURE;
    STDMETHOD(GetMaxRate)   (THIS_ REF(UINT32)ulRate) PURE;
    STDMETHOD(GetPredata)   (THIS_ REF(UINT32)ulBytes) PURE;
    STDMETHOD(GetPreroll)   (THIS_ REF(UINT32)ulMs) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXRateDescription::GetValue
     *	Purpose:
     */
    STDMETHOD(GetValue)   (THIS_ UINT32 id, REF(UINT32)ulValue) PURE;

    /* Bandwidth allocated to each stream group -- indexed by stream group number */
    STDMETHOD_(UINT32,GetNumBandwidthAllocations) (THIS) PURE;
    STDMETHOD_(UINT32*,GetBandwidthAllocationArray) (THIS) PURE;

    STDMETHOD_(UINT32,GetNumRules) (THIS) PURE;
    STDMETHOD_(UINT32*,GetRuleArray) (THIS) PURE;

    STDMETHOD_(BOOL,IsSelectable) (THIS) PURE;
    STDMETHOD_(BOOL,IsSwitchable) (THIS) PURE;

    STDMETHOD(ExcludeFromSelection) (THIS_ BOOL bExcludeFromSelection, EHXSelectableReason eSelReason) PURE;
    STDMETHOD(ExcludeFromSwitching) (THIS_ BOOL bExcludeFromSwitching, EHXSwitchableReason eSwiReason) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXRateDescManager
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXRateDescManager:
 * 
 *	{8467996C-9097-4721-9D4D-D8A6D4BB0303}
 * 
 */
DEFINE_GUID(IID_IHXRateDescManager, 
    0x8467996c, 0x9097, 0x4721, 0x9d, 0x4d, 0xd8, 0xa6, 0xd4, 0xbb, 0x03, 0x03);

#undef  INTERFACE
#define INTERFACE   IHXRateDescManager

DECLARE_INTERFACE_(IHXRateDescManager, IUnknown)
{
    /*
     * IUnknown Methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    STDMETHOD(GetDescriptionCount)   (THIS_ REF(UINT32)ulCount) PURE;
    STDMETHOD(GetRateDescription)(THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc) PURE;
    STDMETHOD(GetCurrentRateDesc)(THIS_ REF(IHXRateDescription*)pRateDesc) PURE;
    STDMETHOD(UpShift)(THIS_ UINT32 ulRate, IHXRateDescResponse* pResp) PURE;
    STDMETHOD(DownShift)(THIS_ UINT32 ulRate, IHXRateDescResponse* pResp) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXRateDescEnumerator
 * 
 *  Purpose:
 * 
 *    Used to enumerate over a set of rate descriptions.
 * 
 *  Methods:
 *  
 *    GetNumRateDescriptions - Returns the number of rate descriptions
 *    GetRateDescription - Returns the rate descriptions at ulIndex
 *    GetNumSelectableRateDescriptions - Returns the number of selectable rate descriptions
 *    GetSelectableRateDescription - Returns the rate descriptions at ulIndex
 *    GetNumSwitchableRateDescriptions - Returns the number of switchable rate descriptions
 *    GetSwitchableRateDescription - Returns the rate descriptions at ulIndex
 *    FindRateDescByRule - Finds the first rate description with a matching ulRuleNumber
 *    FindRateDescByExactAvgRate - Finds the first rate description with a matching ulAvgRate
 *    FindRateDescByClosestAvgRate - Finds the closest (lower or equal) rate description to ulAvgRate
 * 
 *	{9A32B0BB-EA2C-47d3-8F44-B180CA6BA2BB}
 * 
 */

DEFINE_GUID(IID_IHXRateDescEnumerator, 
    0x9a32b0bb, 0xea2c, 0x47d3, 0x8f, 0x44, 0xb1, 0x80, 0xca, 0x6b, 0xa2, 0xbb);

#undef  INTERFACE
#define INTERFACE   IHXRateDescEnumerator

DECLARE_INTERFACE_(IHXRateDescEnumerator, IUnknown)
{
    /*
     * IUnknown Methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    STDMETHOD_(UINT32,GetNumRateDescriptions) (THIS) PURE;
    STDMETHOD(GetRateDescription)(THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc) PURE;

    STDMETHOD_(UINT32,GetNumSelectableRateDescriptions) (THIS) PURE;
    STDMETHOD(GetSelectableRateDescription)(THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc) PURE;

    STDMETHOD_(UINT32,GetNumSwitchableRateDescriptions) (THIS) PURE;
    STDMETHOD(GetSwitchableRateDescription)(THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc) PURE;

    STDMETHOD(FindRateDescByRule)(THIS_ UINT32 ulRuleNumber, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc) PURE;
    STDMETHOD(FindRateDescByExactAvgRate)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc) PURE;
    STDMETHOD(FindRateDescByClosestAvgRate)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc) PURE;
    STDMETHOD(FindRateDescByMidpoint)(THIS_ UINT32 ulAvgRate, BOOL bRequireSelectable, BOOL bRequireSwitchable, REF(IHXRateDescription*)pRateDesc) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXUberStreamManager
 * 
 *  Purpose:
 * 
 *    Used to control the aggregate or per-stream rate description
 * 
 *  Methods:
 *  
 *    GetNumStreamGroups - Returns the total number of stream groups
 *    GetStreamGroup - Gets a stream group
 *    FindStreamGroupByLogicalStream - Maps a logical stream number to a stream group number
 *    GetNumLogicalStreams - Returns the total number of logical streams
 *    GetLogicalStream - Gets a logical stream
 *    GetSelectedLogicalStreamNum - Gets the selected logical stream number for a stream group.  Can
 *      only be called after initial rate description has been committed.
 *    
 *    SetAggregateRateDesc - Sets the current aggregate rate description.  Note that
 *      switchable/selected flags are not checked.
 *    GetCurrentAggregateRateDesc - Gets the current aggregate rate desc
 *    CommitInitialAggregateRateDesc - Commits the initial rate description -- must be 
 *      called before Upshift/Downshifts can occur
 *    IsInitalAggregateRateDescCommitted - Determines if the initial rate description has 
 *      been committed
 *    Upshift - Upshifts to switchable rate closest (lowest or equal) to ulRate.  
 *      If ulRate is 0, will shift to the next highest switchable rate.
 *    Downshift - Downshifts to switchable rate closest (lowest or equal) to ulRate.  
 *      If ulRate is 0, will shift to the next lowest switchable rate
 *    GetNextSwitchableRateDesc - Returns the next switchable stream
 *    
 *    SetStreamGroupRateDesc - Sets the stream group's current rate description.  Note that
 *      switchable/selected flags are not checked.
 *    GetCurrentStreamGroupRateDesc - Gets the stream group's current rate desc
 *    CommitInitialAggregateRateDesc - Commits the initial rate description -- must be 
 *      called before Upshift/Downshifts can occur
 *    IsInitalStreamGroupRateDescCommitted - Determines if the initial rate description has 
 *      been committed
 *    Upshift - Upshifts to switchable rate closest (lowest or equal) to ulRate.  
 *      If ulRate is 0, will shift to the next highest switchable rate.
 *    Downshift - Downshifts to switchable rate closest (lowest or equal) to ulRate.  
 *      If ulRate is 0, will shift to the next lowest switchable rate
 *    
 *    SubscribeLogicalStreamRule - Subscribes to a logical stream rule.  Note that
 *      switchable/selected flags are not checked.
 *    UnsubscribeLogicalStreamRule - Unsubscribes from a logical stream rule.  Note that
 *      switchable/selected flags are not checked.
 *    
 *	{E7903A70-43F5-4aee-A36B-EBFE3C9B49EA}
 * 
 */

DEFINE_GUID(IID_IHXUberStreamManager, 
    0xe7903a70, 0x43f5, 0x4aee, 0xa3, 0x6b, 0xeb, 0xfe, 0x3c, 0x9b, 0x49, 0xea);

#undef  INTERFACE
#define INTERFACE   IHXUberStreamManager

DECLARE_INTERFACE_(IHXUberStreamManager, IHXRateDescEnumerator)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /* IHXUberStreamManager methods */
    STDMETHOD_(UINT32,GetNumStreamGroups) (THIS) PURE;
    STDMETHOD(GetStreamGroup)(THIS_ UINT32 ulStreamGroupNum, REF(IHXRateDescEnumerator*)pStreamGroupEnum) PURE;
    STDMETHOD(FindStreamGroupByLogicalStream)(THIS_ UINT32 ulLogicalStreamNum, REF(UINT32)ulStreamGroupNum) PURE;    
    STDMETHOD_(UINT32,GetNumLogicalStreams) (THIS) PURE;
    STDMETHOD(GetLogicalStream)(THIS_ UINT32 ulLogicalStreamNum, REF(IHXRateDescEnumerator*)pLogicalStreamEnum) PURE;
    STDMETHOD(GetSelectedLogicalStreamNum)(THIS_ UINT32 ulStreamGroupNum, REF(UINT32)ulSelectedLogicalStreamNum) PURE;

    /* Aggregate rate control */
    STDMETHOD(SetAggregateRateDesc)(THIS_ IHXRateDescription* pRateDesc, IHXRateDescResponse* pResp) PURE;
    STDMETHOD(GetCurrentAggregateRateDesc)(THIS_ REF(IHXRateDescription*)pRateDesc) PURE;
    STDMETHOD(CommitInitialAggregateRateDesc) (THIS) PURE;
    STDMETHOD_(BOOL,IsInitalAggregateRateDescCommitted) (THIS) PURE;
    STDMETHOD(UpshiftAggregate)(THIS_ UINT32 ulRate, IHXRateDescResponse* pResp, BOOL bIsClientInitiated = FALSE) PURE;
    STDMETHOD(DownshiftAggregate)(THIS_ UINT32 ulRate, IHXRateDescResponse* pResp, BOOL bIsClientInitiated = FALSE) PURE;
    STDMETHOD(GetLowestAvgRate)(THIS_ UINT32 ulStreamGroupNum, REF(UINT32) ulLowestAvgRate) PURE;
    STDMETHOD(SetDownshiftOnFeedbackTimeoutFlag)(THIS_ BOOL bFlag) PURE;

    /* Per-stream rate control */
    STDMETHOD(SelectLogicalStream)(THIS_ UINT32 ulStreamGroupNum,
                                         UINT32 ulLogicalStreamNum) PURE;
    STDMETHOD(SetStreamGroupRateDesc)(THIS_ UINT32 ulStreamGroupNum, 
                                        UINT32 ulLogicalStreamNum, 
                                        IHXRateDescription* pRateDesc, 
                                        IHXStreamRateDescResponse* pResp) PURE;
    STDMETHOD(GetCurrentStreamGroupRateDesc)(THIS_ UINT32 ulStreamGroupNum, REF(IHXRateDescription*)pRateDesc) PURE;
    STDMETHOD(CommitInitialStreamGroupRateDesc) (THIS_ UINT32 ulStreamGroupNum) PURE;
    STDMETHOD_(BOOL,IsInitalStreamGroupRateDescCommitted) (THIS_ UINT32 ulStreamGroupNum) PURE;
    STDMETHOD(GetNextSwitchableRateDesc)(THIS_ REF(IHXRateDescription*)pRateDesc) PURE;
    STDMETHOD(GetNextSwitchableStreamGroupRateDesc)(THIS_ UINT32 ulStreamGroupNum, REF(IHXRateDescription*)pRateDesc) PURE;

    STDMETHOD(UpshiftStreamGroup)   (THIS_ UINT32 ulStreamGroupNum, 
                                    UINT32 ulRate, 
                                    IHXStreamRateDescResponse* pResp,
                                    BOOL bIsClientInitiated = FALSE) PURE;
    STDMETHOD(DownshiftStreamGroup) (THIS_ UINT32 ulStreamGroupNum, 
                                    UINT32 ulRate, 
                                    IHXStreamRateDescResponse* pResp,
                                    BOOL bIsClientInitiated = FALSE) PURE;

    STDMETHOD(SubscribeLogicalStreamRule)(THIS_ UINT32 ulLogicalStreamNum, 
                                        UINT32 ulRuleNum, 
                                        IHXStreamRateDescResponse* pResp) PURE;
    STDMETHOD(UnsubscribeLogicalStreamRule)(THIS_ UINT32 ulLogicalStreamNum, 
                                        UINT32 ulRuleNum, 
                                        IHXStreamRateDescResponse* pResp) PURE;
    STDMETHOD(DetermineSelectableStreams)(THIS_ const StreamAdaptationParams* pStreamAdaptationParams = NULL) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXUberStreamManagerInit
 *
 *  Purpose:
 * 
 *    Used to initialize the uber stream manager with file/stream headers
 * 
 *  Methods:
 *  
 *    Init - Set bCheckAverageBandwidth to TRUE to require logical stream rules
 *      to have an AverageBandwidth property
 *    SetASMSource - Used to set the ASM source, if the uber stream manager 
 *      will be used for rate shifting.
 *    SetFileHeader - Used to set the file header
 *    SetStreamHeader - Used to set logical stream headers
 * 
 *  // {B9DB235B-DEF7-4e22-B94E-6ECDCEB7B31B}
 *
 */

DEFINE_GUID(IID_IHXUberStreamManagerInit,
0xb9db235b, 0xdef7, 0x4e22, 0xb9, 0x4e, 0x6e, 0xcd, 0xce, 0xb7, 0xb3, 0x1b);

#undef  INTERFACE
#define INTERFACE   IHXUberStreamManagerInit

DECLARE_INTERFACE_(IHXUberStreamManagerInit, IUnknown)
{   
    /* IUnknown methods */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /* IHXUberStreamManagerInit methods */
    STDMETHOD(Init) (THIS_ BOOL bCheckAverageBandwidth) PURE;
    STDMETHOD(SetASMSource) (THIS_ IHXASMSource* pASMSource) PURE; 
    STDMETHOD(SetFileHeader) (THIS_ IHXValues* pFileHeader) PURE; 
    STDMETHOD(SetStreamHeader) (THIS_ UINT32 ulLogicalStreamNum, IHXValues* pStreamHeader) PURE;
};

HX_RESULT CreateUberStreamManager(IHXCommonClassFactory* pCCF, IHXUberStreamManagerInit** ppUberStreamManager);

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXUberStreamManagerConfig
 *
 *  Purpose:
 * 
 *    Used to configure the uber stream manager
 * 
 *  Methods:
 *  
 *    SetRateSelectionInfoObject - Provides rate selection info object to inputsource.
 * 
 *  // {280D65EA-7B81-42b0-AAC6-151A94E7BB2E}
 *
 */

DEFINE_GUID(IID_IHXUberStreamManagerConfig,
0x280d65ea, 0x7b81, 0x42b0, 0xaa, 0xc6, 0x15, 0x1a, 0x94, 0xe7, 0xbb, 0x2e);

#undef  INTERFACE
#define INTERFACE   IHXUberStreamManagerConfig

DECLARE_INTERFACE_(IHXUberStreamManagerConfig, IUnknown)
{   
    /* IUnknown methods */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /* IHXUberStreamManagerConfig methods */
    STDMETHOD(SetRateSelectionInfoObject) (THIS_ IHXRateSelectionInfo* pRateSelInfo) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXRateDescResponse
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXRateDescResponse:
 * 
 *	{1E8E5CB1-36EE-4209-9DCC-4B661F7BAA59}
 * 
 */

DEFINE_GUID(IID_IHXRateDescResponse, 
    0x1e8e5cb1, 0x36ee, 0x4209, 0x9d, 0xcc, 0x4b, 0x66, 0x1f, 0x7b, 0xaa, 0x59);
 
#undef  INTERFACE
#define INTERFACE   IHXRateDescResponse

DECLARE_INTERFACE_(IHXRateDescResponse, IUnknown)
{
    /*
     * IUnknown Methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRateDescResponse::ShiftDone
     *	Purpose:
     *	    
     *	Prameters:
     */
    STDMETHOD(ShiftDone)  (THIS_ HX_RESULT status, IHXRateDescription* pRateDesc) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXStreamRateDescManager
 *
 *  Purpose:
 *
 *    Extends IHXRateDescManager to provide stream level rate control methods.
 *
 *  Methods:
 *
 *    Init - Used to set the stream selection params
 *    VerifyStreams - Enumerates through all rate descriptions, and enables/disables
 *      the rate descriptions not suitable for the stream selection params
 *    SelectInitialRateDesc - Selects an appropriate initial bitrate from the set
 *       of enabled bitrates
 *
 *
 *  IID_IHXStreamRateDescManager:
 *
 *  {4D56585B-E47A-46e0-BAF6-146213BA94AB}
 *
 */

DEFINE_GUID(IID_IHXStreamRateDescManager,
    0x4d56585b, 0xe47a, 0x46e0, 0xba, 0xf6, 0x14, 0x62, 0x13, 0xba, 0x94, 0xab);

#undef  INTERFACE
#define INTERFACE   IHXStreamRateDescManager

DECLARE_INTERFACE_(IHXStreamRateDescManager, IHXRateDescManager)
{
    /*
     * IUnknown Methods
     */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXRateDescManager Methods
     */
    STDMETHOD(GetDescriptionCount)   (THIS_ REF(UINT32)ulCount) PURE;
    STDMETHOD(GetRateDescription)(THIS_ UINT32 ulIndex, REF(IHXRateDescription*)pRateDesc) PURE;
    STDMETHOD(GetCurrentRateDesc)(THIS_ REF(IHXRateDescription*)pRateDesc) PURE;
    STDMETHOD(UpShift)(THIS_ UINT32 ulRate, IHXRateDescResponse* pResp) PURE;
    STDMETHOD(DownShift)(THIS_ UINT32 ulRate, IHXRateDescResponse* pResp) PURE;

    /************************************************************************
     *  Method:
     *      IHXStreamRateDescManager::UpshiftAggregate
     *
     *  Purpose:
     *    Upshift stream rate for the session
     *
     *  Parameters:
     *      pResp : Response obj
     */
    STDMETHOD(UpshiftAggregate)(THIS_ IHXRateDescResponse* pResp) PURE;

    /************************************************************************
     *  Method:
     *      IHXStreamRateDescManager::DownshiftAggregate
     *
     *  Purpose:
     *    Downshift stream rate for the session
     *
     *  Parameters:
     *      pResp : Response obj
     */
    STDMETHOD(DownshiftAggregate)(THIS_ IHXRateDescResponse* pResp) PURE;

    /************************************************************************
     *  Method:
     *      IHXStreamRateDescManager::UpshiftLogicalStream
     *
     *  Purpose:
     *    Upshift passed Logical Stream
     *
     *  Parameters:
     *      unLogicalStream: logical stream number.
     *      pResp : Response obj
     */
    STDMETHOD(UpshiftLogicalStream)(THIS_ UINT16 unLogicalStream, 
                                    IHXStreamRateDescResponse* pResp) PURE;

    /************************************************************************
     *  Method:
     *      IHXStreamRateDescManager::DownshiftLogicalStream
     *
     *  Purpose:
     *    Downshift passed Logical Stream
     *
     *  Parameters:
     *      unLogicalStream: logical stream number.
     *      pResp : Response obj
     */
    STDMETHOD(DownshiftLogicalStream)(THIS_ UINT16 unLogicalStream, 
                                      IHXStreamRateDescResponse* pResp) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXStreamRateDescResponse
 *
 *  Purpose: counterpart core stream level Rate Description manager.
 *
 *  IID_IHXStreamRateDescResponse:
 *
 *  {544D3CD9-8ED8-4473-BD83-EC5290EC0839}
 *
 */

DEFINE_GUID(IID_IHXStreamRateDescResponse,
    0x544d3cd9, 0x8ed8, 0x4473, 0xbd, 0x83, 0xec, 0x52, 0x90, 0xec, 0x8, 0x39);

#undef  INTERFACE
#define INTERFACE   IHXStreamRateDescResponse

DECLARE_INTERFACE_(IHXStreamRateDescResponse, IHXRateDescResponse)
{
    /*
     * IUnknown Methods
     */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXRateDescResponse Methods
     */
    STDMETHOD(ShiftDone)  (THIS_ HX_RESULT status, IHXRateDescription* pRateDesc) PURE;

    /************************************************************************
     *  Method:
     *      IHXStreamRateDescResponse::ShiftDone
     *  
     *  Purpose:
     *
     *  Prameters:
     */
    STDMETHOD(ShiftDone)  (THIS_ HX_RESULT status, UINT16 unLogicalStream, IHXRateDescription* pRateDesc) PURE;
};
#endif /* _ISIFS_H_ */
