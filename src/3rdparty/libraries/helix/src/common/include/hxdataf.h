/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdataf.h,v 1.9 2007/07/06 20:43:41 jfinnecy Exp $
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

#ifndef _DATAFILE_H_
#define _DATAFILE_H_

typedef _INTERFACE	IUnknown			IUnknown;
typedef _INTERFACE	IHXBuffer			IHXBuffer;
typedef _INTERFACE	IHXDataFile			IHXDataFile;
typedef _INTERFACE	IHXDataFileFactory		IHXDataFileFactory;
typedef _INTERFACE	IHXCallback			IHXCallback;

// IHXDataFile::Read status codes
// HX_FILESTATUS_PHYSICAL_EOF	: Found the physical end of file
// HX_FILESTATUS_ERROR		: Unknown error occurred, might not be fatal, try more reads
// HX_FILESTATUS_LOGICAL_EOF	: Found the logical end of file
// HX_FILESTATUS_DATA_PENDING	: Data source is expecting more data, try read again later, indefinitely
// HX_FILESTATUS_FATAL_ERROR	: Unrecoverable error occurred, do not try to read again
//
#define HX_FILESTATUS_PHYSICAL_EOF    (0)
#define HX_FILESTATUS_ERROR	     (-1)
#define HX_FILESTATUS_LOGICAL_EOF  (-111)
#define HX_FILESTATUS_DATA_PENDING (-222)
#define HX_FILESTATUS_FATAL_ERROR  (-333)

enum 
{
    HX_FILEFLAG_READ = 1,
    HX_FILEFLAG_WRITE = 2,
    HX_FILEFLAG_BINARY = 4,
    HX_FILEFLAG_NOTRUNC = 8
};

/*
 * 
 *  Interface:
 *
 *	IHXDataFileFactory
 *
 *  Purpose:
 *
 *	This interface provides an API for creating datafile objects
 *      based on passed in options and platform compiled for.
 *
 *		IHXDataFileFactory
 *		|
 *		IHXDataFile
 *		|__________________________________________________...
 *		|		|                |                |
 *		Buffered	UnBuffered       Mem.Mapped       Async
 *
 *  IID_IHXDataFileFactory: {00000F00-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXDataFileFactory,	0x00000F00, 0xb4c8, 0x11d0, 0x99, 0x95, 
					0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);
#undef  INTERFACE
#define INTERFACE   IHXDataFileFactory

DECLARE_INTERFACE_(IHXDataFileFactory, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;
    
    STDMETHOD_(ULONG32, AddRef)		(THIS) PURE;
    
    STDMETHOD_(ULONG32, Release)	(THIS) PURE;
    
    /*
     *  IHXDataFileFactory methods
     */

    /* Creates a datafile for buffered or unbuffered I/O
     * By default create an IHXDataFile for BUFFERED file I/O 
     * for all platforms, except SOLARIS, where UNBUFFERED 
     * file I/O is required.
     */
    STDMETHOD(CreateFile)		(THIS_
					REF(IHXDataFile*) pDataFile,
					IUnknown*  pContext,
					REF(IUnknown*) pPersistantObject,
					HXBOOL bDisableMemoryMappedIO,
					UINT32 ulChunkSize,
					HXBOOL bEnableFileLocking,
					HXBOOL bPreferAsyncIO
					) PURE;
};

