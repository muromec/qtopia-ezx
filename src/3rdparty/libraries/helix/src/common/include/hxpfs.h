/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpfs.h,v 1.2 2007/07/06 20:43:42 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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



#ifndef _HXPFS_H
#define _HXPFS_H

/*
 * Forward declarations of interfaces defined in this file:
 */
typedef _INTERFACE  IHXPresentationFeatureManager   IHXPresentationFeatureManager;
typedef _INTERFACE  IHXPresentationFeatureSink      IHXPresentationFeatureSink;

/*
 * Forward declarations of interfaces referenced in this file:
 */
typedef _INTERFACE  IHXValues                       IHXValues;
typedef _INTERFACE  IHXBuffer                       IHXBuffer;


/*
 * PFS Defines:
 */
#define PRESENTATION_FEATURE_SELECTION_REGISTRY_PREFIX          "PFS"
#define PRESENTATION_FEATURE_SELECTION_REGISTRY_CURVAL_STR      "curValue"
#define PRESENTATION_FEATURE_SELECTION_REGISTRY_OPTIONS_STR     "options"
#define PRESENTATION_FEATURE_SELECTION_REGISTRY_OPTIONS_FREEFORM_STR     "_<freeform>"
// P.F.Event string defines:
#define PRESENTATION_FEATURE_SELECTION_EVENT_PREFIX             "pfs"
#define PRESENTATION_FEATURE_SELECTION_EVENT_PREFIX_WITH_SEPARATOR "pfs."
#define PRESENTATION_FEATURE_SELECTION_EVENT_LEN_OF_PREFIX_WITH_SEPARATOR     4
#define PRESENTATION_FEATURE_SELECTION_EVENT_SEPARATORS_SET     ".:"
#define PRESENTATION_FEATURE_SELECTION_EVENT_CURVALCHANGED_STR  "currentSettingChanged"
#define PRESENTATION_FEATURE_SELECTION_EVENT_FEATURECHANGED_STR "featureChanged"
//  NOTE: if you add another event-type str, above, and it's shorter than any
// of the existing ones, you need to modify the below to use it instead:
#define PRESENTATION_FEATURE_SELECTION_EVENT_TYPE_MIN_LEN       strlen("featureChanged")
// + 1 + 1 is for: + PF name min len + separator len:
#define PRESENTATION_FEATURE_SELECTION_EVENT_MIN_LEN            (PRESENTATION_FEATURE_SELECTION_EVENT_LEN_OF_PREFIX_WITH_SEPARATOR + 1 + 1 + PRESENTATION_FEATURE_SELECTION_EVENT_TYPE_MIN_LEN)

#define PRESENTATION_FEATURE_SELECTION_PF_UNKNOWN                       "_UNKNOWN_PF_"
//  These match up with established SMIL2 global user settings:
#define PRESENTATION_FEATURE_SELECTION_PF_LANGUAGE                      "language"
#define PRESENTATION_FEATURE_SELECTION_PF_CAPTIONS                      "captions"
#define PRESENTATION_FEATURE_SELECTION_PF_SUBTITLES                     "subtitles"
#define PRESENTATION_FEATURE_SELECTION_PF_OVERDUB_OR_CAPTION            "overdub_or_caption"
/*  These are "known" likely PFs that might hook to some global user pref in
    the future:
#define PRESENTATION_FEATURE_SELECTION_PF_BUG                           "bug"
#define PRESENTATION_FEATURE_SELECTION_PF_CURRENTLY_PLAYING_INFO_MODE   "currently_playing_info-mode"
#define PRESENTATION_FEATURE_SELECTION_PF_MAX_CHANNELS                  "max_channels"
#define PRESENTATION_FEATURE_SELECTION_PF_CHANNEL                       "channel"
#define PRESENTATION_FEATURE_SELECTION_PF_PIP_AVAILABLE                 "pip-available"
#define PRESENTATION_FEATURE_SELECTION_PF_PIP_MODE                      "pip-mode"
#define PRESENTATION_FEATURE_SELECTION_PF_PIP_POSITION                  "pip-position"
#define PRESENTATION_FEATURE_SELECTION_PF_PIP_MAX_CHANNELS              "pip-max_channels"
#define PRESENTATION_FEATURE_SELECTION_PF_PIP_CHANNEL                   "pip-channel"
#define PRESENTATION_FEATURE_SELECTION_PF_SAP                           "SAP"
 */

#define PRESENTATION_FEATURE_SELECTION_USE_FIRST_PF_OPTION_AS_DEFAULT   "_USE_FIRST_PF_OPTION_AS_DEFAULT_"

