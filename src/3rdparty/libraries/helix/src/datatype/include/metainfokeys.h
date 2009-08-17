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

#ifndef _METAINFOKEYS_H_
#define _METAINFOKEYS_H_ 

#if (defined HELIX_FEATURE_3GPP_METAINFO || defined HELIX_FEATURE_SERVER)
#define _3GPP_META_INFO_TITLE_KEY			"Title"
#define _3GPP_META_INFO_AUTHOR_KEY			"Author"
#define _3GPP_META_INFO_COPYRIGHT_KEY			"Copyright"
#ifdef HELIX_FEATURE_3GPP_METAINFO
#define _3GPP_META_INFO_DESCRIPTION_KEY			"Abstract"
#define _3GPP_META_INFO_PERFORMER_KEY			"Performer"
#define _3GPP_META_INFO_GENRE_KEY			"Genre"
#define _3GPP_META_INFO_RATING_KEY			"Rating"
#define _3GPP_META_INFO_RATING_ENTITY_KEY		"RatingEntity"
#define _3GPP_META_INFO_RATING_INFO_KEY			"RatingInfo"
#define _3GPP_META_INFO_CLASSIFICATION_ENTITY_KEY	"ClassificationEntity"
#define _3GPP_META_INFO_CLASSIFICATION_INFO_KEY		"ClassificationInfo"
#define _3GPP_META_INFO_CLASSIFICATION_TABLE_KEY	"ClassificationTable"
#define _3GPP_META_INFO_KEYWORD_COUNT_KEY		"KeywordCount"
#define _3GPP_META_INFO_KEYWORD_KEY			"Keyword%d"
#define _3GPP_META_INFO_LOCATION_NAME_KEY		"LocationName"
#define _3GPP_META_INFO_LOCATION_ASTRONOMICAL_BODY_KEY	"LocationAstronomicalBody"
#define _3GPP_META_INFO_LOCATION_ADDITIONAL_NOTES_KEY	"LocationAdditionalNotes"
#define _3GPP_META_INFO_LOCATION_ROLE_KEY		"LocationRole"
#define _3GPP_META_INFO_LOCATION_LONGITUDE_KEY		"LocationLongitude"
#define _3GPP_META_INFO_LOCATION_LATITUDE_KEY		"LocationLatitude"
#define _3GPP_META_INFO_LOCATION_ALTITUDE_KEY		"LocationAltitude"
#endif // HELIX_FEATURE_3GPP_METAINFO
#endif // HELIX_FEATURE_3GPP_METAINFO || HELIX_FEATURE_SERVER

#endif // _METAINFOKEYS_H_

