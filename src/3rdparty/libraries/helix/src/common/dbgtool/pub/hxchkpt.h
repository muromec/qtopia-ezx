/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxchkpt.h,v 1.8 2007/07/06 20:35:09 jfinnecy Exp $
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

/* These Macros are useful when trying to tune performance to see approximately how long in milliseconds certain lines of
 * code are taking.  The output will display approximately how long it took for the lines between checkpoints.
 * 
 * Available macros in this file and how to use them:
 * 
 * HX_ENABLE_CHECKPOINTS_FOR_MODULE(pModuleName, pOutputFileName)
 * This macro should be used once for the entire dll or exe you are compiling.  It declares a global that is used by the
 * entire module.  A good place to put this for example would be in the dllmain.cpp file.
 *
 * pModuleName -	The name of the directory in which the dll or exe you are compiling is located.  This is used 
 *			in the log when printing out the file name.
 * pOutputFileName -	The name of the output file name where checkpoint data is dumped
 * 
 * HX_LOG_INITIAL_CHECKPOINT(pFunctionName)
 * This macro should be used once at the beginning of each function or method you are interested in timing.
 * 
 * pFunctionName -	The name of the function that the checkpoints are in
 *
 * HX_LOG_CHECKPOINT(pComment)
 * This macro should be used whenver timing blocks of code are of interest.  This can be used repeatedly in each function.
 *
 * pComment -		The comment passed in will be dumped to the log file.  It is used as a hint to what was being executed
 *			for this checkpoint instead of having to refer back to the code.
 *
 * HX_LOG_BLOCK( pComment )
 * This macro logs entry to and exit from a brace delimited block. It writes out two checkpoints - the entry checkpoint 
 * will be the comment prefixed with 'Enter', the exit checkpoint will be the comment prefixed with 'Exit'
 * Note that this macro cannot be used in the same block as HX_LOG_INITIAL_CHECKPOINT.
 *
 * pComment -		An identifier for the block. Since blocks often correspond to functions this will often be the function name.
 *
 * 	   
 * LOG FILE FORMAT
 * The log file is written to in append mode and is several fields delimited by a ;
 * It can easily be loaded into a spreadsheet app since all of them should be able to read delimted text files.
 * Here is a breakdown of the individual fields:
 * Field1: Time elapsed since the last checkpoint
 * Field2: Time elapsed since the first checkpoint in the function
 * Field3: Time elapsed since the first checkpoint in the module
 * Field4: Tick count as reported from the OS
 * Field5: Path to file name
 * Field6: Function name
 * Field7: Line Number of checkpoint
 * Field8: Comment about what the checkpoint is timing
 */

#ifndef _HXCHKPT_H_
#define _HXCHKPT_H_

#if defined( ENABLE_CHECKPOINTS2 ) || defined( ENABLE_PERFORMANCE2 )

#include "hxperf.h"

#elif defined( ENABLE_CHECKPOINTS )

#include "hlxclib/stdio.h"
#include "hxtick.h"
#include "hxmap.h"

#define MAX_CHECKPOINTLISTS_PER_MODULE	256
#define MAX_CHECKPOINTS_PER_FUNCTION	1024
#define MAX_ACCUMULATORS_PER_FUNCTION	64

// This class represents the data that is unique per checkpoint
class CHXCheckpoint
{
public:
    CHXCheckpoint() :
	m_ulLineNumber(0),
	m_ulTime(0)
    {
    }

    ~CHXCheckpoint()
    {
    }

    // These are public members for access speed since the checkpoint code should be as lightweight as possible.  I am
    // also trying to reduce the number of lines of code since this is all inline in a header file.
    UINT32		m_ulLineNumber;
    UINT32		m_ulTime;
    char		m_pStaticComment[ 64 ]; /* Flawfinder: ignore */
};


class CHXAccumCheckpoint : 
    public CHXCheckpoint
{
public:
    CHXAccumCheckpoint() :
	CHXCheckpoint(),
	m_ulID( 0 ),
	m_ulMarker( 0 )
    {
    }

public:
    UINT32 		m_ulID;
    UINT32		m_ulMarker;
};


