/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxchkpt2.h,v 1.9 2007/07/06 20:35:09 jfinnecy Exp $
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


/*
    @description This is another cut at the RN checkpoint macros. It is designed to plugin seamlessly, 
    it defines the same macros as the main hxchkpt.h header.
      Removes checkpoint lists - all checkpoints now stored in the checkpoint manager
      Does not support accumulators
      Designed to produce output that is then post processed
*/

#ifndef _HXCHKPT2_H_
#define _HXCHKPT2_H_

#ifndef _RNPERFORMANCE_H_
#error This file must not be included on its own. You should include hxperf.h
#endif

#ifdef ENABLE_CHECKPOINTS2

#include "hlxclib/stdio.h"
#include "hxtick.h"

#ifdef SECTIONS_ONLY
// If we're just logging sections we don't need to allocate so much space for checkpoints
#define MAX_CHECKPOINTS_PER_MODULE 100
#define MAX_COMMENT_CHARACTERS 100
#else
#define MAX_CHECKPOINTS_PER_MODULE 600000
#define MAX_COMMENT_CHARACTERS 100000
#endif


struct CHXCheckpoint2
{
    enum CheckpointType { ENTER_BLOCK, EXIT_BLOCK, CHECKPOINT };
    
    const char* m_pFunctionName;
    const char* m_pComment;
    const char* m_pFileName;
    UINT32 m_ulLineNumber;
    double m_dTime;
    CheckpointType m_type;
};

class CHXCheckpointManager2
{
public:
    CHXCheckpointManager2( const char* pModuleName, const char* pOutputFileName )
    : m_pModuleName( pModuleName ),
      m_pOutputFileName( pOutputFileName ),
      m_ulCheckpointIndex( 0 ),
      m_pComments( m_comments )
    {
    }

    ~CHXCheckpointManager2()
    {
	// Since writing to a file slows down startup time timing data is only dumped to the file when the global checkpoint
	// manager object associated with a dll or exe is destroyed meaning an unload or program termination occured.
	DumpCheckpoints();
    }

    void AddCheckpoint( const char* pFunctionName, const char* pComment, const char* pFileName, UINT32 ulLineNumber, double dTime, CHXCheckpoint2::CheckpointType type )
    {
	if( m_ulCheckpointIndex < MAX_CHECKPOINTS_PER_MODULE )
	{
	    m_CheckpointArray[ m_ulCheckpointIndex ].m_pFunctionName = pFunctionName;
	    m_CheckpointArray[ m_ulCheckpointIndex ].m_pComment = pComment;
	    m_CheckpointArray[ m_ulCheckpointIndex ].m_pFileName = pFileName;
	    m_CheckpointArray[ m_ulCheckpointIndex ].m_ulLineNumber = ulLineNumber;
	    m_CheckpointArray[ m_ulCheckpointIndex ].m_dTime = dTime;
	    m_CheckpointArray[ m_ulCheckpointIndex ].m_type = type;
	}

	++m_ulCheckpointIndex;
    }

    const char* AddCheckpointCopy( const char* pFunctionName, const char* pComment, const char* pFileName, UINT32 ulLineNumber, double dTime, CHXCheckpoint2::CheckpointType type )
    {
    	const char* result = "Unknown";
    	
	if( m_ulCheckpointIndex < MAX_CHECKPOINTS_PER_MODULE )
	{
	    m_CheckpointArray[ m_ulCheckpointIndex ].m_pFunctionName = pFunctionName;
	    m_CheckpointArray[ m_ulCheckpointIndex ].m_pFileName = pFileName;
	    m_CheckpointArray[ m_ulCheckpointIndex ].m_ulLineNumber = ulLineNumber;
	    m_CheckpointArray[ m_ulCheckpointIndex ].m_dTime = dTime;
	    m_CheckpointArray[ m_ulCheckpointIndex ].m_type = type;

	    const int len = strlen( pComment );
	    if( m_pComments < m_comments + MAX_COMMENT_CHARACTERS && m_pComments + len + 1 < m_comments + MAX_COMMENT_CHARACTERS )
	    {
	    	m_CheckpointArray[ m_ulCheckpointIndex ].m_pComment = m_pComments;
	    	memcpy( m_pComments, pComment, len + 1 ); /* Flawfinder: ignore */
	    }
	    else
	    {
	    	m_CheckpointArray[ m_ulCheckpointIndex ].m_pComment = "Unknown";
	    }
	    result = m_pComments;
	    m_pComments += len + 1;
	}

	++m_ulCheckpointIndex;

	return result;
    }

    const char* CommentPrefix( CHXCheckpoint2::CheckpointType type )
    {
    	switch( type )
    	{
    	    case CHXCheckpoint2::ENTER_BLOCK:
    	    	return "Enter ";
    	    	break;
    	    case CHXCheckpoint2::EXIT_BLOCK:
    	    	return "Exit ";
    	    	break;
    	    case CHXCheckpoint2::CHECKPOINT:
    	    	return "";
    	    	break;
    	}

    	return "";
    }
    
