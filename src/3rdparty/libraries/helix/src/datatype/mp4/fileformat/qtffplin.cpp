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

/****************************************************************************
 * Defines
 */
#define NO_STREAM_SET	0xFFFF
#define FSWCHR_MAX_CHILD_COUNT    20

#define MAX_PER_BANDWIDTH_SET_UBERRULEBOOK_SIZE(numStreams) (53 + 35 * (numStreams))


/****************************************************************************
 * Includes
 */
#include "mp4fformat.ver"

#include "hxtypes.h"
#include "hxcom.h"
#include "hxstrutl.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxformt.h"
#include "hxplugn.h"
#include "hxpends.h"
#include "hxengin.h"
#include "hxerror.h"
#include "hxupgrd.h"
#include "hxmon.h"
#include "hxbrdcst.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "defslice.h"
#include "qtres.h"
#include "hxsdesc.h"
#include "hxstring.h"
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
#include "hxprdnld.h"       // IHXMediaBytesToMediaDur, IHXPDStatusMgr
#endif /* #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS) */
#include "qtffplin.h"
#include "netbyte.h"
#include "hxver.h"
#include "hxxres.h"
#include "hxxrsmg.h"
#include "dbcs.h"
#include "pckunpck.h"
#include "stream_desc_hlpr.h"
#include "altidset.h"
#include "altgrpitr.h"

#include "sdpchunk.h"
#include "sdppyldinfo.h"

#include "qtffrefcounter.h"
#include "qtpktasm.h"
#include "qtoffsetmpr.h"

#include "metainfokeys.h"

/****************************************************************************
 * Constants
 */
#define SET_AS_BUFFER_PROPERTY              TRUE

#define QTFF_AU_PREFIX		"FileFormat/"
#define QTFF_AU_PREFIX_SIZE	(sizeof(QTFF_AU_PREFIX) - 1)
#define MAX_EXTENSION_SIZE	256

#define QTBUFFERFRAGMENT_POOL_SIZE	    50
#define QTBUFFERFRAGMENT_INITIAL_POOL_SIZE  5

const char* const CQTFileFormat::zm_pDescription    = "RealNetworks Mpeg4 File Format Plugin";
const char* const CQTFileFormat::zm_pCopyright      = HXVER_COPYRIGHT;
const char* const CQTFileFormat::zm_pMoreInfoURL    = HXVER_MOREINFO;

#define EXT_3GP	    "3gp"
#define EXT_3G2	    "3g2"
#define EXT_MP4	    "mp4"
#define EXT_M4A	    "m4a"
#define EXT_MOV	    "mov", "qt"

#define ONM_3GP	    "3GPP-MP4 Files (*.3gp, *.3g2)"
#define ONM_MP4	    "MP4 Files (*.mp4)"
#define ONM_MOV	    "QuickTime Files (*.mov, *.qt)"

const char* const CQTFileFormat::zm_pFileMimeTypes[]  = {"application/x-pn-quicktime-stream", "audio/3gpp", "video/3gpp", "audio/x-m4a", "video/mp4", "audio/mp4", "video/3gpp2", NULL};

const char* const CQTFileFormat::zm_pFileExtensions[] = {EXT_MOV, EXT_MP4, EXT_3GP, EXT_3G2, EXT_M4A, NULL};

const char* const CQTFileFormat::zm_pFileOpenNames[] = {ONM_MOV, ONM_MP4, ONM_3GP, NULL};

const char* const CQTFileFormat::zm_pPacketFormats[]  = {"rtp", "rdt", NULL};


/****************************************************************************
 * Globals
 */
g_base_nRefCount_qtff_TypeModifier INT32 g_base_nRefCount_qtff = 0;


class BandwidthSet
{
public:
    BandwidthSet(void)
	: m_ulBandwidth(0)
	, m_pAltIDSet(NULL)
    {
	m_pAltIDSet = new CHXAltIDSet;
    }

    ~BandwidthSet()
    {
	HX_DELETE(m_pAltIDSet);
    }

    UINT32	 m_ulBandwidth;
    CHXAltIDSet* m_pAltIDSet;
};


/****************************************************************************
 *  Constructor/Destructor
 */
CQTFileFormat::CQTFileFormat()
    : m_pContext(NULL)
    , m_pClassFactory(NULL)
    , m_pFFResponse(NULL)
    , m_pScheduler(NULL)
    , m_pErrorMessages(NULL)
    , m_bQTLicensed(FALSE)
    , m_bMP4Licensed(FALSE)
    , m_b3GPPRel6Licensed(FALSE)
    , m_bViewSourceRequest(FALSE)
    , m_uFormatFlavor(MAX_QTFORMAT_FLAVOR)
    , m_pRequest(NULL)
    , m_pFileSwitcher(NULL)
    , m_pAtomizer(NULL)
    , m_pPacketAssembler(NULL)
    , m_ulPendingSeekTime(0)
    , m_pPacketCache(NULL)
    , m_uNextPacketStreamNum(NO_STREAM_SET)
    , m_State(QTFF_Offline)
#ifdef QTCONFIG_BFRAG_FACTORY
    , m_pBufferFragmentFactory(NULL)
#endif	// QTCONFIG_BFRAG_FACTORY
    , m_bUnregulatedStreamDataFlow(FALSE)
    , m_lRefCount(0)
    //  Progressive Download assistance data:
    , m_ulFileDuration(0)
    , m_bFileStructureEstablished(FALSE)
    , m_ulStreamMetaInfoMask(META_INFO_NONE)
    , m_pOffsetToTimeMapper(NULL)
    , m_pFileObject(NULL)
    , m_ulNextPacketTime(0)
{
    g_nRefCount_qtff++;
}

CQTFileFormat::~CQTFileFormat()
{
    Close();
    g_nRefCount_qtff--;
}


/************************************************************************
 *  IHXPlugin methods
 */
/************************************************************************
 *  IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed 
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP CQTFileFormat::InitPlugin(IUnknown* /*IN*/ pContext)
{
    HX_RESULT retVal = HXR_OK;

    if (pContext)
    {
	m_pContext = pContext;
	m_pContext->AddRef();
    }
    else
    {
	retVal = HXR_INVALID_PARAMETER;
    }

    /*
     * Check for license
     */     
    if (SUCCEEDED(retVal))
    {
	retVal = CheckLicense();
    }

    if (SUCCEEDED(retVal))
    {
	HX_ASSERT(!m_pErrorMessages);
	pContext->QueryInterface(IID_IHXErrorMessages, 
	    (void**) &m_pErrorMessages);

	retVal = pContext->QueryInterface(IID_IHXCommonClassFactory,
		    (void**)&m_pClassFactory);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = pContext->QueryInterface(IID_IHXScheduler,
		    (void**)&m_pScheduler);
    }

    return retVal;
}

/************************************************************************
 *  IHXPlugin::GetPluginInfo
 *  Purpose:
 *    Returns the basic information about this plugin. Including:
 *
 *    bLoadMultiple	whether or not this plugin DLL can be loaded
 *			multiple times. All File Formats must set
 *			this value to TRUE.
 *    pDescription	which is used in about UIs (can be NULL)
 *    pCopyright	which is used in about UIs (can be NULL)
 *    pMoreInfoURL	which is used in about UIs (can be NULL)
 */
