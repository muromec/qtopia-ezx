/****************************************************************************
**
** This file is part of the Qtopia Opensource Edition Package.
**
** Copyright (C) 2008 Trolltech ASA.
**
** Contact: Qt Extended Information (info@qtextended.org)
**
** This file may be used under the terms of the GNU General Public License
** versions 2.0 as published by the Free Software Foundation and appearing
** in the file LICENSE.GPL included in the packaging of this file.
**
** Please review the following information to ensure GNU General Public
** Licensing requirements will be met:
**     http://www.fsf.org/licensing/licenses/info/GPLv2.html.
**
**
****************************************************************************/

#include <QScreen>
#include <QDebug>

#include "gstreamersinkwidget.h"

#include "gstreamerqtopiavideosink.h"


namespace gstreamer
{

static GstVideoSinkClass*   parentClass;


// {{{ QtopiaVideoSink

/*!
    \class gstreamer::QtopiaVideoSink
    \internal
*/

GstCaps* QtopiaVideoSink::get_caps(GstBaseSink* sink)
{
    GstCaps* caps = gst_caps_copy(gst_pad_get_pad_template_caps(sink->sinkpad));
    if (GST_CAPS_IS_SIMPLE(caps)) {
        GstStructure*   data = gst_caps_get_structure(caps, 0);
        int             bpp = QScreen::instance()->depth() <= 16 ? 16 : 32;

        gst_structure_set(data, "bpp", G_TYPE_INT, bpp, NULL);
    }

    return caps;
}

gboolean QtopiaVideoSink::set_caps(GstBaseSink* sink, GstCaps* caps)
{
    gboolean rc = TRUE;

    if (GST_CAPS_IS_SIMPLE(caps)) {
        GstStructure*       data = gst_caps_get_structure(caps, 0);
        QtopiaVideoSink*    self = G_TYPE_CHECK_INSTANCE_CAST(sink, QtopiaVideoSinkClass::get_type(), QtopiaVideoSink);

        gst_structure_get_int(data, "width", &self->width);
        gst_structure_get_int(data, "height", &self->height);
        gst_structure_get_int(data, "bpp", &self->bpp);
        gst_structure_get_int(data, "depth", &self->depth);

        self->widget->setVideoSize(self->width, self->height);
    }
    else
        rc = FALSE;

    return rc;
}

GstStateChangeReturn QtopiaVideoSink::change_state(GstElement* element, GstStateChange transition)
{
    return GST_ELEMENT_CLASS(parentClass)->change_state(element, transition);
}

GstFlowReturn QtopiaVideoSink::render(GstBaseSink* sink, GstBuffer* buf)
{
    GstFlowReturn   rc = GST_FLOW_OK;

    if (buf != 0)
    {
        QtopiaVideoSink*    self = G_TYPE_CHECK_INSTANCE_CAST(sink, QtopiaVideoSinkClass::get_type(), QtopiaVideoSink);

        self->widget->paint(QImage(GST_BUFFER_DATA(buf),
                                   self->width,
                                   self->height,
                                   self->bpp == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32));
    }
    else
        rc = GST_FLOW_ERROR;

    return rc;
}

static GstStaticPadTemplate template_factory =
    GST_STATIC_PAD_TEMPLATE("sink",
                            GST_PAD_SINK,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("video/x-raw-rgb, "
                                            "framerate = (fraction) [ 0, MAX ], "
                                            "width = (int) [ 1, MAX ], "
                                            "height = (int) [ 1, MAX ]"));

void QtopiaVideoSink::base_init(gpointer g_class)
{
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(g_class),
                                       gst_static_pad_template_get(&template_factory));
}


void QtopiaVideoSink::instance_init(GTypeInstance *instance, gpointer g_class)
{
    Q_UNUSED(g_class);

    QtopiaVideoSink* self = reinterpret_cast<QtopiaVideoSink*>(instance);

    self->widget = 0;
    self->width = 0;
    self->height = 0;
    self->bpp = 0;
    self->depth = 0;
}
// }}}

// {{{ QtopiaVideoSinkClass
void QtopiaVideoSinkClass::class_init(gpointer g_class, gpointer class_data)
{
    Q_UNUSED(class_data);

    GstBaseSinkClass*   gstBaseSinkClass = (GstBaseSinkClass*)g_class;
    GstElementClass*    gstElementClass = (GstElementClass*)g_class;

    parentClass = reinterpret_cast<GstVideoSinkClass*>(g_type_class_peek_parent(g_class));

    // base
    gstBaseSinkClass->get_caps = QtopiaVideoSink::get_caps;
    gstBaseSinkClass->set_caps = QtopiaVideoSink::set_caps;
//    gstbasesink_class->buffer_alloc = buffer_alloc;
//    gstbasesink_class->get_times = get_times;
    gstBaseSinkClass->preroll = QtopiaVideoSink::render;
    gstBaseSinkClass->render = QtopiaVideoSink::render;

    // element
    gstElementClass->change_state = QtopiaVideoSink::change_state;
}

GType QtopiaVideoSinkClass::get_type()
{
    static GType type = 0;

    if (type == 0)
    {
        static const GTypeInfo info =
        {
            sizeof(QtopiaVideoSinkClass),                       // class_size
            QtopiaVideoSink::base_init,                         // base_init
            NULL,                                               // base_finalize

            QtopiaVideoSinkClass::class_init,                   // class_init
            NULL,                                               // class_finalize
            NULL,                                               // class_data

            sizeof(QtopiaVideoSink),                            // instance_size
            0,                                                  // n_preallocs
            QtopiaVideoSink::instance_init,                     // instance_init
            0                                                   // value_table
        };

        type = g_type_register_static(GST_TYPE_VIDEO_SINK,
                                     "QtopiaVideoSink",
                                      &info,
                                      GTypeFlags(0));
    }

    return type;
}
// }}}

}   // ns gstreamer
