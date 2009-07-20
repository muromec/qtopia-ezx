/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: smiltype.h,v 1.3 2007/07/06 21:58:18 jfinnecy Exp $
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

#ifndef _SMILTYPE_H_
#define _SMILTYPE_H_

typedef enum
{
    PersistentUnknown,
    PersistentRAM,
    PersistentSMIL
} PersistentType;

typedef enum
{
    XMLAttrTypeCDATA,
    XMLAttrTypeID,
    XMLAttrTypeIDREF,
    XMLAttrTypeIDREFS,
    XMLAttrTypeENTITY,
    XMLAttrTypeENTITIES,
    XMLAttrTypeNMTOKEN,
    XMLAttrTypeNMTOKENS,
    XMLAttrTypeEnumerated
} XMLAttributeType;

typedef enum
{
    SMIL2ElemA,
    SMIL2ElemAnchor,
    SMIL2ElemAnimate,
    SMIL2ElemAnimateColor,
    SMIL2ElemAnimateMotion,
    SMIL2ElemAnimation,
    SMIL2ElemArea,
    SMIL2ElemAudio,
    SMIL2ElemBody,
    SMIL2ElemBrush,
    SMIL2ElemCustomAttributes,
    SMIL2ElemCustomTest,
    SMIL2ElemExcl,
    SMIL2ElemHead,
    SMIL2ElemImg,
    SMIL2ElemLayout,
    SMIL2ElemMeta,
    SMIL2ElemMetadata,
    SMIL2ElemPar,
    SMIL2ElemParam,
    SMIL2ElemPrefetch,
    SMIL2ElemPriorityClass,
    SMIL2ElemRef,
    SMIL2ElemRegPoint,
    SMIL2ElemRegion,
    SMIL2ElemRootLayout,
    SMIL2ElemSeq,
    SMIL2ElemSet,
    SMIL2ElemSmil,
    SMIL2ElemSwitch,
    SMIL2ElemText,
    SMIL2ElemTextstream,
    SMIL2ElemTopLayout,
    SMIL2ElemTransition,
    SMIL2ElemVideo,
    SMIL2ElemRNParam,
    SMIL2ElemRNRendererList,
    SMIL2ElemRNRenderer,
    XMLEventsElemListener,
    NumSMIL2Elements // NOTE: THIS SHOULD ALWAYS BE LAST!!!
} SMIL2Element;

