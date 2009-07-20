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
/////////////////////////////////////////////////////////////////////////////
// 
//  mp4arch.h

#ifndef _MP4ARCH_H_
#define _MP4ARCH_H_


/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxmap.h"
#include "hxslist.h"
#include "carray.h"
#include "archivr2.h"


class ASMRuleBook;
class CPNSaveFile;
class CHXTimestampConverter;

typedef enum
{
    UNKNOWN_CONNECTION,
    AUDIO_CONNECTION,
    VIDEO_CONNECTION
} CONNECTION_TYPE;

class CStubFileResponse : public IHXFileResponse
{
public:
    CStubFileResponse() { m_lRefCount = 0; }
    ~CStubFileResponse() { }
    STDMETHOD(QueryInterface)		(THIS_
					REFIID		riid,
					void**		ppvObj)
    {
	if (IsEqualIID(riid, IID_IUnknown))
	{
	    AddRef();
	    *ppvObj = (IUnknown*)this;
	    return HXR_OK;
	}
	else if (IsEqualIID(riid, IID_IHXFileResponse))
	{
	    AddRef();
	    *ppvObj = (IHXFileResponse*)this;
	    return HXR_OK;
	}
	*ppvObj = NULL;
	return HXR_NOINTERFACE;
    }
    // Reference counting is not properly implemented; it only exists here to
    // make IUnknown happy, but in actuality the only instance of this class should
    // be a member of simplearchiver
    STDMETHOD_(ULONG32,AddRef)		(THIS)
    {    return InterlockedIncrement(&m_lRefCount); }
    STDMETHOD_(ULONG32,Release)		(THIS)
    {   
	if (InterlockedDecrement(&m_lRefCount) > 0)
	    return m_lRefCount;
	return 0;
    }

    STDMETHOD(InitDone)(HX_RESULT	    status)
    { return HXR_OK; }
    STDMETHOD(CloseDone)(THIS_
					HX_RESULT	    status)
    { return HXR_OK; }
    STDMETHOD(ReadDone)(THIS_ 
					HX_RESULT	    status,
					IHXBuffer*	    pBuffer)
    { return HXR_OK; }
    STDMETHOD(WriteDone)(THIS_ 
					HX_RESULT	    status)
    { return HXR_OK; }
    STDMETHOD(SeekDone)(THIS_ 
					HX_RESULT	    status)
    { return HXR_OK; }
private:
    UINT32 m_lRefCount;
};

class CMP4Atom;

/////////////////////////////////////////////////////////////////////////////
// 
//  Class: CMP4Archiver
//
class CMP4Archiver : public CBaseArchiver2
{
public:
    CMP4Archiver(IUnknown* pContext, 
		 IHXFileWriterMonitor* pMonitor, 
		 IHXPropertyAdviser* pAdviser);

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID		riid,
					void**		ppvObj);
    STDMETHOD_(ULONG32,AddRef)		(THIS);
    STDMETHOD_(ULONG32,Release)		(THIS);

    /*
     *  IRMAFileCreatorResponse
     */
    STDMETHOD(ArchiveDirReady)		(HX_RESULT status);
    STDMETHOD(ArchiveFileReady)		(HX_RESULT status,
					IHXFileObject* pFileObject);
    /*
     * Method for writing or updating a specific atom
     */
    STDMETHOD(WriteAtom)                (THIS_ CMP4Atom* pAtom, BOOL bIncludeChildren);

private:
    virtual ~CMP4Archiver();
	
    BOOL				m_bAborted;
    IHXFileObject*			m_pSaveFile;
    UINT32                              m_ulFileOffset;
    CONNECTION_TYPE			m_ConnectionType;
    CHXString				m_OutputFileName;
    CHXString				m_TempOutputFileName;
    CStubFileResponse			m_StubResponse;
    BOOL				m_bResolvedOutputConflict;
    BOOL				m_bDone;

    IHXFileObject*			m_pOutputFileRenamer;

    HX_RESULT   WriteBuffer( IHXBuffer* pBuffer );
    HX_RESULT	CreateArchiveFileObject	    ();
    HX_RESULT	CreateSaveFile		    (IHXFileObject* pFileObject);
    void	CloseSaveFile		    ();

    void	NotifyAbruptEnd		(BOOL bAbruptEnd = TRUE);

protected:

    virtual HX_RESULT OnAbort		    ();
    virtual HX_RESULT OnNewMetaInfo	    (IHXValues* pMetaInfo);
    virtual HX_RESULT OnNewFileHeader	    (IHXValues* pHeader);
    virtual HX_RESULT OnNewStreamHeader	    (IHXValues* pHeader);
    virtual HX_RESULT OnNewPacket	    (IHXPacket* pPacket);
    virtual HX_RESULT CreateFileObjects	    ();
    virtual HX_RESULT OnDone		    ();
};

#endif /* _MP4ARCH_H_ */