/*
 * 
 *  Interface:
 *
 *	IHXDataFile
 *
 *  Purpose:
 *
 *	This interface provides an API for basic file access to datafiles. 
 *	The datafile object is crated based on passed in options and
 *	platform compiled for.
 *
 *		IHXDataFileFactory
 *		|
 *		IHXDataFile
 *		|__________________________________________________...
 *		|		|                |                |
 *		Buffered	UnBuffered       Mem.Mapped       Async
 *
 *  IID_IHXDataFile:        {00000F01-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXDataFile,		0x00000F01, 0xb4c8, 0x11d0, 0x99, 0x95, 
					0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);
#undef  INTERFACE
#define INTERFACE   IHXDataFile

DECLARE_INTERFACE_(IHXDataFile, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;
    
    STDMETHOD_(ULONG32, AddRef)		(THIS) PURE;
    
    STDMETHOD_(ULONG32, Release)	(THIS) PURE;
    
    /*
     *  IHXDataFile methods
     */

    /* Bind DataFile Object with FileName */
    STDMETHOD_(void, Bind)		(THIS_
					const char* FileName) PURE;

    /* Creates a datafile using the specified mode
     * uOpenMode --- File Open mode - HX_FILE_READ/HX_FILE_WRITE/HX_FILE_BINARY
     */
    STDMETHOD(Create)			(THIS_
					UINT16 uOpenMode) PURE;

    /* Open will open a file with the specified mode
     */
    STDMETHOD(Open)			(THIS_
					UINT16 uOpenMode) PURE;

    /* Close closes a file 
     * If the reference count on the IHXDataFile object is greater than 1, 
     * then the underlying file cannot be safely closed, so Close() becomes 
     * a noop in that case. Close it only when the object is destroyed. 
     * This would be safe, but could lead to a file descriptor leak.
     */
    STDMETHOD(Close)			(THIS) PURE;

    /* Name returns the currently bound file name in FileName.
     * and returns TRUE, if the a name has been bound.  Otherwise
     * FALSE is returned.
     */
    STDMETHOD_(HXBOOL, Name)		(THIS_
					REF(IHXBuffer*) pFileName) PURE;

    /*
     * IsOpen returns TRUE if file is open.  Otherwise FALSE.
     */
    STDMETHOD_(HXBOOL, IsOpen)		(THIS) PURE;

    /* Seek moves the current file position to the offset from the
     * fromWhere specifier returns current position of file or -1 on
     * error.
     */
    STDMETHOD(Seek)			(THIS_
					ULONG32 offset, UINT16 fromWhere) PURE;

    /* Tell returns the current file position in the file */
    STDMETHOD_(ULONG32, Tell)		(THIS) PURE;

    /* Read reads up to count bytes of data into buf.
     * returns the number of bytes read, EOF, or -1 if the read failed OR one of the following status codes
     * HX_FILESTATUS_PHYSICAL_EOF	: Found the physical end of file
     * HX_FILESTATUS_ERROR		: Unknown error occurred, might not be fatal, try more reads
     * HX_FILESTATUS_LOGICAL_EOF	: Found the logical end of file
     * HX_FILESTATUS_DATA_PENDING	: Data source is expecting more data, try read again later, indefinitely
     * HX_FILESTATUS_FATAL_ERROR	: Unrecoverable error occurred, do not try to read again
     */
    STDMETHOD_(ULONG32, Read)		(THIS_
					REF(IHXBuffer*) pBuf, 
					ULONG32 count) PURE;

    /* Write writes up to count bytes of data from buf.
     * returns the number of bytes written, or -1 if the write failed 
     */
    STDMETHOD_(ULONG32, Write)		(THIS_
					REF(IHXBuffer*) pBuf) PURE;

    /* Flush out the data in case of unbuffered I/O
     */
    STDMETHOD(Flush)			(THIS) PURE;

    /*
     * Return info about the data file such as permissions, time of creation
     * size in bytes, etc.
     */
    STDMETHOD(Stat)			(THIS_
					struct stat* buffer) PURE;

    /* Delete File */
    STDMETHOD(Delete)			(THIS) PURE;

    /* Return the file descriptor */
    STDMETHOD_(INT16, GetFd)		(THIS) PURE;

    /* GetLastError returns the platform specific file error */
    STDMETHOD(GetLastError)		(THIS) PURE;

    /* GetLastError returns the platform specific file error in
     * string form.
     */
    STDMETHOD_(void, GetLastError)	(THIS_
					REF(IHXBuffer*) err) PURE;
};


/*
 * 
 *  Interface:
 *
 *	IID_IHXAsyncDataFile
 *
 *  Purpose:
 *
 *	This interface provides an API for basic data file operations and
 *  asynchronous read, write and seek operations.
 *
 */
DEFINE_GUID(IID_IHXAsyncDataFile,   0x972bacc0, 0xaff, 0x11d7, 0xac, 
				    0x45, 0x0, 0x1, 0x2, 0x51, 0xb3, 0x40);		

#undef  INTERFACE
#define INTERFACE   IHXAsyncDataFile