typedef enum
{
    SMIL2AttrAbstract,
    SMIL2AttrAccesskey,
    SMIL2AttrAccumulate,
    SMIL2AttrActuate,
    SMIL2AttrAdditive,
    SMIL2AttrAlt,
    SMIL2AttrAttributeName,
    SMIL2AttrAttributeType,
    SMIL2AttrAuthor,
    SMIL2AttrBackground_Color,
    SMIL2AttrBackgroundColor,
    SMIL2AttrBandwidth,
    SMIL2AttrBegin,
    SMIL2AttrBorderColor,
    SMIL2AttrBorderWidth,
    SMIL2AttrBottom,
    SMIL2AttrBy,
    SMIL2AttrCalcMode,
    SMIL2AttrClass,
    SMIL2AttrClip_Begin,
    SMIL2AttrClipBegin,
    SMIL2AttrClip_End,
    SMIL2AttrClipEnd,
    SMIL2AttrClose,
    SMIL2AttrColor,
    SMIL2AttrContent,
    SMIL2AttrCoords,
    SMIL2AttrCopyright,
    SMIL2AttrCustomTest,
    SMIL2AttrDefaultState,
    SMIL2AttrDestinationLevel,
    SMIL2AttrDestinationPlaystate,
    SMIL2AttrDirection,
    SMIL2AttrDur,
    SMIL2AttrEnd,
    SMIL2AttrEndProgress,
    SMIL2AttrEndsync,
    SMIL2AttrErase,
    SMIL2AttrExternal,
    SMIL2AttrFadeColor,
    SMIL2AttrFill,
    SMIL2AttrFillDefault,
    SMIL2AttrFit,
    SMIL2AttrFragment,
    SMIL2AttrFrom,
    SMIL2AttrHeight,
    SMIL2AttrHigher,
    SMIL2AttrHorzRepeat,
    SMIL2AttrHref,
    SMIL2AttrId,
    SMIL2AttrLeft,
    SMIL2AttrLongdesc,
    SMIL2AttrLower,
    SMIL2AttrMax,
    SMIL2AttrMediaRepeat,
    SMIL2AttrMediaSize,
    SMIL2AttrMediaTime,
    SMIL2AttrMin,
    SMIL2AttrName,
    SMIL2AttrNohref,
    SMIL2AttrOpen,
    SMIL2AttrOrigin,
    SMIL2AttrOverride,
    SMIL2AttrPauseDisplay,
    SMIL2AttrPeers,
    SMIL2AttrReadIndex,
    SMIL2AttrRegAlign,
    SMIL2AttrRegion,
    SMIL2AttrRegionName,
    SMIL2AttrRegPoint,
    SMIL2AttrRepeat,
    SMIL2AttrRepeatCount,
    SMIL2AttrRepeatDur,
    SMIL2AttrRestart,
    SMIL2AttrRestartDefault,
    SMIL2AttrRight,
    SMIL2AttrSensitivity,
    SMIL2AttrShape,
    SMIL2AttrShow,
    SMIL2AttrShowBackground,
    SMIL2AttrSkip_Content,
    SMIL2AttrSoundLevel,
    SMIL2AttrSourceLevel,
    SMIL2AttrSourcePlaystate,
    SMIL2AttrSrc,
    SMIL2AttrStartProgress,
    SMIL2AttrSubtype,
    SMIL2AttrSyncBehavior,
    SMIL2AttrSyncBehaviorDefault,
    SMIL2AttrSyncTolerance,
    SMIL2AttrSyncToleranceDefault,
    SMIL2AttrSystem_Bitrate,
    SMIL2AttrSystem_Captions,
    SMIL2AttrSystem_Language,
    SMIL2AttrSystem_Overdub_Or_Caption,
    SMIL2AttrSystem_Required,
    SMIL2AttrSystem_Screen_Depth,
    SMIL2AttrSystem_Screen_Size,
    SMIL2AttrSystemAudioDesc,
    SMIL2AttrSystemBitrate,
    SMIL2AttrSystemCaptions,
    SMIL2AttrSystemComponent,
    SMIL2AttrSystemCPU,
    SMIL2AttrSystemLanguage,
    SMIL2AttrSystemOperatingSystem,
    SMIL2AttrSystemOverdubOrSubtitle,
    SMIL2AttrSystemRequired,
    SMIL2AttrSystemScreenDepth,
    SMIL2AttrSystemScreenSize,
    SMIL2AttrTabindex,
    SMIL2AttrTarget,
    SMIL2AttrTargetElement,
    SMIL2AttrTitle,
    SMIL2AttrTo,
    SMIL2AttrTop,
    SMIL2AttrTransIn,
    SMIL2AttrTransOut,
    SMIL2AttrType,
    SMIL2AttrUid,
    SMIL2AttrValue,
    SMIL2AttrValues,
    SMIL2AttrValuetype,
    SMIL2AttrVertRepeat,
    SMIL2AttrWidth,
    SMIL2AttrXmlBase,
    SMIL2AttrXmlLang,
    SMIL2AttrXmlns,
    SMIL2AttrZ_Index,
    SMIL2AttrRNBackgroundOpacity,
    SMIL2AttrRNChromaKey,
    SMIL2AttrRNChromaKeyOpacity,
    SMIL2AttrRNChromaKeyTolerance,
    SMIL2AttrRNMediaOpacity,
    SMIL2AttrRNOpacity,
    SMIL2AttrRNDelivery,
    SMIL2AttrRNHandledBy,
    SMIL2AttrRNSendTo,
    SMIL2AttrRNContextWindow,
    SMIL2AttrRNSystemComponent,
    SMIL2AttrRNResizeBehavior,
    XMLEventsAttrEvent,
    XMLEventsAttrObserver,
    XMLEventsAttrTarget,
    XMLEventsAttrHandler,
    XMLEventsAttrPhase,
    XMLEventsAttrPropagate,
    XMLEventsAttrDefaultAction,
    SMIL2AttrRNAccessErrorBehavior,
    NumSMIL2Attributes // NOTE: THIS SHOULD ALWAYS BE LAST!!!
} SMIL2Attribute;

