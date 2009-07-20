/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ramffpln.h,v 1.5 2007/02/08 18:24:16 ehyche Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#ifndef _RAMFFPLN_H_
#define _RAMFFPLN_H_

#include "hxplayvelocity.h"

/////////////////////////////////////////////////////////////////////////////
// 
//  Class:
//
//  	CRAMFileFormat
//
//  Purpose:
//
//  	Example implementation of a basic file format.
//

class CRAMFileFormat :	public IHXPlugin, 
			public IHXFileFormatObject, 
			public IHXFileResponse,
			public IHXInterruptSafe,
                        public IHXPlaybackVelocity
{
private:
    friend class CRealAudioEventResponse;

    LONG32			m_lRefCount;
    IUnknown*			m_pContext;

    HXBOOL			m_bFirstReadDone;
    HXBOOL			m_bHeaderSent;
    UINT32			m_ulPersistentVersion;
    IHXFileObject*		m_pFileObject;
    IHXFormatResponse*		m_pFFResponse;
    IHXCommonClassFactory*	m_pCommonClassFactory;
    IHXRequest*		m_pRequest;
    IHXBuffer*			m_pBuffer;

    HX_RESULT			m_uLastError;

    typedef enum
    {
	Ready, 
	InitPending,
	ReadPending
    }
    State;

    State			m_state;

    static const char*		zm_pDescription;
    static const char*		zm_pCopyright;
    static const char*		zm_pMoreInfoURL;

    static const char*		zm_pFileMimeTypes[];
    static const char*		zm_pStreamMimeTypes[];
    static const char*		zm_pFileExtensions[];
    static const char*		zm_pFileOpenNames[];

    ~CRAMFileFormat();


public:
    CRAMFileFormat();

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    // *** IHXPlugin methods ***

    /************************************************************************
     *	Method:
     *	    IHXPlugin::GetPluginInfo
     *	Purpose:
     *	    Returns the basic information about this plugin. Including:
     *
     *	    bLoadMultiple	whether or not this plugin DLL can be loaded
     *				multiple times. All File Formats must set
     *				this value to TRUE.
     *	    pDescription	which is used in about UIs (can be NULL)
     *	    pCopyright		which is used in about UIs (can be NULL)
     *	    pMoreInfoURL	which is used in about UIs (can be NULL)
     */
    STDMETHOD(GetPluginInfo)	(THIS_
				REF(HXBOOL)        /*OUT*/ bLoadMultiple,
				REF(const char*) /*OUT*/ pDescription,
				REF(const char*) /*OUT*/ pCopyright,
				REF(const char*) /*OUT*/ pMoreInfoURL,
				REF(ULONG32)	 /*OUT*/ ulVersionNumber
				);

    /************************************************************************
     *	Method:
     *	    IHXPlugin::InitPlugin
     *	Purpose:
     *	    Initializes the plugin for use. This interface must always be
     *	    called before any other method is called. This is primarily needed 
     *	    so that the plugin can have access to the context for creation of
     *	    IHXBuffers and IMalloc.
     */
    STDMETHOD(InitPlugin)   (THIS_
			    IUnknown*   /*IN*/  pContext);

    /*
     *	IHXFileFormatObject methods
     */

    STDMETHOD(GetFileFormatInfo)(THIS_
				REF(const char**) /*OUT*/ pFileMimeTypes,
				REF(const char**) /*OUT*/ pFileExtensions,
				REF(const char**) /*OUT*/ pFileOpenNames
				);

    STDMETHOD(InitFileFormat)	
			(THIS_
		        IHXRequest*		/*IN*/	pRequest, 
			IHXFormatResponse*	/*IN*/	pFormatResponse,
			IHXFileObject*		/*IN*/  pFileObject);

    STDMETHOD(GetFileHeader)	(THIS);

    STDMETHOD(GetStreamHeader)	(THIS_
				UINT16 unStreamNumber);

    STDMETHOD(GetPacket)	(THIS_
				UINT16 unStreamNumber);

    STDMETHOD(Seek)		(THIS_
				ULONG32 ulOffset);

    STDMETHOD(Close)		(THIS);


    // *** IHXFileResponse methods ***

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileResponse::InitDone
    //  Purpose:
    //  	Notification interface provided by users of the IHXFileObject
    //  	interface. This method is called by the IHXFileObject when the
    //  	initialization of the file is complete.
    //
    STDMETHOD(InitDone)		(THIS_
				HX_RESULT	status);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileResponse::CloseDone
    //  Purpose:
    //  	Notification interface provided by users of the IHXFileObject
    //  	interface. This method is called by the IHXFileObject when the
    //  	close of the file is complete.
    //
    STDMETHOD(CloseDone)	(THIS_
				HX_RESULT	status);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileResponse::ReadDone
    //  Purpose:
    //  	Notification interface provided by users of the IHXFileObject
    //  	interface. This method is called by the IHXFileObject when the
    //  	last read from the file is complete and a buffer is available.
    //
    STDMETHOD(ReadDone)		(THIS_ 
				HX_RESULT	    status,
				IHXBuffer*	    pBuffer);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileResponse::WriteDone
    //  Purpose:
    //  	Notification interface provided by users of the IHXFileObject
    //  	interface. This method is called by the IHXFileObject when the
    //  	last write to the file is complete.
    //
    STDMETHOD(WriteDone)	(THIS_ 
				HX_RESULT	    status);

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileResponse::WriteDone
    //  Purpose:
    //  	Notification interface provided by users of the IHXFileObject
    //  	interface. This method is called by the IHXFileObject when the
    //  	last seek in the file is complete.
    //
    STDMETHOD(SeekDone)		(THIS_ 
				HX_RESULT	    status);

   /************************************************************************
     *	Method:
     *	    IHXFileResponse::FileObjectReady
     *	Purpose:
     *	    Notification interface provided by users of the IHXFileObject
     *	    interface. This method is called by the IHXFileObject when the
     *	    requested FileObject is ready. It may return NULL with 
     *	    HX_RESULT_FAIL if the requested filename did not exist in the 
     *	    same pool.
     */
    STDMETHOD(FileObjectReady)		(THIS_
					HX_RESULT status,
					IHXFileObject* pFileObject);

    /*
     *	IHXInterruptSafe methods
     */

    /************************************************************************
     *	Method:
     *	    IHXInterruptSafe::IsInterruptSafe
     *	Purpose:
     *	    This is the function that will be called to determine if
     *	    interrupt time execution is supported.
     */
    STDMETHOD_(HXBOOL,IsInterruptSafe)	(THIS) 
    					{ return TRUE; };

    // IHXPlaybackVelocity methods
    STDMETHOD(InitVelocityControl)         (THIS_ IHXPlaybackVelocityResponse* pResponse) { return HXR_OK; }
    STDMETHOD(QueryVelocityCaps)           (THIS_ REF(IHXPlaybackVelocityCaps*) rpCaps) { return HXR_NOTIMPL; }
    STDMETHOD(SetVelocity)                 (THIS_ INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch) { return HXR_OK; }
    STDMETHOD_(INT32,GetVelocity)          (THIS) { return 0; }
    STDMETHOD(SetKeyFrameMode)             (THIS_ HXBOOL bKeyFrameMode) { return HXR_OK; }
    STDMETHOD_(HXBOOL,GetKeyFrameMode)     (THIS) { return FALSE; }
    STDMETHOD_(UINT32,GetKeyFrameInterval) (THIS) { return 0; }
    STDMETHOD(CloseVelocityControl)        (THIS) { return HXR_OK; }
};

#endif /* _RAMFFPLN_H_ */

