
#ifndef __GST_AMS_DEC_H__
#define __GST_AMS_DEC_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <gst/gst.h>
#include <gst/video/gstvideodecoder.h>
#include "ams_api_codec.h"
#include "ams_api_decode.h"
#include "ams_api_board.h"


#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif


G_BEGIN_DECLS

#define GST_TYPE_AMS_DEC \
  (gst_ams_dec_get_type())
#define GST_AMS_DEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AMS_DEC,GstAMSDec))
#define GST_AMS_DEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_AMS_DEC,GstAMSDecClass))
#define GST_IS_AMS_DEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AMS_DEC))
#define GST_IS_AMS_DEC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_AMS_DEC))
#define GST_AMS_DEC_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_AMS_DEC, GstAMSDecClass))

typedef struct _GstAMSDec GstAMSDec;
typedef struct _GstAMSDecClass GstAMSDecClass;

struct _GstAMSDec
{
  GstVideoDecoder            base_video_decoder;
  ams_codec_context_t       *decoder;
  GstVideoCodecState        *input_state;
  GstVideoCodecState        *output_state;
  gint                       skipframes;// -1 - all frames; 0 - IDR/I frame pass; > 0 - one frame of every skip frames, and I/IDR frame pass
  struct {
      guint16                capacity;
      guint16                size;
      guint8                *data;
  } header;
};

struct _GstAMSDecClass
{
  GstVideoDecoderClass base_video_decoder_class;
  /*virtual function to open*/
  gboolean (*open)  (GstAMSDec *dec);
  gboolean (*close) (GstAMSDec *dec);
  gint     (*predecode) (GstAMSDec *dec, guint8 *data, guint size);
};

GType gst_ams_dec_get_type (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GstAMSDec, gst_object_unref)

G_END_DECLS


#endif /* __GST_AMS_DEC_H__ */
