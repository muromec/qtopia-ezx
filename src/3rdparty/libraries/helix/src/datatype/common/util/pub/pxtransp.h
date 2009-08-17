#ifndef PXTRANSP_H
#define PXTRANSP_H

// Forward declarations
typedef _INTERFACE IHXValues IHXValues;

HX_RESULT ParseTransparencyParameters(IHXValues* pValues,
                                      REF(UINT32) rulBgOpacity,
                                      REF(HXBOOL)   rbBgOpacitySpecified,
                                      REF(UINT32) rulMediaOpacity,
                                      REF(HXBOOL)   rbMediaOpacitySpecified,
                                      REF(UINT32) rulChromaKey,
                                      REF(HXBOOL)   rbChromaKeySpecified,
                                      REF(UINT32) rulChromaKeyTolerance,
                                      REF(UINT32) rulChromaKeyOpacity,
                                      REF(HXBOOL)   rbAlphaChannelNeeded);
HXBOOL DoesChromaKeyChannelMatch(UINT32 ulColor, UINT32 ulChromaKey, UINT32 ulChromaKeyTol);
HXBOOL DoesChromaKeyMatch(UINT32 ulColor, UINT32 ulChromaKey, UINT32 ulChromaKeyTol);

#define ARGB32_ALPHA(a) (((a) & 0xFF000000) >> 24)
#define ARGB32_RED(a)   (((a) & 0x00FF0000) >> 16)
#define ARGB32_GREEN(a) (((a) & 0x0000FF00) >>  8)
#define ARGB32_BLUE(a)   ((a) & 0x000000FF)

#endif