typedef enum
{
    SMIL2AttrCollCore,
    SMIL2AttrCollI18N,
    SMIL2AttrCollBasicTiming,
    SMIL2AttrCollTest,
    SMIL2AttrCollTiming,
    SMIL2AttrCollSubregion,
    SMIL2AttrCollMediaElement,
    NumSMIL2AttributeCollections // NOTE: THIS SHOULD ALWAYS BE LAST!!!
} SMIL2AttributeCollection;

typedef enum
{
    AccumulateNone,
    AccumulateSum
} AnimateAccumulate;

typedef enum
{
    ActuateOnRequest,
    ActuateOnLoad
} AnchorActuate;

typedef enum
{
    AdditiveReplace,
    AdditiveSum
} AnimateAdditive;

typedef enum
{
    CalcModeDiscrete,
    CalcModeLinear,
    CalcModePaced
} AnimateCalcMode;

typedef enum
{
    ViewportCloseOnRequest,
    ViewportCloseWhenNotActive
} ViewportClose;

typedef enum
{
    DestinationPlaystatePlay,
    DestinationPlaystatePause,
    DestinationPlaystateStop
} AnchorDestinationPlaystate;

typedef enum
{
    TransitionDirectionForward,
    TransitionDirectionReverse
} TransitionDirection;

typedef enum
{
    EraseWhenDone,
    EraseNever
} EraseType;

typedef enum
{
    FillRemove,
    FillFreeze,
    FillHold,
    FillTransition,
    FillAuto,
    FillDefault
} FillType;

typedef enum
{
    FillDefaultRemove,
    FillDefaultFreeze,
    FillDefaultHold,
    FillDefaultTransition,
    FillDefaultAuto,
    FillDefaultInherit
} FillDefaultType;

typedef enum
{
    FitFill,
    FitHidden,
    FitMeet,
    FitScroll,
    FitSlice
} Fit;

typedef enum
{
    DeliveryClient,
    DeliveryServer
} ParamDelivery;

typedef enum
{
    HigherStop,
    HigherPause
} PriorityClassHigher;

typedef enum
{
    LowerDefer,
    LowerNever
} PriorityClassLower;

typedef enum
{
    MediaRepeatPreserve,
    MediaRepeatStrip
} MediaRepeat;

typedef enum
{
    ViewportOpenOnStart,
    ViewportOpenWhenActive
} ViewportOpen;

typedef enum
{
    OverrideVisible,
    OverrideHidden
} CustomTestOverride;

typedef enum
{
    PauseDisplayDisable,
    PauseDisplayHide,
    PauseDisplayShow
} PriorityClassPauseDisplay;

typedef enum
{
    PeersStop,
    PeersPause,
    PeersDefer,
    PeersNever
} PriorityClassPeers;

typedef enum
{
    RegAlignTopLeft,
    RegAlignTopMid,
    RegAlignTopRight,
    RegAlignMidLeft,
    RegAlignCenter,
    RegAlignMidRight,
    RegAlignBottomLeft,
    RegAlignBottomMid,
    RegAlignBottomRight
} RegAlign;

typedef enum
{
    ShapeRect,
    ShapeCircle,
    ShapePoly,
    ShapeDefault
} AreaShape;

typedef enum
{
    ShowNew,
    ShowPause,
    ShowReplace
} AreaShow;