STDMETHODIMP CQTFileFormat::GetPluginInfo
(
    REF(HXBOOL)		bLoadMultiple,
    REF(const char*)	pDescription,
    REF(const char*)	pCopyright,
    REF(const char*)	pMoreInfoURL,
    REF(ULONG32)	ulVersionNumber
)
{
    bLoadMultiple = TRUE;   // Must be true for file formats.

    pDescription    = (const char*) zm_pDescription;
    pCopyright	    = (const char*) zm_pCopyright;
    pMoreInfoURL    = (const char*) zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

/************************************************************************
 *  IHXFileFormatObject methods
 */
/************************************************************************
 *  GetFileFormatInfo
 *  Purpose:
 *    If this object is a file format object this method returns
 *    information vital to the instantiation of file format plugins.
 *    If this object is not a file format object, it should return
 *    HXR_UNEXPECTED.
 */
STDMETHODIMP CQTFileFormat::GetFileFormatInfo
(
    REF(const char**) /*OUT*/ pFileMimeTypes,
    REF(const char**) /*OUT*/ pFileExtensions,
    REF(const char**) /*OUT*/ pFileOpenNames
)
{
    pFileMimeTypes  = (const char**) zm_pFileMimeTypes;
    pFileExtensions = (const char**) zm_pFileExtensions;
    pFileOpenNames  = (const char**) zm_pFileOpenNames;

    return HXR_OK;
}

/************************************************************************
 *  InitFileFormat
 */
STDMETHODIMP CQTFileFormat::InitFileFormat
(
    IHXRequest*		/*IN*/	pRequest, 
    IHXFormatResponse*		/*IN*/	pFileFormatResponse,
    IHXFileObject*		/*IN*/  pFileObject
)
{
    HX_RESULT retVal = HXR_OK;

    if (m_State != QTFF_Offline)
    {
	return HXR_UNEXPECTED;
    }

    m_State = QTFF_Init;

    if ((pFileFormatResponse == NULL) ||
	(pFileObject == NULL))
    {
	retVal = HXR_FAIL;
    }

    HX_RELEASE(m_pFileObject);
    m_pFileObject = pFileObject;
    if (m_pFileObject != NULL)
    {
        m_pFileObject->AddRef();
    }

    m_ulNextPacketTime = 0;

    HX_RELEASE(m_pFFResponse);
    m_pFFResponse = pFileFormatResponse;
    m_pFFResponse->AddRef();

    HX_RELEASE(m_pRequest);
    m_pRequest = pRequest;
    if (m_pRequest)
    {
	m_pRequest->AddRef();
    }

    if (SUCCEEDED(retVal) && m_pRequest)
    {
	IHXValues* pRequestHeaders = NULL;
	if (SUCCEEDED(m_pRequest->GetRequestHeaders(pRequestHeaders)) &&
	    pRequestHeaders)
	{
	    ULONG32 ulVal = 0;

	    if (SUCCEEDED(pRequestHeaders->GetPropertyULONG32(
		    "ViewSourceInfoHeaders",
		    ulVal)))
	    {
		m_bViewSourceRequest = ((ulVal == 0) ? FALSE : TRUE);
	    }

	    IHXBuffer* pBuffer = NULL;
	    retVal = pRequestHeaders->GetPropertyCString("AcceptMetaInfo",
					     pBuffer);
	    if(SUCCEEDED(retVal))
	    {
		retVal = ExtractAcceptMetaInfo(pBuffer);
	    }

            // We want to proceed even if metadata fails.
            retVal = HXR_OK;

	    HX_RELEASE(pBuffer);
	}
	HX_RELEASE(pRequestHeaders);
    }

    if (SUCCEEDED(retVal))
    {
	HX_RELEASE(m_pFileSwitcher);
	HX_RELEASE(m_pAtomizer);

#ifdef QTCONFIG_FSWITCHER
	m_pFileSwitcher = (IHXFileSwitcher*) new CFileSwitcher();
#else	// QTCONFIG_FSWITCHER
	m_pFileSwitcher = (IHXFileSwitcher*) new CFileSwitcherPassthrough();
#endif	// QTCONFIG_FSWITCHER
	if (m_pFileSwitcher)
	{
	    m_pFileSwitcher->AddRef();
	}
	else
	{
	    retVal = HXR_OUTOFMEMORY;
	}
    }

    if (SUCCEEDED(retVal))
    {
	m_pAtomizer = new CAtomizer();
	if (m_pAtomizer)
	{
	    m_pAtomizer->AddRef();
	}
	else
	{
	    retVal = HXR_OUTOFMEMORY;
	}
    }

    if (SUCCEEDED(retVal))
    {
	m_pPacketAssembler = new CQTPacketAssembler();
	if (m_pPacketAssembler)
	{
	    m_pPacketAssembler->AddRef();
	    retVal = m_pPacketAssembler->Init(this);
	}
	else
	{
	    retVal = HXR_OUTOFMEMORY;
	}
    }

    if (SUCCEEDED(retVal))
    {
	retVal = m_pFileSwitcher->Init(pFileObject,
				       HX_FILE_READ | HX_FILE_BINARY,
				       (IHXFileResponse*) this,
				       m_pContext,
				       FSWCHR_MAX_CHILD_COUNT);
    }
	
    if (FAILED(retVal))
    {
	m_State = QTFF_Error;
    }

    return retVal;
}

HX_RESULT
CQTFileFormat::ExtractAcceptMetaInfo(IHXBuffer* pRequestedInfoBuffer)
{
    HX_RESULT res = HXR_OK;

    if(!pRequestedInfoBuffer)
    {
	res = HXR_POINTER;
	HX_ASSERT(FALSE);
	return res;
    }

    const char* pRequestedInfo = NULL;
    if (pRequestedInfoBuffer)
    {
	pRequestedInfo = (const char*) pRequestedInfoBuffer->GetBuffer();
    }

    if(*pRequestedInfo == '*')
    {
	m_ulStreamMetaInfoMask |= META_INFO_ALL;
    }
#ifdef QTCONFIG_SERVER
    else if (pRequestedInfo)
    {	    
	const char* pName;
	ULONG32 ulNameLength;

	// Set requested Meta Info
	do
	{
	    pName = pRequestedInfo;
	    
	    while ((*pRequestedInfo != '\0') &&
		   (*pRequestedInfo != ','))
	    {
		pRequestedInfo++;
	    }
	    
	    ulNameLength = (ULONG32) (pRequestedInfo - pName);

	    if (ulNameLength > 0)
	    {
		if ((strlen(QT_WIDTH_METANAME) == ulNameLength) &&
		    (strncmp(pName, QT_WIDTH_METANAME, ulNameLength) == 0))
		{
		    m_ulStreamMetaInfoMask |= META_INFO_WIDTH;
		}
		else if ((strlen(QT_HEIGHT_METANAME) == ulNameLength) &&
			 (strncmp(pName, QT_HEIGHT_METANAME, ulNameLength) == 0))
		{
		    m_ulStreamMetaInfoMask |= META_INFO_HEIGHT;
		}
	    }

	    if (*pRequestedInfo == '\0')
	    {
		break;
	    }

	    pRequestedInfo++;
	} while (TRUE);
    } 
#endif	// QTCONFIG_SERVER

    return res;
}


#if (defined HELIX_FEATURE_3GPP_METAINFO || defined HELIX_FEATURE_SERVER)
HX_RESULT CQTFileFormat::SetPropertyOnHeader(HX_RESULT status,
                                             IHXValues* pHeader,
                                             const char* key,
                                             const char* value,
                                             ULONG32 valueLength)
{
    // ignore empty property values rather than failing
    if (SUCCEEDED(status) && value)
    {
        // Check for BYTE_ORDER_MARK (0xFEFF) and, if it exists, indicate UTF16 encoding type
        // Note: All meta-info strings should be set as buffer properties
        if (valueLength >= 2 && value[0] == 0xFE && value[1] == 0xFF)
        {
            status = SetCStringPropertyWithNullTermEx(pHeader, 
                                          key,
                                          (UINT8*)value,
                                          (UINT32)valueLength,
                                          m_pContext, 
                                          HX_TEXT_ENCODING_TYPE_UTF16BE,
                                          SET_AS_BUFFER_PROPERTY);
        }
        else
        {
            status = SetCStringPropertyCCFWithNullTerm(pHeader, 
                                           key,
                                           (BYTE*)value,
                                           (UINT32)valueLength,
                                           m_pContext,
                                           SET_AS_BUFFER_PROPERTY);
        }
    }

    return status;
}

HX_RESULT CQTFileFormat::Set3GPPAssetInfoOnHeader(HX_RESULT status,
                                                  IHXValues* pHeader)
{
    // 3GP Asset Info: Title, Description, Performer, Author, Genre
    status = SetPropertyOnHeader(status, pHeader, _3GPP_META_INFO_TITLE_KEY,
                 (const char*) (m_MovieInfo.GetTitle()),
                 m_MovieInfo.GetTitleLength());
    status = SetPropertyOnHeader(status, pHeader, _3GPP_META_INFO_AUTHOR_KEY,
                 (const char*) (m_MovieInfo.GetAuthor()),
                 m_MovieInfo.GetAuthorLength());
    status = SetPropertyOnHeader(status, pHeader, _3GPP_META_INFO_COPYRIGHT_KEY,
                 (const char*) (m_MovieInfo.GetCopyright()),
                 m_MovieInfo.GetCopyrightLength());

#ifdef HELIX_FEATURE_3GPP_METAINFO
    status = SetPropertyOnHeader(status, pHeader, _3GPP_META_INFO_DESCRIPTION_KEY,
                 (const char*) (m_MovieInfo.GetDescription()),
                 m_MovieInfo.GetDescriptionLength());
    status = SetPropertyOnHeader(status, pHeader, _3GPP_META_INFO_PERFORMER_KEY,
                 (const char*) (m_MovieInfo.GetPerformer()),
                 m_MovieInfo.GetPerformerLength());
    status = SetPropertyOnHeader(status, pHeader, _3GPP_META_INFO_GENRE_KEY,
                 (const char*) (m_MovieInfo.GetGenre()),
                 m_MovieInfo.GetGenreLength());

    // 3GP Asset Info: Rating
    status = SetPropertyOnHeader(status, pHeader, _3GPP_META_INFO_RATING_KEY,
                 (const char*) (m_MovieInfo.GetRatingCriteria()),
                 m_MovieInfo.GetRatingCriteriaLength());
    status = SetPropertyOnHeader(status, pHeader, _3GPP_META_INFO_RATING_ENTITY_KEY,
                 (const char*) (m_MovieInfo.GetRatingEntity()),
                 m_MovieInfo.GetRatingEntityLength());
    status = SetPropertyOnHeader(status, pHeader, _3GPP_META_INFO_RATING_INFO_KEY,
                 (const char*) (m_MovieInfo.GetRatingInfo()),
                 m_MovieInfo.GetRatingInfoLength());

    // 3GP Asset Info: Classification
    status = SetPropertyOnHeader(status, pHeader, _3GPP_META_INFO_CLASSIFICATION_ENTITY_KEY,
                 (const char*) (m_MovieInfo.GetClassEntity()),
                  m_MovieInfo.GetClassEntityLength());
    status = SetPropertyOnHeader(status, pHeader, _3GPP_META_INFO_CLASSIFICATION_INFO_KEY,
                 (const char*) (m_MovieInfo.GetClassInfo()),
                 m_MovieInfo.GetClassInfoLength());
    if (SUCCEEDED(status))
    {
        ULONG32 classTable = (ULONG32) m_MovieInfo.GetClassTable();
        if (classTable)
            pHeader->SetPropertyULONG32(_3GPP_META_INFO_CLASSIFICATION_TABLE_KEY, 
                                        classTable);
    }

    // 3GP Asset Info: Keyword Count
    ULONG32 keywordCount = 0L;
    if (SUCCEEDED(status))
    {
        keywordCount = (ULONG32) m_MovieInfo.GetKeywordCount();
        if (keywordCount)
            pHeader->SetPropertyULONG32(_3GPP_META_INFO_KEYWORD_COUNT_KEY, keywordCount);
    }

    // 3GP Asset Info: Keywords
    for (ULONG32 ulIndex = 0; ulIndex < keywordCount; ulIndex++)
    {
        // "AssetKeyword" is 12 chars
        // keyCount is 8 bytes => "%d" at most is "255" => 3 chars
        // null char '\0' is 1 char 
        // we'll allocate 64 chars to be really safe
        char key[64];

        SafeSprintf(key, 64, _3GPP_META_INFO_KEYWORD_KEY, ulIndex);
        status = SetPropertyOnHeader(status, pHeader, key,
                     (const char*) (m_MovieInfo.GetKeyword(ulIndex)),
                     m_MovieInfo.GetKeywordLength(ulIndex));
    }

    // 3GP Asset Info: Location Info
    status = SetPropertyOnHeader(status, pHeader, 
                 _3GPP_META_INFO_LOCATION_NAME_KEY,
                 (const char*) m_MovieInfo.GetLocationName(),
                 m_MovieInfo.GetLocationNameLength());
    status = SetPropertyOnHeader(status, pHeader,
                 _3GPP_META_INFO_LOCATION_ASTRONOMICAL_BODY_KEY,
                 (const char*) m_MovieInfo.GetAstronomicalBody(),
                 m_MovieInfo.GetAstronomicalBodyLength());
    status = SetPropertyOnHeader(status, pHeader,
                 _3GPP_META_INFO_LOCATION_ADDITIONAL_NOTES_KEY,
                 (const char*) m_MovieInfo.GetLocationAdditionalNotes(),
                 m_MovieInfo.GetLocationAdditionalNotesLength());
    if (SUCCEEDED(status))
    {
        INT16 locationRole = m_MovieInfo.GetLocationRole();
        if (locationRole > (INT16) 0)
            pHeader->SetPropertyULONG32(_3GPP_META_INFO_LOCATION_ROLE_KEY, 
                         (ULONG32) locationRole);
    }

    LONG32 long_wholePart = m_MovieInfo.GetLongitude_WholePart();
    LONG32 long_fractionalPart = m_MovieInfo.GetLongitude_FractionalPart();
    LONG32 lat_wholePart = m_MovieInfo.GetLatitude_WholePart();
    LONG32 lat_fractionalPart = m_MovieInfo.GetLatitude_FractionalPart();
    LONG32 alt_wholePart = m_MovieInfo.GetAltitude_WholePart();
    LONG32 alt_fractionalPart = m_MovieInfo.GetAltitude_FractionalPart();

    // if any part of the GPS coordinates are invalid, then all
    // coordinates are invalid
    if (long_wholePart > -99999 && long_fractionalPart > -99999 &&
        lat_wholePart > -99999 && lat_fractionalPart > -99999 &&
        alt_wholePart > -99999 && alt_fractionalPart > -99999 )
    {
        char strPos[50];

        // 3GP Asset Info: Location Info: Longitude
        SafeSprintf(strPos, 50, "%ld.%lu", long_wholePart, long_fractionalPart);
        status = SetPropertyOnHeader(status, pHeader,
                    _3GPP_META_INFO_LOCATION_LONGITUDE_KEY, strPos, strlen(strPos));

        // 3GP Asset Info: Location Info: Latitude
        SafeSprintf(strPos, 50, "%ld.%lu", lat_wholePart, lat_fractionalPart);
        status = SetPropertyOnHeader(status, pHeader,
                    _3GPP_META_INFO_LOCATION_LATITUDE_KEY, strPos, strlen(strPos));

        // 3GP Asset Info: Location Info: Altitude
        SafeSprintf(strPos, 50, "%ld.%lu", alt_wholePart, alt_fractionalPart);
        status = SetPropertyOnHeader(status, pHeader,
                    _3GPP_META_INFO_LOCATION_ALTITUDE_KEY, strPos, strlen(strPos));
    }
#endif // HELIX_FEATURE_3GPP_METAINFO

    return status;
}
#endif // HELIX_FEATURE_3GPP_METAINFO || HELIX_FEATURE_SERVER

HX_RESULT CQTFileFormat::MakeFileHeader(HX_RESULT status)
{
    HX_RESULT retVal;
    IHXValues* pHeader = NULL;

    // Check the license
#ifdef QTCONFIG_SERVER
    if (SUCCEEDED(status))
    {
        switch (m_TrackManager.GetFType())
        {
            case QT_FTYPE_QT:
            case QT_FTYPE_UNKNOWN:
            {
                if(!m_bQTLicensed)
                {
                    ReportError(IDS_ERR_QT_NOTLICENSED, HXR_NOT_LICENSED);
                    return m_pFFResponse->FileHeaderReady(
                        HXR_NOT_LICENSED, NULL);
                }
                break;
            }
            case QT_FTYPE_MP4:
            {
		UINT32 ulBrand = 0;
		m_TrackManager.GetMajorBrand(&ulBrand);

		switch (ulBrand)
		{
		// If major brand is a rel6 brand, check that the 3GPP rel6 license is enabled
		case QT_3gg6:
		case QT_3gp6:
		case QT_3gr6:
		case QT_3gs6:
		    if (!m_b3GPPRel6Licensed)
		    {
			if (m_pErrorMessages)
			{
			    m_pErrorMessages->Report(HXLOG_ALERT, HXR_NOT_LICENSED, 0,
				"This server(proxy) is NOT licensed to deliver 3GPP "
				"streams. A Player attempting to access 3GPP Rel6 "
				"content has been disconnected. Please contact "
				"RealNetworks to obtain a license for this feature.\n", 
				NULL);
			}

			return m_pFFResponse->FileHeaderReady(
			    HXR_NOT_LICENSED, NULL);
		    }

		    break;

		// Otherwise, just check that the MP4 license is enabled
		default:

		    if (!m_bMP4Licensed)
		    {
			ReportError(IDS_ERR_MP4_NOTLICENSED, HXR_NOT_LICENSED);
			return m_pFFResponse->FileHeaderReady(HXR_NOT_LICENSED, 
			    NULL);
		    }

		    break;
		}
            }
        }
    }

    // Reject unsupported brands
    if (SUCCEEDED(status))
    {
	UINT32 ulBrand = 0;
	HX_RESULT resBrand = m_TrackManager.GetMajorBrand(&ulBrand);

	HXBOOL bIsSupportedBrand = TRUE;	
	if (SUCCEEDED(resBrand))
	{
	    // Reject 3gg6 (general) and 3gr6 (progressive download) profiles
	    if (ulBrand == QT_3gg6 || ulBrand == QT_3gr6)
	    {
		bIsSupportedBrand = FALSE;
	    }

	    // Reject probable rel7 / rel8 / rel9 brands
	    else
	    {
		const UINT32 kul3GPPProfileMask = 0xffff00ff;
		UINT32 ulMaskedBrand = ulBrand & kul3GPPProfileMask;

		const UINT32 kul3GPPRel7 = QT_ENCODE_TYPE('3', 'g', 0, '7');
		const UINT32 kul3GPPRel8 = QT_ENCODE_TYPE('3', 'g', 0, '8');
		const UINT32 kul3GPPRel9 = QT_ENCODE_TYPE('3', 'g', 0, '9');

		if (ulMaskedBrand == kul3GPPRel7 || ulMaskedBrand == kul3GPPRel8 || 
		    ulMaskedBrand == kul3GPPRel9)
		{
		    bIsSupportedBrand = FALSE;
		}
	    }
	}	

	// Log error message, if necessary
	if (!bIsSupportedBrand)
	{
	    char szBrand[10] = "(unknown)";	    

	    // Copy the brand 4cc
	    if (SUCCEEDED(resBrand))
	    {
		szBrand[0] = (char) (ulBrand >> 24);
		szBrand[1] = (char) (ulBrand >> 16);
		szBrand[2] = (char) (ulBrand >> 8);
		szBrand[3] = (char) (ulBrand);
		szBrand[4] = '\0';
	    }

	    char szBuf[512] = ""; /* Flawfinder: ignore */

            if (ulBrand == QT_3gr6)
            {
                SafeSprintf(szBuf, 512, "This server(proxy) does not support "
                    "3GPP files with a major brand of %s "
                    "(Progressive Download profile). "
                    "Please use only files that are encoded with the "
                    "\"Streaming Server profile (include hint tracks)\". "
                    "See 3GPP TS 26.244, Rel-6.", szBrand);
            }
            else if (ulBrand == QT_3gg6)
            {
                SafeSprintf(szBuf, 512, "This server(proxy) does not support "
                    "3GPP files with a major brand of %s "
                    "(General profile). "
                    "Please use only files that are encoded with the "
                    "\"Streaming Server profile (include hint tracks)\". "
                    "See 3GPP TS 26.244, Rel-6.", szBrand);
            }
            else
            {
                SafeSprintf(szBuf, 512, "This server(proxy) does not support "
                    "3GPP files with a major brand of %s "
                    "(Not Supported 3GPP Release - 7,8 or 9 Files).", szBrand);
            }

	    m_pErrorMessages->Report(HXLOG_ALERT, HXR_FAIL, 0, szBuf, NULL);

	    return m_pFFResponse->FileHeaderReady(
		HXR_FAIL, NULL);
	}
    }
#endif	// QTCONFIG_SERVER

    // Create needed buffers
    if (SUCCEEDED(status))
    {
	status = m_pClassFactory->CreateInstance(CLSID_IHXValues,
						 (void**) &pHeader);
    }

#if (defined HELIX_FEATURE_3GPP_METAINFO || defined HELIX_FEATURE_SERVER)
    // Set Title (if it's available)
    if (SUCCEEDED(status) && m_MovieInfo.GetName())
    {
        status = SetCStringPropertyCCFWithNullTerm(pHeader, 
                                                _3GPP_META_INFO_TITLE_KEY, 
                                                (BYTE*) (m_MovieInfo.GetName()), 
                                                m_MovieInfo.GetNameLength(),
                                                m_pContext,
                                                SET_AS_BUFFER_PROPERTY);
    }
#endif //defined HELIX_FEATURE_3GPP_METAINFO || defined HELIX_FEATURE_SERVER
    
#ifdef HELIX_FEATURE_3GPP_METAINFO
    // 3GPP Asset Info
    // When it is available, this 3GPP Asset Info should override any meta-info
    // obtained from more general means
    if (SUCCEEDED(status))
    {
        status = Set3GPPAssetInfoOnHeader(status, pHeader);
    }
#endif // HELIX_FEATURE_3GPP_METAINFO

    if (SUCCEEDED(status))
    {
	status = GetSessionIdentity(pHeader, &m_MovieInfo);
    }

#ifdef QTCONFIG_ALTERNATE_STREAMS
#ifdef QTCONFIG_LEGACY_ALTERNATE_STREAMS
    // Set stream groupings based on QuickTime AlternateGroup SDP entries.
    if (SUCCEEDED(status))
    {
	UINT16 uStrmIdx;
	ULONG32 ulGroupID;
	ULONG32 ulGroupBitrate;
	IQTTrack* pTrack;
	IHXValues* pGroupInfo = NULL;	

	for (uStrmIdx = 0; 
	     uStrmIdx < m_TrackManager.GetNumStreams();
	     uStrmIdx++)
	{
	    pTrack = m_TrackManager.GetStreamTrack(uStrmIdx);

	    if (pTrack->GetSDPLength() > 0)
	    {
		SDPParseChunk(
		    (char*) pTrack->GetSDP(),
		    pTrack->GetSDPLength(),
		    pGroupInfo,
		    m_pClassFactory,
		    SDPCTX_Group,
		    FALSE);

		if (pGroupInfo &&
		    (pGroupInfo->GetPropertyULONG32("AlternateGroupID",
						    ulGroupID)
		     == HXR_OK))
		{
		    if (pGroupInfo->GetPropertyULONG32("AlternateGroupBitrate",
						       ulGroupBitrate)
			!= HXR_OK)
		    {
			ulGroupBitrate = 0;
		    }

		    status = m_TrackManager.AddStreamToGroup(
			uStrmIdx,
			(UINT16) ulGroupID,
			ulGroupBitrate);
		}
						   
		HX_RELEASE(pGroupInfo);

		if (FAILED(status))
		{
		    break;
		}
	    }
	}
    }
#endif	// QTCONFIG_LEGACY_ALTERNATE_STREAMS

    IHXValues2* pParsedSDP = NULL;

#if defined(QTCONFIG_BANDWIDTH_PARTITIONING) || defined(QTCONFIG_LANGUAGE_SELECTION)
    // Parse the SDP at session level and extract bandwidth and language
    // associations from it.
    if (SUCCEEDED(status) && (m_MovieInfo.GetSDPLength() != 0))
    {
	// Create container for parsed SDP
	status = m_pClassFactory->CreateInstance(CLSID_IHXValues2, (void**)&pParsedSDP);
	if (SUCCEEDED(status))
	{
	    status = SetCStringPropertyCCFWithNullTerm(pParsedSDP,
						       "SDPData",
						       (BYTE*) m_MovieInfo.GetSDP(),
						       m_MovieInfo.GetSDPLength(),
						       m_pContext);
	}

	// Parse SDP
	if (SUCCEEDED(status))
	{
	    status = HXStreamDescriptionHelper::
		ParseAndMergeSDPData(m_pContext, pParsedSDP);
	}

#ifdef QTCONFIG_LANGUAGE_SELECTION
	// TODO: Discover lamguage associations with track ids and inform 
	//       track manager of them.
	//       Track manager needs to be the keeper of stream to language 
	//       association and take it into account when performing
	//       SubscribeDefault() below.
#endif	// QTCONFIG_LANGUAGE_SELECTION

	if (status != HXR_OK)
	{
	    // always make this successful since not essential
	    HX_RELEASE(pParsedSDP);
	    status = HXR_OK;
	}
    }
#endif	// QTCONFIG_BANDWIDTH_PARTITIONING || QTCONFIG_LANGUAGE_SELECTION

    if (SUCCEEDED(status))
    {
	status = m_TrackManager.InitStreamGroups(this,
						 m_pPacketAssembler);
    }

    // On the client, make default subscriptions and eliminate 
    // superfluous streams.
    if (SUCCEEDED(status) && 
	(m_TrackManager.GetEType() == QT_ETYPE_CLIENT))
    {
	status = m_TrackManager.SubscribeDefault();

	// If we are running in player, eliminate inactive stream tracks
	if (SUCCEEDED(status))
	{
	    status = m_TrackManager.RemoveInactiveStreams();
	}
    }

    // Once final stream set has been determined via default subscription
    // and removal of inactive streams, build uber ASM rule book
    // describing the bandwidth partitioning accross the media streams
    // for various bandwidth ranges.
    if (SUCCEEDED(status) && pParsedSDP)
    {
	// Failure of uber rule book is not catastrophic - 
	// disregard return code
	MakeASMUberRuleBook(pHeader, pParsedSDP);
    }

    HX_RELEASE(pParsedSDP);

#endif	// QTCONFIG_ALTERNATE_STREAMS

    // Set General properties, Stream Count and duration
    if (SUCCEEDED(status))
    {
	pHeader->SetPropertyULONG32("IsRealDataType", 0);
	pHeader->SetPropertyULONG32("StreamCount",  
				    m_TrackManager.GetNumStreams());
	m_ulFileDuration = (ULONG32) ((1000.0 * ((double) m_MovieInfo.GetMovieDuration())) /
				      m_MovieInfo.GetMovieTimeScale());
	pHeader->SetPropertyULONG32("Duration", m_ulFileDuration);
    }

    if (SUCCEEDED(status) && m_TrackManager.AreStreamGroupsPresent())
    {
        pHeader->SetPropertyULONG32("StreamGroupCount",  
				    m_TrackManager.GetNumStreamGroups());
    }

    // Set the licensing information: Basically this information is used by the
    // Broadcast Receiver plug-in to check and see if the RealServer is actually
    // licensed to receive this content
#ifdef QTCONFIG_SERVER
    if(SUCCEEDED(status) && (!m_bViewSourceRequest))
    {
        switch(m_TrackManager.GetFType())
        {
            case QT_FTYPE_QT:
            case QT_FTYPE_UNKNOWN:
            {
                IHXBuffer* pFileTypeBuf = NULL;
                IHXBuffer* pLicenseKeyBuf = NULL;
                m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pFileTypeBuf);
                m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pLicenseKeyBuf);
                pFileTypeBuf->Set((const BYTE*)"QuickTime", strlen("QuickTime") + 1);
                pLicenseKeyBuf->Set((const BYTE*)REGISTRY_QUICKTIME_ENABLED, strlen(REGISTRY_QUICKTIME_ENABLED) + 1);

                pHeader->SetPropertyCString("FileType", pFileTypeBuf);
                pHeader->SetPropertyCString("LicenseKey", pLicenseKeyBuf);
                pHeader->SetPropertyULONG32("DefaultLicenseValue", LICENSE_QUICKTIME_ENABLED);
                HX_RELEASE(pFileTypeBuf);
                HX_RELEASE(pLicenseKeyBuf);
                break;
            }
            case QT_FTYPE_MP4:
            {
		UINT32 ulBrand = 0;
		m_TrackManager.GetMajorBrand(&ulBrand);

		switch (ulBrand)
		{
		case QT_3gg6:
		case QT_3gp6:
		case QT_3gr6:
		case QT_3gs6:
		    {
			IHXBuffer* pFileTypeBuf = NULL;
			IHXBuffer* pLicenseKeyBuf = NULL;
			m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pFileTypeBuf);
			m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pLicenseKeyBuf);
			pFileTypeBuf->Set((const BYTE*)"3GPPRel6FileFormat", strlen("3GPPRel6FileFormat") + 1);
			pLicenseKeyBuf->Set((const BYTE*)REGISTRY_3GPP_REL6_ENABLED, strlen(REGISTRY_3GPP_REL6_ENABLED) + 1);

			pHeader->SetPropertyCString("FileType", pFileTypeBuf);
			pHeader->SetPropertyCString("LicenseKey", pLicenseKeyBuf);
			pHeader->SetPropertyULONG32("DefaultLicenseValue", LICENSE_3GPP_REL6_ENABLED);
			HX_RELEASE(pFileTypeBuf);
			HX_RELEASE(pLicenseKeyBuf);
			break;
		    }

		default:
		    {
			IHXBuffer* pFileTypeBuf = NULL;
			IHXBuffer* pLicenseKeyBuf = NULL;
			m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pFileTypeBuf);
			m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pLicenseKeyBuf);
			pFileTypeBuf->Set((const BYTE*)"MPEG4", strlen("MPEG4") + 1);
			pLicenseKeyBuf->Set((const BYTE*)REGISTRY_REALMP4_ENABLED, strlen(REGISTRY_REALMP4_ENABLED) + 1);

			pHeader->SetPropertyCString("FileType", pFileTypeBuf);
			pHeader->SetPropertyCString("LicenseKey", pLicenseKeyBuf);
			pHeader->SetPropertyULONG32("DefaultLicenseValue", LICENSE_REALMP4_ENABLED);
			HX_RELEASE(pFileTypeBuf);
			HX_RELEASE(pLicenseKeyBuf);
			break;
		    }
		}
            }
        }
    }
