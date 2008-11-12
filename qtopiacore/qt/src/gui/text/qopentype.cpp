/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qdebug.h"
#include "qopentype_p.h"
#include "qfontengine_ft_p.h"
#if defined(Q_WS_QWS) && !defined(QT_NO_QWS_QPF2)
#include "qfontengine_qpf_p.h"
#endif
#include "qscriptengine_p.h"

#ifndef QT_NO_OPENTYPE
//  --------------------------------------------------------------------------------------------------------------------
// Open type support
//  --------------------------------------------------------------------------------------------------------------------

//#define OT_DEBUG

static inline char *tag_to_string(FT_ULong tag)
{
    static char string[5];
    string[0] = (tag >> 24)&0xff;
    string[1] = (tag >> 16)&0xff;
    string[2] = (tag >> 8)&0xff;
    string[3] = tag&0xff;
    string[4] = 0;
    return string;
}
#ifdef OT_DEBUG
static void dump_string(HB_Buffer buffer)
{
    for (uint i = 0; i < buffer->in_length; ++i) {
        qDebug("    %x: cluster=%d", buffer->in_string[i].gindex, buffer->in_string[i].cluster);
    }
}
#define DEBUG qDebug
#else
#define DEBUG if (1) ; else qDebug
#endif

#define DefaultLangSys 0xffff
#define DefaultScript FT_MAKE_TAG('D', 'F', 'L', 'T')

enum {
    RequiresGsub = 1,
    RequiresGpos = 2
};

struct OTScripts {
    unsigned int tag;
    int flags;
};
static const OTScripts ot_scripts [] = {
    // Common
    { FT_MAKE_TAG('l', 'a', 't', 'n'), 0 },
    // Greek
    { FT_MAKE_TAG('g', 'r', 'e', 'k'), 0 },
    // Cyrillic
    { FT_MAKE_TAG('c', 'y', 'r', 'l'), 0 },
    // Armenian
    { FT_MAKE_TAG('a', 'r', 'm', 'n'), 0 },
    // Hebrew
    { FT_MAKE_TAG('h', 'e', 'b', 'r'), 1 },
    // Arabic
    { FT_MAKE_TAG('a', 'r', 'a', 'b'), 1 },
    // Syriac
    { FT_MAKE_TAG('s', 'y', 'r', 'c'), 1 },
    // Thaana
    { FT_MAKE_TAG('t', 'h', 'a', 'a'), 1 },
    // Devanagari
    { FT_MAKE_TAG('d', 'e', 'v', 'a'), 1 },
    // Bengali
    { FT_MAKE_TAG('b', 'e', 'n', 'g'), 1 },
    // Gurmukhi
    { FT_MAKE_TAG('g', 'u', 'r', 'u'), 1 },
    // Gujarati
    { FT_MAKE_TAG('g', 'u', 'j', 'r'), 1 },
    // Oriya
    { FT_MAKE_TAG('o', 'r', 'y', 'a'), 1 },
    // Tamil
    { FT_MAKE_TAG('t', 'a', 'm', 'l'), 1 },
    // Telugu
    { FT_MAKE_TAG('t', 'e', 'l', 'u'), 1 },
    // Kannada
    { FT_MAKE_TAG('k', 'n', 'd', 'a'), 1 },
    // Malayalam
    { FT_MAKE_TAG('m', 'l', 'y', 'm'), 1 },
    // Sinhala
    { FT_MAKE_TAG('s', 'i', 'n', 'h'), 1 },
    // Thai
    { FT_MAKE_TAG('t', 'h', 'a', 'i'), 1 },
    // Lao
    { FT_MAKE_TAG('l', 'a', 'o', ' '), 1 },
    // Tibetan
    { FT_MAKE_TAG('t', 'i', 'b', 't'), 1 },
    // Myanmar
    { FT_MAKE_TAG('m', 'y', 'm', 'r'), 1 },
    // Georgian
    { FT_MAKE_TAG('g', 'e', 'o', 'r'), 0 },
    // Hangul
    { FT_MAKE_TAG('h', 'a', 'n', 'g'), 1 },
    // Ogham
    { FT_MAKE_TAG('o', 'g', 'a', 'm'), 0 },
    // Runic
    { FT_MAKE_TAG('r', 'u', 'n', 'r'), 0 },
    // Khmer
    { FT_MAKE_TAG('k', 'h', 'm', 'r'), 1 }
};
enum { NumOTScripts = sizeof(ot_scripts)/sizeof(OTScripts) };