// This class represents a way to categorize the checkpoints associated with a function or method.  It contains the list
// of checkpoints for that function or method and the filename where that function or method is located.
class CHXCheckpointList
{
public:
    CHXCheckpointList(const char* pFunctionName, const char* pSourceFileName);

    ~CHXCheckpointList()
    {
    }

    void AddCheckpoint(const char* pComment, UINT32 ulLineNumber, UINT32 ulTime)
    {
	if( m_ulCheckpointIndex < MAX_CHECKPOINTS_PER_FUNCTION )
	{
	    CHXCheckpoint* pCheckpoint = &m_CheckpointArray[m_ulCheckpointIndex];
	    pCheckpoint->m_ulLineNumber = ulLineNumber;
	    pCheckpoint->m_ulTime = ulTime;
	    UINT32 copySize = sizeof(pCheckpoint->m_pStaticComment) - 1;
	    ::strncpy( pCheckpoint->m_pStaticComment, pComment, copySize ); /* Flawfinder: ignore */
	    pCheckpoint->m_pStaticComment[copySize] = '\0';
	    m_ulCheckpointIndex++;
	}
	else
	{
    	    for( int i = 0; i < MAX_CHECKPOINTS_PER_FUNCTION; ++i )
	    {
		CHXCheckpoint* pCheckpoint = &m_CheckpointArray[i];
		UINT32 copySize = sizeof(pCheckpoint->m_pStaticComment) - 1;
		::strncpy( pCheckpoint->m_pStaticComment, "***ERROR*** - too many checkpoints in checkpoint list", copySize ); /* Flawfinder: ignore */
		pCheckpoint->m_pStaticComment[copySize] = '\0';
	    }
	}
    }

    void PrimeAccumulator( UINT32 id, const char* pComment, UINT32 ulLineNumber, UINT32 ulTime )
    {
	void* pValue = NULL;
	if( m_accumulators.Lookup( id, pValue ) )
	{
	    CHXAccumCheckpoint* pCheckpoint = (CHXAccumCheckpoint*) pValue;
	    pCheckpoint->m_ulMarker = ulTime;
	}
	else if( m_ulAccumulatorIndex < MAX_ACCUMULATORS_PER_FUNCTION )
	{
	    CHXAccumCheckpoint* pCheckpoint = &m_AccumulatorArray[ m_ulAccumulatorIndex++ ];
	    m_accumulators.SetAt( id, pCheckpoint );

	    pCheckpoint->m_ulID = id;
	    pCheckpoint->m_ulLineNumber = ulLineNumber;
	    pCheckpoint->m_ulMarker = ulTime;
	    UINT32 copySize = sizeof(pCheckpoint->m_pStaticComment) - 1;
	    ::strncpy( pCheckpoint->m_pStaticComment, pComment, copySize ); /* Flawfinder: ignore */
	    pCheckpoint->m_pStaticComment[copySize] = '\0';
	}
	else
	{
	    for( int i = 0; i < MAX_ACCUMULATORS_PER_FUNCTION; ++i )
	    {
		CHXAccumCheckpoint* pCheckpoint = &m_AccumulatorArray[ i ];
		UINT32 copySize = sizeof(pCheckpoint->m_pStaticComment) - 1;
		::strncpy( pCheckpoint->m_pStaticComment, "***ERROR*** - too many accumulator checkpoints in checkpoint list", copySize ); /* Flawfinder: ignore */
		pCheckpoint->m_pStaticComment[copySize] = '\0';
	    }
	}
    }

    void UpdateAccumulator( UINT32 id, UINT32 ulTime )
    {
	void* pValue = NULL;
	if( m_accumulators.Lookup( id, pValue ) )
	{
	    CHXAccumCheckpoint* pCheckpoint = (CHXAccumCheckpoint*) pValue;
	    pCheckpoint->m_ulTime += ulTime - pCheckpoint->m_ulMarker;
	    pCheckpoint->m_ulMarker = 0;
	}
    }

