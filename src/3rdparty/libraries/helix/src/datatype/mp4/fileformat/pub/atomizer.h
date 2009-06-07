/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef _ATOMIZER_H_
#define _ATOMIZER_H_

/****************************************************************************
 *  Defines
 */
#define ATOMIZE_ALL	0

#ifdef QTCONFIG_SPEED_OVER_SIZE
#define QTATOMIZER_INLINE inline
#else	// QTCONFIG_SPEED_OVER_SIZE
#define QTATOMIZER_INLINE /**/
#endif	// QTCONFIG_SPEED_OVER_SIZE

/****************************************************************************
 *  Includes
 */
#include "hxatmzr.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "fswtchr.h"
#include "hxerror.h"

/****************************************************************************
 * 
 *  Class:
 *	CAtomizer
 *
 *  Purpose:
 *	Loads QT atoms into the atom tree from a file.
 *
 */
class CAtomizer :   public IHXFileResponse,
                    public IHXThreadSafeMethods,
		    public IHXAtomizationCommander
{
public:
    /*
     *	Constructor
     */
    CAtomizer(void);

    /*
     *	Main Interface
     */
    HX_RESULT Init(IUnknown* pSource,
	           IUnknown* pResponse,
		   IUnknown* pCommander = NULL);

    void Close(void);

    HX_RESULT Atomize(	ULONG32 ulOffset = 0,
			ULONG32 ulSize = ATOMIZE_ALL);

    const CQTAtom* GetRootAtom() {return (const CQTAtom*)m_pRoot;}

    /*
     *	IUnknown methods
     */
    STDMETHOD (QueryInterface )	    (THIS_ 
				    REFIID ID, 
				    void** ppInterfaceObj);

    STDMETHOD_(UINT32, AddRef )	    (THIS);

    STDMETHOD_(UINT32, Release)	    (THIS);

    /*
     *  IHXFileResponse methods
     */
    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //    IHXFileResponse::InitDone
    //  Purpose:
    //    Notification interface provided by users of the IHXFileObject
    //    interface. This method is called by the IHXFileObject when the
    //    initialization of the file is complete, and the Mime type is
    //    available for the request file. If the URL is not valid for the
    //    file system, the status HXR_FAILED should be returned,
    //    with a mime type of NULL. If the URL is valid but the mime type
    //    is unknown, then the status HXR_OK should be returned with
    //    a mime type of NULL.
    //
    STDMETHOD(InitDone)	    (THIS_
			    HX_RESULT	    status)
    {
	HX_ASSERT(HXR_NOTIMPL == HXR_OK);
	return HXR_NOTIMPL;
    }

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileResponse::CloseDone
    //  Purpose:
    //  	Notification interface provided by users of the IHXFileObject
    //  	interface. This method is called by the IHXFileObject when the
    //  	close of the file is complete.
    //
    STDMETHOD(CloseDone)	(THIS_
				HX_RESULT	status)
    {
	HX_ASSERT(HXR_NOTIMPL == HXR_OK);
	return HXR_NOTIMPL;
    }

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
				HX_RESULT	    status)
    {
	HX_ASSERT(HXR_NOTIMPL == HXR_OK);
	return HXR_NOTIMPL;
    }

    /////////////////////////////////////////////////////////////////////////
    //  Method:
    //  	IHXFileResponse::SeekDone
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
					IHXFileObject* pFileObject)
    {
	HX_ASSERT(HXR_NOTIMPL == HXR_OK);
	return HXR_NOTIMPL;
    }

    /*
     *	IHXAtomizationCommander methods
     */
    STDMETHOD_(QTAtomizerCmd,GetAtomCommand)   (THIS_
						QTAtomType AtomType,
						CQTAtom *pParent)
    {
	return ATMZR_CMD_LOAD;
    }

   ///////////////////////////////////////////////////////////////////////////
   //  Method:
   //		IHXThreadSafeMethods::IsThreadSafe
   //  Purpose:
   //		This routine returns threadsafeness information about the file object
   //		which is used by the server for improved performance.
   //
   STDMETHOD_(UINT32,IsThreadSafe) (THIS);

private:
    typedef enum
    {
	ATMZR_Offline,
	ATMZR_Ready, 
	ATMZR_ProcHeader,
	ATMZR_ProcNewHeader,
	ATMZR_ProcExtendedSize,
	ATMZR_MakeAtom,
	ATMZR_ProcBody
    } AtomizerState;

    typedef enum
    {
	ATMZR_CBSTEP_Read,
	ATMZR_CBSTEP_Seek
    } CallbackStep;

    class CRecursionCallback : public IHXCallback
    {
    public:
	CRecursionCallback(CAtomizer *pAtomizer);
	~CRecursionCallback();

	/*
	 *  IHXCallback methods
	 */
	STDMETHOD(Func)		(THIS);
	    
        /*
	 *  IUnknown methods
	 */
	STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj);
	STDMETHOD_(ULONG32,AddRef)	(THIS);
	STDMETHOD_(ULONG32,Release)	(THIS);

    private:
	INT32	    m_lRefCount;
	CAtomizer*  m_pAtomizer;
    };

    HX_RESULT ReadData(ULONG32 ulSize);
    HX_RESULT SeekData(ULONG32 ulOffset, HXBOOL bRelative = FALSE);

    inline HX_RESULT ReadDataCB(ULONG32 ulSize);
    inline HX_RESULT SeekDataCB(ULONG32 ulOffset, HXBOOL bRelative = FALSE);

    HXBOOL IsOffsetInRange(ULONG32 ulOffset)
    {
	return (m_ulFinalOffset == ATOMIZE_ALL) || 
	       (ulOffset < m_ulFinalOffset);
    }

    QTATOMIZER_INLINE HX_RESULT AdjustCurrentRoot(void);
    void CompleteAtomization(HX_RESULT status);

    AtomizerState m_State;
    IHXFileSwitcher *m_pFileSwitcher;
    IHXAtomizerResponse *m_pResponse;
    IHXAtomizationCommander *m_pCommander;    
    IHXScheduler *m_pScheduler;
    IHXErrorMessages*	m_pErrorMessages;

    HXBOOL m_bSyncAccessEnabled;
    HXBOOL m_bPreferLinearAccess;

    CQTAtom *m_pRoot;
    CQTAtom *m_pCurrentRoot;
    CQTAtom *m_pNewAtom;

    ULONG32 m_ulStartOffset;
    ULONG32 m_ulFinalOffset;
    ULONG32 m_ulCurrentOffset;
    ULONG32 m_ulNewAtomOffset;
    ULONG32 m_ulNewAtomDataSize;
    QTAtomType m_AtomType;
    ULONG32 m_ulTotalSize;

    CRecursionCallback *m_pRecursionCallback;
    ULONG32 m_ulRecursionCount;
    CallbackStep m_CallbackStep;
    ULONG32 m_ulSize;
    HXBOOL m_bRelative;

    LONG32 m_lRefCount;

    friend class CRecursionCallback;

    ~CAtomizer(); 

};

#endif  // _ATOMIZER_H_
