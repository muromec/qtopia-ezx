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

#ifndef _RIFF_H_
#define _RIFF_H_

#include "hxfiles.h"
#include "hxengin.h"

#define MAXLEVELS 64

// To improve performance under the Simple File System, we restrict reads to
// a maximum of this value (in the future, this should be registry based):
#define MAX_READ_SIZE (1 << 16)

class CRIFFResponse;
typedef _INTERFACE IHXFileObject IHXFileObject;

class CRIFFReader : public IHXFileResponse
{
public:
    CRIFFReader(IUnknown*, CRIFFResponse*, IHXFileObject*);
    ~CRIFFReader();

    // CRIFFReader methods
    /*
     * Open: Opens the given file for reading
     */
    HX_RESULT Open(char* filename);

    /*
     * Close: Close the file
     */
    HX_RESULT Close();
    /* FindChunk:
     * At the current level of the file, search for a chunk with the given id.
     * If not relative, start at the beginning of the file.  If relative,
     * Start from the beginning of next chunk (or at the current file position
     * if it is already pointing at the start of a chunk)
     */
    HX_RESULT FindChunk(UINT32 id, HXBOOL bRelative);
    /* Descend:
     * If pointing at a list or riff chunk, begin parsing the contents
     * of the list
     */
    HX_RESULT Descend();
    /* Ascend:
     * Go back up a level from a previous Descend
     */
    HX_RESULT Ascend();
    /*
     * Read:
     * Read the given number of bytes from the file at the current
     * position
     */
    HX_RESULT Read(UINT32);
    /*
     * Seek:
     * Seek to the given position _within_the_current_chunk_.
     * If relative, seek relative to the current file position.
     * If not relative, seek relative to the beginning of the chunk
     */
    HX_RESULT Seek(UINT32, HXBOOL);
    /*
     * FileSeek:
     * Seek to the absolute position in the file.  Mainly for file formats
     * that store index chunks with file relative offsets.  Does not
     * check to see if a chunk boundary is crossed, make sure you know
     * what you're doing!
     */
    HX_RESULT FileSeek(UINT32);
    /*
     * GetChunk:
     * Read an entire chunk into a buffer
     */
    HX_RESULT GetChunk();

    HX_RESULT FindNextChunk();

    UINT32  CurrentOffset() { return m_ulCurOffset - m_ulThisChunkOffset; };

    UINT32  FileType();
    UINT32  FileSubtype();

    UINT32 m_ulFileSpecifiedReadSize;

    // IHXFileResponse methods
    STDMETHOD(QueryInterface) (THIS_
                   REFIID riid,
                   void** ppvObj);
    STDMETHOD_(ULONG32,AddRef) (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    STDMETHOD(InitDone)      (THIS_
                  HX_RESULT status);
    STDMETHOD(ReadDone)      (THIS_
                  HX_RESULT status,
                  IHXBuffer* pBuffer);
    STDMETHOD(SeekDone)      (THIS_
                  HX_RESULT status);
    STDMETHOD(CloseDone)     (THIS_
                  HX_RESULT status);
    STDMETHOD(WriteDone)     (THIS_
                  HX_RESULT status);

     /************************************************************************
     *  Method:
     *      IHXFileResponse::FileObjectReady
     *  Purpose:
     *      Notification interface provided by users of the IHXFileObject
     *      interface. This method is called by the IHXFileObject when the
     *      requested FileObject is ready. It may return NULL with
     *      HX_RESULT_FAIL if the requested filename did not exist in the
     *      same pool.
     */
    STDMETHOD(FileObjectReady)  (THIS_
                HX_RESULT status,
                IHXFileObject* pFileObject);

    // Special getlong and getshort that can handle files in either byte
    // order (depending on what file type we think we're dealing with)
    INT32 GetLong(UCHAR* data)
    {
    if(m_bLittleEndian)
        return
        (INT32)((((unsigned long)data[3]) << 24) | (((unsigned long)data[2]) << 16) | (((unsigned long)data[1]) << 8) | (unsigned long)data[0]);
    else
        return
            (INT32)((((unsigned long)data[0]) << 24) | (((unsigned long)data[1]) << 16) | (((unsigned long)data[2]) << 8) | (unsigned long)data[3]);
    };

    INT16 GetShort(UCHAR* data)
    {
    if(m_bLittleEndian)
        return (data[1] << 8) | data[0];
    else
        return (data[0] << 8) | data[1];
    };

    UINT32 GetListType();
    UINT32 GetOffset();
    UINT32 GetChunkType();
    UINT32 GetChunkRawSize(void);

private:
    struct LevelInfo
    {
    LevelInfo()
        : m_startOffset(0)
        , m_nextChunkOffset(0)
        , started(FALSE)
    {}

    UINT32 m_startOffset;
    UINT32 m_nextChunkOffset;
    HXBOOL   started;
    };

    INT32                 m_lRefCount;

    IHXFileObject*       m_pFileObject;
    CRIFFResponse*        m_pResponse;
    IUnknown*             m_pContext;
    HXBOOL                  m_bFileIsOpen;
    char*                 m_pFilename;
    UINT32                m_ulFindChunkId;
    UINT32                m_ulChunkBodyLen;
    HXBOOL                  m_bLittleEndian;
    UINT32                m_ulLevel;
    UINT32                m_ulSeekOffset;
    UINT32                m_ulCurOffset;
    UINT32                m_ulFileType;
    UINT32                m_ulSubFileType;
    UINT32                m_ulChunkType;
    UINT32                m_ulChunkSubType;
    LevelInfo             m_levelInfo[MAXLEVELS];
    UINT32                m_ulThisChunkOffset;
    UINT32                m_ulGetChunkType;
    UINT32                m_ulSizeDiff;

    // For chunks > MAX_READ_SIZE, our reassembly buffer and associated
    // variables:
    IHXBuffer*           m_pReassemblyBuffer;
    UINT32                m_ulChunkBytesRead;
    UINT32                m_ulChunkSize;

    typedef enum
    {
    RS_Ready,
    RS_InitPending,
    RS_ChunkHeaderReadPending,
    RS_FileStartSeekPending,
    RS_ChunkBodySeekPending,
    RS_GetFileTypePending,
    RS_AscendSeekPending,
    RS_GetActualFileTypePending,
    RS_DataReadPending,
    RS_GetListTypePending,
    RS_UserSeekPending,
    RS_ReadChunkHeaderPending,
    RS_GetChunkHeaderPending,
    RS_ReadChunkBodyPending
    }
    RiffState;

    RiffState m_state;

    HX_RESULT InternalClose();

};

#endif