    void AccumulateValue( UINT32 id, const char* pComment, UINT32 ulLineNumber, UINT32 ulValue )
    {
	void* pValue = NULL;
	if( m_accumulators.Lookup( id, pValue ) )
	{
	    CHXAccumCheckpoint* pCheckpoint = (CHXAccumCheckpoint*) pValue;
	    pCheckpoint->m_ulTime += ulValue;
	}
	else if( m_ulAccumulatorIndex < MAX_ACCUMULATORS_PER_FUNCTION )
	{
	    CHXAccumCheckpoint* pCheckpoint = &m_AccumulatorArray[ m_ulAccumulatorIndex++ ];
	    m_accumulators.SetAt( id, pCheckpoint );

	    pCheckpoint->m_ulID = id;
	    pCheckpoint->m_ulLineNumber = ulLineNumber;
	    pCheckpoint->m_ulTime = ulValue;
	    UINT32 copySize = sizeof(pCheckpoint->m_pStaticComment) - 1;
	    ::strncpy( pCheckpoint->m_pStaticComment, pComment, copySize ); /* Flawfinder: ignore */
	    pCheckpoint->m_pStaticComment[copySize] = '\0';
	}
	else
	{
	    for( int i = 0; i < MAX_ACCUMULATORS_PER_FUNCTION; ++i )
	    {
		CHXAccumCheckpoint* pCheckpoint = &m_AccumulatorArray[ i ];
		UINT32 copySize = sizeof(pCheckpoint->m_pStaticComment) - 1;
		::strncpy( pCheckpoint->m_pStaticComment, "***ERROR*** - too many accumulator checkpoints in checkpoint list", copySize ); /* Flawfinder: ignore */
		pCheckpoint->m_pStaticComment[copySize] = '\0';
	    }
	}
    }

    // These are public members for access speed since the checkpoint code should be as lightweight as possible.  I am
    // also trying to reduce the number of lines of code since this is all inline in a header file.
    const char*	    m_pFunctionName;
    const char*	    m_pSourceFileName;
    UINT32	    m_ulCheckpointIndex;
    CHXCheckpoint   m_CheckpointArray[MAX_CHECKPOINTS_PER_FUNCTION];

    UINT32	    	m_ulAccumulatorIndex;
    CHXAccumCheckpoint  m_AccumulatorArray[MAX_ACCUMULATORS_PER_FUNCTION];

    CHXMapLongToObj m_accumulators;
};

// This class manages an array CHXCheckpointList objects.  It uses these to dump out timing information
class CHXCheckpointManager
{
public:
    CHXCheckpointManager(const char* pModuleName, const char* pOutputFileName ) :
	m_pModuleName(pModuleName),
	m_pOutputFileName(pOutputFileName),
	m_ulCheckpointListIndex(0)
    {
    }

    ~CHXCheckpointManager()
    {
	// Since writing to a file slows down startup time timing data is only dumped to the file when the global checkpoint
	// manager object associated with a dll or exe is destroyed meaning an unload or program termination occured.
	DumpCheckpoints();
    }

    void AddCheckpointList(CHXCheckpointList* pCheckpointList)
    {
	printf( "AddCheckpointList %p to manager %p\n", pCheckpointList, this );

	if( m_ulCheckpointListIndex < MAX_CHECKPOINTLISTS_PER_MODULE )
	{
	    m_pCheckpointListArray[m_ulCheckpointListIndex++] = pCheckpointList;
	}
	else
	{
	    for( int i = 0; i < MAX_CHECKPOINTLISTS_PER_MODULE; ++i )
	    {
		CHXCheckpointList* pCheckpointList = m_pCheckpointListArray[ i ];

		for( int i = 0; i < MAX_CHECKPOINTS_PER_FUNCTION; ++i )
		{
		    CHXCheckpoint* pCheckpoint = &pCheckpointList->m_CheckpointArray[i];
		    UINT32 copySize = sizeof(pCheckpoint->m_pStaticComment) - 1;
		    ::strncpy( pCheckpoint->m_pStaticComment, "***ERROR*** - too many checkpoint lists in module", copySize ); /* Flawfinder: ignore */
		    pCheckpoint->m_pStaticComment[copySize] = '\0';
		}
	    }
	}
    }

