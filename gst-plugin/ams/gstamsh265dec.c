

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "gstamselements.h"
#include "gstamsh265dec.h"


#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

GST_DEBUG_CATEGORY_STATIC (gst_amsh265dec_debug);
#define GST_CAT_DEFAULT gst_amsh265dec_debug

static gboolean gst_amsh265_dec_open(GstAMSDec *dec);

static gboolean gst_amsh265_dec_close(GstAMSDec *dec);

static gint     gst_amsh265_dec_predecode(GstAMSDec *dec, guint8 *data, guint size);

static gboolean amsh265dec_element_init (GstPlugin * plugin);


static GstStaticPadTemplate gst_amsh265_dec_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h265, stream-format=(string)byte-stream, alignment=(string)nal, profile = (string) { constrained-baseline, baseline, main, high, high-10 }")
    );

static GstStaticPadTemplate gst_amsh265_dec_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ I420, I420_10LE }"))
    );
//I420_10BE
#define parent_class gst_amsh265_dec_parent_class
G_DEFINE_TYPE (GstAMSH265Dec, gst_amsh265_dec, GST_TYPE_AMS_DEC);
GST_ELEMENT_REGISTER_DEFINE_CUSTOM (amsh265dec, amsh265dec_element_init);

static void
gst_amsh265_dec_class_init (GstAMSH265DecClass * klass)
{
  GstElementClass *element_class;
  GstAMSDecClass *ams_class;
  GST_AMS_LOG("init");

  element_class = GST_ELEMENT_CLASS (klass);
  ams_class = GST_AMS_DEC_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &gst_amsh265_dec_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_amsh265_dec_src_template);

  gst_element_class_set_static_metadata (element_class,
      "AMSCODEC H265 Decoder", "Decoder/Video",  "H265 video decoder", 
      "lirui <lirui@smart-core.cn>, lirui <lirui-buaa@sohu.com>");

  ams_class->open      = GST_DEBUG_FUNCPTR (gst_amsh265_dec_open);
  ams_class->close     = GST_DEBUG_FUNCPTR (gst_amsh265_dec_close);  
  ams_class->predecode = GST_DEBUG_FUNCPTR (gst_amsh265_dec_predecode);
}

static void
gst_amsh265_dec_init (GstAMSH265Dec * self)
{
  GST_DEBUG_OBJECT (self, "init");
  GST_AMS_LOG("init");
  self->parser = NULL;
}

static gboolean gst_amsh265_dec_open(GstAMSDec *dec) {
  GstAMSH265Dec *self =  GST_AMSH265_DEC(dec);
  ams_codec_param   ams_param;
  int ret = 0;
  GST_DEBUG_OBJECT (self, "open");
  dec->decoder->codec_id = AMS_CODEC_ID_H265;

  ret = ams_decode_create(dec->decoder);
  if (ret < 0) {
      GST_ERROR_OBJECT (dec, "Failed to create ams h265 decode context");
      return FALSE;
  }
    memset(&ams_param, 0, sizeof(ams_codec_param));
    if ((dec->decoder->width > 32) && dec->decoder->height > 32) {
        ams_param.downscale_w = dec->decoder->width;
        ams_param.downscale_h = dec->decoder->height;
    }

    ret = ams_decode_set_parameters(dec->decoder, &ams_param);
    if (ret < 0) {
        GST_ERROR_OBJECT(dec, "Failed to set param for ams h265 decoder\n");
        ams_free_codec_context(dec->decoder);
        dec->decoder = NULL;
        return FALSE;
    }

  self->parser = gst_h265_parser_new();
  return TRUE;
}

static gboolean gst_amsh265_dec_close(GstAMSDec *dec) {
  GstAMSH265Dec *self =  GST_AMSH265_DEC(dec);
  GST_DEBUG_OBJECT (self, "close");
  if (self->parser != NULL) {
      gst_h265_parser_free(self->parser);
  }
  self->parser = NULL;
  return TRUE;
}