#endif	// QTCONFIG_SERVER

    // Set Requested Meta Information
    if (SUCCEEDED(status))
    {
	IHXValues* pHeaders = NULL;
	
	if (m_pRequest &&
	    (m_pRequest->GetRequestHeaders(pHeaders) == HXR_OK) &&
	    pHeaders)
	{	    
	    pHeaders->Release();
	}

	status = AddMetaInfo(pHeader);
	
	// We no longer need the request so release it
	HX_RELEASE(m_pRequest);
    }

    // Indicate that ASMRuleBook can be dropped from transmission without
    // invalidating the stream.  Dropping ASMRuleBook may prevent client side
    // rate adaptation.
    if (SUCCEEDED(status) && (m_TrackManager.GetEType() != QT_ETYPE_CLIENT))
    {
	    //If we are inside SLTA let us not make it optional. Send ASMRuleBook always!
        IHXRemoteBroadcastServices* pRBServices = NULL;
        if (FAILED(m_pContext->QueryInterface(IID_IHXRemoteBroadcastServices, (void**)&pRBServices)))
        {            
            pHeader->SetPropertyULONG32("ASMRuleBookOptional", 1);
        }
        else
        {            
            HX_RELEASE(pRBServices);
        }	
    }

    // Set View Source specific information
#ifdef HELIX_FEATURE_VIEWSOURCE
    if (SUCCEEDED(status) && m_bViewSourceRequest)
    {
	IHXBuffer* pCStringBuffer = NULL;

	status = m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
						 (void**) &pCStringBuffer);
	if (SUCCEEDED(status))
	{
	    const char* pString = "Not Hinted";

	    if (m_TrackManager.IsHinted())
	    {
		pString = "Hinted";
	    }

	    status = pCStringBuffer->Set((UINT8*) pString, strlen(pString) + 1);
	}

	if (SUCCEEDED(status))
	{
	    status = pHeader->SetPropertyCString("Characteristic", pCStringBuffer);
	}

	HX_RELEASE(pCStringBuffer);

	if (SUCCEEDED(status))
	{
	    status = pHeader->SetPropertyULONG32("ViewSourceInfoHeaders", 1);
	}
    }