typedef enum
{
    ShowBackgroundAlways,
    ShowBackgroundWhenActive
} ShowBackground;

typedef enum
{
    SourcePlaystatePlay,
    SourcePlaystatePause,
    SourcePlaystateStop
} AnchorSourcePlaystate;

typedef enum
{
    TransitionSubtypeLeftToRight,
    TransitionSubtypeTopToBottom,
    TransitionSubtypeTopLeft,
    TransitionSubtypeTopRight,
    TransitionSubtypeBottomRight,
    TransitionSubtypeBottomLeft,
    TransitionSubtypeTopCenter,
    TransitionSubtypeRightCenter,
    TransitionSubtypeBottomCenter,
    TransitionSubtypeLeftCenter,
    TransitionSubtypeCornersIn,
    TransitionSubtypeCornersOut,
    TransitionSubtypeVertical,
    TransitionSubtypeHorizontal,
    TransitionSubtypeDiagonalBottomLeft,
    TransitionSubtypeDiagonalTopLeft,
    TransitionSubtypeDoubleBarnDoor,
    TransitionSubtypeDoubleDiamond,
    TransitionSubtypeDown,
    TransitionSubtypeLeft,
    TransitionSubtypeUp,
    TransitionSubtypeRight,
    TransitionSubtypeRectangle,
    TransitionSubtypeDiamond,
    TransitionSubtypeCircle,
    TransitionSubtypeFourPoint,
    TransitionSubtypeFivePoint,
    TransitionSubtypeSixPoint,
    TransitionSubtypeHeart,
    TransitionSubtypeKeyhole,
    TransitionSubtypeClockwiseTwelve,
    TransitionSubtypeClockwiseThree,
    TransitionSubtypeClockwiseSix,
    TransitionSubtypeClockwiseNine,
    TransitionSubtypeTwoBladeVertical,
    TransitionSubtypeTwoBladeHorizontal,
    TransitionSubtypeFourBlade,
    TransitionSubtypeClockwiseTop,
    TransitionSubtypeClockwiseRight,
    TransitionSubtypeClockwiseBottom,
    TransitionSubtypeClockwiseLeft,
    TransitionSubtypeClockwiseTopLeft,
    TransitionSubtypeCounterClockwiseBottomLeft,
    TransitionSubtypeClockwiseBottomRight,
    TransitionSubtypeCounterClockwiseTopRight,
    TransitionSubtypeCenterTop,
    TransitionSubtypeCenterRight,
    TransitionSubtypeFanOutVertical,
    TransitionSubtypeFanOutHorizontal,
    TransitionSubtypeFanInVertical,
    TransitionSubtypeFanInHorizontal,
    TransitionSubtypeParallelVertical,
    TransitionSubtypeParallelHorizontal,
    TransitionSubtypeParallelDiagonal,
    TransitionSubtypeOppositeVertical,
    TransitionSubtypeOppositeHorizontal,
    TransitionSubtypeParallelDiagonalTopLeft,
    TransitionSubtypeParallelDiagonalBottomLeft,
    TransitionSubtypeTop,
    TransitionSubtypeBottom,
    TransitionSubtypeTopLeftHorizontal,
    TransitionSubtypeTopLeftVertical,
    TransitionSubtypeTopLeftDiagonal,
    TransitionSubtypeTopRightDiagonal,
    TransitionSubtypeBottomRightDiagonal,
    TransitionSubtypeBottomLeftDiagonal,
    TransitionSubtypeTopLeftClockwise,
    TransitionSubtypeTopRightClockwise,
    TransitionSubtypeBottomRightClockwise,
    TransitionSubtypeBottomLeftClockwise,
    TransitionSubtypeTopLeftCounterClockwise,
    TransitionSubtypeTopRightCounterClockwise,
    TransitionSubtypeBottomRightCounterClockwise,
    TransitionSubtypeBottomLeftCounterClockwise,
    TransitionSubtypeVerticalTopSame,
    TransitionSubtypeVerticalBottomSame,
    TransitionSubtypeVerticalTopLeftOpposite,
    TransitionSubtypeVerticalBottomLeftOpposite,
    TransitionSubtypeHorizontalLeftSame,
    TransitionSubtypeHorizontalRightSame,
    TransitionSubtypeHorizontalTopLeftOpposite,
    TransitionSubtypeHorizontalTopRightOpposite,
    TransitionSubtypeDiagonalBottomLeftOpposite,
    TransitionSubtypeDiagonalTopLeftOpposite,
    TransitionSubtypeTwoBoxTop,
    TransitionSubtypeTwoBoxBottom,
    TransitionSubtypeTwoBoxLeft,
    TransitionSubtypeTwoBoxRight,
    TransitionSubtypeFourBoxVertical,
    TransitionSubtypeFourBoxHorizontal,
    TransitionSubtypeVerticalLeft,
    TransitionSubtypeVerticalRight,
    TransitionSubtypeHorizontalLeft,
    TransitionSubtypeHorizontalRight,
    TransitionSubtypeFromLeft,
    TransitionSubtypeFromTop,
    TransitionSubtypeFromRight,
    TransitionSubtypeFromBottom,
    TransitionSubtypeCrossfade,
    TransitionSubtypeFadeToColor,
    TransitionSubtypeFadeFromColor
} TransitionSubtype;