QOpenType::QOpenType(QFontEngine *fe, FT_Face _face)
    : fontEngine(fe), face(_face), gdef(0), gsub(0), gpos(0), current_script(0xffffffff)
    , allocated(0)
{
    Q_ASSERT(NumOTScripts == (int)QUnicodeTables::ScriptCount);

    hb_buffer_new(face->memory, &hb_buffer);
    tmpAttributes = 0;
    tmpLogClusters = 0;

    kerning_feature_selected = false;
    glyphs_substituted = false;

    FT_Error error;
    if ((error = HB_Load_GDEF_Table(face, &gdef))) {
        DEBUG("error loading gdef table: %d", error);
        gdef = 0;
    }

    DEBUG() << "trying to load gsub table";
    if ((error = HB_Load_GSUB_Table(face, &gsub, gdef))) {
        gsub = 0;
        if (error != FT_Err_Table_Missing) {
            DEBUG("error loading gsub table: %d", error);
        } else {
            DEBUG("face doesn't have a gsub table");
        }
    }

    if ((error = HB_Load_GPOS_Table(face, &gpos, gdef))) {
        gpos = 0;
        DEBUG("error loading gpos table: %d", error);
    }

    for (uint i = 0; i < QUnicodeTables::ScriptCount; ++i)
        supported_scripts[i] = checkScript(i);
}

QOpenType::~QOpenType()
{
    if (gpos)
        HB_Done_GPOS_Table(gpos);
    if (gsub)
        HB_Done_GSUB_Table(gsub);
    if (gdef)
        HB_Done_GDEF_Table(gdef);
    if (hb_buffer)
        hb_buffer_free(hb_buffer);
    if (tmpAttributes)
        free(tmpAttributes);
    if (tmpLogClusters)
        free(tmpLogClusters);
}

bool QOpenType::checkScript(unsigned int script)
{
    Q_ASSERT(script < QUnicodeTables::ScriptCount);

    uint tag = ot_scripts[script].tag;
    int requirements = ot_scripts[script].flags;

    if (requirements & RequiresGsub) {
        if (!gsub)
            return false;

        FT_UShort script_index;
        FT_Error error = HB_GSUB_Select_Script(gsub, tag, &script_index);
        if (error) {
            DEBUG("could not select script %d in GSub table: %d", (int)script, error);
            error = HB_GSUB_Select_Script(gsub, FT_MAKE_TAG('D', 'F', 'L', 'T'), &script_index);
            if (error)
                return false;
        }
    }

    if (requirements & RequiresGpos) {
        if (!gpos)
            return false;

        FT_UShort script_index;
        FT_Error error = HB_GPOS_Select_Script(gpos, script, &script_index);
        if (error) {
            DEBUG("could not select script in gpos table: %d", error);
            error = HB_GPOS_Select_Script(gpos, FT_MAKE_TAG('D', 'F', 'L', 'T'), &script_index);
            if (error)
                return false;
        }

    }
    return true;
}