#endif	// HELIX_FEATURE_VIEWSOURCE

    if (FAILED(status))
    {
	HX_RELEASE(pHeader);
    }

    m_bFileStructureEstablished = TRUE;

    retVal = m_pFFResponse->FileHeaderReady(status, pHeader);

    HX_RELEASE(pHeader);

    return retVal;
}

HX_RESULT CQTFileFormat::ObtainStreamHeader(UINT16 unStreamNumber,
					    IHXValues* &pHeader)
{
    HX_RESULT retVal = HXR_OK;
    IQTTrack* pTrack = NULL;
    
    pHeader = NULL;

    // Check Entry requirements
    if (m_State != QTFF_Ready)
    {
	retVal = HXR_UNEXPECTED;
    }

    if (SUCCEEDED(retVal))
    {
	retVal = HXR_INVALID_PARAMETER;
	if (unStreamNumber < m_TrackManager.GetNumStreams())
	{
	    retVal = HXR_OK;
	}
    }

    // See if made header is already available
    if (SUCCEEDED(retVal))
    {
	pTrack = m_TrackManager.GetStreamTrack(unStreamNumber);

	retVal = pTrack->BuildStreamHeader(pHeader,
					   &m_MovieInfo,
					   &m_TrackManager);
    }

    // Set the stream number
    if (SUCCEEDED(retVal))
    {
	HX_ASSERT(pHeader);
	retVal = pHeader->SetPropertyULONG32("StreamNumber", unStreamNumber);
    }

    // Set Stream Group Number if grouping exist for any stream
    if (SUCCEEDED(retVal))
    {
	if (m_TrackManager.AreStreamGroupsPresent())
	{
	    retVal = pHeader->SetPropertyULONG32(
		"StreamGroupNumber", 
		m_TrackManager.GetStreamGroupNumber(unStreamNumber));

	    // Mark the stream as default stream in the group if so
	    if (SUCCEEDED(retVal))
	    {
		if (m_TrackManager.IsStreamDefaultAlternate(unStreamNumber))
		{
		    retVal = pHeader->SetPropertyULONG32("DefaultStream", 1);
		}
	    }
	}
    }


    // Indicate that ASMRuleBook can be dropped from transmission without
    // invalidating the stream.  Dropping ASMRuleBook may prevent client side
    // rate adaptation.
    if (SUCCEEDED(retVal) && (m_TrackManager.GetEType() != QT_ETYPE_CLIENT))
    {
	    //If we are inside SLTA let us not make it optional. Send ASMRuleBook always!
        IHXRemoteBroadcastServices* pRBServices = NULL;
        if (FAILED(m_pContext->QueryInterface(IID_IHXRemoteBroadcastServices, (void**)&pRBServices)))
        {            
            pHeader->SetPropertyULONG32("ASMRuleBookOptional", 1);
        }
        else
        {            
            HX_RELEASE(pRBServices);
        }	
    }

    return retVal;
}

#ifdef QTCONFIG_BANDWIDTH_PARTITIONING