typedef enum
{
    SystemCaptionsOn,
    SystemCaptionsOff
} SystemCaptions;

typedef enum
{
    OverdubOrCaptionOverdub,
    OverdubOrCaptionCaption
} SystemOverdubOrCaption;

typedef enum
{
    AudioDescOn,
    AudioDescOff
} SystemAudioDesc;

typedef enum
{
    OverdubOrSubtitleOverdub,
    OverdubOrSubtitleSubtitle
} SystemOverdubOrSubtitle;

typedef enum
{
    TransitionTypeBarWipe,
    TransitionTypeBoxWipe,
    TransitionTypeFourBoxWipe,
    TransitionTypeBarnDoorWipe,
    TransitionTypeDiagonalWipe,
    TransitionTypeBowTieWipe,
    TransitionTypeMiscDiagonalWipe,
    TransitionTypeVeeWipe,
    TransitionTypeBarnVeeWipe,
    TransitionTypeZigZagWipe,
    TransitionTypeBarnZigZagWipe,
    TransitionTypeIrisWipe,
    TransitionTypeTriangleWipe,
    TransitionTypeArrowHeadWipe,
    TransitionTypePentagonWipe,
    TransitionTypeHexagonWipe,
    TransitionTypeEllipseWipe,
    TransitionTypeEyeWipe,
    TransitionTypeRoundRectWipe,
    TransitionTypeStarWipe,
    TransitionTypeMiscShapeWipe,
    TransitionTypeClockWipe,
    TransitionTypePinWheelWipe,
    TransitionTypeSingleSweepWipe,
    TransitionTypeFanWipe,
    TransitionTypeDoubleFanWipe,
    TransitionTypeDoubleSweepWipe,
    TransitionTypeSaloonDoorWipe,
    TransitionTypeWindshieldWipe,
    TransitionTypeSnakeWipe,
    TransitionTypeSpiralWipe,
    TransitionTypeParallelSnakesWipe,
    TransitionTypeBoxSnakesWipe,
    TransitionTypeWaterfallWipe,
    TransitionTypePushWipe,
    TransitionTypeSlideWipe,
    TransitionTypeFade
} TransitionType;

typedef enum
{
    ValuetypeData,
    ValuetypeRef,
    ValuetypeObject
} ParamValuetype;

