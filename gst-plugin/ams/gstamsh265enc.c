
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <string.h>
#include <gst/tag/tag.h>
#include <gst/base/base.h>
#include <gst/video/video.h>

#include "gstamselements.h"
#include "gstamsh265enc.h"

GST_DEBUG_CATEGORY_STATIC (gst_amsh265enc_debug);
#define GST_CAT_DEFAULT gst_amsh265enc_debug

enum
{
    PROP_0,
    PROP_VPU_PROFILE,//    PROP_VPU_LEVEL,
};

static void gst_amsh265_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static void gst_amsh265_enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_amsh265_enc_open(GstAMSEnc *enc);

static gboolean gst_amsh265_enc_close(GstAMSEnc *enc);

static GstCaps* gst_amsh265_enc_getcaps(GstAMSEnc *enc);

static gboolean amsh265enc_element_init (GstPlugin * plugin);

static GstStaticPadTemplate gst_amsh265_enc_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, format = (string) { I420, I420_10LE }, width = (int) [128, 8192], height = (int) [128, 8192], framerate = (fraction) [ 0/1, MAX ]")
    );

static GstStaticPadTemplate gst_amsh265_enc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h265, stream-format=(string)byte-stream, alignment=(string)nal, profile = (string) { main, main-10 }")
    );

#define parent_class gst_amsh265_enc_parent_class
G_DEFINE_TYPE (GstAMSH265Enc, gst_amsh265_enc, GST_TYPE_AMS_ENC);  
GST_ELEMENT_REGISTER_DEFINE_CUSTOM (amsh265enc, amsh265enc_element_init);

static void
gst_amsh265_enc_class_init (GstAMSH265EncClass * klass)
{
    GObjectClass    *gobject_class;
    GstElementClass *element_class;
    GstAMSEncClass  *ams_class;
    GST_AMS_LOG("init");

    gobject_class = G_OBJECT_CLASS (klass);
    element_class = GST_ELEMENT_CLASS (klass);
    ams_class = GST_AMS_ENC_CLASS (klass);

    gobject_class->set_property = gst_amsh265_enc_set_property;
    gobject_class->get_property = gst_amsh265_enc_get_property;

    g_object_class_install_property (gobject_class, PROP_VPU_PROFILE,
      g_param_spec_uint ("profile","H265 profile",
          "H265 profile [0-1]",
          0, 1, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));


    gst_element_class_add_static_pad_template (element_class,
      &gst_amsh265_enc_src_template);
    gst_element_class_add_static_pad_template (element_class,
      &gst_amsh265_enc_sink_template);

    gst_element_class_set_static_metadata (element_class,
      "AMSCODEC H265 Encoder", "Encoder/Video",  "H265 video Encoder", 
      "lirui <lirui@smart-core.cn>, lirui <lirui-buaa@sohu.com>");

    ams_class->open      = GST_DEBUG_FUNCPTR (gst_amsh265_enc_open);
    ams_class->close     = GST_DEBUG_FUNCPTR (gst_amsh265_enc_close);
    ams_class->getcaps   = GST_DEBUG_FUNCPTR (gst_amsh265_enc_getcaps);
}

static void gst_amsh265_enc_init (GstAMSH265Enc * encoder) {
    GstAMSEnc *enc = GST_AMS_ENC (encoder);
    GST_DEBUG_OBJECT (encoder, "init");
    GST_AMS_LOG("init");
    enc->encoder->profile = AMS_PROFILE_H265_MAIN;
    enc->encoder->level   = AMS_LEVEL_H265_5_1;
}

static void gst_amsh265_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec) {
    GstAMSEnc *enc = GST_AMS_ENC (object);
    GstAMSH265Enc *self = GST_AMSH265_ENC (enc);
    GST_DEBUG_OBJECT (self, "set_property");
    GST_AMS_LOG("set_property");

    switch (prop_id) {
        case PROP_VPU_PROFILE:
            enc->encoder->profile =  g_value_get_uint(value);
            break;
        default:
        break;
    }

}

static void gst_amsh265_enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec) {
    GstAMSEnc *enc = GST_AMS_ENC (object);
    GstAMSH265Enc *self = GST_AMSH265_ENC (enc);
    GST_DEBUG_OBJECT (self, "get_property");
    GST_AMS_LOG("get_property");

    switch (prop_id) {
        case PROP_VPU_PROFILE:
            g_value_set_uint (value, enc->encoder->profile);
            break;
        default:
        break;
    }

}

static gboolean gst_amsh265_enc_open(GstAMSEnc *enc) {
    GstAMSH265Enc *self =  GST_AMSH265_ENC(enc);
    GST_DEBUG_OBJECT (self, "open");
//    GST_AMS_LOG("open");
    enc->encoder->codec_id = AMS_CODEC_ID_H265;
    if (ams_encode_create(enc->encoder) < 0) {
        GST_ERROR_OBJECT (enc, "Failed to create ams h265 encode context");
        return FALSE;
    }

    return TRUE;
}

static gboolean gst_amsh265_enc_close(GstAMSEnc *enc) {
    GstAMSH265Enc *self =  GST_AMSH265_ENC(enc);
    GST_DEBUG_OBJECT (self, "close");
//    GST_AMS_LOG("close");
    if (ams_encode_close(enc->encoder) < 0) {
        GST_ERROR_OBJECT (enc, "Failed to close ams h265 encode context");
        return FALSE;
    }
    return TRUE;
}

static GstCaps* gst_amsh265_enc_getcaps(GstAMSEnc *enc) {
    GstCaps *allowed_caps = NULL;
    GstCaps *outcaps = NULL;
//    GstStructure *allowed_s = NULL;
    GstStructure *s = NULL;

    GstAMSH265Enc *self =  GST_AMSH265_ENC(enc);
    GST_DEBUG_OBJECT (self, "getcaps");
//    GST_AMS_LOG("getcaps");

    outcaps = gst_static_pad_template_get_caps (&gst_amsh265_enc_src_template);
    outcaps = gst_caps_make_writable (outcaps);
    allowed_caps = gst_pad_get_allowed_caps (GST_VIDEO_ENCODER_SRC_PAD (enc));
    allowed_caps = gst_caps_make_writable (allowed_caps);
    allowed_caps = gst_caps_fixate (allowed_caps);
//    allowed_s = gst_caps_get_structure (allowed_caps, 0);
    s = gst_caps_get_structure (outcaps, 0);
//    profile = gst_structure_get_string (allowed_s, "profile");
    if (enc->encoder->pix_fmt == AMS_PIX_FMT_YUV420P10LE) {
        gst_structure_set (s, "profile", G_TYPE_STRING, "main-10", NULL);
        enc->encoder->profile = AMS_PROFILE_H265_MAIN10;
    } else {
        gst_structure_set (s, "profile", G_TYPE_STRING, "main", NULL);
        enc->encoder->profile = AMS_PROFILE_H265_MAIN;
    }
    gst_caps_get_structure (outcaps, 0);
    gst_structure_set (s, "level", G_TYPE_STRING, "5.1", NULL);
    enc->encoder->level   = AMS_LEVEL_H265_5_1;
    gst_clear_caps (&allowed_caps);
    return outcaps;
}

static gboolean
amsh265enc_element_init (GstPlugin * plugin) {
   GST_DEBUG_CATEGORY_INIT (gst_amsh265enc_debug, "amsh265enc", 0, "debug category for AMSCODEC H265 Encoder");
   return gst_element_register (plugin, "amsh265enc", GST_RANK_PRIMARY,  GST_TYPE_AMSH265_ENC);
}