HX_RESULT CQTFileFormat::MakeASMUberRuleBook(IHXValues* pHeader,
					      IHXValues* pParsedSDP)
{
    CHXBWASAltGroupIterator bwIterator;
    UINT32 ulNumBWSets = 0;
    UINT32 ulMaxBookSize = 0;
    BandwidthSet* pBandwidthSet = NULL;
    UINT32* pulBandwidthPartition = NULL;
    HX_RESULT retVal;

    retVal = bwIterator.Init(pParsedSDP);

    // Find out number of non-empty bandwidth sets and maximum rule books size
    if (retVal == HXR_OK)
    {
	UINT32 ulNumStreamsInSet;
	CHXAltIDSet altIDSet;

	do
	{
	    if (SUCCEEDED(bwIterator.GetAltIDSet(altIDSet)))
	    {
		ulNumStreamsInSet = altIDSet.GetCount();

		if (ulNumStreamsInSet > 0)
		{
		    ulNumBWSets++;
		    ulMaxBookSize += 
			MAX_PER_BANDWIDTH_SET_UBERRULEBOOK_SIZE(ulNumStreamsInSet);
		}   
	    }

	    bwIterator.Next();
	} while (bwIterator.More());
    }

    // Allocate bandwidth table
    if (retVal == HXR_OK)
    {
	pBandwidthSet = new BandwidthSet [ulNumBWSets];
	pulBandwidthPartition = new UINT32 [m_TrackManager.GetNumStreamGroups()];

	retVal = HXR_OUTOFMEMORY;
	if (pBandwidthSet)
	{
	    retVal = HXR_OK;
	}
    }

    // Populate table with non-empty bandwidth sets
    if (retVal == HXR_OK)
    {
	retVal = bwIterator.Init(pParsedSDP);
    }

    if (retVal == HXR_OK)
    {
	UINT32 ulIdx = 0;

	do
	{
	    if (!pBandwidthSet[ulIdx].m_pAltIDSet)
	    {
		retVal = HXR_OUTOFMEMORY;
		break;
	    }

	    if (SUCCEEDED(bwIterator.GetAltIDSet(*(pBandwidthSet[ulIdx].m_pAltIDSet))))
	    {
		if (pBandwidthSet[ulIdx].m_pAltIDSet->GetCount() != 0)
		{
		    retVal = bwIterator.GetBandwidth(pBandwidthSet[ulIdx].m_ulBandwidth);
		    ulIdx++;
		}
	    }

	    bwIterator.Next();
	} while (bwIterator.More() && (retVal == HXR_OK));
    }

    // Sort the table by bandwidth
    if (retVal == HXR_OK)
    {
	// Sort by bandwidth
	UINT32 ulIdx1;
	UINT32 ulIdx2;
	CHXAltIDSet* pTempAltIdSet;
	UINT32 ulTempBandwidth;

	for (ulIdx1 = 0; ulIdx1 < ulNumBWSets; ulIdx1++)
	{
	    for (ulIdx2 = ulIdx1 + 1; ulIdx2 < ulNumBWSets; ulIdx2++)
	    {
		if (pBandwidthSet[ulIdx1].m_ulBandwidth > 
		    pBandwidthSet[ulIdx2].m_ulBandwidth)
		{
		    ulTempBandwidth = pBandwidthSet[ulIdx1].m_ulBandwidth;
		    pTempAltIdSet = pBandwidthSet[ulIdx1].m_pAltIDSet;
		    pBandwidthSet[ulIdx1].m_ulBandwidth = pBandwidthSet[ulIdx2].m_ulBandwidth;
		    pBandwidthSet[ulIdx1].m_pAltIDSet = pBandwidthSet[ulIdx2].m_pAltIDSet;
		    pBandwidthSet[ulIdx2].m_ulBandwidth = ulTempBandwidth;
		    pBandwidthSet[ulIdx2].m_pAltIDSet = pTempAltIdSet;
		}
	    }
	}
    }

    // Construct the ASM Uber Rule Book
    /* Sample:
       #($Bandwidth < 56000),Stream0Bandwidth=12000,Stream1Bandwidth=16000;
       #($Bandwidth >= 56000) && ($Bandwidth < 128000),Stream0Bandwidth=12000,Stream1Bandwidth=44000;
       #($Bandwidth >= 128000),Stream0Bandwidth = 44000,Stream1Bandwidth=80000;
    */
    if (retVal == HXR_OK)
    {
	char* pRuleBook = new char [ulMaxBookSize];
	char* pRuleBookWriter = pRuleBook;

	retVal = HXR_OUTOFMEMORY;

	if (pRuleBook)
	{
	    *pRuleBook = '\0';
	    retVal = HXR_OK;
	}

	if (retVal == HXR_OK)
	{
	    UINT32 ulIdx = 0;
	    UINT32 ulPreviousBandwidth = 0;
	    UINT32 ulBandwidth;
	    UINT32 ulBandwidthTo;
	    char* pRuleStart;
	    
	    do
	    {
		pRuleStart = pRuleBookWriter;

		ulBandwidth = pBandwidthSet[ulIdx].m_ulBandwidth;
		
		// For redundant bandwidth entries, pick the first one
		if ((ulIdx == 0) ||
		    (ulBandwidth != ulPreviousBandwidth))
		{
		    UINT16 ulTrackCount;
		    UINT32 ulTrackId;
		    UINT32 ulTrackIdx = 0;
		    UINT32 ulPartitionCount = 0;

		    // Form rule condtion based on bandwidth
		    if (ulIdx == 0)
		    {
			// first entry
			if (ulNumBWSets > 1)
			{
			    // We are dealing with multiple bandwidth sets
			    ulBandwidthTo = pBandwidthSet[ulIdx + 1].m_ulBandwidth;
			
			    pRuleBookWriter += SafeSprintf(pRuleBookWriter, 
				ulMaxBookSize - (pRuleBookWriter - pRuleBook), 
				"#($Bandwidth < %d)",
				ulBandwidthTo);
			}
		    }
		    else if ((ulIdx + 1) < ulNumBWSets)
		    {
			// mid entry (not first and not last)
			ulBandwidthTo = pBandwidthSet[ulIdx + 1].m_ulBandwidth;
			
			pRuleBookWriter += SafeSprintf(pRuleBookWriter, 
			    ulMaxBookSize - (pRuleBookWriter - pRuleBook), 
			    "#($Bandwidth >= %d) && ($Bandwidth < %d)",
			    ulBandwidth,
			    ulBandwidthTo);
		    }
		    else
		    {
			// last entry
			pRuleBookWriter += SafeSprintf(pRuleBookWriter, 
			    ulMaxBookSize - (pRuleBookWriter - pRuleBook), 
			    "#($Bandwidth >= %d)",
			    ulBandwidth);
		    }

		    // Pupulate bandwidth partitioning for the rule

		    // Clear the partition table
		    memset(pulBandwidthPartition, 
			   0, 
			   m_TrackManager.GetNumStreamGroups() * sizeof(UINT32));

		    ulTrackCount = pBandwidthSet[ulIdx].m_pAltIDSet->GetCount();

		    HX_ASSERT(ulTrackCount != 0);

		    // Form the partition table
		    do
		    {
			retVal = pBandwidthSet[ulIdx].m_pAltIDSet->
			    GetAltID(ulTrackIdx, ulTrackId);
			if (retVal == HXR_OK)
			{
			    UINT16 uStreamNumber;
			    CQTTrack* pQTTrack;

			    uStreamNumber = m_TrackManager.GetStreamNumberByTrackId(ulTrackId);

			    if (uStreamNumber < m_TrackManager.GetNumStreams())
			    {
				pQTTrack = m_TrackManager.GetTrackById(ulTrackId);
				if (pQTTrack)
				{
				    pQTTrack->ObtainTrackBandwidth(
					pulBandwidthPartition[m_TrackManager.GetStreamGroupNumber(uStreamNumber)]);
				}
			    }
			}

			ulTrackIdx++;
		    } while ((ulTrackIdx < ulTrackCount) && (retVal == HXR_OK));

		    // Build Rule Book Based on partition table
		    ulTrackIdx = 0;
		    ulPartitionCount = 0;

		    while ((ulTrackIdx < m_TrackManager.GetNumStreamGroups()) &&
			   (retVal == HXR_OK))
		    {
			if (pulBandwidthPartition[ulTrackIdx])
			{
			    if ((ulPartitionCount != 0) || (ulNumBWSets > 1))
			    {
				pRuleBookWriter += SafeSprintf(pRuleBookWriter, 
				    ulMaxBookSize - (pRuleBookWriter - pRuleBook), 
				    ",Stream%dBandwidth=%d",
				    ulTrackIdx,
				    pulBandwidthPartition[ulTrackIdx]);
			    }
			    else
			    {
				pRuleBookWriter += SafeSprintf(pRuleBookWriter, 
				    ulMaxBookSize - (pRuleBookWriter - pRuleBook), 
				    "Stream%dBandwidth=%d",
				    ulTrackIdx,
				    pulBandwidthPartition[ulTrackIdx]);
			    }

			    ulPartitionCount++;
			}

			ulTrackIdx++;
		    }

		    // If a rule was written, terminate it
		    if ((retVal == HXR_OK) && (pRuleStart != pRuleBookWriter))
		    {
			 *pRuleBookWriter = ';';
			 pRuleBookWriter++;
			 *pRuleBookWriter = '\0';
		    }
		}
		
		ulPreviousBandwidth = ulBandwidth;
		
		ulIdx++;
	    } while ((ulIdx < ulNumBWSets) && (retVal == HXR_OK));
	}

	if (SUCCEEDED(retVal) && (*pRuleBook != '\0'))
	{
	    retVal = SetCStringPropertyCCF(pHeader,
					   "ASMRuleBook",
					   pRuleBook,
					   m_pContext);
	}

	HX_VECTOR_DELETE(pRuleBook);
    }
    
    HX_VECTOR_DELETE(pBandwidthSet);
    HX_VECTOR_DELETE(pulBandwidthPartition);

    return retVal;
}

#else // QTCONFIG_BANDWIDTH_PARTITIONING

HX_RESULT CQTFileFormat::MakeASMUberRuleBook(IHXValues* pHeader,
					      IHXValues* pParsedSDP)
{
    return HXR_NOTIMPL;
}

#endif // QTCONFIG_BANDWIDTH_PARTITIONING

HX_RESULT CQTFileFormat::GetSessionIdentity(IHXValues* pHeader,
					    CQT_MovieInfo_Manager* pMovieInfo)
{
    return HXR_OK;
}

/************************************************************************
 *  GetPacket
 *  Method:
 *	IHXFileFormatObject::GetPacket
 *  Purpose:
 *	Called by controller to ask the file format for the next packet
 *	for a particular stream in the file. The file format should call
 *	IHXFileFormatResponse::Data*() for the IHXFileFormatResponse
 *	object that was passed in during initialization, when the packet
 *	is available.
 */
STDMETHODIMP CQTFileFormat::GetPacket(UINT16 unStreamNumber)
{
    HX_RESULT retVal;

    if (unStreamNumber < m_TrackManager.GetNumStreams())
    {
	switch (m_State)
	{
	case QTFF_Ready:
	    if ((m_bUnregulatedStreamDataFlow && 
		 m_pPacketCache && 
		 (!m_pPacketCache[unStreamNumber].bStreamDone)) ||
		(unStreamNumber == m_uNextPacketStreamNum))
	    {
		// Process this packet request
		// - but replace stream's cache first
		HX_ASSERT(m_pPacketCache);

		m_State = QTFF_GetPacket;
		return m_TrackManager.GetStreamTrack(unStreamNumber)->
				GetPacket(unStreamNumber);
	    }
	    else if (m_pPacketCache)
	    {
		// Delay processing packet for stream until its turn
		if (!m_pPacketCache[unStreamNumber].bStreamDone)
		{
		    if (!m_pPacketCache[unStreamNumber].bPending)
		    {
			m_pPacketCache[unStreamNumber].bPending = TRUE;
			return HXR_OK;
		    }
		    else
		    {
			retVal = HXR_UNEXPECTED;
		    }
		}
		else
		{
		    retVal = m_pFFResponse->StreamDone(unStreamNumber);
		}
	    }
	    else
	    {
		// Packet Cache not primed - start priming

		// To save memory, release streams not used
		m_TrackManager.RemoveInactiveStreams(TRUE);

		// When data flow is regulated, packets for all streams
		// are returned in sorted order (streams are merge sorted).
		// On servers, this behavior is undesireable as servers use
		// stream independet buffer control.
		// Note: Disabling regulation needs to be signaled with a new
		// interface.
		// if (m_TrackManager.GetEType() == QT_ETYPE_SERVER)
		// {
		//    m_bUnregulatedStreamDataFlow = TRUE;
		// }

		m_State = QTFF_PrimeCache;
		m_pPacketCache = new 
		    CPacketCache[m_TrackManager.GetNumStreams()];
		if (m_pPacketCache)
		{
		    m_pPacketCache[unStreamNumber].bPending = TRUE;

		    if (m_TrackManager.IsStreamTrackActive(0))
		    {
			return m_TrackManager.GetStreamTrack(0)->
				    GetPacket(0);
		    }
		    else
		    {
			return PacketReady(0, 
					   HXR_STREAM_DONE,
					   NULL);
		    }
		}
		else
		{
		    retVal = HXR_OUTOFMEMORY;
		    m_State = QTFF_Error;
		}
	    }
	    break;

	case QTFF_GetPacket:
	case QTFF_PrimeCache:
	    // Delay processing packet for stream until Ready
	    HX_ASSERT(m_pPacketCache);

	    if (!m_pPacketCache[unStreamNumber].bStreamDone)
	    {
		if (!m_pPacketCache[unStreamNumber].bPending)
		{
		    m_pPacketCache[unStreamNumber].bPending = TRUE;
		    return HXR_OK;
		}
		else
		{
		    retVal = HXR_UNEXPECTED;
		}
	    }
	    else
	    {
		retVal = m_pFFResponse->StreamDone(unStreamNumber);
	    }
	    break;

	default:
	    retVal = HXR_UNEXPECTED;
	    break;
	}
    }
    else
    {
	retVal = HXR_INVALID_PARAMETER;
    }

    return retVal;
}

/************************************************************************
 *  Seek
 *  Purpose:
 *	Called by controller to tell the file format to seek to the 
 *	nearest packet to the requested offset. The file format should 
 *	call IHXFileFormatResponse::SeekDone() for the IHXFileFormat-
 *	Session object that was passed in during initialization, when 
 *	the seek has completed.
 */
STDMETHODIMP CQTFileFormat::Seek(ULONG32 ulOffset)
{
    UINT16 uStreamNum;
    HX_RESULT retVal = HXR_OK;

    switch (m_State)
    {
    case QTFF_Ready:
	for (uStreamNum = 0;
	     uStreamNum < m_TrackManager.GetNumStreams();
	     uStreamNum++)
	{
	    if (m_TrackManager.IsStreamTrackActive(uStreamNum))
	    {
		m_TrackManager.GetStreamTrack(uStreamNum)->Seek(ulOffset);
	    }
	}

	m_uNextPacketStreamNum = NO_STREAM_SET;
	HX_VECTOR_DELETE(m_pPacketCache);

	retVal = m_pFFResponse->SeekDone(HXR_OK);
	break;

    case QTFF_GetPacket:
    case QTFF_PrimeCache:
    case QTFF_SeekPending:
	m_State = QTFF_SeekPending;
	m_ulPendingSeekTime = ulOffset;
	break;

    default:
	retVal = HXR_UNEXPECTED;
	break;
    }

    return retVal;
}

/************************************************************************
 *  WriteDone
 *  Purpose:
 *	Notification interface provided by users of the IHXFileObject
 *	interface. This method is called by the IHXFileObject when the
 *	last write to the file is complete.
 */