typedef enum
{
    NamespaceSystemComponent,
    NamespaceSizeControl,
    NamespaceAlphaControl,
    NamespaceParam,
    NamespaceSendTo,
    NamespaceHandledBy,
    NamespaceRendererList,
    NamespaceAccessErrorBehavior,
    NamespaceAllSMIL2Extensions,
    NamespaceSMIL2AccessKeyTiming,
    NamespaceSMIL2AudioLayout,
    NamespaceSMIL2BasicAnimation,
    NamespaceSMIL2BasicContentControl,
    NamespaceSMIL2BasicInlineTiming,
    NamespaceSMIL2BasicLayout,
    NamespaceSMIL2BasicLinking,
    NamespaceSMIL2BasicMedia,
    NamespaceSMIL2BasicTimeContainers,
    NamespaceSMIL2BasicTransistions,
    NamespaceSMIL2BrushMedia,
    NamespaceSMIL2CustomTestAttributes,
    NamespaceSMIL2EventTiming,
    NamespaceSMIL2ExclTimeContainers,
    NamespaceSMIL2FillDefault,
    NamespaceSMIL2HierarchicalLayout,
    NamespaceSMIL2InlineTransitions,
    NamespaceSMIL2LinkingAttributes,
    NamespaceSMIL2MediaAccessibility,
    NamespaceSMIL2MediaClipMarkers,
    NamespaceSMIL2MediaClipping,
    NamespaceSMIL2MediaDescription,
    NamespaceSMIL2MediaMarkerTiming,
    NamespaceSMIL2MediaParam,
    NamespaceSMIL2Metainformation,
    NamespaceSMIL2MinMaxTiming,
    NamespaceSMIL2MultiArcTiming,
    NamespaceSMIL2MultiWindowLayout,
    NamespaceSMIL2ObjectLinking,
    NamespaceSMIL2PrefetchControl,
    NamespaceSMIL2RepeatTiming,
    NamespaceSMIL2RepeatValueTiming,
    NamespaceSMIL2RestartDefault,
    NamespaceSMIL2RestartTiming,
    NamespaceSMIL2SkipContentControl,
    NamespaceSMIL2SplineAnimation,
    NamespaceSMIL2Structure,
    NamespaceSMIL2SyncbaseTiming,
    NamespaceSMIL2SyncBehavior,
    NamespaceSMIL2SyncBehaviorDefault,
    NamespaceSMIL2SyncMaster,
    NamespaceSMIL2TimeContainerAttributes,
    NamespaceSMIL2TimeManipulations,
    NamespaceSMIL2TransitionModifiers,
    NamespaceSMIL2WallclockTiming,
    NamespaceXMLEvents,
    NamespaceNotImplemented // NOTE!! This should always be last
} SupportedNamespace;

typedef enum
{
    ResizeZoom,
    ResizePercentOnly
} ResizeBehavior;

typedef enum
{
    WithinUnknown = 0,
    WithinSeq = 1,
    WithinSeqInPar = 2,
    WithinPar = 3
} ElementWithinTag;

typedef enum
{
    RepeatUnknown,
    RepeatReplica,
    RepeatIndefiniteOnGroup,
    RepeatIndefiniteOnMe
} RepeatTag;

typedef enum
{
    UpdateUnknown,
    UpdateDuration,
    UpdateDelay,
    UpdateAll
} UpdateTag;

typedef enum
{
    SmilBeginTimeList,
    SmilEndTimeList
} SmilTimingListType;

typedef enum
{
    PrefetchUnknown = 0,
    PrefetchTime,
    PrefetchTimePercent,
    PrefetchBytes,
    PrefetchBytesPercent,
    PrefetchBandwidth,
    PrefetchBandwidthPercent
    , PrefetchMaxAllowedPlus1
} PrefetchType;

typedef enum
{
    HandledByRPBrowser,
    HandledByRPContextWin,
    HandledByRPEngine,
    HandledByAuto
} HandledBy;

typedef enum
{
    SendToRPBrowser,
    SendToRPContextWin,
    SendToOSDefaultBrowser,
    SendToRPEngine
} SendTo;

typedef enum
{
    ContextWindowAuto,
    ContextWindowOpenAtStart
} ContextWindow;

typedef enum
{
    PhaseCapture,
    PhaseDefault
} Phase;

