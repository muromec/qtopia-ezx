/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxlogoutputs.h,v 1.4 2003/09/19 15:51:48 dcollins Exp $
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

#ifndef _HXLOGOUTPUT_H_
#define _HXLOGOUTPUT_H_

_INTERFACE  IUnknown;
_INTERFACE  IHXBuffer;

class Process;

///////////////////////////////////////////////////////////////////////////////
// Interface:
//
//      IHXLogOutput
//
// Purpose:
//
//      Interface for the LogOutput class.
//
// IID_IHXLogOutput:
//
//      {88EC448C-136C-4733-9403-DC43409DCA29}
//
///////////////////////////////////////////////////////////////////////////////


// {88EC448C-136C-4733-9403-DC43409DCA29}
DEFINE_GUID(IID_IHXLogOutput, 0x88ec448c, 0x136c, 0x4733, 0x94, 0x3, 0xdc,
            0x43, 0x40, 0x9d, 0xca, 0x29);


#define CLSID_IHXLogOutput  IID_IHXLogOutput

#undef INTERFACE
#define INTERFACE IHXLogOutput


DECLARE_INTERFACE_(IHXLogOutput, IUnknown)
{
    // IUnknown Methods
    STDMETHOD(QueryInterface)               (THIS_
                                            REFIID riid,
                                            void** ppvObj) PURE;

    STDMETHOD_(UINT32,AddRef)               (THIS) PURE;

    STDMETHOD_(UINT32,Release)              (THIS) PURE;


    // IID_IHXLogOutput Methods
    STDMETHOD(Output) (THIS_
                       IHXBuffer* pBuffer) PURE;
    STDMETHOD(Output) (THIS_
                       const char *pString) PURE;
};



///////////////////////////////////////////////////////////////////////////////
// Interface:
//
//      IHXLogFileOutput
//
// Purpose:
//
//      Interface for the LogOutput class.
//
// IID_IHXLogFileOutput:
//
//      {C5C9C037-53FA-4478-8BFD-E49E588BF21F}
//
///////////////////////////////////////////////////////////////////////////////


// {C5C9C037-53FA-4478-8BFD-E49E588BF21F}
DEFINE_GUID(IID_IHXLogFileOutput, 
0xc5c9c037, 0x53fa, 0x4478, 0x8b, 0xfd, 0xe4, 0x9e, 0x58, 0x8b, 0xf2, 0x1f);

#define CLSID_IHXLogFileOutput  IID_IHXLogFileOutput

#undef INTERFACE
#define INTERFACE IHXLogFileOutput


DECLARE_INTERFACE_(IHXLogFileOutput, IHXLogOutput)
{
    // IUnknown Methods
    STDMETHOD(QueryInterface)               (THIS_
                                            REFIID riid,
                                            void** ppvObj) PURE;

    STDMETHOD_(UINT32,AddRef)               (THIS) PURE;

    STDMETHOD_(UINT32,Release)              (THIS) PURE;


    // IID_IHXLogFileOutput Methods

    // CrashOutput
    STDMETHOD(CrashOutput)                  (THIS_
                                             const char *pString) PURE;

    // Log Pruning Functions
    STDMETHOD_(UINT32,GetPruneThreshold)    (THIS) PURE;
    STDMETHOD(SetPruneThreshold)            (THIS_
                                             UINT32 ulMaxLogSpace) PURE;

    // Size Rolling Functions
    STDMETHOD_(INT32,GetRollSize)           (THIS) PURE;
    STDMETHOD(SetRollSize)                  (THIS_
                                             UINT32 nRollSize) PURE;
    
    // Relative Time Rolling Functions
    STDMETHOD_(INT32,GetRollTimeOffset)     (THIS) PURE;
    STDMETHOD(SetRollTimeOffset)            (THIS_
                                             UINT32 nRollTimeOffset) PURE;
    STDMETHOD(SetRollTimeOffset)            (THIS_
                                             IHXBuffer* pRollTimeOffset) PURE;
                                            
    // Absolute Time Rolling Functions
    STDMETHOD_(const char*,GetRollTimeAbs)  (THIS) PURE;
    STDMETHOD(SetRollTimeAbs)               (THIS_
                                             const char *pRollTimeAbs) PURE;

    // No Rolling
    STDMETHOD(SetNoRoll)                    (THIS) PURE;

    // Set BaseFilename
    STDMETHOD_(const char*, GetBaseFilename) (THIS) PURE;                                             
    STDMETHOD(SetBaseFilename)              (THIS_
                                             const char *szBaseFilename) PURE;

    // Set Log Directory
    STDMETHOD_(const char*, GetLogDirectory) (THIS) PURE;
    STDMETHOD(SetLogDirectory)              (THIS_
                                             const char *szLogDirectory) PURE;
};

#endif /* _HXLOGOUTPUT_H_ */