    void DumpCheckpoints()
    {
#ifdef DUMP_CHECKPOINT_ONLY_IF_EXISTS
	FILE* pFile = ::fopen(m_pOutputFileName, "r+");
	if(pFile)
	{
		fseek(pFile, 0, SEEK_END);
	}	
#else
	FILE* pFile = ::fopen(m_pOutputFileName, "a+");
#endif
	if (pFile)
	{
	    UINT32 i, j;

	    ::fprintf(pFile, "Format==> ElapsedTimeFromPreviousCheckpoint;ElapsedTimeFromStartOfFunction;ElapsedTimeSinceFirstModuleCheckpoint;OSTime;ModuleAndFileName;FunctionName;LineNumber;Comment\n");
	    for (i = 0; i < m_ulCheckpointListIndex; i++)
	    {
		for (j = 0; j < m_pCheckpointListArray[i]->m_ulCheckpointIndex; j++)
		{

		    ::fprintf(pFile, "%lu;%lu;%lu;%lu;%s\\%s;%s;%lu;%s\n",	(j ? (m_pCheckpointListArray[i]->m_CheckpointArray[j].m_ulTime - m_pCheckpointListArray[i]->m_CheckpointArray[j-1].m_ulTime) : 0),
										m_pCheckpointListArray[i]->m_CheckpointArray[j].m_ulTime - m_pCheckpointListArray[i]->m_CheckpointArray[0].m_ulTime,
										m_pCheckpointListArray[i]->m_CheckpointArray[j].m_ulTime - m_pCheckpointListArray[0]->m_CheckpointArray[0].m_ulTime,
										m_pCheckpointListArray[i]->m_CheckpointArray[j].m_ulTime,
										m_pModuleName,
										m_pCheckpointListArray[i]->m_pSourceFileName,
										m_pCheckpointListArray[i]->m_pFunctionName,
										m_pCheckpointListArray[i]->m_CheckpointArray[j].m_ulLineNumber,
										m_pCheckpointListArray[i]->m_CheckpointArray[j].m_pStaticComment);
		}
	    }

	    // Write out accumulators
	    ::fprintf( pFile, "\n\nAccumulators Format==> TotalTime;ModuleAndFileName;FunctionName;LineNumber;Comment\n" );
	    for (i = 0; i < m_ulCheckpointListIndex; i++)
	    {
		for (j = 0; j < m_pCheckpointListArray[i]->m_ulAccumulatorIndex; j++)
		{
		    CHXAccumCheckpoint* pCP = (CHXAccumCheckpoint*) &m_pCheckpointListArray[i]->m_AccumulatorArray[ j ];
		    ::fprintf(pFile, "%lu;%s\\%s;%s;%lu;%s\n", pCP->m_ulTime,
							    m_pModuleName,
							    m_pCheckpointListArray[i]->m_pSourceFileName,
							    m_pCheckpointListArray[i]->m_pFunctionName,
							    pCP->m_ulLineNumber,
							    pCP->m_pStaticComment);
		}
	    }

	    ::fprintf( pFile, "\n\n\n" );
	    ::fclose(pFile);
	}
    }

protected:
    CHXCheckpointList*  m_pCheckpointListArray[ MAX_CHECKPOINTLISTS_PER_MODULE ];
    const char*		m_pModuleName;
    const char*		m_pOutputFileName;
    UINT32		m_ulCheckpointListIndex;
};

extern CHXCheckpointManager g_HXCheckpointManager;


inline
CHXCheckpointList::CHXCheckpointList(const char* pFunctionName, const char* pSourceFileName) :
    m_pFunctionName(pFunctionName),
    m_pSourceFileName(pSourceFileName),
    m_ulCheckpointIndex(0)
{
    extern CHXCheckpointManager g_HXCheckpointManager;	
    g_HXCheckpointManager.AddCheckpointList(this);
}

#define HX_ENABLE_CHECKPOINTS_FOR_MODULE(pModuleName, pOutputFileName)				\
	    CHXCheckpointManager g_HXCheckpointManager(pModuleName, pOutputFileName);

