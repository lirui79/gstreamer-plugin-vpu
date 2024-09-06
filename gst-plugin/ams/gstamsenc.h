
#ifndef __GST_AMS_ENC_H__
#define __GST_AMS_ENC_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <gst/gst.h>
#include <gst/video/gstvideoencoder.h>
#include "ams_api_codec.h"
#include "ams_api_encode.h"
#include "ams_api_board.h"


#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif


G_BEGIN_DECLS

#define GST_TYPE_AMS_ENC \
  (gst_ams_enc_get_type())
#define GST_AMS_ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AMS_ENC,GstAMSEnc))
#define GST_AMS_ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_AMS_ENC,GstAMSEncClass))
#define GST_IS_AMS_ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AMS_ENC))
#define GST_IS_AMS_ENC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_AMS_ENC))
#define GST_AMS_ENC_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_AMS_ENC, GstAMSEncClass))

typedef struct _GstAMSEnc GstAMSEnc;
typedef struct _GstAMSEncClass GstAMSEncClass;

struct _GstAMSEnc
{
  GstVideoEncoder            base_video_encoder;
  ams_codec_context_t       *encoder;
  GstVideoCodecState        *input_state;

  struct {
      guint8                 state;// 0 - no get header, 1 - get header, 2 - header input IDR frame
      guint16                size;//
      guint8                 data[256];
  } header;
};

struct _GstAMSEncClass
{
  GstVideoEncoderClass       base_video_encoder_class;
    /*virtual function to open*/
  gboolean (*open)  (GstAMSEnc *enc);
  gboolean (*close) (GstAMSEnc *enc);
  GstCaps* (*getcaps) (GstAMSEnc *enc);
};

GType gst_ams_enc_get_type (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GstAMSEnc, gst_object_unref)

G_END_DECLS


#endif /* __GST_AMS_ENC_H__ */