    void DumpCheckpoints()
    {
	// We must append to the log files because the DLL might be loaded and unloaded several times.
	FILE* pFile = ::fopen( m_pOutputFileName, "a+" );

	if (pFile)
	{
	    if( m_ulCheckpointIndex > MAX_CHECKPOINTS_PER_MODULE )
	    {
		::fprintf( pFile, "***ERROR*** tried to store %ld checkpoints, only room for %ld\n", m_ulCheckpointIndex, MAX_CHECKPOINTS_PER_MODULE );
		::fprintf( pFile, "\n\n\n" );
	    }

	    if( m_pComments >= m_comments + MAX_COMMENT_CHARACTERS )
    	    {
		::fprintf( pFile, "***ERROR*** tried to copy %ld comment characters, only room for %ld\n", m_pComments, MAX_COMMENT_CHARACTERS );
		::fprintf( pFile, "\n\n\n" );
	    }
	    
	    ::fprintf(pFile, "Format==> ElapsedTimeFromPreviousCheckpoint;ElapsedTimeFromStartOfFunction;ElapsedTimeSinceFirstModuleCheckpoint;OSTime;ModuleAndFileName;FunctionName;LineNumber;Comment\n");

	    for(UINT32 j = 0; j < HX_MIN( MAX_CHECKPOINTS_PER_MODULE, m_ulCheckpointIndex ); j++)
	    {
		::fprintf(pFile, "%.6lf;%.6lf;%.6lf;%.6lf;%s\\%s;%s;%lu;%s%s\n",
		    (j ? (m_CheckpointArray[j].m_dTime - m_CheckpointArray[j-1].m_dTime) : 0),
		    m_CheckpointArray[j].m_dTime - m_CheckpointArray[0].m_dTime,
		    m_CheckpointArray[j].m_dTime,
		    m_CheckpointArray[j].m_dTime,
		    m_pModuleName,
		    m_CheckpointArray[j].m_pFileName,
		    m_CheckpointArray[j].m_pFunctionName,
		    m_CheckpointArray[j].m_ulLineNumber,
		    CommentPrefix( m_CheckpointArray[j].m_type ),
		    m_CheckpointArray[j].m_pComment );
	    }


	    ::fclose(pFile);
	}
    }

protected:
    CHXCheckpoint2  m_CheckpointArray[ MAX_CHECKPOINTS_PER_MODULE ];
    const char*	    m_pModuleName;
    const char*	    m_pOutputFileName;
    UINT32	    m_ulCheckpointIndex;
    // This is the array used to store comments that must be copied because they are not static strings that will stick around in the module
    char	    m_comments[ MAX_COMMENT_CHARACTERS ]; /* Flawfinder: ignore */
    char*	    m_pComments;
};


extern CHXCheckpointManager2 g_HXCheckpointManager2;					\

static const char* g_HXFunctionName = "Unknown";