static gint     gst_amsh265_dec_predecode(GstAMSDec *dec, guint8 *data, guint size) {
  GstAMSH265Dec *self =  GST_AMSH265_DEC(dec); 
  GstH265NalUnit nalu;
//  GstH265SliceHdr slice;
  GstH265ParserResult pres;
  GST_DEBUG_OBJECT (self, "predecode");
  pres = gst_h265_parser_identify_nalu (self->parser, data, 0, size, &nalu);
  if (pres == GST_H265_PARSER_NO_NAL_END)
     pres = GST_H265_PARSER_OK;

  if (pres != GST_H265_PARSER_OK) {
      return -1;
  }

  switch (nalu.type) {
    case GST_H265_NAL_SLICE_TRAIL_N:// = 0,
    case GST_H265_NAL_SLICE_TRAIL_R:// = 1,
    case GST_H265_NAL_SLICE_TSA_N:// = 2,
    case GST_H265_NAL_SLICE_TSA_R:// = 3,
    case GST_H265_NAL_SLICE_STSA_N:// = 4,
    case GST_H265_NAL_SLICE_STSA_R:// = 5,
    case GST_H265_NAL_SLICE_RADL_N:// = 6,
    case GST_H265_NAL_SLICE_RADL_R:// = 7,
    case GST_H265_NAL_SLICE_RASL_N:// = 8,
    case GST_H265_NAL_SLICE_RASL_R:// = 9,
         break;
    case GST_H265_NAL_SLICE_BLA_W_LP:// = 16,
    case GST_H265_NAL_SLICE_BLA_W_RADL:// = 17,
    case GST_H265_NAL_SLICE_BLA_N_LP:// = 18,
    case GST_H265_NAL_SLICE_IDR_W_RADL:// = 19,
    case GST_H265_NAL_SLICE_IDR_N_LP:// = 20,
    case GST_H265_NAL_SLICE_CRA_NUT:// = 21,
    case RESERVED_IRAP_NAL_TYPE_MIN:// = 22
    case RESERVED_IRAP_NAL_TYPE_MAX:// =  23
         return  0;// IDR I frame
    case GST_H265_NAL_VPS:// = 32,
         dec->header.size = 0;
    case GST_H265_NAL_SPS:// = 33,
    case GST_H265_NAL_PPS:// = 34,
         memcpy(dec->header.data + dec->header.size, data, size);
         dec->header.size += size;
         return -3;
    case GST_H265_NAL_AUD:// = 35,
         return -2;      
    case GST_H265_NAL_EOS:// = 36,
    case GST_H265_NAL_EOB:// = 37,
    case GST_H265_NAL_FD:// = 38,
         break;
    case GST_H265_NAL_PREFIX_SEI:// = 39,
    case GST_H265_NAL_SUFFIX_SEI:// = 40
         memcpy(dec->header.data + dec->header.size, data, size);
         dec->header.size += size;
         return -3;
    default:
         break;
  }

  if (nalu.type >= 32) {// no vcl
      return 0x0F;
  }

/*
  pres = gst_h265_parser_parse_slice_hdr (self->parser, &nalu, &slice);
  if (GST_H265_IS_I_SLICE (&slice)) {
      return 0;// I frame
  }//*/

  if (dec->skipframes == 0) {
      return -4;// lost no I frames
  }

/*
  if (GST_H265_IS_P_SLICE (&slice)) {
      return 1;
  }

  if (GST_H265_IS_B_SLICE (&slice)) {
      return 2;
  }
*/
  return 1;
}

static gboolean
amsh265dec_element_init (GstPlugin * plugin)
{
   GST_DEBUG_CATEGORY_INIT (gst_amsh265dec_debug, "amsh265dec", 0, "debug category for AMSCODEC H265 Decoder");
   return gst_element_register (plugin, "amsh265dec", GST_RANK_PRIMARY,  GST_TYPE_AMSH265_DEC);
}