#define HX_SETUP_CHECKPOINTLIST( pFunctionName )					      	\
	    static CHXCheckpointList HXFunctionCheckpointList(pFunctionName, __FILE__);
	    

		
#define HX_LOG_CHECKPOINT(pComment)							      	\
	    HXFunctionCheckpointList.AddCheckpoint(pComment, __LINE__, HX_GET_BETTERTICKCOUNT());

#define HX_PRIME_ACCUMULATOR(id, pComment)							\
	    HXFunctionCheckpointList.PrimeAccumulator( id, pComment, __LINE__, HX_GET_BETTERTICKCOUNT() );

#define HX_UPDATE_ACCUMULATOR(id)								\
	    HXFunctionCheckpointList.UpdateAccumulator(id, HX_GET_BETTERTICKCOUNT() );

#define HX_ACCUMULATE(id, pComment, data)					       		\
	    HXFunctionCheckpointList.AccumulateValue(id, pComment,  __LINE__, data );


#define HX_LOG_INITIAL_CHECKPOINT(pFunctionName)					      	\
	    HX_SETUP_CHECKPOINTLIST( pFunctionName )					      	\
	    HX_LOG_CHECKPOINT( "Initial Function Checkpoint" )
	    
#define HX_LOG_SINGLESHOT_CHECKPOINT( pFunctionName, pComment)				      	\
	    {										      	\
		static HXBOOL doneCheckPoint = FALSE;					      	\
		if( !doneCheckPoint )							      	\
		{									      	\
		    HX_SETUP_CHECKPOINTLIST( pFunctionName )				      	\
		    HX_LOG_CHECKPOINT( pComment )                        		      	\
		    doneCheckPoint = TRUE;						      	\
		}									      	\
	    }


#define HX_LOG_BLOCK( pBlockName )							\
    HX_SETUP_CHECKPOINTLIST( "a" );							\
    class CHXBlockLogger								\
    {											\
    public:										\
	CHXBlockLogger( CHXCheckpointList* pCheckpointList )				\
	: m_pCheckpointList( pCheckpointList )						\
	{										\
	    m_pCheckpointList->AddCheckpoint( "Enter " pBlockName, __LINE__, HX_GET_BETTERTICKCOUNT());	\
	}										\
	~CHXBlockLogger()								\
	{										\
	    m_pCheckpointList->AddCheckpoint( "Exit " pBlockName, __LINE__, HX_GET_BETTERTICKCOUNT());	\
	}										\
    private:										\
	CHXCheckpointList* m_pCheckpointList;						\
    };											\
    CHXBlockLogger blockLogger1278( &HXFunctionCheckpointList );



#define HX_LOG_START_SECTION( pSectionName )				      	\
    HX_LOG_SINGLESHOT_CHECKPOINT( "", "--- Start section " pSectionName );

#define HX_LOG_END_SECTION( pSectionName )				      	\
    HX_LOG_SINGLESHOT_CHECKPOINT( "", "--- End section " pSectionName );

#else /* ENABLE_CHECKPOINTS && ENABLE_CHECKPOINTS2 not Defined */

#define HX_ENABLE_CHECKPOINTS_FOR_MODULE(pModuleName, pOutputFileName)
#define HX_SETUP_CHECKPOINTLIST( pFunctionName )
#define HX_LOG_CHECKPOINT(pComment)
#define HX_PRIME_ACCUMULATOR(id, pComment)
#define HX_UPDATE_ACCUMULATOR(id)
#define HX_ACCUMULATE(id, pComment, data)
#define HX_LOG_INITIAL_CHECKPOINT(pFunctionName)
#define HX_LOG_SINGLESHOT_CHECKPOINT( pFunctionName, pComment)
#define HX_LOG_BLOCK( pBlockName )
#define HX_LOG_START_SECTION( pSectionName )
#define HX_LOG_END_SECTION( pSectionName )

#endif /* ENABLE_CHECKPOINTS */

#endif /* _HXCHKPT_H_ */