//
// HXGetDoubleTickCount()
// return a double in milliseconds
#if defined(_WIN32) 
inline double HXGetDoubleTickCount()
{
    static double frequency = 0.0;

    if( frequency == 0.0 )
    {
	QueryPerformanceFrequency( &QueryPerformanceFrequencyResult );
	frequency = 4294967296.0;
	frequency *= QueryPerformanceFrequencyResult.HighPart;
	frequency += QueryPerformanceFrequencyResult.LowPart;
    }

    QueryPerformanceCounter( &QueryPerformanceCounterResult );
    double result = 4294967296.0;
    result *= QueryPerformanceCounterResult.HighPart;
    result += QueryPerformanceCounterResult.LowPart;
    result /= frequency;

    result *= 1000.0;

    return result;
}
#elif defined(_UNIX) && !defined(_VXWORKS)
// GetTickCount in pnmisc/unix/getticcount.c chops off accuracy
// this version doesn't
// returns a double in millisecond with microsecond accuracy
inline double HXGetDoubleTickCount()
{
    struct timeval tv;

    gettimeofday (&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000.0;
}
#endif

#if defined(_WIN32) || ( defined(_UNIX) && !defined(_VXWORKS))
#define HX_GET_BETTERTICKCOUNT2() HXGetDoubleTickCount()
#else
#define  HX_GET_BETTERTICKCOUNT2()     HX_GET_BETTERTICKCOUNT()
#endif

#define HX_ENABLE_CHECKPOINTS_FOR_MODULE( pModuleName, pOutputFileName )		\
    CHXCheckpointManager2 g_HXCheckpointManager2(pModuleName, pOutputFileName);

#define HX_SETUP_CHECKPOINTLIST( pFunctionName )

// We don't support accumulators
#define HX_PRIME_ACCUMULATOR(id, pComment)
#define HX_UPDATE_ACCUMULATOR(id)
#define HX_ACCUMULATE(id, pComment, data)

#define HX_LOG_CHECKPOINT_INTEHXAL(pComment, type )						\
    g_HXCheckpointManager2.AddCheckpoint( g_HXFunctionName, pComment, __FILE__, __LINE__, HX_GET_BETTERTICKCOUNT2(), type );

#define HX_LOG_CHECKPOINT_INTEHXAL_COPY(pComment, type)						\
    g_HXCheckpointManager2.AddCheckpointCopy( g_HXFunctionName, pComment, __FILE__, __LINE__, HX_GET_BETTERTICKCOUNT2(), type );

#define HX_LOG_SINGLESHOT_CHECKPOINT_INTEHXAL( pFunctionName, pComment)			\
    {										      	\
	static HXBOOL doneCheckPoint = FALSE;					      	\
	if( !doneCheckPoint )							      	\
	{									      	\
	    HX_LOG_CHECKPOINT_INTEHXAL( pComment, CHXCheckpoint2::CHECKPOINT )		\
	    doneCheckPoint = TRUE;						      	\
	}									      	\
    }

#ifdef SECTIONS_ONLY

// We are only interesting in logging the start and end of sections. All other macros 
// compile out completely. This is done to provide a bare minimum profiling that will 
// be almost completely unaffected by the presence of the profiling code.

#define HX_LOG_CHECKPOINT(pComment)

#define HX_LOG_CHECKPOINT_COPY(pComment)

#define HX_LOG_INITIAL_CHECKPOINT(pFunctionName)

#define HX_LOG_SINGLESHOT_CHECKPOINT( pFunctionName, pComment)

#define HX_LOG_BLOCK( pBlockName )

#define HX_LOG_BLOCK_COPY( pBlockName )

#define HX_LOG_START_SECTION( pSectionName )						\
    HX_LOG_SINGLESHOT_CHECKPOINT_INTEHXAL( "", "--- Start section " pSectionName );

#define HX_LOG_END_SECTION( pSectionName )						\
    HX_LOG_SINGLESHOT_CHECKPOINT_INTEHXAL( "", "--- End section " pSectionName );

#else

#define HX_LOG_CHECKPOINT(pComment)   HX_LOG_CHECKPOINT_INTEHXAL( pComment, CHXCheckpoint2::CHECKPOINT )

#define HX_LOG_CHECKPOINT_COPY(pComment)   HX_LOG_CHECKPOINT_INTEHXAL_COPY( pComment, CHXCheckpoint2::CHECKPOINT )

#define HX_LOG_INITIAL_CHECKPOINT(pFunctionName)					\
	    HX_SETUP_CHECKPOINTLIST( pFunctionName )					\
	    HX_LOG_CHECKPOINT( "Initial Function Checkpoint" )

#define HX_LOG_SINGLESHOT_CHECKPOINT( pFunctionName, pComment ) HX_LOG_SINGLESHOT_CHECKPOINT_INTEHXAL( pFunctionName, pComment )

#define HX_LOG_BLOCK( pBlockName )							\
    class CHXBlockLogger2								\
    {											\
    public:										\
	CHXBlockLogger2()								\
	{										\
	    HX_LOG_CHECKPOINT_INTEHXAL( pBlockName, CHXCheckpoint2::ENTER_BLOCK );	\
	}										\
	~CHXBlockLogger2()								\
	{										\
	    HX_LOG_CHECKPOINT_INTEHXAL( pBlockName, CHXCheckpoint2::EXIT_BLOCK );	\
	}										\
    };											\
    CHXBlockLogger2 blockLogger1278

#define HX_LOG_BLOCK_COPY( pBlockName )							\
    class CHXBlockLogger3								\
    {											\
    public:										\
	CHXBlockLogger3( const char* pBName  )						\
	{										\
	    pBlockName_ = HX_LOG_CHECKPOINT_INTEHXAL_COPY( pBName, CHXCheckpoint2::ENTER_BLOCK );	\
	}										\
	~CHXBlockLogger3()								\
	{										\
	    HX_LOG_CHECKPOINT_INTEHXAL_COPY( pBlockName_, CHXCheckpoint2::EXIT_BLOCK );	\
	}										\
    private:										\
    	const char* pBlockName_;							\
    };											\
    CHXBlockLogger3 blockLogger12783( pBlockName )

#define HX_LOG_START_SECTION( pSectionName )						\
    HX_LOG_SINGLESHOT_CHECKPOINT( "", "--- Start section " pSectionName );

#define HX_LOG_END_SECTION( pSectionName )						\
    HX_LOG_SINGLESHOT_CHECKPOINT( "", "--- End section " pSectionName );

#endif


#endif



#endif // _GEMCHKPT_H_