DECLARE_INTERFACE_(IHXAsyncDataFile, IHXDataFile)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;
    
    STDMETHOD_(ULONG32, AddRef)		(THIS) PURE;
    
    STDMETHOD_(ULONG32, Release)	(THIS) PURE;
    
    /*
     *  IHXDataFile methods
     */

    /* Bind DataFile Object with FileName */
    STDMETHOD_(void, Bind)		(THIS_
					const char* FileName) PURE;

    /* Creates a datafile using the specified mode
     * uOpenMode --- File Open mode - HX_FILE_READ/HX_FILE_WRITE/HX_FILE_BINARY
     */
    STDMETHOD(Create)			(THIS_
					UINT16 uOpenMode) PURE;

    /* Open will open a file with the specified mode
     */
    STDMETHOD(Open)			(THIS_
					UINT16 uOpenMode) PURE;

    /* Close closes a file 
     * If the reference count on the IHXDataFile object is greater than 1, 
     * then the underlying file cannot be safely closed, so Close() becomes 
     * a noop in that case. Close it only when the object is destroyed. 
     * This would be safe, but could lead to a file descriptor leak.
     */
    STDMETHOD(Close)			(THIS) PURE;

    /* Name returns the currently bound file name in FileName.
     * and returns TRUE, if the a name has been bound.  Otherwise
     * FALSE is returned.
     */
    STDMETHOD_(HXBOOL, Name)		(THIS_
					REF(IHXBuffer*) pFileName) PURE;

    /*
     * IsOpen returns TRUE if file is open.  Otherwise FALSE.
     */
    STDMETHOD_(HXBOOL, IsOpen)		(THIS) PURE;

    /* Seek moves the current file position to the offset from the
     * fromWhere specifier returns current position of file or -1 on
     * error.
     */
    STDMETHOD(Seek)			(THIS_
					ULONG32 offset, UINT16 fromWhere) PURE;

    /* Tell returns the current file position in the file */
    STDMETHOD_(ULONG32, Tell)		(THIS) PURE;

    /* Read reads up to count bytes of data into buf.
     * returns the number of bytes read, EOF, or -1 if the read failed 
     */
    STDMETHOD_(ULONG32, Read)		(THIS_
					REF(IHXBuffer*) pBuf, 
					ULONG32 count) PURE;

    /* Write writes up to count bytes of data from buf.
     * returns the number of bytes written, or -1 if the write failed 
     */
    STDMETHOD_(ULONG32, Write)		(THIS_
					REF(IHXBuffer*) pBuf) PURE;

    /* Flush out the data in case of unbuffered I/O
     */
    STDMETHOD(Flush)			(THIS) PURE;

    /*
     * Return info about the data file such as permissions, time of creation
     * size in bytes, etc.
     */
    STDMETHOD(Stat)			(THIS_
					struct stat* buffer) PURE;

    /* Delete File */
    STDMETHOD(Delete)			(THIS) PURE;

    /* Return the file descriptor */
    STDMETHOD_(INT16, GetFd)		(THIS) PURE;

    /* GetLastError returns the platform specific file error */
    STDMETHOD(GetLastError)		(THIS) PURE;

    /* GetLastError returns the platform specific file error in
     * string form.
     */
    STDMETHOD_(void, GetLastError)	(THIS_
					REF(IHXBuffer*) err) PURE;


    /*
     *  IHXAsyncDataFile
     */
    STDMETHOD(SetReceiver)		(THIS_
					IHXCallback* pCallback) PURE;

    STDMETHOD(SeekAsync)		(THIS_
					REF(HX_RESULT) status, 
					ULONG32 offset, UINT16 fromWhere) PURE;

    STDMETHOD(ReadAsync)		(THIS_
					REF(ULONG32) ulReadSize,
					REF(IHXBuffer*) pBuf, 
					ULONG32 count) PURE;

    STDMETHOD(WriteAsync)		(THIS_
					REF(ULONG32) ulWrittenSize,
					REF(IHXBuffer*) pBuf) PURE;
};


#if defined(_SYMBIAN)
#include "platform/symbian/ihxsymbfsessionmgr.h"
#endif	// _SYMBIAN

#endif /* _DATAFILE_H_ */
