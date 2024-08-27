

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "gstamselements.h"
#include "gstamsh264dec.h"


#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

GST_DEBUG_CATEGORY_STATIC (gst_amsh264dec_debug);
#define GST_CAT_DEFAULT gst_amsh264dec_debug

static gboolean gst_amsh264_dec_open(GstAMSDec *dec);

static gboolean gst_amsh264_dec_close(GstAMSDec *dec);

static gint     gst_amsh264_dec_predecode(GstAMSDec *dec, guint8 *data, guint size);

static gboolean amsh264_element_init (GstPlugin * plugin);


static GstStaticPadTemplate gst_amsh264_dec_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h264, stream-format=(string)byte-stream, alignment=(string)nal, profile = (string) { constrained-baseline, baseline, main, high, high-10 }")
    );

static GstStaticPadTemplate gst_amsh264_dec_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ I420, I420_10LE }"))
    );
//I420_10BE
#define parent_class gst_amsh264_dec_parent_class
G_DEFINE_TYPE (GstAMSH264Dec, gst_amsh264_dec, GST_TYPE_AMS_DEC);
GST_ELEMENT_REGISTER_DEFINE_CUSTOM (amsh264dec, amsh264_element_init);

static void
gst_amsh264_dec_class_init (GstAMSH264DecClass * klass)
{
  GstElementClass *element_class;
  GstAMSDecClass *ams_class;

  element_class = GST_ELEMENT_CLASS (klass);
  ams_class = GST_AMS_DEC_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &gst_amsh264_dec_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_amsh264_dec_src_template);

  gst_element_class_set_static_metadata (element_class,
      "AMSCODEC H264 Decoder", "Decoder/Video",  "H264 video decoder", 
      "lirui <lirui@smart-core.cn>, lirui <lirui-buaa@sohu.com>");

  ams_class->open      = GST_DEBUG_FUNCPTR (gst_amsh264_dec_open);
  ams_class->close     = GST_DEBUG_FUNCPTR (gst_amsh264_dec_close);  
  ams_class->predecode = GST_DEBUG_FUNCPTR (gst_amsh264_dec_predecode);
}

static void
gst_amsh264_dec_init (GstAMSH264Dec * self)
{
  GST_DEBUG_OBJECT (self, "init");
  self->parser = NULL;
}

static gboolean gst_amsh264_dec_open(GstAMSDec *dec) {
  GstAMSH264Dec *self =  GST_AMSH264_DEC(dec);
  ams_codec_param   ams_param;
  int ret = 0;
  GST_DEBUG_OBJECT (self, "open");
  dec->decoder->codec_id = AMS_CODEC_ID_H264;

  ret = ams_decode_create(dec->decoder);
  if (ret < 0) {
      GST_ERROR_OBJECT (dec, "Failed to create ams h264 decode context");
      return FALSE;
  }
    memset(&ams_param, 0, sizeof(ams_codec_param));
    if ((dec->decoder->width > 32) && dec->decoder->height > 32) {
        ams_param.downscale_w = dec->decoder->width;
        ams_param.downscale_h = dec->decoder->height;
    }

    ret = ams_decode_set_parameters(dec->decoder, &ams_param);
    if (ret < 0) {
        GST_ERROR_OBJECT(dec, "Failed to set param for ams h264 decoder\n");
        ams_free_codec_context(dec->decoder);
        dec->decoder = NULL;
        return FALSE;
    }

  self->parser = gst_h264_nal_parser_new ();
  return TRUE;
}

static gboolean gst_amsh264_dec_close(GstAMSDec *dec) {
  GstAMSH264Dec *self =  GST_AMSH264_DEC(dec);
  GST_DEBUG_OBJECT (self, "close");
  if (self->parser != NULL) {
      gst_h264_nal_parser_free(self->parser);
  }
  self->parser = NULL;
  return TRUE;
}

static gint     gst_amsh264_dec_predecode(GstAMSDec *dec, guint8 *data, guint size) {
  GstAMSH264Dec *self =  GST_AMSH264_DEC(dec); 
  GstH264NalUnit nalu;
  GstH264ParserResult pres;
  GST_DEBUG_OBJECT (self, "predecode");
  pres = gst_h264_parser_identify_nalu (self->parser, data, 0, size, &nalu);
  if (pres == GST_H264_PARSER_NO_NAL_END)
     pres = GST_H264_PARSER_OK;

  switch (nalu.type) {
    case GST_H264_NAL_SPS:
         dec->header.size = 0;
    case GST_H264_NAL_PPS:
    case GST_H264_NAL_SEI:
         memcpy(dec->header.data + dec->header.size, data, size);
         dec->header.size += size;
         return -2;
    break;
    case GST_H264_NAL_AU_DELIMITER:
         return -1;
    break;
    case GST_H264_NAL_SLICE_IDR:
         return  0;
    default:
        break;
  }

  if (dec->skipframes == 0) {
      if (nalu.type != GST_H264_NAL_SLICE_IDR) {
          return -3;
      }
  }
  return 1;
}

static gboolean
amsh264_element_init (GstPlugin * plugin)
{
   GST_DEBUG_CATEGORY_INIT (gst_amsh264dec_debug, "amsh264dec", 0, "debug category for AMSCODEC H264 Decoder");
   return gst_element_register (plugin, "amsh264dec", GST_RANK_PRIMARY,  GST_TYPE_AMSH264_DEC);
}

