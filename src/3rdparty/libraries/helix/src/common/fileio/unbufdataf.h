/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: unbufdataf.h,v 1.7 2007/07/06 20:35:11 jfinnecy Exp $
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

#ifndef _UNBUFFERED_DATA_FILE_H_
#define _UNBUFFERED_DATA_FILE_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxdataf.h"

class UnBufferedDataFile : public IHXDataFile
{
public:
					UnBufferedDataFile(IUnknown* pContext);
    virtual				~UnBufferedDataFile();

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj);
    
    STDMETHOD_(ULONG32, AddRef)		(THIS);
    
    STDMETHOD_(ULONG32, Release)	(THIS);
    
    /*
     *  IHXDataFile methods
     */

    /* Bind DataFile Object with FileName */
    STDMETHOD_(void, Bind)		(THIS_
					const char* FileName);

    /* Creates a datafile using the specified mode
     * uOpenMode --- File Open mode - HX_FILE_READ/HX_FILE_WRITE/HX_FILE_BINARY
     */
    STDMETHOD(Create)			(THIS_
					UINT16 uOpenMode);

    /* Open will open a file with the specified mode
     */
    STDMETHOD(Open)			(THIS_
					UINT16 uOpenMode);

    /* Close closes a file 
     * If the reference count on the IHXDataFile object is greater than 1, 
     * then the underlying file cannot be safely closed, so Close() becomes 
     * a noop in that case. Close it only when the object is destroyed. 
     * This would be safe, but could lead to a file descriptor leak.
     */
    STDMETHOD(Close)			(THIS);

    /* Name returns the currently bound file name in FileName.
     * and returns TRUE, if the a name has been bound.  Otherwise
     * FALSE is returned.
     */
    STDMETHOD_(HXBOOL, Name)		(THIS_
					REF(IHXBuffer*) pFileName);

    /*
     * IsOpen returns TRUE if file is open.  Otherwise FALSE.
     */
    STDMETHOD_(HXBOOL, IsOpen)		(THIS);

    /* Seek moves the current file position to the offset from the
     * fromWhere specifier returns current position of file or -1 on
     * error.
     */
    STDMETHOD(Seek)			(THIS_
					ULONG32 offset, UINT16 fromWhere);

    /* Tell returns the current file position in the file */
    STDMETHOD_(ULONG32, Tell)		(THIS);

    /* Read reads up to count bytes of data into buf.
     * returns the number of bytes read, EOF, or -1 if the read failed 
     */
    STDMETHOD_(ULONG32, Read)		(THIS_
					REF(IHXBuffer*) pBuf, 
					ULONG32 count);

    /* Write writes up to count bytes of data from buf.
     * returns the number of bytes written, or -1 if the write failed 
     */
    STDMETHOD_(ULONG32, Write)		(THIS_
					REF(IHXBuffer*) pBuf); 

    /* Flush out the data in case of unbuffered I/O
     */
    STDMETHOD(Flush)			(THIS);

    /*
     * Return info about the data file such as permissions, time of creation
     * size in bytes, etc.
     */
    STDMETHOD(Stat)			(THIS_
					struct stat* buffer);

    /* Return the file descriptor */
    STDMETHOD_(INT16, GetFd)		(THIS);

    /* GetLastError returns the platform specific file error */
    STDMETHOD(GetLastError)		(THIS);

    /* GetLastError returns the platform specific file error in
     * string form.
     */
    STDMETHOD_(void, GetLastError)	(THIS_
					REF(IHXBuffer*) err);

protected:
    LONG32				m_lRefCount;
    UINT32				m_ulLastError;
    IUnknown*				m_pContext;
    IHXBuffer*				m_pFilename;
    int					m_nFD;
};

#endif /* _UNBUFFERED_DATA_FILE_H_ */