void QOpenType::selectScript(QShaperItem *item, unsigned int script, const Features *features)
{
    if (current_script == script && kerning_feature_selected == item->kerning_enabled)
        return;

    has_features = false;
    Q_ASSERT(script < QUnicodeTables::ScriptCount);
    // find script in our list of supported scripts.
    uint tag = ot_scripts[script].tag;

    if (gsub && features) {
#ifdef OT_DEBUG
        {
            HB_FeatureList featurelist = gsub->FeatureList;
            int numfeatures = featurelist.FeatureCount;
            DEBUG("gsub table has %d features", numfeatures);
            for (int i = 0; i < numfeatures; i++) {
                HB_FeatureRecord *r = featurelist.FeatureRecord + i;
                DEBUG("   feature '%s'", tag_to_string(r->FeatureTag));
            }
        }
#endif
        HB_GSUB_Clear_Features(gsub);
        FT_UShort script_index;
        FT_Error error = HB_GSUB_Select_Script(gsub, tag, &script_index);
        if (!error) {
            DEBUG("script %s has script index %d", tag_to_string(script), script_index);
            while (features->tag) {
                FT_UShort feature_index;
                error = HB_GSUB_Select_Feature(gsub, features->tag, script_index, 0xffff, &feature_index);
                if (!error) {
                    DEBUG("  adding feature %s", tag_to_string(features->tag));
                    HB_GSUB_Add_Feature(gsub, feature_index, features->property);
                    has_features = true;
                }
                ++features;
            }
        }
    }

    // reset
    kerning_feature_selected = false;

    if (gpos) {
        HB_GPOS_Clear_Features(gpos);
        FT_UShort script_index;
        FT_Error error = HB_GPOS_Select_Script(gpos, tag, &script_index);
        if (!error) {
#ifdef OT_DEBUG
            {
                HB_FeatureList featurelist = gpos->FeatureList;
                int numfeatures = featurelist.FeatureCount;
                DEBUG("gpos table has %d features", numfeatures);
                for(int i = 0; i < numfeatures; i++) {
                    HB_FeatureRecord *r = featurelist.FeatureRecord + i;
                    FT_UShort feature_index;
                    HB_GPOS_Select_Feature(gpos, r->FeatureTag, script_index, 0xffff, &feature_index);
                    DEBUG("   feature '%s'", tag_to_string(r->FeatureTag));
                }
            }
#endif
            FT_ULong *feature_tag_list_buffer;
            error = HB_GPOS_Query_Features(gpos, script_index, 0xffff, &feature_tag_list_buffer);
            if (!error) {
                FT_ULong *feature_tag_list = feature_tag_list_buffer;
                while (*feature_tag_list) {
                    FT_UShort feature_index;
                    if (*feature_tag_list == FT_MAKE_TAG('k', 'e', 'r', 'n')) {
                        if (!item->kerning_enabled) {
                            ++feature_tag_list;
                            continue;
                        }
                        kerning_feature_selected = true;
                    }
                    error = HB_GPOS_Select_Feature(gpos, *feature_tag_list, script_index, 0xffff, &feature_index);
                    if (!error) {
                        HB_GPOS_Add_Feature(gpos, feature_index, PositioningProperties);
                        has_features = true;
                    }
                    ++feature_tag_list;
                }
                FT_Memory memory = gpos->memory;
                FREE(feature_tag_list_buffer);
            }
        }
    }

    current_script = script;
}


extern void qt_heuristicPosition(QShaperItem *item);

bool QOpenType::shape(QShaperItem *item, const unsigned int *properties)
{
    if (!has_features)
        return true;

    length = item->num_glyphs;

    hb_buffer_clear(hb_buffer);

    if (allocated < length) {
        tmpAttributes = (QGlyphLayout::Attributes *) realloc(tmpAttributes, length*sizeof(QGlyphLayout::Attributes));
        tmpLogClusters = (unsigned int *) realloc(tmpLogClusters, length*sizeof(unsigned int));
        allocated = length;
    }
    for (int i = 0; i < length; ++i) {
        hb_buffer_add_glyph(hb_buffer, item->glyphs[i].glyph, properties ? properties[i] : 0, i);
        tmpAttributes[i] = item->glyphs[i].attributes;
        tmpLogClusters[i] = item->log_clusters[i];
    }

#ifdef OT_DEBUG
    DEBUG("-----------------------------------------");
//     DEBUG("log clusters before shaping:");
//     for (int j = 0; j < length; j++)
//         DEBUG("    log[%d] = %d", j, item->log_clusters[j]);
    DEBUG("original glyphs: %p", item->glyphs);
    for (int i = 0; i < length; ++i)
        DEBUG("   glyph=%4x", hb_buffer->in_string[i].gindex);
//     dump_string(hb_buffer);
#endif

    // ### FT_LOAD_NO_HINTING might give problems here, see comment about MingLiu in qfontengine_ft.cpp
    loadFlags = item->flags & QTextEngine::DesignMetrics ? FT_LOAD_NO_HINTING : FT_LOAD_DEFAULT;

    glyphs_substituted = false;
    if (gsub) {
        uint error = HB_GSUB_Apply_String(gsub, hb_buffer);
        if (error && error != HB_Err_Not_Covered)
            return false;
        glyphs_substituted = (error != HB_Err_Not_Covered);
    }

#ifdef OT_DEBUG
//     DEBUG("log clusters before shaping:");
//     for (int j = 0; j < length; j++)
//         DEBUG("    log[%d] = %d", j, item->log_clusters[j]);
    DEBUG("shaped glyphs:");
    for (int i = 0; i < length; ++i)
        DEBUG("   glyph=%4x", hb_buffer->in_string[i].gindex);
    DEBUG("-----------------------------------------");
//     dump_string(hb_buffer);
#endif

    return true;
}