STDMETHODIMP CQTFileFormat::WriteDone(HX_RESULT status)
{
    return HXR_UNEXPECTED;
}

/************************************************************************
 *  SeekDone
 *  Purpose:
 *	Notification interface provided by users of the IHXFileObject
 *	interface. This method is called by the IHXFileObject when the
 *	last seek in the file is complete.
 */
STDMETHODIMP CQTFileFormat::SeekDone(HX_RESULT status)
{
    return HXR_UNEXPECTED;
}

/************************************************************************
 *  Close
 */
STDMETHODIMP CQTFileFormat::Close()
{
    m_State = QTFF_Error;

    m_bFileStructureEstablished = FALSE;
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    HX_DELETE(m_pOffsetToTimeMapper);
#endif	// HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS

    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pFFResponse);
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pErrorMessages);
    if (m_pFileSwitcher)
    {
	m_pFileSwitcher->Close(this);
	m_pFileSwitcher->Release();
	m_pFileSwitcher = NULL;
    }
    if (m_pAtomizer)
    {
	m_pAtomizer->Close();
	m_pAtomizer->Release();
	m_pAtomizer = NULL;
    }
    HX_RELEASE(m_pPacketAssembler);
#ifdef QTCONFIG_BFRAG_FACTORY
    HX_RELEASE(m_pBufferFragmentFactory);
#endif	// QTCONFIG_BFRAG_FACTORY
    HX_VECTOR_DELETE(m_pPacketCache);

    if (m_pFileObject)
    {
	m_pFileObject->Close();
	HX_RELEASE(m_pFileObject);
    }

    m_TrackManager.CloseTracks();

    return HXR_OK;
}

/************************************************************************
 *  IHXFileResponse
 */
/************************************************************************
 *  InitDone
 *  Purpose:
 *    Notification interface provided by users of the IHXFileObject
 *    interface. This method is called by the IHXFileObject when the
 *    initialization of the file is complete, and the Mime type is
 *    available for the request file. If the URL is not valid for the
 *    file system, the status HXR_FAILED should be returned,
 *    with a mime type of NULL. If the URL is valid but the mime type
 *    is unknown, then the status HXR_OK should be returned with
 *    a mime type of NULL.
 */
STDMETHODIMP CQTFileFormat::InitDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_OK;

    switch (m_State)
    {
    case QTFF_Init:
	if (SUCCEEDED(status))
	{
	    status = m_pAtomizer->Init((IUnknown*)(IHXFileSwitcher*) m_pFileSwitcher,
				       (IUnknown*)(IHXAtomizerResponse*) this,
				       (IUnknown*)(IHXAtomizationCommander*) this);
	}
	
	m_State = SUCCEEDED(status) ? QTFF_Ready : QTFF_Error;

	retVal = m_pFFResponse->InitDone(status);
	break;

    default:
	retVal = HXR_UNEXPECTED;
	break;
    }

    return retVal;
}


/************************************************************************
 *  CloseDone
 *  Purpose:
 *	Notification interface provided by users of the IHXFileObject
 *	interface. This method is called by the IHXFileObject when the
 *	close of the file is complete.
 */
STDMETHODIMP CQTFileFormat::CloseDone(HX_RESULT status)
{
    // nothing to do
    return HXR_OK;
}

/************************************************************************
 *  ReadDone
 *  Purpose:
 *	Notification interface provided by users of the IHXFileObject
 *	interface. This method is called by the IHXFileObject when the
 *	last read from the file is complete and a buffer is available.
 */
STDMETHODIMP CQTFileFormat::ReadDone(HX_RESULT status,
					    IHXBuffer* pBuffer)
{
    return HXR_UNEXPECTED;
}

/************************************************************************
 *  IHXAtomizationCommander
 */
/************************************************************************
 *  GetAtomCommand
 */
QTAtomizerCmd CQTFileFormat::GetAtomCommand(QTAtomType AtomType, CQTAtom* pParent)
{
    QTAtomizerCmd eCmd = ATMZR_CMD_LOAD;

    switch (AtomType)
    {
    case QT_udta:
	if (pParent->GetType() == QT_HXROOT)
	{
	    eCmd = ATMZR_CMD_SKIP;
	}
	break;

    default:
	// do nothing;
	break;
    }

    return eCmd;
}


/************************************************************************
 *  IHXAtomizerResponse
 */
/************************************************************************
 *  AtomReady
 */
HX_RESULT CQTFileFormat::AtomReady(HX_RESULT status, CQTAtom* pRootAtom)
{
    HX_RESULT retVal = HXR_UNEXPECTED;
    CQTPacketizerFactory* pPacketizerFactory = NULL;

    switch (m_State)
    {
    case QTFF_Atomize:
	if (SUCCEEDED(status))
	{
	    HX_ASSERT(pRootAtom);
	    
	    pRootAtom->AddRef();
	    
	    if (m_pAtomizer)
	    {
		m_pAtomizer->Close();
		m_pAtomizer->Release();
		m_pAtomizer = NULL;
	    } 
	}
	else
	{
	    pRootAtom = NULL;
	}
	
	if (SUCCEEDED(status))
	{
	    IHXClientEngine* pPlayer = NULL;
	    if (SUCCEEDED(m_pContext->QueryInterface(IID_IHXClientEngine, 
						     (void**)&pPlayer)))
	    {
		m_TrackManager.SetEType(QT_ETYPE_CLIENT);
	    }
	    else
	    {
		m_TrackManager.SetEType(QT_ETYPE_SERVER);
	    }
	    HX_RELEASE(pPlayer);
	}

	if (SUCCEEDED(status))
	{
	    status = m_TrackManager.ManageTracks(pRootAtom);
	}

#ifdef QTCONFIG_PACKETIZER_FACTORY
	if (SUCCEEDED(status))
	{
	    pPacketizerFactory = BuildPacketizerFactory();
	    if (!pPacketizerFactory)
	    {
		status = HXR_OUTOFMEMORY;
	    }
	}
#endif	// QTCONFIG_PACKETIZER_FACTORY
	
	if (SUCCEEDED(status))
	{
	    HXBOOL bIgnoreHintTracks = FALSE;
	    do
	    {
		status = m_TrackManager.ReadyTracks(bIgnoreHintTracks, 
	    					    m_bViewSourceRequest);

    	    	if (SUCCEEDED(status))
		{
		    status = m_MovieInfo.Init(pRootAtom->FindPresentChild(QT_moov),
					      &m_TrackManager);
		}

    	    	if (SUCCEEDED(status))
    	    	{    	    	
	    	    /*
	    	    * return some error to indicate try something else
	    	    */
	    	    status = m_TrackManager.InitTracks(this, 
					    	       m_pPacketAssembler,
					    	       pPacketizerFactory);
	    	    	
                    /* 
                        status is the return value from CheckForcePacketization()
                        (which is called by InitTracks()), if status == HXR_NOT_SUPPORTED 
                        then ForcePacketization is enabled for this mime type
                    */
    		    if ((HXR_NOT_SUPPORTED == status) && 
			(bIgnoreHintTracks = !bIgnoreHintTracks))
		    {
			m_MovieInfo.Clear();
			m_TrackManager.ResetTracks();
		    }
		    else
		    {
		    	/*
	    	    	* Until we add support for unhinted content on the server....
	    	    	* 
	    	    	* Verify that hint tracks are present and log a warning message if
	    	    	* they're missing
	    	    	*/	    	    	
		    	WarnIfNotHinted(status, bIgnoreHintTracks);
    
		    	/*
		     	* XXXGo - Change warning to take into account
		     	* hintrack and license for dynamic pktization
		     	* e.g. force pktization yet not licensed for dynamic pktization...
		     	*/		     
	    	    	
	    	    	if (FAILED(status) && m_bViewSourceRequest)
	    	    	{
		    	    // For view source, proceed attempting to return as much
		    	    // information as possible
		    	    status = HXR_OK;
	    	    	}

			break;
		    }
		}
		else
		{
		    break;
		}
	    } while (bIgnoreHintTracks);
	}
	
	HX_DELETE(pPacketizerFactory);

#ifdef QTCONFIG_BFRAG_FACTORY
	if (SUCCEEDED(status))
	{
	    HX_RELEASE(m_pBufferFragmentFactory);
	    m_pBufferFragmentFactory = new CBufferFragmentFactory(
		m_TrackManager.GetNumStreams() * QTBUFFERFRAGMENT_POOL_SIZE,
		m_TrackManager.GetNumStreams() * QTBUFFERFRAGMENT_INITIAL_POOL_SIZE);

	    retVal = HXR_OUTOFMEMORY;
	    if (m_pBufferFragmentFactory)
	    {
		m_pBufferFragmentFactory->AddRef();
		retVal = HXR_OK;
	    }
	}
#endif	// QTCONFIG_BFRAG_FACTORY

	HX_RELEASE(pRootAtom);

	m_State = SUCCEEDED(status) ? QTFF_Ready : QTFF_Error;

	retVal = MakeFileHeader(status);
	break;

    default:
	// nothing to do
	break;
    }

    return retVal;
}


/************************************************************************
 *  IHXASMSource
 */
/************************************************************************
 *  Subscribe
 */
STDMETHODIMP CQTFileFormat::Subscribe(UINT16 uStreamNum,
				      UINT16 uRuleNumber)
{
    return m_TrackManager.Subscribe(uStreamNum, uRuleNumber);
}

/************************************************************************
 *  Unsubscribe
 */
STDMETHODIMP CQTFileFormat::Unsubscribe(UINT16 uStreamNum,
					UINT16 uRuleNumber)
{
    return m_TrackManager.Unsubscribe(uStreamNum, uRuleNumber);
}


/************************************************************************
 *      Method:
 *          IHXPacketFormat::GetSupportedPacketFormats
 *      Purpose:
 *          Obtains a list of packet formats supported by this file format
 */
