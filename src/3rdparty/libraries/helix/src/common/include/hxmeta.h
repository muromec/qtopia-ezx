/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmeta.h,v 1.4 2007/07/06 20:43:42 jfinnecy Exp $
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

#ifndef _HXMETA_
#define _HXMETA_

#define HX_RAM_ENTRY_TAG	"##"
#define HX_RAM_ENTRY_TAGSIZE	2

#define HX_RAM20_START_TAG	"## .RAM_V2.0_START"
#define HX_RAM20_START_TAGSIZE	22
#define HX_RAM20_END_TAG	"## .RAM_V2.0_END"
#define HX_RAM20_END_TAGSIZE	20

#define HX_RAM30_START_TAG	"## .RAM_V3.0_START"
#define HX_RAM30_START_TAGSIZE	22
#define HX_RAM30_END_TAG	"## .RAM_V3.0_END"
#define HX_RAM30_END_TAGSIZE	20

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef struct _HXxWindow   HXxWindow;

typedef _INTERFACE	IUnknown			IUnknown;
typedef _INTERFACE	IHXFileSystemObject		IHXFileSystemObject;
typedef _INTERFACE	IHXFileObject			IHXFileObject;
typedef _INTERFACE	IHXRequest			IHXRequest;

typedef _INTERFACE	IHXMetaGroup			IHXMetaGroup;
typedef _INTERFACE	IHXSiteLayout			IHXSiteLayout;


typedef _INTERFACE	IHXMetaFileFormatObject	IHXMetaFileFormatObject;
typedef _INTERFACE	IHXMetaFileFormatResponse	IHXMetaFileFormatResponse;

