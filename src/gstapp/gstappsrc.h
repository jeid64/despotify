/* GStreamer
 * Copyright (C) 2007 David Schleef <ds@schleef.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _GST_APP_SRC_H_
#define _GST_APP_SRC_H_

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

G_BEGIN_DECLS

#define GST_TYPE_APP_SRC \
  (gst_app_src_get_type())
#define GST_APP_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_APP_SRC,GstAppSrc))
#define GST_APP_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_APP_SRC,GstAppSrcClass))
#define GST_IS_APP_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_APP_SRC))
#define GST_IS_APP_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_APP_SRC))

typedef struct _GstAppSrc GstAppSrc;
typedef struct _GstAppSrcClass GstAppSrcClass;
typedef struct _GstAppSrcPrivate GstAppSrcPrivate;

/**
 * GstAppStreamType:
 * @GST_APP_STREAM_TYPE_STREAM: No seeking is supported in the stream, such as a
 * live stream.
 * @GST_APP_STREAM_TYPE_SEEKABLE: The stream is seekable but seeking might not
 * be very fast, such as data from a webserver. 
 * @GST_APP_STREAM_TYPE_RANDOM_ACCESS: The stream is seekable and seeking is fast,
 * such as in a local file.
 *
 * The stream type.
 */
typedef enum
{
  GST_APP_STREAM_TYPE_STREAM,
  GST_APP_STREAM_TYPE_SEEKABLE,
  GST_APP_STREAM_TYPE_RANDOM_ACCESS
} GstAppStreamType;

struct _GstAppSrc
{
  GstBaseSrc basesrc;

  /*< private >*/
  GstAppSrcPrivate *priv;

  /*< private >*/
  gpointer     _gst_reserved[GST_PADDING];
};

struct _GstAppSrcClass
{
  GstBaseSrcClass basesrc_class;

  /* signals */
  void          (*need_data)       (GstAppSrc *src, guint length);
  void          (*enough_data)     (GstAppSrc *src);
  gboolean      (*seek_data)       (GstAppSrc *src, guint64 offset);

  /* actions */
  GstFlowReturn (*push_buffer)     (GstAppSrc *src, GstBuffer *buffer);
  GstFlowReturn (*end_of_stream)   (GstAppSrc *src);

  /*< private >*/
  gpointer     _gst_reserved[GST_PADDING];
};

GType gst_app_src_get_type(void);

void             gst_app_src_set_caps         (GstAppSrc *appsrc, const GstCaps *caps);
GstCaps*         gst_app_src_get_caps         (GstAppSrc *appsrc);

void             gst_app_src_set_size         (GstAppSrc *appsrc, gint64 size);
gint64           gst_app_src_get_size         (GstAppSrc *appsrc);

void             gst_app_src_set_stream_type  (GstAppSrc *appsrc, GstAppStreamType type);
GstAppStreamType gst_app_src_get_stream_type  (GstAppSrc *appsrc);

void             gst_app_src_set_max_bytes    (GstAppSrc *appsrc, guint64 max);
guint64          gst_app_src_get_max_bytes    (GstAppSrc *appsrc);

void             gst_app_src_set_latency      (GstAppSrc *appsrc, guint64 min, guint64 max);
void             gst_app_src_get_latency      (GstAppSrc *appsrc, guint64 *min, guint64 *max);

GstFlowReturn    gst_app_src_push_buffer      (GstAppSrc *appsrc, GstBuffer *buffer);
GstFlowReturn    gst_app_src_end_of_stream    (GstAppSrc *appsrc);

G_END_DECLS

#endif