bool QOpenType::positionAndAdd(QShaperItem *item, int availableGlyphs, bool doLogClusters)
{
    if (!has_features)
        return true;

    bool glyphs_positioned = false;
    if (gpos) {
        switch (fontEngine->type()) {
#ifndef QT_NO_FREETYPE
        case QFontEngine::Freetype:
            face = static_cast<QFontEngineFT *>(fontEngine)->lockFace();
            break;
#endif
#if defined(Q_WS_QWS) && !defined(QT_NO_QWS_QPF2) && !defined(QT_NO_FREETYPE)
        case QFontEngine::QPF2:
            face = static_cast<QFontEngineQPF *>(fontEngine)->lockFace();
            break;
#endif
        default:
            Q_ASSERT(false);
        }
        memset(hb_buffer->positions, 0, hb_buffer->in_length*sizeof(HB_PositionRec));
        // #### check that passing "false,false" is correct
        glyphs_positioned = HB_GPOS_Apply_String(face, gpos, loadFlags, hb_buffer, false, false) != HB_Err_Not_Covered;
        switch (fontEngine->type()) {
        case QFontEngine::Freetype:
#ifndef QT_NO_FREETYPE
            static_cast<QFontEngineFT *>(fontEngine)->unlockFace();
            break;
#endif
#if defined(Q_WS_QWS) && !defined(QT_NO_QWS_QPF2) && !defined(QT_NO_FREETYPE)
        case QFontEngine::QPF2:
            static_cast<QFontEngineQPF *>(fontEngine)->unlockFace();
            break;
#endif
        default:
            break;
        }
    }

    if (!glyphs_substituted && !glyphs_positioned)
        return true; // nothing to do for us

    // make sure we have enough space to write everything back
    if (availableGlyphs < (int)hb_buffer->in_length) {
        item->num_glyphs = hb_buffer->in_length;
        return false;
    }

    QGlyphLayout *glyphs = item->glyphs;

    for (unsigned int i = 0; i < hb_buffer->in_length; ++i) {
        glyphs[i].glyph = hb_buffer->in_string[i].gindex;
        glyphs[i].attributes = tmpAttributes[hb_buffer->in_string[i].cluster];
        if (i && hb_buffer->in_string[i].cluster == hb_buffer->in_string[i-1].cluster)
            glyphs[i].attributes.clusterStart = false;
    }
    item->num_glyphs = hb_buffer->in_length;

    if (doLogClusters) {
        // we can't do this for indic, as we pass the stuf in syllables and it's easier to do it in the shaper.
        unsigned short *logClusters = item->log_clusters;
        int clusterStart = 0;
        int oldCi = 0;
        for (unsigned int i = 0; i < hb_buffer->in_length; ++i) {
            int ci = hb_buffer->in_string[i].cluster;
            //         DEBUG("   ci[%d] = %d mark=%d, cmb=%d, cs=%d",
            //                i, ci, glyphAttributes[i].mark, glyphAttributes[i].combiningClass, glyphAttributes[i].clusterStart);
            if (!glyphs[i].attributes.mark && glyphs[i].attributes.clusterStart && ci != oldCi) {
                for (int j = oldCi; j < ci; j++)
                    logClusters[j] = clusterStart;
                clusterStart = i;
                oldCi = ci;
            }
        }
        for (int j = oldCi; j < length; j++)
            logClusters[j] = clusterStart;
    }

    // calulate the advances for the shaped glyphs
//     DEBUG("unpositioned: ");
    if (glyphs_substituted)
        item->font->recalcAdvances(item->num_glyphs, glyphs, QFlag(item->flags));

    // positioning code:
    if (gpos && glyphs_positioned) {
        HB_Position positions = hb_buffer->positions;

//         DEBUG("positioned glyphs:");
        for (unsigned int i = 0; i < hb_buffer->in_length; i++) {
//             DEBUG("    %d:\t orig advance: (%d/%d)\tadv=(%d/%d)\tpos=(%d/%d)\tback=%d\tnew_advance=%d", i,
//                    glyphs[i].advance.x.toInt(), glyphs[i].advance.y.toInt(),
//                    (int)(positions[i].x_advance >> 6), (int)(positions[i].y_advance >> 6),
//                    (int)(positions[i].x_pos >> 6), (int)(positions[i].y_pos >> 6),
//                    positions[i].back, positions[i].new_advance);
            // ###### fix the case where we have y advances. How do we handle this in Uniscribe?????
            QFixed xValue = QFixed::fromFixed(item->flags & QTextEngine::RightToLeft
                                              ? -positions[i].x_advance : positions[i].x_advance);
            QFixed yValue = QFixed::fromFixed(-positions[i].y_advance);
            if (!(item->flags & QTextEngine::DesignMetrics)) {
                xValue = xValue.round();
                yValue = yValue.round();
            }

            if (positions[i].new_advance) {
                glyphs[i].advance.x = xValue;
                glyphs[i].advance.y = yValue;
            } else {
                glyphs[i].advance.x += xValue;
                glyphs[i].advance.y += yValue;
            }

            int back = 0;
            glyphs[i].offset.x = QFixed::fromFixed(positions[i].x_pos);
            glyphs[i].offset.y = QFixed::fromFixed(positions[i].y_pos);
            while (positions[i - back].back) {
                back += positions[i - back].back;
                glyphs[i].offset.x += QFixed::fromFixed(positions[i - back].x_pos);
                glyphs[i].offset.y += QFixed::fromFixed(positions[i - back].y_pos);
            }
            glyphs[i].offset.y = -glyphs[i].offset.y;

            if (item->flags & QTextEngine::RightToLeft) {
                // ### may need to go back multiple glyphs like in ltr
                back = positions[i].back;
                while (back--) {
                    glyphs[i].offset.x -= glyphs[i-back].advance.x;
                    glyphs[i].offset.y -= -glyphs[i-back].advance.y;
                }
            } else {
                back = 0;
                while (positions[i - back].back) {
                    back += positions[i - back].back;
                    glyphs[i].offset.x -= glyphs[i-back].advance.x;
                    glyphs[i].offset.y -= -glyphs[i-back].advance.y;
                }
            }
//             DEBUG("   ->\tadv=%d\tpos=(%d/%d)",
//                    glyphs[i].advance.x.toInt(), glyphs[i].offset.x.toInt(), glyphs[i].offset.y.toInt());
        }
        item->kerning_applied = kerning_feature_selected;
    } else {
        qt_heuristicPosition(item);
    }

#ifdef OT_DEBUG
    if (doLogClusters) {
        DEBUG("log clusters after shaping:");
        for (int j = 0; j < length; j++)
            DEBUG("    log[%d] = %d", j, item->log_clusters[j]);
    }
    DEBUG("final glyphs:");
    for (int i = 0; i < (int)hb_buffer->in_length; ++i)
        DEBUG("   glyph=%4x char_index=%d mark: %d cmp: %d, clusterStart: %d advance=%d/%d offset=%d/%d",
               glyphs[i].glyph, hb_buffer->in_string[i].cluster, glyphs[i].attributes.mark,
               glyphs[i].attributes.combiningClass, glyphs[i].attributes.clusterStart,
               glyphs[i].advance.x.toInt(), glyphs[i].advance.y.toInt(),
               glyphs[i].offset.x.toInt(), glyphs[i].offset.y.toInt());
    DEBUG("-----------------------------------------");
#endif
    return true;
}

#endif // QT_NO_FREETYPE