#define PRESENTATION_FEATURE_SELECTION_PF_DEFAULT_UNKNOWN               "(unknown)"
/* XXXEH- Don't use these.  Instead, get the detault (initial) value
   based on:
    1. Application-set user pref (if matching one exists)
    2. Authored default for the PF, e.g., in SMIL <head>...</head>
    3. first option in the options passed in to the first call to
       SetPresentationFeature() of the particular PF:
#define PRESENTATION_FEATURE_SELECTION_PF_LANGUAGE_DEFAULT              "en"
#define PRESENTATION_FEATURE_SELECTION_PF_CAPTIONS_DEFAULT              "off"
#define PRESENTATION_FEATURE_SELECTION_PF_SUBTITLES_DEFAULT             "off"
#define PRESENTATION_FEATURE_SELECTION_PF_OVERDUB_OR_CAPTION_DEFAULT    "caption"
#define PRESENTATION_FEATURE_SELECTION_PF_BUG_DEFAULT                   "off"
#define PRESENTATION_FEATURE_SELECTION_PF_CURRENTLY_PLAYING_INFO_MODE_DEFAULT "off"
#define PRESENTATION_FEATURE_SELECTION_PF_MAX_CHANNELS_DEFAULT          PRESENTATION_FEATURE_SELECTION_PF_DEFAULT_UNKNOWN
#define PRESENTATION_FEATURE_SELECTION_PF_CHANNEL_DEFAULT               "1"
#define PRESENTATION_FEATURE_SELECTION_PF_PIP_AVAILABLE_DEFAULT         "no"
#define PRESENTATION_FEATURE_SELECTION_PF_PIP_MODE_DEFAULT              "off"
#define PRESENTATION_FEATURE_SELECTION_PF_PIP_POSITION_DEFAULT          "upper_right"
#define PRESENTATION_FEATURE_SELECTION_PF_PIP_MAX_CHANNELS_DEFAULT      PRESENTATION_FEATURE_SELECTION_PF_DEFAULT_UNKNOWN
#define PRESENTATION_FEATURE_SELECTION_PF_PIP_CHANNEL_DEFAULT           "1"
#define PRESENTATION_FEATURE_SELECTION_PF_SAP_DEFAULT                   "off"
 END XXXEH- .*/

enum PfChangeEventType 
{
    PF_EVENT_INVALID_EVENT = 0,
    PF_EVENT_PF_ADDED,
    PF_EVENT_PF_FEATURE_CHANGED,
    PF_EVENT_PF_CURRENT_VALUE_CHANGED,
    PF_EVENT_PF_REMOVED,
    //  Always add new events before the following line:
    PF_EVENT_UNKNOWN_EVENT
};



/****************************************************************************
 *
 *  Interface:
 *
 *    IHXPresentationFeatureManager
 *
 *  Purpose:
 *    This provides a way for 
 *  
 *  IID_IHXPresentationFeatureManager:
 * 
 *      {4BD50AF6-D1BA-40f6-BCF3-5F272B7CC7DD}
 *
 */
DEFINE_GUID(IID_IHXPresentationFeatureManager, 0x4bd50af6, 0xd1ba, 0x40f6,
            0xbc, 0xf3, 0x5f, 0x27, 0x2b, 0x7c, 0xc7, 0xdd);

#undef INTERFACE
#define INTERFACE IHXPresentationFeatureManager

DECLARE_INTERFACE_(IHXPresentationFeatureManager, IUnknown)
{
    STDMETHOD (SetPresentationFeature)        (THIS_
                                               const char* /*IN*/ pFeatureName,
                                               IHXBuffer*  /*IN*/ pFeatureInitialValue,
                                               IHXValues*  /*IN*/ pFeatureOptions) PURE;
    STDMETHOD (GetPresentationFeature)        (THIS_ const char* pFeatureName,
                                               REF(IHXBuffer*) /*OUT*/ pFeatureSetting,
                                               REF(IHXValues*) /*OUT*/ pFeatureOptions) PURE;
    STDMETHOD (SetPresentationFeatureValue)   (THIS_
                                               const char* /*IN*/ pFeatureName,
                                               IHXBuffer*  /*IN*/ pRequestedPFCurrentSetting) PURE;
    STDMETHOD (GetPresentationFeatureValue)   (THIS_ const char* /*IN*/ pFeatureName,
                                               REF(IHXBuffer*)  /*OUT*/ pFeatureSetting) PURE;
    STDMETHOD (RemovePresentationFeature)     (THIS_ const char* /*IN*/ pFeatureName) PURE;
    STDMETHOD (GetPresentationFeatures)       (THIS_ REF(IHXValues*) /*OUT*/ pFeatures) PURE;
    STDMETHOD (SetGlobalUserPreference)       (THIS_ const char* /*IN*/ pFeatureName) PURE;
    STDMETHOD (AddPresentationFeatureSink)    (THIS_ IHXPresentationFeatureSink* pSink) PURE;
    STDMETHOD (RemovePresentationFeatureSink) (THIS_ IHXPresentationFeatureSink* pSink) PURE;
    STDMETHOD (ParsePFEvent)                  (THIS_ const char* pszPFEvent,
                                               REF(PfChangeEventType) /*OUT*/ thePfChangeEvent,
                                               REF(IHXBuffer*) /*OUT*/ pFeatureName,
                                               REF(IHXBuffer*) /*OUT*/ pFeatureSetting) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *    IHXPresentationFeatureSink
 *
 *  Purpose:
 *    This provides a way for 
 *  
 *  IID_IHXPresentationFeatureSink:
 * 
 *      {77E41CED-EC8D-47ee-B162-C597C948D27E}
 *
 */
DEFINE_GUID(IID_IHXPresentationFeatureSink, 0x77e41ced, 0xec8d, 0x47ee,
            0xb1, 0x62, 0xc5, 0x97, 0xc9, 0x48, 0xd2, 0x7e);

#undef INTERFACE
#define INTERFACE IHXPresentationFeatureSink

DECLARE_INTERFACE_(IHXPresentationFeatureSink, IUnknown)
{
    STDMETHOD(PresentationFeatureCurrentSettingChanged) (THIS_
                                                       const char* pFeatureName,
                                                       IHXBuffer* pNewValue) PURE;
    STDMETHOD(PresentationFeatureChanged)      (THIS_
                                                       const char* pFeatureName) PURE;
};



#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXPresentationFeatureManager)
DEFINE_SMART_PTR(IHXPresentationFeatureSink)

#endif // _HXPFS_H