STDMETHODIMP CQTFileFormat::GetSupportedPacketFormats
(
    REF(const char**) /*OUT*/ pPacketFormats
)
{
    pPacketFormats = (const char**) zm_pPacketFormats;
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXPacketFormat::SetPacketFormat
 *      Purpose:
 *          Sets the packet type for this file format
 */
STDMETHODIMP CQTFileFormat::SetPacketFormat
(
    const char* pPacketFormat
)
{
    if (strcasecmp(pPacketFormat, "rtp") == 0)
    {
        m_ulPacketFormat = QTFF_RTP_FORMAT;
    }
    else
    {
        m_ulPacketFormat = QTFF_RDT_FORMAT;
    }
    return HXR_OK;
}


/************************************************************************
 *  Method:
 *      IHXMediaBytesToMediaDur::ConvertFileOffsetToDur
 *  Purpose:
 *      Gets the duration associated with the 
 *      
 *      With ulLastReadOffset, the FF can match up where its last Read
 *      was with what it knows the dur was for that Read and thus
 *      better estimate the duration at the given file offset.
 *
 *      It is possible that the duration is resolved to be indefinite
 *      in which case HX_PROGDOWNLD_INDEFINITE_DURATION is returned.
 *
 *      Also, ulFileSize can be HX_PROGDOWNLD_UKNOWN_FILE_SIZE and
 *      this method should still try to associate a duration with the
 *      bytes up to the last read.
 *
 *      If this fails (returns HXR_FAIL) then calling object should
 *      instead notify its IHXPDStatusMgr that the file size
 *      changed but that it doesn't have new dur info.  That way, at
 *      least the observer can know (& show) progress is happening.
 *
 *  A way to unit test this functionality:
 *  Place the following in the beginning of GetPacket method:
 *
 *  #define TEST_FILE_SIZE  <size of the test file>
 *  UINT32 ulDur = 0;
 *  ConvertFileOffsetToDur((UINT32) (TEST_FILE_SIZE/((double) RAND_MAX) * rand()),
 *                         TEST_FILE_SIZE,
 *                         ulDur);
 */
STDMETHODIMP
CQTFileFormat::ConvertFileOffsetToDur(UINT32 /*IN*/ ulLastReadOffset,
                                      UINT32 /*IN*/ ulCompleteFileSize,
                                      REF(UINT32) /*OUT*/ ulREFDur)
{
    HX_RESULT retVal = HXR_OK;
    UINT32 ulDurOut = HX_PROGDOWNLD_UNKNOWN_DURATION;

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    if (m_bFileStructureEstablished)
    {
	if (!m_pOffsetToTimeMapper)
	{
	    m_pOffsetToTimeMapper = new CQTOffsetToTimeMapper();

	    retVal = HXR_OUTOFMEMORY;
	    if (m_pOffsetToTimeMapper)
	    {
		retVal = HXR_OK;
	    }

	    if (SUCCEEDED(retVal))
	    {
		retVal = m_pOffsetToTimeMapper->Init(&m_TrackManager,
						     m_ulFileDuration);
	    }
	}

	if (SUCCEEDED(retVal))
	{
	    retVal = m_pOffsetToTimeMapper->Map(ulLastReadOffset, ulDurOut);
	}

	// We are always asked for new file offsets downloaded which are always
	// same or beyond where the file format is currently obtaining
	// data from.  Thus, it is valid to enforce that mapped offset time
	// must be beyond or same as packet time currently ready for
	// dispatch in the file format.
	if (SUCCEEDED(retVal))
	{
	    if (ulDurOut < m_ulNextPacketTime)
	    {
		ulDurOut = m_ulNextPacketTime;
	    }
	}

	// If we fail, the duration is unknown.
	// No fatal errors in this process.
	retVal = HXR_OK;
    }
    else
    {
	// If file structure is not established yet, we know that
	// playable duration is zero since we are asked only for the file
	// offsets that have been downloaded (not some yet to be
	// downloaded file offsets).
	ulDurOut = 0;
    }
#endif	// HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS

    if (SUCCEEDED(retVal))
    {
	ulREFDur = ulDurOut;
    }

    return retVal;
}


/************************************************************************
 *  Method:
 *      IHXMediaBytesToMediaDur::GetFileDuration
 *  Purpose:
 *      Gets the duration associated with the entire file.
 *
 *      It is possible that the duration is resolved to be indefinite
 *      in which case HX_PROGDOWNLD_INDEFINITE_DURATION is returned.
 *
 *      Callers can pass in HX_PROGDOWNLD_UKNOWN_FILE_SIZE if ulFileSize
 *      is not known to them and FF may still be able to determine
 *      ulREFDur.  However, if FF can't determine the duration of whole
 *      file (e.g., in a SMIL2 file), it returns HXR_FAIL and ulREFDur is
 *      set to HX_PROGDOWNLD_UNKNOWN_DURATION.
 */
STDMETHODIMP
CQTFileFormat::GetFileDuration(UINT32 /*IN*/ ulCompleteFileSize,
                               REF(UINT32) /*OUT*/ ulREFDur)
{
    HX_RESULT retVal = HXR_OK;
    UINT32 ulDuration = HX_PROGDOWNLD_UNKNOWN_DURATION;

    if (m_bFileStructureEstablished)
    {
	ulDuration = m_ulFileDuration;
    }

    ulREFDur = ulDuration;

    return retVal;
} 


/************************************************************************
 *  IQTTrackResponse Interface
 */
/************************************************************************
 *  PacketReady
 */
HX_RESULT CQTFileFormat::PacketReady(UINT16 uStreamNum, 
				     HX_RESULT status, 
				     IHXPacket* pPacket)
{
    HX_RESULT retVal;
    IHXPacket *pPacketToSend;

    switch (m_State)
    {
    case QTFF_GetPacket:
	pPacketToSend = m_pPacketCache[uStreamNum].pPacket;

	HX_ASSERT(pPacketToSend);

	if (status == HXR_OK)
	{
	    HX_ASSERT(pPacket);

	    m_pPacketCache[uStreamNum].pPacket = pPacket;
	    pPacket->AddRef();
	}
	else
	{
	    m_pPacketCache[uStreamNum].pPacket = NULL;
	    m_pPacketCache[uStreamNum].bStreamDone = TRUE;
	    if (status == HXR_STREAM_DONE)
	    {
		status = HXR_OK;
	    }
	}

	AddRef();   // Make Sure we do not die as a result of PacketReady

	m_pPacketCache[uStreamNum].bPending = FALSE;
	retVal = m_pFFResponse->PacketReady(status, pPacketToSend);

	if (pPacketToSend)
	{
	    pPacketToSend->Release();
	}

	// Make sure the state did not change as result of PacketReady()
	// The possible change is due to invocation of Close on PacketReady
	if (m_State == QTFF_GetPacket)
	{
	    m_State = QTFF_Ready;

	    // see if a pending request needs to be fullfilled
	    m_uNextPacketStreamNum = GetNextPacketStreamNum();
	    if ((m_uNextPacketStreamNum != NO_STREAM_SET) &&
		m_pPacketCache[m_uNextPacketStreamNum].bPending)
	    {
		m_State = QTFF_GetPacket;
		retVal = m_TrackManager.GetStreamTrack(m_uNextPacketStreamNum)->
			    GetPacket(m_uNextPacketStreamNum);
	    }
	}

	Release();

	return retVal;

    case QTFF_PrimeCache:
	if (status == HXR_OK)
	{
	    HX_ASSERT(pPacket);

	    m_pPacketCache[uStreamNum].pPacket = pPacket;
	    pPacket->AddRef();
	}
	else
	{
	    m_pPacketCache[uStreamNum].pPacket = NULL;
	    m_pPacketCache[uStreamNum].bStreamDone = TRUE;
	    if (status == HXR_STREAM_DONE)
	    {
		if (m_pPacketCache[uStreamNum].bPending)
		{
		    AddRef();   // Make sure we do not die

		    m_pPacketCache[uStreamNum].bPending = FALSE;
		    retVal = m_pFFResponse->StreamDone(uStreamNum);

		    if (m_State != QTFF_PrimeCache)
		    {
			// StreamDone must have closed us
			Release();

			break;
		    }

		    Release();
		}
	    }
	    else
	    {
		// Fatal failure
		retVal = HandleFailure(status);

		break;
	    }
	}

	uStreamNum++;
	if (uStreamNum < m_TrackManager.GetNumStreams())
	{
	    // we still have stream packets to prime
	    if (m_TrackManager.IsStreamTrackActive(uStreamNum))
	    {
		retVal = m_TrackManager.GetStreamTrack(uStreamNum)->
			    GetPacket(uStreamNum);
	    }
	    else
	    {
		retVal = PacketReady(uStreamNum, 
				     HXR_STREAM_DONE,
				     NULL);
	    }
	}
	else
	{
	    // we are done priming the packets
	    retVal = HXR_OK;
	    m_State = QTFF_Ready;

	    // see if a pending request needs to be fullfilled
	    m_uNextPacketStreamNum = GetNextPacketStreamNum();
	    if ((m_uNextPacketStreamNum != NO_STREAM_SET) &&
		m_pPacketCache[m_uNextPacketStreamNum].bPending)
	    {
		m_State = QTFF_GetPacket;
		retVal = m_TrackManager.
			    GetStreamTrack(m_uNextPacketStreamNum)->
				GetPacket(m_uNextPacketStreamNum);
	    }
	}
	break;

    case QTFF_SeekPending:
	HX_VECTOR_DELETE(m_pPacketCache);
	m_uNextPacketStreamNum = NO_STREAM_SET;
	m_State = QTFF_Ready;

	retVal = Seek(m_ulPendingSeekTime);
	break;

    default:
	retVal = HXR_UNEXPECTED;
	break;
    }

    return retVal;
}


/************************************************************************
 *  Protected Methods
 */
CQTPacketizerFactory* CQTFileFormat::BuildPacketizerFactory(void)
{
#ifdef QTCONFIG_PACKETIZER_FACTORY
    return new CQTPacketizerFactory();
#else	// QTCONFIG_PACKETIZER_FACTORY
    return NULL;
#endif	// QTCONFIG_PACKETIZER_FACTORY
}

/************************************************************************
 *  Private Methods
 */
/************************************************************************
 *  GetNextPacketStreamNum
 */
inline UINT16 CQTFileFormat::GetNextPacketStreamNum(void)
{
    UINT16 uStreamNum;
    ULONG32 ulLowestTime = 0xFFFFFFFF;
    UINT16 uNextPacketStream = NO_STREAM_SET;

    HX_ASSERT(m_pPacketCache);

    for (uStreamNum = 0;
	 uStreamNum < m_TrackManager.GetNumStreams();
	 uStreamNum++)
    {
	if (!m_pPacketCache[uStreamNum].bStreamDone)
	{
	    HX_ASSERT(m_pPacketCache[uStreamNum].pPacket);

	    if (m_bUnregulatedStreamDataFlow)
	    {
		if (m_pPacketCache[uStreamNum].bPending)
		{
		    // In the unregulated mode, we do not keep track
		    // of m_ulNextPacketTime.
		    return uStreamNum;
		}
	    }
	    else if (ulLowestTime >
		     m_pPacketCache[uStreamNum].pPacket->GetTime())
	    {
		uNextPacketStream = uStreamNum;
		ulLowestTime = m_pPacketCache[uStreamNum].
				    pPacket->GetTime();
		m_ulNextPacketTime = ulLowestTime;
	    }
	}
    }

    return uNextPacketStream;
}

/************************************************************************
 *  HandleFailure
 */
HX_RESULT CQTFileFormat::HandleFailure(HX_RESULT status)
{
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(FAILED(status));

    switch (m_State)
    {
    case QTFF_PrimeCache:
	{
	    UINT16 uStreamNum = 0;
	    m_State = QTFF_Error;
	    
	    AddRef();

	    while (m_pPacketCache &&
		   (uStreamNum < m_TrackManager.GetNumStreams()))
	    {
		if (m_pPacketCache[uStreamNum].bPending)
		{
		    m_pPacketCache[uStreamNum].bPending = FALSE;
		    m_pFFResponse->PacketReady(status, NULL);
		}
		uStreamNum++;
	    }

	    Release();
	}
	break;

    default:
	retVal = HXR_UNEXPECTED;
	break;
    }

    return retVal;
}

/************************************************************************
 *  a standard way of pulling a error string out of a resource file in a 
 *  localization friendly manner :)
 */
void 
CQTFileFormat::ReportError(UINT32 ulErrorID, HX_RESULT retVal)
{
    // Try to get the string from the resource manager
    CHXString cErrStr;
    HX_RESULT errRet = GetResourceErrorString(ulErrorID, cErrStr);
    if (errRet != HXR_OK)
    {
        switch (ulErrorID)
        {
            case IDS_ERR_QT_NOTLICENSED:
                cErrStr = ERRSTR_QT_NOTLICENSED;
                break;
            default:
                cErrStr = ERRSTR_QT_GENERALERROR;
                break;
        }
    }
 
    if (m_pErrorMessages)
    {
        m_pErrorMessages->Report(HXLOG_CRIT, retVal, 0, (const char*) cErrStr,  NULL);
    }
}

HX_RESULT 
CQTFileFormat::GetResourceErrorString(UINT32 ulErrorID, CHXString& rErrorStr)
{
    IHXExternalResourceManager* pResMgr = NULL;
    HX_RESULT retVal = m_pContext->QueryInterface(IID_IHXExternalResourceManager, (void**) &pResMgr);
    if (retVal != HXR_OK)
    {
        return retVal;
    }

	IHXExternalResourceReader* pResRdr = NULL;
    retVal = pResMgr->CreateExternalResourceReader(CORE_RESOURCE_SHORT_NAME, pResRdr);
    if (retVal != HXR_OK)
    {
        HX_RELEASE(pResMgr);
        return retVal;
    }

    IHXXResource* pRes = pResRdr->GetResource(HX_RT_STRING, ulErrorID);
    if(!pRes)
    {
        HX_RELEASE(pResRdr);
        HX_RELEASE(pResMgr);
        return HXR_FAIL;
    }

    // Assign the error string to the out parameter
    rErrorStr = (const char*) pRes->ResourceData();

    // Release all references
    HX_RELEASE(pRes);
    HX_RELEASE(pResRdr);
    HX_RELEASE(pResMgr);

    return HXR_OK;
}

/************************************************************************
 *  AddMetaInfo
 */
HX_RESULT CQTFileFormat::AddMetaInfo(IHXValues* pHeader)
{
    HXBOOL bAddAll;

    ULONG32 ulPresentationWidth = 0;
    ULONG32 ulPresentationHeight = 0;
    
    HX_ASSERT(pHeader);

    if ((m_TrackManager.GetEType() == QT_ETYPE_CLIENT) || 
	(m_ulStreamMetaInfoMask & META_INFO_ALL) ||
	(m_ulStreamMetaInfoMask & META_INFO_WIDTH) ||
	(m_ulStreamMetaInfoMask & META_INFO_HEIGHT))
    {
	UINT16 uStreamCount;
	IQTTrack* pTrack;
	ULONG32 ulTrackWidth;
	ULONG32 ulTrackHeight;

	// Collect Meta Info
	for (uStreamCount = m_TrackManager.GetNumStreams();
	     uStreamCount != 0;
	     uStreamCount--)
	{
	    pTrack = m_TrackManager.GetStreamTrack(uStreamCount - 1);
	
	    ulTrackWidth = pTrack->GetTrackWidth();
	    ulTrackHeight = pTrack->GetTrackHeight();

	    if (ulPresentationWidth < ulTrackWidth)
	    {
		ulPresentationWidth = ulTrackWidth;
	    }

	    if (ulPresentationHeight < ulTrackHeight)
	    {
		ulPresentationHeight = ulTrackHeight;
	    }
	}
    }

    bAddAll = ((m_TrackManager.GetEType() == QT_ETYPE_CLIENT) || 
	       (m_ulStreamMetaInfoMask & META_INFO_ALL));

    if (bAddAll)
    {
	if (ulPresentationWidth > 0)
	{
	    pHeader->SetPropertyULONG32(QT_WIDTH_METANAME, 
					ulPresentationWidth);
	}
	if (ulPresentationHeight > 0)
	{
	    pHeader->SetPropertyULONG32(QT_HEIGHT_METANAME, 
					ulPresentationHeight);
	}
    }
#ifdef QTCONFIG_SERVER
    else if((m_ulStreamMetaInfoMask & META_INFO_ALL) ||
	    (m_ulStreamMetaInfoMask & META_INFO_WIDTH) ||
	    (m_ulStreamMetaInfoMask & META_INFO_HEIGHT))
    {	    
	if (ulPresentationWidth > 0 &&
	    ((m_ulStreamMetaInfoMask & META_INFO_ALL) ||
	    (m_ulStreamMetaInfoMask & META_INFO_WIDTH)))
	{
	    pHeader->SetPropertyULONG32(QT_WIDTH_METANAME, 
					ulPresentationWidth);
	}

	if (ulPresentationHeight > 0 &&
	    ((m_ulStreamMetaInfoMask & META_INFO_ALL) ||
	    (m_ulStreamMetaInfoMask & META_INFO_HEIGHT)))
	{
	    pHeader->SetPropertyULONG32(QT_HEIGHT_METANAME, 
					ulPresentationHeight);
	}
    } 
#endif	// QTCONFIG_SERVER

    return HXR_OK;
}

#ifdef QTCONFIG_SERVER

/************************************************************************
 *  CheckLicense
 */
HX_RESULT CQTFileFormat::CheckLicense(void)
{
    HX_ASSERT(m_pContext);
    
    IHXClientEngine* pPlayer = NULL;
    IHXRemoteBroadcastServices* pRBServices = NULL;
    if (SUCCEEDED(m_pContext->QueryInterface(IID_IHXClientEngine, (void**)&pPlayer))
        || SUCCEEDED(m_pContext->QueryInterface(IID_IHXRemoteBroadcastServices, (void**)&pRBServices)))
    {
        // Data types are always licensed on the Player and iqslta
        m_bQTLicensed = TRUE;
	m_bMP4Licensed = TRUE;
	m_b3GPPRel6Licensed = TRUE;
    }
    else
    {    
        // On the Server, check the license section of the registry
        IHXRegistry* pRegistry = NULL;        
	if (HXR_OK != m_pContext->QueryInterface(IID_IHXRegistry, 
				(void**)&pRegistry))
	{
	    return HXR_UNEXPECTED;
	}

        INT32 nLicensed = 0;
        if (!SUCCEEDED(pRegistry->GetIntByName(REGISTRY_QUICKTIME_ENABLED, 
            nLicensed)))
        {
            /* take default */
            nLicensed = LICENSE_QUICKTIME_ENABLED;
        }
        m_bQTLicensed = (nLicensed) ? (TRUE) : (FALSE);

        if (!SUCCEEDED(pRegistry->GetIntByName(REGISTRY_REALMP4_ENABLED, 
            nLicensed)))
        {
            /* take default */
            nLicensed = LICENSE_REALMP4_ENABLED;
        }
        m_bMP4Licensed = (nLicensed) ? (TRUE) : (FALSE);

        if (!SUCCEEDED(pRegistry->GetIntByName(REGISTRY_3GPP_REL6_ENABLED, 
            nLicensed)))
        {
            /* take default */
            nLicensed = LICENSE_3GPP_REL6_ENABLED;
        }
        m_b3GPPRel6Licensed = (nLicensed) ? (TRUE) : (FALSE);

        HX_RELEASE(pRegistry);
    }
    HX_RELEASE(pPlayer);
    HX_RELEASE(pRBServices);

    return HXR_OK;
}

/************************************************************************
 *  WarnIfNotHinted
 */
void 
CQTFileFormat::WarnIfNotHinted(HX_RESULT status, HXBOOL bIgnoreHintTracks)
{    
    if (! ((m_TrackManager.GetEType() == QT_ETYPE_SERVER) && 
            m_pErrorMessages))
    {
        return;
    }
    
    if (HXR_NOT_LICENSED == status)
    {
        //sufficient error message already emitted
        return;
    }
    
    /*
     * log what's going on...
     */    
    if ((status == HXR_NOT_SUPPORTED) && 
        (bIgnoreHintTracks || (!m_TrackManager.IsHinted())))
    {	
	const char* pszMsg = "Unable to stream (%s) as packetizer is not available.";
        const char* pReqURL = NULL;        
        UINT32 ulMaxURLLen = 2048;
        UINT32 ulBuffLen = ulMaxURLLen + strlen(pszMsg) + 1;

        if (m_pRequest && SUCCEEDED(m_pRequest->GetURL(pReqURL)) && pReqURL)
        {
            //truncate (the left side) of the URL if it's too long
            UINT32 ulURLLen = strlen(pReqURL);
            if (ulURLLen >  ulMaxURLLen)
            {
                pReqURL += ulURLLen - ulMaxURLLen;  
            }
        }
        else 
        {
            pReqURL = " ";
        }

        NEW_FAST_TEMP_STR(pszTmpStr, 4096, ulBuffLen);
        snprintf(pszTmpStr, ulBuffLen, pszMsg, pReqURL); 
        m_pErrorMessages->Report(HXLOG_WARNING, HXR_OK, 0, pszTmpStr, NULL);
        DELETE_FAST_TEMP_STR(pszTmpStr);
    }
}

#else	// ! QTCONFIG_SERVER

HX_RESULT CQTFileFormat::CheckLicense(void)
{
    return HXR_OK;
}

void CQTFileFormat::WarnIfNotHinted(HX_RESULT status, HXBOOL bIgnoreHintTracks)
{
    return;
}

#endif	// IFDEF QTCONFIG_SERVER

/************************************************************************
 *	Method:
 *	    IHXThreadSafeMethods::IsThreadSafe
 */
STDMETHODIMP_(UINT32) CQTFileFormat::IsThreadSafe()
{
    return HX_THREADSAFE_METHOD_FF_GETPACKET |
	   HX_THREADSAFE_METHOD_FSR_READDONE;
}

/************************************************************************
 *  IUnknown methods
 */
STDMETHODIMP CQTFileFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal;

    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*) (IHXPlugin*) this},
	{ GET_IIDHANDLE(IID_IHXPlugin), (IHXPlugin*) this},
	{ GET_IIDHANDLE(IID_IHXFileFormatObject), (IHXFileFormatObject*) this},
	{ GET_IIDHANDLE(IID_IHXAtomizerResponse), (IHXAtomizerResponse*) this},
	{ GET_IIDHANDLE(IID_IHXAtomizationCommander), (IHXAtomizationCommander*) this},