typedef enum
{
    PropagateStop,
    PropagateContinue
} Propagate;

typedef enum
{
    DefaultActionCancel,
    DefaultActionPerform
} DefaultAction;

typedef enum
{
    AccessErrorBehaviorInherit,
    AccessErrorBehaviorContinue,
    AccessErrorBehaviorStop
} AccessErrorBehavior;

typedef enum
{
    CSS2TypeAuto,
    CSS2TypeInherit,
    CSS2TypeLength,
    CSS2TypePercentage,
    CSS2TypeInteger,
    CSS2TypeColor,
    CSS2TypeTransparent
} CSS2Type;

typedef struct
{
    double         m_dLeft;
    CSS2Type       m_eLeftType;
    double         m_dTop;
    CSS2Type       m_eTopType;
    double         m_dRight;
    CSS2Type       m_eRightType;
    double         m_dBottom;
    CSS2Type       m_eBottomType;
    double         m_dWidth;
    CSS2Type       m_eWidthType;
    double         m_dHeight;
    CSS2Type       m_eHeightType;
}
LayoutRect;

typedef struct
{
    double   m_dLeft;
    CSS2Type m_eLeftType;
    double   m_dTop;
    CSS2Type m_eTopType;
    double   m_dRight;
    CSS2Type m_eRightType;
    double   m_dBottom;
    CSS2Type m_eBottomType;
    RegAlign m_eRegAlign;
}
RegPoint;

typedef enum
{
      SmilRestartNever
    , SmilRestartWhenNotActive
    , SmilRestartAlways
    , SmilRestartDefault // /Default val for restart
    , SmilRestartInherit // /Default val for restartDefault
}SmilElementRestart;

typedef enum
{
      SmilSyncBehaviorInvalid
    , SmilSyncBehaviorCanSlip
    , SmilSyncBehaviorLocked
    , SmilSyncBehaviorIndependent
    , SmilSyncBehaviorDefault // /Default val for syncBehavior.
    , SmilSyncBehaviorInherit // /Default val for syncBehaviorDefault.
}SMILSyncBehaviorType;

typedef enum
{
      SMILPriorityClassPauseDisplayInvalid
    , SMILPriorityClassPauseDisplayDisable
    , SMILPriorityClassPauseDisplayHide
    , SMILPriorityClassPauseDisplayShow // /Default for pauseDisplay.
}SMILPriorityClassPauseDisplay;

typedef enum
{
      SMILPriorityClassPeersHigherLowerInvalid
    , SMILPriorityClassStop  // /Default for peers.
    , SMILPriorityClassPause // /Default for higher.
    , SMILPriorityClassDefer // /Default for lower.
    , SMILPriorityClassNever
}SMILPriorityClassPeersHigherLowerVal;

typedef enum
{
      SMILPriorityClassPeersHigherLowerAttribInvalid
    , SMILPriorityClassPeers
    , SMILPriorityClassHigher
    , SMILPriorityClassLower
}SMILPriorityClassPeersHigherLowerAttrib;

typedef enum
{
      SMILLinkPlaystateInvalid
    , SMILLinkPlaystatePlay
    , SMILLinkPlaystatePause
    , SMILLinkPlaystateStop
}SMILLinkPlaystate;

typedef enum
{
    SMILUnknown = 0,
    SMILAAnchor,
    SMILAnchor,
    SMILAnimate,
    SMILAnimateColor,
    SMILAnimateMotion,
    SMILAnimation,
    SMILArea,
    SMILAudio,
    SMILBody,
    SMILBrush,
    SMILCustomAttributes,
    SMILCustomTest,
    SMILExcl,
    SMILHead,
    SMILImg,
    SMILBasicLayout,
    SMILMeta,
    SMILMetadata,
    SMILPar,
    SMILParam,
    SMILPrefetch,
    SMILPriorityClass,
    SMILRef,
    SMILRegPoint,
    SMILRegion,
    SMILRootLayout,
    SMILSeq,
    SMILSet,
    SMILSmil,
    SMILSwitch,
    SMILText,
    SMILTextstream,
    SMILViewport,
    SMILTransition,
    SMILVideo,
    SMILEndPar,
    SMILEndSeq,
    SMILEndExcl,
    SMILEndPriorityClass,
    SMILRNRendererList,
    SMILRendererPreFetch,
    SMILEndAAnchor
} SMILNodeTag;