enum GROUP_TYPE
{
    SEQUENCE_GROUP,
    PARALLEL_GROUP,
    SWITCH_GROUP
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXMetaTrack
 *
 *  Purpose:
 *
 *	This interface allows access to a track's properties with special
 *	functions to get a track's layout & channel.
 *
 *
 *  IID_IHXMetaTrack:
 *
 *	{00000E01-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXMetaTrack, 0x00000E01, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59); 

#undef  INTERFACE
#define INTERFACE   IHXMetaTrack

DECLARE_INTERFACE_(IHXMetaTrack, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXMetaTrack methods
     */
    /************************************************************************
     *	Method:
     *	    IHXMetaGroup::GetStreamCount
     *	Purpose:
     *	    Gets a count of the number of streams exposed by this track
     */
    STDMETHOD_(INT32,GetStreamCount)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXMetaTrack::GetStreamById
     *	Purpose:
     *	    Gets a stream object whose "ID" property equals nStreamID
     *	    file group.
     */
    STDMETHOD(GetStreamById) 	(THIS_
			  	INT32		/*IN*/  nStreamID,
   			  	REF(IUnknown*)	/*OUT*/ pStream) PURE;

    /************************************************************************
     *	Method:
     *	    IHXMetaTrack::GetStreamByName
     *	Purpose:
     *	    Gets a stream object whose "ID" property equals nStreamID
     *	    file group.
     */
    STDMETHOD(GetStreamByName) 	(THIS_
			  	const char*	/*IN*/  pName,
   			  	REF(IUnknown*)	/*OUT*/ pStream) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXMetaGroup
 *
 *  Purpose:
 *
 *	This interface allows access to a group's properties with special
 *	functions to get a track's layout.
 *
 *
 *  IID_IHXMetaGroup:
 *
 *	{00000E02-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXMetaGroup, 0x00000E02, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59); 

#undef  INTERFACE
#define INTERFACE   IHXMetaGroup

DECLARE_INTERFACE_(IHXMetaGroup, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXMetaGroup methods
     */

    /************************************************************************
     *	Method:
     *	    IHXMetaGroup::GetTrackCount
     *	Purpose:
     *	    Gets a count of the number of tracks exposed by this meta file
     *	    group.
     */
    STDMETHOD_(INT32,GetTrackCount)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXMetaGroup::GetTrack
     *	Purpose:
     *	    Gets a track object of the numbered track exposed by this meta 
     *	    file group.
     */
    STDMETHOD(GetTrack)	(THIS_
			INT32			/*IN*/  nTrackNumber,
   			REF(IHXMetaTrack*)	/*OUT*/ pTrack) PURE;

    /************************************************************************
     *	Method:
     *	    IHXMetaGroup::GetLayoutCount
     *	Purpose:
     *	    Gets a count of the number of Layouts exposed by this meta file
     *	    group.
     */
    STDMETHOD_(INT32,GetLayoutCount)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXMetaGroup::GetLayout
     *	Purpose:
     *	    Gets a Layout object of the numbered Layout exposed by this meta 
     *	    file group.
     */
    STDMETHOD(GetLayoutSite)(THIS_
  			REF(IHXSiteLayout*)	/*OUT*/ pLayout) PURE;

    STDMETHOD(GetLayout)(THIS_
			INT32			/*IN*/  nLayoutNumber,
   			REF(IUnknown*)		/*OUT*/ pUnknown)PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXMetaFileFormatObject
 * 
 *  Purpose:
 * 
 *	This interface allows a new meta-file format to be pluged into
 *	the client application.
 * 
 *  IID_IHXMetaFileFormatObject:
 * 
 *	{00000E05-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXMetaFileFormatObject, 0x00000E05, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXMetaFileFormatObject

DECLARE_INTERFACE_(IHXMetaFileFormatObject, IUnknown)
{
	/*
	 *	IUnknown methods
	 */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

	/*
	 *	IHXMetaFileFormatObject methods
	 */

    /************************************************************************
     *	Method:
     *	    IHXMetaFileFormatObject::GetMetaFileFormatInfo
     *	Purpose:
     *	    Returns information vital to the instantiation of meta file 
     *	    format plugins.
     */
    STDMETHOD(GetMetaFileFormatInfo)
			(THIS_
			REF(const char**) /*OUT*/ pFileMimeTypes,
			REF(const char**) /*OUT*/ pFileExtensions,
			REF(const char**) /*OUT*/ pFileOpenNames
			) PURE;

    STDMETHOD(InitMetaFileFormat)	
			(THIS_
    			IHXRequest*	    	    /*IN*/  pRequest,
			IHXMetaFileFormatResponse* /*IN*/  pMetaResponse,
			IHXFileObject*		    /*IN*/  pFileObject
			) PURE;

    /************************************************************************
     *	Method:
     *	    IHXMetaGroup::GetGroupCount
     *	Purpose:
     *	    Gets a count of the number of Groups exposed by this meta file
     *	    group.
     */
    STDMETHOD_(INT32,GetGroupCount)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXMetaGroup::GetGroup
     *	Purpose:
     *	    Gets a Group object of the numbered Group exposed by this meta 
     *	    file group.
     */
    STDMETHOD(GetGroup)	(THIS_
			INT32			/*IN*/  nGroupNumber,
   			REF(IHXMetaGroup*)	/*OUT*/ pGroup) PURE;

    STDMETHOD(Close)		(THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXMetaFileFormatResponse
 * 
 *  Purpose:
 * 
 *	This interface allows a new meta-file format to be pluged into
 *	the client application.
 * 
 *  IID_IHXMetaFileFormatResponse:
 * 
 *	{00000E06-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXMetaFileFormatResponse, 0x00000E06, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXMetaFileFormatResponse

DECLARE_INTERFACE_(IHXMetaFileFormatResponse, IUnknown)
{
	/*
	 *	IUnknown methods
	 */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

	/*
	 *	IHXMetaFileFormatResponse methods
	 */

    /************************************************************************
     *	Method:
     *	    IHXMetaFileFormatResponse::InitDone
     *	Purpose:
     *	    Call to inform the metafile format response that the metafile
     *	    format plugin has completed the initialization of the metafile,
     *	    and is ready to respond to queries.
     *
     *	    Use HXR_OK if the create was succesful
     *
     *	    Use HXR_FAILED if the contents of the file was not valid
     *	    or if the creation failed for some reason.
     */
    STDMETHOD(InitDone)   (THIS_
			    HX_RESULT status) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteLayout
 *
 *  Purpose:
 *
 *	Interface for IHXSiteLayout...
 *
 *  IID_IHXSiteLayout:
 *
 *	{00000E07-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSiteLayout, 0x00000E07, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSiteLayout

DECLARE_INTERFACE_(IHXSiteLayout, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXSiteLayout methods called the client core.
     */
    STDMETHOD_(ULONG32,GetLayoutSiteGroupCount)	(THIS) PURE;

    STDMETHOD(GetLayoutSiteGroup)		(THIS_ 
						ULONG32		nGroupNumber,
						REF(IUnknown*)	pLSG) PURE;
};



#endif /* _HXMETA_ */
