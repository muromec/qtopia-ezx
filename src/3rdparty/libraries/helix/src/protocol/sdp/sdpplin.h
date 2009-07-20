/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sdpplin.h,v 1.6 2007/07/06 20:51:37 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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


#ifndef _SDPPLIN_H_
#define _SDPPLIN_H_

#include "mdescparse.h"
#include "mdescgen.h"

class CSDPStreamDescription :        public IHXPlugin, 
                                public IHXStreamDescription,
                                public IHXStreamDescriptionSettings,
                                public IHXRTPPayloadInfo
{
private:
    LONG32                            m_lRefCount;
    MediaDescParser*                  m_pDescParser;
    MediaDescGenerator*               m_pDescGenerator;
    IUnknown*                         m_pContext;
    IHXCommonClassFactory*           m_pCCF;

    static const char* const          zm_pDescription;
    static const char* const          zm_pCopyright;
    static const char* const          zm_pMoreInfoURL;

    static const char* const          zm_pStreamDescriptionMimeType;
   
    ~CSDPStreamDescription();


    HX_RESULT        Update();
    HX_RESULT   SpecComplianceCheck(UINT16 nValues, IHXValues** ppValueArray);
    
public:
    CSDPStreamDescription();

    char* EscapeBuffer(const char*, UINT32);
    char* EscapeBuffer(const char*);

    char* UnescapeBuffer(const char*, UINT32);
    char* UnescapeBuffer(const char*);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)        (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)        (THIS);

    STDMETHOD_(ULONG32,Release)        (THIS);

    // *** IHXPlugin methods ***

    /************************************************************************
     *        Method:
     *            IHXPlugin::GetPluginInfo
     *        Purpose:
     *            Returns the basic information about this plugin. Including:
     *
     *            bLoadMultiple        whether or not this plugin DLL can be loaded
     *                                multiple times. All File Formats must set
     *                                this value to TRUE.
     *            pDescription        which is used in about UIs (can be NULL)
     *            pCopyright                which is used in about UIs (can be NULL)
     *            pMoreInfoURL        which is used in about UIs (can be NULL)
     */
    STDMETHOD(GetPluginInfo)        (THIS_
                                REF(HXBOOL)        /*OUT*/ bLoadMultiple,
                                REF(const char*) /*OUT*/ pDescription,
                                REF(const char*) /*OUT*/ pCopyright,
                                REF(const char*) /*OUT*/ pMoreInfoURL,
                                REF(ULONG32)         /*OUT*/ ulVersionNumber
                                );

    /************************************************************************
     *        Method:
     *            IHXPlugin::InitPlugin
     *        Purpose:
     *            Initializes the plugin for use. This interface must always be
     *            called before any other method is called. This is primarily needed 
     *            so that the plugin can have access to the context for creation of
     *            IHXBuffers and IMalloc.
     */
    STDMETHOD(InitPlugin)   (THIS_
                            IUnknown*   /*IN*/  pContext);

    // *** IHXStreamDescription methods ***

    /************************************************************************
     *        Method:
     *            IHXStreamDescription::GetStreamDescriptionInfo
     *        Purpose:
     *            Get info to initialize the stream description plugin
     */
    STDMETHOD(GetStreamDescriptionInfo)
                                (THIS_
                                REF(const char*) /*OUT*/ pMimeTypes);

    /************************************************************************
     *        Method:
     *            IHXStreamDescription::GetValues
     *        Purpose:
     *            Transform a media description string into an IHXValues object
     */
    STDMETHOD(GetValues)    
                                (THIS_
                                IHXBuffer*          /*IN*/ pDescription,
                                REF(UINT16)          /*OUT*/ nValues,
                                REF(IHXValues**) /*OUT*/ pValueArray
                                );

    /************************************************************************
     *        Method:
     *            IHXStreamDescription::GetDescription
     *        Purpose:
     *            Transform an IHXValues object into a stream description string.
     *            plugins.
     */
    STDMETHOD(GetDescription)        
                        (THIS_
                        UINT16                            /*IN*/  nValues,
                        IHXValues**                    /*IN*/  pValueArray,
                        REF(IHXBuffer*)            /*OUT*/ pDescription
                        );

     // *** IHXStreamDescriptionSettings methods ***

    STDMETHOD(SetOption)(const char* pKey, IHXBuffer* pVal);
    STDMETHOD(GetOption)(const char* pKey, REF(IHXBuffer*) pVal);

     // *** IHXRTPPayloadInfo methods ***

    /************************************************************************
     *        Method:
     *            IHXRTPPayloadInfo::PayloadSupported
     *        Purpose:
     *            Returns TRUE if this payload type is handled by this interface
     */
    STDMETHOD_(HXBOOL, IsPayloadSupported)            (THIS_
                                UINT32      /*IN*/  ulRTPPayloadType);

    /************************************************************************
     *        Method:
     *            IHXRTPPayloadInfo::GetTimestampConversionFactors
     *        Purpose:
     *            Retrieves the RTP and RMA factors for RTP to RMA timestamp ratio.
     *      RTP->RMA is RTPTimestamp * RTPFactor / HXFactor
     *      RMA->RTP is HXTimestamp * HXFactor / RTPFactor
     *  Returns:
     *            HXR_OK if payload is supported, HXR_FAIL if not.
     */
    STDMETHOD(GetTimestampConversionFactors)            (THIS_
                                    UINT32      /*IN*/  ulRTPPayloadType,
                                REF(UINT32) /*OUT*/ ulRTPFactor,
                                REF(UINT32) /*OUT*/ ulHXFactor);

    /************************************************************************
     *        Method:
     *            IHXRTPPayloadInfo::IsTimestampDeliverable
     *        Purpose:
     *            Returns TRUE if this payload type is timestamp deliverable
     */
    STDMETHOD_(HXBOOL, IsTimestampDeliverable)            (THIS_
                                UINT32      /*IN*/  ulRTPPayloadType);
};

#endif /* _MHPLIN_H_ */