#ifdef QTCONFIG_ALTERNATE_STREAMS
	{ GET_IIDHANDLE(IID_IHXASMSource), (IHXASMSource*) this},
#endif	// QTCONFIG_ALTERNATE_STREAMS
	{ GET_IIDHANDLE(IID_IHXPacketFormat), (IHXPacketFormat*) this},
	{ GET_IIDHANDLE(IID_IHXFileSwitcher), (IHXFileSwitcher*) m_pFileSwitcher},
	{ GET_IIDHANDLE(IID_IHXCommonClassFactory), m_pClassFactory},
	{ GET_IIDHANDLE(IID_IHXScheduler), m_pScheduler},
    { GET_IIDHANDLE(IID_IHXErrorMessages), m_pErrorMessages} 
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
      , { GET_IIDHANDLE(IID_IHXMediaBytesToMediaDur), (IHXMediaBytesToMediaDur*)this}
#endif /* #if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS). */
    };

    retVal = QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    if (FAILED(retVal))
    {
	if (IsEqualIID(riid, IID_IHXPDStatusMgr))
	{
	    // Pass the progressive download manager query to file object.
	    // This approach of handling progressive download works only for 
	    // self contained .mp4 files.
	    // The multi-file .mp4s are such small use case that are not
	    // worth the complexity at this time.
	    if (m_pFileObject)
	    {
		retVal = m_pFileObject->QueryInterface(riid, ppvObj);
	    }
	}
    }
#endif	// HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS

    return retVal;
}

STDMETHODIMP_(ULONG32) CQTFileFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CQTFileFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/************************************************************************
 *  GetFileHeader
 *  Purpose:
 *	Called by controller to ask the file format for the number of
 *	headers in the file. The file format should call the 
 *	IHXFormatResponse::StreamCountReady() for the IHXFileFormat-
 *	Session object that was passed in during initialization, when the
 *	header count is available.
 */
STDMETHODIMP CQTFileFormat::GetFileHeader()
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_State == QTFF_Ready)
    {
	if (m_pAtomizer)
	{
	    // This must be the first call to GetFileHeader - Atomize
	    m_State = QTFF_Atomize;
	    retVal = m_pAtomizer->Atomize();
	}
	else
	{
	    retVal = MakeFileHeader(HXR_OK);
	}
    }
    
    return retVal;
}


/************************************************************************
 *  GetStreamHeader
 *  Purpose:
 *	Called by controller to ask the file format for the header for
 *	a particular stream in the file. The file format should call 
 *	IHXFileFormatResponse::HeaderReady() for the IHXFileFormatResponse
 *	object that was passed in during initialization, when the header
 *	is available.
 */
STDMETHODIMP CQTFileFormat::GetStreamHeader(UINT16 unStreamNumber)
{
    HX_RESULT retVal;
    IHXValues* pHeader;

    retVal = ObtainStreamHeader(unStreamNumber, pHeader);

    if (SUCCEEDED(retVal))
    {
	retVal = m_pFFResponse->StreamHeaderReady(retVal, pHeader);
    }

    HX_RELEASE(pHeader);

    return retVal;
}