typedef enum
{
    SMILEventSourceNone,
    SMILEventSourceBegin,
    SMILEventSourceEnd,
    SMILEventSourceFirst,
    SMILEventSourceLast,
    SMILEventSourceAll,
    SMILEventSourceID,
    SMILEventSourceClock
} SMILEventSourceTag;

typedef enum
{
    SMILSyncAttrNone,
    SMILSyncAttrBegin,
    SMILSyncAttrEnd,
    SMILSyncAttrDur,
    SMILSyncAttrEndsync,
    SMILSyncAttrClipBegin,
    SMILSyncAttrClipEnd,
    SMILSyncAttrSyncTolerance,
    SMILSyncAttrSyncToleranceDefault,
    SMILSyncAttrSyncBehavior,
    SMILSyncAttrSyncBehaviorDefault,
    SMILSyncAttrMin,
    SMILSyncAttrMax
} SMILSyncAttributeTag;

typedef enum
{
    SMILErrorNone,
    SMILErrorGeneralError,
    SMILErrorBadXML,
    SMILErrorNotSMIL,
    SMILErrorDuplicateID,
    SMILErrorNonexistentID,
    SMILErrorNoBodyTag,
    SMILErrorNoBodyElements,
    SMILErrorUnrecognizedTag,
    SMILErrorUnrecognizedAttribute,
    SMILErrorUnexpectedTag,
    SMILErrorBadDuration,
    SMILErrorBadAttribute,
    SMILErrorBadFragment,
    SMILErrorRequiredAttributeMissing,
    SMILErrorSyncAttributeMissing,
    SMILErrorUnexpectedContent,
    SMILErrorSMIL10Document,
    SMILErrorIndefiniteNotSupported,
    SMILErrorMetaDatatype,
    SMILErrorRootLayoutHeightWidthRequired,
    SMILErrorBadID,
    SMILErrorNoSources,
    SMILErrorBadTimeValue,
    SMILErrorBadWallClockValue,
    SMILErrorTimeValueNotallowed
// XXXMEH - these don't seem to be used, and also
// require a dependency on hxresult.h, xmlreslt.h,
// so try removing them for now
//    SMILXMLUnknownError             = HXR_XML_GENERALERROR,
//    SMILXMLErrorNoClose             = HXR_XML_NOCLOSE,
//    SMILXMLErrorBadAttribute        = HXR_XML_BADATTRIBUTE,
//    SMILXMLErrorNoValue             = HXR_XML_NOVALUE,
//    SMILXMLErrorMissingQuote        = HXR_XML_MISSINGQUOTE,
//    SMILXMLErrorBadEndTag           = HXR_XML_BADENDTAG,
//    SMILXMLErrorNoTagType           = HXR_XML_NOTAGTYPE,
//    SMILXMLErrorIllegalID           = HXR_XML_ILLEGALID
} SMILErrorTag;

#define SMILTIME_PAUSED_INDEFINITELY    ((LONG32)2147483642)  /* 0x7FFFFFFA */
#define SMILTIME_DEFERRED_INDEFINITELY  ((LONG32)2147483645)  /* 0x7FFFFFFD */
 
#define SMILTIME_NEGATIVE_INFINITY      ((LONG32)-2147483647) /* 0x80000001 */
#define SMILTIME_INFINITY               ((LONG32)2147483647)  /* 0x7FFFFFFF */
 
#define SMILTIME_INVALID                ((UINT32)-1)          /* 0xFFFFFFFF */

#endif /* _SMILTYPE_H_ */
