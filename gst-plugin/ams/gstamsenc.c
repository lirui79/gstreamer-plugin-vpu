
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <gst/tag/tag.h>
#include <gst/base/base.h>
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

#include "gstamselements.h"
#include "gstamsenc.h"


GST_DEBUG_CATEGORY_STATIC (gst_amsenc_debug);
#define GST_CAT_DEFAULT gst_amsenc_debug

enum
{
    PROP_0,
    PROP_VPU_BOARD_IDX,
    PROP_VPU_CORE_IDX,
    PROP_VPU_TRANSCODE,

    PROP_VPU_BITRATE,
    PROP_VPU_MAXBITRATE,

    PROP_VPU_GOPMODE,
    PROP_VPU_RCMODE,
    PROP_VPU_CULEVELRCENABLE,

    PROP_VPU_QP,
    PROP_VPU_MINQP,
    PROP_VPU_MAXQP,

    PROP_VPU_MINQPI,
    PROP_VPU_MAXQPI,
    PROP_VPU_MINQPP,
    PROP_VPU_MAXQPP,
    PROP_VPU_MINQPB,
    PROP_VPU_MAXQPB,

    PROP_VPU_HVSQPENABLE,
    PROP_VPU_HVSQPSCALE,
    PROP_VPU_MAXDELTAQP,
    PROP_VPU_BGDETECTENABLE,
    PROP_VPU_BGLAMBDAQP,
    PROP_VPU_BGDELTAQP,

    PROP_VPU_CONFWINTOP,
    PROP_VPU_CONFWINBOT,
    PROP_VPU_CONFWINLEFT,
    PROP_VPU_CONFWINRIGHT,

    PROP_VPU_FIDRHE,//forcedIdrHeaderEnable
    PROP_VPU_ROI_ENABLE,//roi_enable;
    PROP_VPU_GOP_SIZE,//gop_size;
    PROP_VPU_VBVBUFFERSIZE,//vbvBufferSize;

    PROP_VPU_HDRMODE,//hdrMode;
    PROP_VPU_HDRCLRPRI,//hdrColourPrimaries;
    PROP_VPU_HDRTRACH,//hdrTransferCharacteristics;
    PROP_VPU_HDRMATCOEF,//hdrMatrixCoeffs;
    PROP_VPU_VIDEOFULLRANGE,//videoFullRange;
};

static void gst_ams_enc_finalize (GObject * object);

static void gst_ams_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static void gst_ams_enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_ams_enc_start (GstVideoEncoder * encoder);

static gboolean gst_ams_enc_stop (GstVideoEncoder * encoder);

static gboolean gst_ams_enc_set_format (GstVideoEncoder *
    encoder, GstVideoCodecState * state);

static GstFlowReturn gst_ams_enc_finish (GstVideoEncoder * encoder);

static GstFlowReturn gst_ams_enc_handle_frame (GstVideoEncoder *
    encoder, GstVideoCodecFrame * frame);

static gboolean gst_ams_enc_sink_event (GstVideoEncoder *
    encoder, GstEvent * event);

static gboolean gst_ams_enc_propose_allocation (GstVideoEncoder * encoder,
    GstQuery * query);

static GstFlowReturn gst_ams_enc_getframes (GstVideoEncoder * encoder, int finished);

static GstFlowReturn gst_ams_enc_frame_to_buffer(GstVideoEncoder * encoder, GstVideoFrame *frame, ams_frame_t * img);

static ams_frame_t * gst_ams_enc_alloc_frame(GstVideoFrame *video_frame, ams_pixel_format_t pix_fmt);

static GstFlowReturn gst_ams_enc_getframe (GstVideoEncoder * encoder, gint *gotframe);

static GstFlowReturn gst_ams_enc_proframe(GstVideoEncoder * encoder, ams_packet_t *pkt);

#define parent_class gst_ams_enc_parent_class
G_DEFINE_TYPE_WITH_CODE (GstAMSEnc, gst_ams_enc, GST_TYPE_VIDEO_ENCODER,
    G_IMPLEMENT_INTERFACE (GST_TYPE_TAG_SETTER, NULL);
    G_IMPLEMENT_INTERFACE (GST_TYPE_PRESET, NULL););

static void
gst_ams_enc_class_init (GstAMSEncClass * klass)
{
    GObjectClass *gobject_class;
    GstVideoEncoderClass *video_encoder_class;

    gobject_class = G_OBJECT_CLASS (klass);
    video_encoder_class = GST_VIDEO_ENCODER_CLASS (klass);

    GST_AMS_LOG("init");
    gobject_class->set_property = gst_ams_enc_set_property;
    gobject_class->get_property = gst_ams_enc_get_property;
    gobject_class->finalize = gst_ams_enc_finalize;

    g_object_class_install_property (gobject_class, PROP_VPU_BOARD_IDX,
      g_param_spec_int ("board-idx", "vpu board index",
          "designate vpu board index, default (-1) ",
          -1, 31, -1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_CORE_IDX,
      g_param_spec_int ("core-idx", "vpu core index",
          "designate vpu core index, default (-1)",
          -1, 1, -1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_TRANSCODE,
      g_param_spec_boolean ("transcode", "decoder part of transcode",
          "Enable transcode", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_BITRATE,
      g_param_spec_uint ("bitrate", "Bitrate",
          "Bitrate (in bits per second)",
          0, 700000000, 1500000,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
              GST_PARAM_MUTABLE_PLAYING)));

    g_object_class_install_property (gobject_class, PROP_VPU_MAXBITRATE,
      g_param_spec_uint ("max-bitrate", "Max Bitrate",
          "Maximum Bitrate (in bits per second)",
          0, 700000000, 2000000,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
              GST_PARAM_MUTABLE_PLAYING)));

    g_object_class_install_property (gobject_class, PROP_VPU_GOPMODE,
      g_param_spec_uint ("gop-mode", "Gop mode",
          "GOP structure, 0:User defined,1:all I,2:IPP,3:IBBB,4:IBPBP,5:IBBBP,6:IPPPP,7:IBBBB,8:RA IB,9:IPP single",
          0, 9, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_RCMODE,
      g_param_spec_uint ("rc-mode", "Rate Control mode",
          "Rate Control mode, 0:crf, 1:cbr, 2:abr, 3:vbr, default:3",
          0, 3, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_CULEVELRCENABLE,
      g_param_spec_boolean ("rc-cu", "Rate Control CU Level",
          "Enable CU level Rate Control", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_QP,
      g_param_spec_uint ("qp", "Quantization Parameter for Rate Control",
          "QP Quantization [0-51], default:30",
          0, 51, 30,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_MINQP,
      g_param_spec_uint ("qp-min", "Minimum Quantization Parameter for Rate Control",
          "QP Min Quantization [0-51], default:8",
          0, 51, 8,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_MAXQP,
      g_param_spec_uint ("qp-max", "Maximum Quantization Parameter for Rate Control",
          "QP Max Quantization [0-51], default:51",
          0, 51, 51,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_MINQPI,
      g_param_spec_uint ("qp-i-min", "Minimum QP of I picture for Rate Control",
          "QP I Min Quantization [0-51], default:8",
          0, 51, 8,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_MAXQPI,
      g_param_spec_uint ("qp-i-max", "Maximum QP of I picture for Rate Control",
          "QP I Max Quantization [0-51], default:51",
          0, 51, 51,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_MINQPP,
      g_param_spec_uint ("qp-p-min", "Minimum QP of P picture for Rate Control",
          "QP P Min Quantization [0-51], default:8",
          0, 51, 8,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_MAXQPP,
      g_param_spec_uint ("qp-p-max", "Maximum QP of P picture for Rate Control",
          "QP P Max Quantization [0-51], default:51",
          0, 51, 51,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_MINQPB,
      g_param_spec_uint ("qp-b-min", "Minimum QP of B picture for Rate Control",
          "QP B Min Quantization [0-51], default:8",
          0, 51, 8,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_MAXQPB,
      g_param_spec_uint ("qp-b-max", "Maximum QP of B picture for Rate Control",
          "QP B Max Quantization [0-51], default:51",
          0, 51, 51,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_HVSQPENABLE,
      g_param_spec_boolean ("hvs-qp", "CU QP adjustment for subjective quality enhancement",
          "Enable CU QP adjustment", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_HVSQPSCALE,
      g_param_spec_uint ("hvs-qp-scale", "QP scaling factor for CU QP adjustment",
          "when hvs-qp enable, QP scaling factor for CU QP adjustment, [0-4], default:2",
          0, 4, 2,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_MAXDELTAQP,
      g_param_spec_uint ("qp-max-delta", "Maximum delta QP for HVS",
          "Maximum delta QP for HVS, [0-12], default:10",
          0, 12, 10,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_BGDETECTENABLE,
      g_param_spec_boolean ("background-detection", "Background Detection",
          "Enable background detection", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_BGLAMBDAQP,
      g_param_spec_uint ("background-lambda-qp", "Minimum lambda QP value to be used in the background area",
          "Minimum lambda QP value to be used in the background area, [0-51], default:20",
          0, 51, 20,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_BGDELTAQP,
      g_param_spec_int ("bg-delta-qp", "difference between the lambda QP value of background and the lambda QP value of foreground",
          "Back ground Delta QP, [-16 - 15], default:3",
          -16, 15, 3,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_CONFWINTOP,
      g_param_spec_int ("conf-win-top", "Top offset of conformance window",
          "Top offset of conformance window, [-1-8192], default:-1",
          -1, 8192, -1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_CONFWINBOT,
      g_param_spec_int ("conf-win-bot", "Bottom offset of conformance window",
          "Bottom offset of conformance window, [-1-8192], default:-1",
          -1, 8192, -1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_CONFWINLEFT,
      g_param_spec_int ("conf-win-left", "Left offset of conformance window",
          "Left offset of conformance window, [-1-8192], default:-1",
          -1, 8192, -1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VPU_CONFWINRIGHT,
      g_param_spec_int ("conf-win-right", "Right offset of conformance window",
          "Right offset of conformance window, [-1-8192], default:-1",
          -1, 8192, -1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

     g_object_class_install_property (gobject_class, PROP_VPU_FIDRHE,
      g_param_spec_uint ("idr-header", "Forced every IDR frame to include VPS/SPS/PPS",
          "Forced Every IDR frame to include VPS/SPS/PPS [0-2], default:0, 0:No froced Header(VPS/SPS/PPS),1:Forced Header before IDR frame,2:Forced Header before key frame",
          0, 2, 2,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

     g_object_class_install_property (gobject_class, PROP_VPU_ROI_ENABLE,
      g_param_spec_boolean ("roi", "Enables ROI map",
          "Enables ROI map. NOTE: It is valid when rate control is on", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

     g_object_class_install_property (gobject_class, PROP_VPU_GOP_SIZE,
      g_param_spec_uint ("gop-size","GOP size",
          "Number of frames between intra frames",
          1, 999, 90,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

     g_object_class_install_property (gobject_class, PROP_VPU_VBVBUFFERSIZE,
      g_param_spec_uint ("vbvbuffer-size","Size of VBV buffer in msec",
          "Size of VBV buffer in msec [10-3000]. default:3000 should be set for 3 seconds. This value is valid when rcEnable is 1. VBV buffer size in bits is EncBitrate * VbvBufferSize / 1000.",
          10, 3000, 3000,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

     g_object_class_install_property (gobject_class, PROP_VPU_HDRMODE,
      g_param_spec_uint ("hdr-mode","HDR Mode",
          "HDR Mode",
          0, 4, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

     g_object_class_install_property (gobject_class, PROP_VPU_HDRCLRPRI,
      g_param_spec_uint ("hdr-color-primaries","HDR Color Primaries",
          "HDR Color Primaries",
          0, 1000, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

     g_object_class_install_property (gobject_class, PROP_VPU_HDRTRACH,
      g_param_spec_uint ("hdr-transfer-characteristics","HDR Transfer Characteristics",
          "HDR Transfer Characteristics",
          0, 1000, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

     g_object_class_install_property (gobject_class, PROP_VPU_HDRMATCOEF,
      g_param_spec_uint ("hdr-matrix-coeffs","HDR Matrix Coeffs",
          "HDR Matrix Coeffs",
          0, 1000, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

     g_object_class_install_property (gobject_class, PROP_VPU_VIDEOFULLRANGE,
      g_param_spec_boolean ("video-full-range", "Enables Video Full Range",
          "Enables Video Full Range", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    video_encoder_class->start = gst_ams_enc_start;
    video_encoder_class->stop = gst_ams_enc_stop;
    video_encoder_class->handle_frame = gst_ams_enc_handle_frame;
    video_encoder_class->set_format = gst_ams_enc_set_format;
    video_encoder_class->finish = gst_ams_enc_finish;
    video_encoder_class->sink_event = gst_ams_enc_sink_event;
    video_encoder_class->propose_allocation = gst_ams_enc_propose_allocation;

    klass->open  = NULL;
    klass->close = NULL;
    klass->getcaps   = NULL;

    GST_DEBUG_CATEGORY_INIT (gst_amsenc_debug, "amsenc", 0, "AMS Encoder");

    gst_type_mark_as_plugin_api (GST_TYPE_AMS_ENC, 0);
}

static void
gst_ams_enc_init (GstAMSEnc * enc)
{
    GST_DEBUG_OBJECT (enc, "init");
    GST_PAD_SET_ACCEPT_TEMPLATE (GST_VIDEO_ENCODER_SINK_PAD (enc));

    enc->input_state  = NULL;
    memset(&(enc->header), 0, sizeof(enc->header));

    GST_AMS_LOG("init");
    enc->encoder      = ams_create_codec_context0(AMS_CODEC_ID_H264);
    enc->encoder->board_id = -1;
    enc->encoder->coreidx  = -1;
    enc->encoder->width    = 1920;
    enc->encoder->height   = 1080;
    enc->encoder->transcode_flag = 0;
    enc->encoder->fps      = 30;
    enc->encoder->pix_fmt  = AMS_PIX_FMT_YUV420P;
    enc->encoder->gop_mode = AMS_GOP_MODE_IBPBP;

    enc->encoder->rc_mode  = AMS_RC_MODE_VBR;
    enc->encoder->qp       = 30;
    enc->encoder->bitrate  = 1500000;
    enc->encoder->maxbitrate = 2000000;
    enc->encoder->forcedIdrHeaderEnable = 2;
    enc->encoder->roi_enable = 0;
    enc->encoder->gop_size  = 90;
    enc->encoder->vbvBufferSize = 3000;

    enc->encoder->minQp  = 8;
    enc->encoder->maxQp  = 51;
    enc->encoder->minQpI = 8;
    enc->encoder->maxQpI = 51;
    enc->encoder->minQpP = 8;
    enc->encoder->maxQpP = 51;
    enc->encoder->minQpB = 8;
    enc->encoder->maxQpB = 51;

    enc->encoder->confWinTop    = -1;
    enc->encoder->confWinBot    = -1;
    enc->encoder->confWinLeft   = -1;
    enc->encoder->confWinRight  = -1;

    enc->encoder->cuLevelRCEnable = 0;
    enc->encoder->hvsQpEnable = 0;
    enc->encoder->hvsQpScale = 2;
    enc->encoder->maxDeltaQp = 10;
    enc->encoder->bgDetectEnable = 0;
    enc->encoder->bgLambdaQp = 20;
    enc->encoder->bgDeltaQp = 3;
    enc->encoder->crfQp = 30;

    /*
    1: HLG  ATSC A/341;          color_primaries=9,transfer_characteristics=18,matrix_coeffs=9
    2: HLG ETSI ETSI TS 101 154; color_primaries=9,transfer_characteristics=14,matrix_coeffs=9
    3: HDR10/10+;                color_primaries=9,transfer_characteristics=16,matrix_coeffs=9
    4: Dolby Vision;             color_primaries=2,transfer_characteristics= 2,matrix_coeffs=2
    */
    enc->encoder->hdrMode = 0;
    enc->encoder->hdrColourPrimaries = 0;
    enc->encoder->hdrTransferCharacteristics = 0;
    enc->encoder->hdrMatrixCoeffs = 0;
    enc->encoder->videoFullRange = 0;
}

static void
gst_ams_enc_finalize (GObject * object)
{
    GstAMSEnc *enc = NULL;

    GST_DEBUG_OBJECT (object, "finalize");
    GST_AMS_LOG("finalize");
    g_return_if_fail (GST_IS_AMS_ENC (object));
    enc = GST_AMS_ENC (object);

    if (enc->input_state) {
        gst_video_codec_state_unref (enc->input_state);
    }
    enc->input_state = NULL;

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_ams_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstAMSEnc *enc = NULL;
    GST_DEBUG_OBJECT (object, "set_property");
    GST_AMS_LOG("set_property");
    g_return_if_fail (GST_IS_AMS_ENC (object));
    enc = GST_AMS_ENC (object);

    switch (prop_id) {
        case PROP_VPU_BOARD_IDX:
            enc->encoder->board_id =  g_value_get_int(value);
            break;
        case PROP_VPU_CORE_IDX:
            enc->encoder->coreidx =  g_value_get_int(value);
            break;
        case PROP_VPU_TRANSCODE:
            enc->encoder->transcode_flag = g_value_get_boolean (value);//(g_value_get_boolean (value) ? 1 : 0);
            break;
        case PROP_VPU_BITRATE:
            enc->encoder->bitrate =  g_value_get_uint(value);
            break;
        case PROP_VPU_MAXBITRATE:
            enc->encoder->maxbitrate =  g_value_get_uint(value);
            break;
        case PROP_VPU_GOPMODE:
            enc->encoder->gop_mode =  g_value_get_uint(value);
            break;
        case PROP_VPU_RCMODE:
            enc->encoder->rc_mode =  g_value_get_uint(value);
            break;
        case PROP_VPU_CULEVELRCENABLE:
             enc->encoder->cuLevelRCEnable = g_value_get_boolean (value);//(g_value_get_boolean (value) ? 1 : 0);
            break;
        case PROP_VPU_QP:
            enc->encoder->crfQp = enc->encoder->qp =  g_value_get_uint(value);
            break;
        case PROP_VPU_MINQP:
            enc->encoder->minQp =  g_value_get_uint(value);
            break;
        case PROP_VPU_MAXQP:
            enc->encoder->maxQp =  g_value_get_uint(value);
            break;
        case PROP_VPU_MINQPI:
            enc->encoder->minQpI =  g_value_get_uint(value);
            break;
        case PROP_VPU_MAXQPI:
            enc->encoder->maxQpI =  g_value_get_uint(value);
            break;
        case PROP_VPU_MINQPP:
            enc->encoder->minQpP =  g_value_get_uint(value);
            break;
        case PROP_VPU_MAXQPP:
            enc->encoder->maxQpP =  g_value_get_uint(value);
            break;
        case PROP_VPU_MINQPB:
            enc->encoder->minQpB =  g_value_get_uint(value);
            break;
        case PROP_VPU_MAXQPB:
            enc->encoder->maxQpB =  g_value_get_uint(value);
            break;
        case PROP_VPU_HVSQPENABLE:
             enc->encoder->hvsQpEnable = g_value_get_boolean (value);//(g_value_get_boolean (value) ? 1 : 0);
            break;
        case PROP_VPU_HVSQPSCALE:
            enc->encoder->hvsQpScale =  g_value_get_uint(value);
            break;
        case PROP_VPU_MAXDELTAQP:
            enc->encoder->maxDeltaQp =  g_value_get_uint(value);
            break;
        case PROP_VPU_BGDETECTENABLE:
             enc->encoder->bgDetectEnable = g_value_get_boolean (value);//(g_value_get_boolean (value) ? 1 : 0);
            break;
        case PROP_VPU_BGLAMBDAQP:
            enc->encoder->bgLambdaQp =  g_value_get_uint(value);
            break;
        case PROP_VPU_BGDELTAQP:
            enc->encoder->bgDeltaQp =  g_value_get_int(value);
            break;
        case PROP_VPU_CONFWINTOP:
            enc->encoder->confWinTop =  g_value_get_int(value);
            break;
        case PROP_VPU_CONFWINBOT:
            enc->encoder->confWinBot =  g_value_get_int(value);
            break;
        case PROP_VPU_CONFWINLEFT:
            enc->encoder->confWinLeft =  g_value_get_int(value);
            break;
        case PROP_VPU_CONFWINRIGHT:
            enc->encoder->confWinRight =  g_value_get_int(value);
            break;
        case PROP_VPU_FIDRHE:
            enc->encoder->forcedIdrHeaderEnable =  g_value_get_uint(value);
            break;
        case PROP_VPU_ROI_ENABLE:
             enc->encoder->roi_enable = g_value_get_boolean (value);//(g_value_get_boolean (value) ? 1 : 0);
            break;
        case PROP_VPU_GOP_SIZE:
            enc->encoder->gop_size =  g_value_get_uint(value);
            break;
        case PROP_VPU_VBVBUFFERSIZE:
            enc->encoder->vbvBufferSize =  g_value_get_uint(value);
            break;
        case PROP_VPU_HDRMODE:
            enc->encoder->hdrMode =  g_value_get_uint(value);
            break;
        case PROP_VPU_HDRCLRPRI:
            enc->encoder->hdrColourPrimaries =  g_value_get_uint(value);
            break;
        case PROP_VPU_HDRTRACH:
            enc->encoder->hdrTransferCharacteristics =  g_value_get_uint(value);
            break;
        case PROP_VPU_HDRMATCOEF:
            enc->encoder->hdrMatrixCoeffs =  g_value_get_uint(value);
            break;
        case PROP_VPU_VIDEOFULLRANGE:
             enc->encoder->videoFullRange = g_value_get_boolean (value);//(g_value_get_boolean (value) ? 1 : 0);
            break;
        default:
            break;
    }
}

static void
gst_ams_enc_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
    GstAMSEnc *enc = NULL;

    GST_DEBUG_OBJECT (object, "get_property");
    GST_AMS_LOG("get_property");
    g_return_if_fail (GST_IS_AMS_ENC (object));
    enc = GST_AMS_ENC (object);

    switch (prop_id) {
        case PROP_VPU_BOARD_IDX:
            g_value_set_int (value, enc->encoder->board_id);
            break;
        case PROP_VPU_CORE_IDX:
            g_value_set_int (value, enc->encoder->coreidx);
            break;
        case PROP_VPU_TRANSCODE:
            g_value_set_boolean (value, enc->encoder->transcode_flag);
            break;
        case PROP_VPU_BITRATE:
            g_value_set_uint (value, enc->encoder->bitrate);
            break;
        case PROP_VPU_MAXBITRATE:
            g_value_set_uint (value, enc->encoder->maxbitrate);
            break;
        case PROP_VPU_GOPMODE:
            g_value_set_uint (value, enc->encoder->gop_mode);
            break;
        case PROP_VPU_RCMODE:
            g_value_set_uint (value, enc->encoder->rc_mode);
            break;
        case PROP_VPU_CULEVELRCENABLE:
            g_value_set_boolean (value, enc->encoder->cuLevelRCEnable);
            break;
        case PROP_VPU_QP:
            g_value_set_uint (value, enc->encoder->crfQp);
            break;
        case PROP_VPU_MINQP:
            g_value_set_uint (value, enc->encoder->minQp);
            break;
        case PROP_VPU_MAXQP:
            g_value_set_uint (value, enc->encoder->maxQp);
            break;
        case PROP_VPU_MINQPI:
            g_value_set_uint (value, enc->encoder->minQpI);
            break;
        case PROP_VPU_MAXQPI:
            g_value_set_uint (value, enc->encoder->maxQpI);
            break;
        case PROP_VPU_MINQPP:
            g_value_set_uint (value, enc->encoder->minQpP);
            break;
        case PROP_VPU_MAXQPP:
            g_value_set_uint (value, enc->encoder->maxQpP);
            break;
        case PROP_VPU_MINQPB:
            g_value_set_uint (value, enc->encoder->minQpB);
            break;
        case PROP_VPU_MAXQPB:
            g_value_set_uint (value, enc->encoder->maxQpB);
            break;
        case PROP_VPU_HVSQPENABLE:
            g_value_set_boolean (value, enc->encoder->hvsQpEnable);
            break;
        case PROP_VPU_HVSQPSCALE:
            g_value_set_uint (value, enc->encoder->hvsQpScale);
            break;
        case PROP_VPU_MAXDELTAQP:
            g_value_set_uint (value, enc->encoder->maxDeltaQp);
            break;
        case PROP_VPU_BGDETECTENABLE:
            g_value_set_boolean (value, enc->encoder->bgDetectEnable);
            break;
        case PROP_VPU_BGLAMBDAQP:
            g_value_set_uint (value, enc->encoder->bgLambdaQp);
            break;
        case PROP_VPU_BGDELTAQP:
            g_value_set_int (value, enc->encoder->bgDeltaQp);
            break;
        case PROP_VPU_CONFWINTOP:
            g_value_set_int (value, enc->encoder->confWinTop);
            break;
        case PROP_VPU_CONFWINBOT:
            g_value_set_int (value, enc->encoder->confWinBot);
            break;
        case PROP_VPU_CONFWINLEFT:
            g_value_set_int (value, enc->encoder->confWinLeft);
            break;
        case PROP_VPU_CONFWINRIGHT:
            g_value_set_int (value, enc->encoder->confWinRight);
            break;
        case PROP_VPU_FIDRHE:
            g_value_set_uint (value, enc->encoder->forcedIdrHeaderEnable);
            break;
        case PROP_VPU_ROI_ENABLE:
            g_value_set_boolean (value, enc->encoder->roi_enable);
            break;
        case PROP_VPU_GOP_SIZE:
            g_value_set_uint (value, enc->encoder->gop_size);
            break;
        case PROP_VPU_VBVBUFFERSIZE:
            g_value_set_uint (value, enc->encoder->vbvBufferSize);
            break;
        case PROP_VPU_HDRMODE:
            g_value_set_uint (value, enc->encoder->hdrMode);
            break;
        case PROP_VPU_HDRCLRPRI:
            g_value_set_uint (value, enc->encoder->hdrColourPrimaries);
            break;
        case PROP_VPU_HDRTRACH:
            g_value_set_uint (value, enc->encoder->hdrTransferCharacteristics);
            break;
        case PROP_VPU_HDRMATCOEF:
            g_value_set_uint (value, enc->encoder->hdrMatrixCoeffs);
            break;
        case PROP_VPU_VIDEOFULLRANGE:
            g_value_set_boolean (value, enc->encoder->videoFullRange);
            break;
        default:
            break;
    }
}

static gboolean
gst_ams_enc_start (GstVideoEncoder * encoder)
{//GstAMSEnc *enc = GST_AMS_ENC (encoder); GstAMSEncClass *klass = GST_AMS_ENC_GET_CLASS (encoder);

    GST_DEBUG_OBJECT (encoder, "start");
    GST_AMS_LOG("start");
    return TRUE;
}

static gboolean
gst_ams_enc_stop (GstVideoEncoder * encoder)
{
    GstAMSEnc *enc = GST_AMS_ENC (encoder);
    GstAMSEncClass *klass = GST_AMS_ENC_GET_CLASS (encoder);

    GST_DEBUG_OBJECT (encoder, "stop");
    GST_AMS_LOG("stop");

    if (enc->input_state) {
        gst_video_codec_state_unref (enc->input_state);
    }
    enc->input_state = NULL;

    if (enc->encoder != NULL) {
        if (klass->close != NULL) {
            klass->close (enc);
        }
        ams_free_codec_context(enc->encoder);
        enc->encoder = NULL;
    }

    return TRUE;
}

static gboolean
gst_ams_enc_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState *state)
{
    GstAMSEnc *enc = GST_AMS_ENC (encoder);
    GstAMSEncClass *klass = GST_AMS_ENC_GET_CLASS (enc);
    GstVideoCodecState *output_state;
    ams_pixel_format_t pix_fmt = AMS_PIX_FMT_YUV420P;
    GstCaps *outcaps = NULL;
    gchar *debug_caps = NULL;

    debug_caps = gst_caps_to_string (state->caps);
    GST_DEBUG_OBJECT (enc, "set_format, caps:%s", debug_caps);
    GST_AMS_LOG("set_format caps:%s", debug_caps);
    g_free (debug_caps);

    if (GST_VIDEO_INFO_FORMAT(&state->info) == GST_VIDEO_FORMAT_I420) {
        pix_fmt = AMS_PIX_FMT_YUV420P;
    } else if (GST_VIDEO_INFO_FORMAT(&state->info) == GST_VIDEO_FORMAT_Y42B) {
        pix_fmt = AMS_PIX_FMT_YUV422;
    } else if (GST_VIDEO_INFO_FORMAT(&state->info) == GST_VIDEO_FORMAT_I420_10LE) {
        pix_fmt = AMS_PIX_FMT_YUV420P10LE;
    } else {
        return FALSE;
    }

    if (enc->input_state) {
        gst_video_codec_state_unref (enc->input_state);
    }
    enc->input_state = gst_video_codec_state_ref (state);

    enc->encoder->width   = GST_VIDEO_INFO_WIDTH (&state->info);
    enc->encoder->height  = GST_VIDEO_INFO_HEIGHT (&state->info);
    enc->encoder->fps     = (GST_VIDEO_INFO_FPS_N (&state->info) / GST_VIDEO_INFO_FPS_D (&state->info));
    enc->encoder->pix_fmt = pix_fmt;
//    GST_AMS_LOG("set_format %d %d %d %d", enc->encoder->width, enc->encoder->height, enc->encoder->fps, enc->encoder->pix_fmt);
    outcaps = klass->getcaps(enc);

    if (klass->open != NULL) {
        klass->open (enc);
    }

    output_state = gst_video_encoder_set_output_state (encoder, outcaps, state);
    gst_video_codec_state_unref (output_state);

    return gst_video_encoder_negotiate (encoder);
}

static GstFlowReturn
gst_ams_enc_finish (GstVideoEncoder * encoder)
{
    GST_DEBUG_OBJECT (encoder, "finish");
    GST_AMS_LOG("finish");
    return gst_ams_enc_getframes(encoder, 1);
}

static GstFlowReturn gst_ams_enc_frame_to_buffer(GstVideoEncoder * encoder, GstVideoFrame *frame, ams_frame_t * img) {
    int deststride, srcstride, height, width, line, comp;
    guint8 *dest = NULL, *src = NULL;
    for (comp = 0; comp < 3; comp++) {
        src  = GST_VIDEO_FRAME_COMP_DATA (frame, comp);
        dest = img->data[comp];
        width = GST_VIDEO_FRAME_COMP_WIDTH (frame, comp) * GST_VIDEO_FRAME_COMP_PSTRIDE (frame, comp);
        height = GST_VIDEO_FRAME_COMP_HEIGHT (frame, comp);
        srcstride = GST_VIDEO_FRAME_COMP_STRIDE (frame, comp);
        //deststride = GST_AMS_ALIGN(img->width, 32);
        deststride = img->width;

        if (img->pix_fmt == AMS_PIX_FMT_YUV420P10LE) {
            deststride = 2 * deststride;
        }

        if(comp != 0) {
           deststride = deststride / 2;
        }

        GST_DEBUG_OBJECT (encoder, "dest(width:height:stride)->(%d:%d:%d) - src(width:height:stride)->(%d:%d:%d)",
           width, height, deststride, img->width, img->height, srcstride);
//        GST_AMS_LOG("dest(width:height:stride)->(%d:%d:%d) - src(width:height:stride)->(%d:%d:%d)",
//           width, height, deststride, img->width, img->height, srcstride);

        if (srcstride == deststride) {
            GST_TRACE_OBJECT (encoder, "Stride matches. Comp %d: %d, copying full plane", comp, srcstride);
//            GST_AMS_LOG("Stride matches. Comp %d: %d, copying full plane", comp, srcstride);
            memcpy (dest, src, srcstride * height);
            continue;
        }

        GST_TRACE_OBJECT (encoder, "Stride mismatch. Comp %d: %d != %d, copying line by line.", comp, srcstride, deststride);
//        GST_AMS_LOG("Stride mismatch. Comp %d: %d != %d, copying line by line.", comp, srcstride, deststride);
        for (line = 0; line < height; line++) {
            memcpy (dest, src, width);
            dest += deststride;
            src += srcstride;
        }
    }

    return GST_FLOW_OK;
}

static ams_frame_t * gst_ams_enc_alloc_frame(GstVideoFrame *video_frame, ams_pixel_format_t pix_fmt) {
    ams_frame_t * img = ams_alloc_frame(pix_fmt, GST_VIDEO_FRAME_WIDTH (video_frame), GST_VIDEO_FRAME_HEIGHT (video_frame));
    if (img == NULL) {
        return NULL;
    }
    img->with_data_flag = 1;
 //   GST_AMS_LOG("handle_frame %d %d %d %d %d %d %d %d", img->width, img->height, img->coded_width, img->coded_height,
 //       img->size, img->linesize[0], img->linesize[1], img->linesize[2]);
   return img;
}

static GstFlowReturn
gst_ams_enc_handle_frame (GstVideoEncoder * encoder,  GstVideoCodecFrame * frame)
{
    GstAMSEnc *enc;
    GstVideoFrame video_frame;
    GstFlowReturn ret = GST_FLOW_OK;
    ams_frame_t *vpu_frame = NULL;
    ams_packet_t *pkt = NULL;
    ams_pixel_format_t pix_fmt = AMS_PIX_FMT_YUV420P;
    int retCode = 0;
    guint retry_count = 0;
    const guint retry_threshold = 1000;

    GST_DEBUG_OBJECT (encoder, "handle_frame");

    enc = GST_AMS_ENC (encoder);
    if (!gst_video_frame_map (&video_frame, &enc->input_state->info, frame->input_buffer, GST_MAP_READ)) {
       gst_video_encoder_finish_frame (encoder, frame);
       return GST_FLOW_OK;
    }

    if (GST_VIDEO_FRAME_FORMAT(&video_frame) == GST_VIDEO_FORMAT_I420) {
        pix_fmt = AMS_PIX_FMT_YUV420P;
    } else if (GST_VIDEO_FRAME_FORMAT(&video_frame) == GST_VIDEO_FORMAT_Y42B) {
        pix_fmt = AMS_PIX_FMT_YUV422;
    } else if (GST_VIDEO_FRAME_FORMAT(&video_frame) == GST_VIDEO_FORMAT_I420_10LE) {
        pix_fmt = AMS_PIX_FMT_YUV420P10LE;
    } else {
        gst_video_frame_unmap (&video_frame);
        gst_video_codec_frame_unref (frame);
        GST_ERROR_OBJECT (encoder, "yuv frame format not support");
        return GST_FLOW_ERROR;
    }

    vpu_frame = gst_ams_enc_alloc_frame(&video_frame, pix_fmt);
    if (vpu_frame == NULL) {
        gst_video_frame_unmap (&video_frame);
        gst_video_codec_frame_unref (frame);
        GST_ERROR_OBJECT (encoder, "ams_alloc_frame yuv return NULL");
        return GST_FLOW_ERROR;
    }

    gst_ams_enc_frame_to_buffer(encoder, &video_frame, vpu_frame);
    gst_video_frame_unmap (&video_frame);

    do{
        gint gotframe = 0;
        retCode = ams_encode_send_frame(enc->encoder, vpu_frame);
        if (retCode >= 0) {
            break;
        }

        ret = gst_ams_enc_getframe(encoder, &gotframe);
        if ((ret == GST_FLOW_ERROR) || (ret == GST_FLOW_EOS) || (gotframe < 0)) {// data eos or process wrong exit
            gst_video_codec_frame_unref (frame);
            ams_free_frame(vpu_frame);
            return ret;
        }

        if ((ret == GST_FLOW_OK) && (gotframe == 1)) {
            continue;
        }

        if (retry_count > retry_threshold) {
            GST_ERROR_OBJECT (encoder, "Give up encode");
            ret = GST_FLOW_ERROR;
            ams_free_frame(vpu_frame);
            break;
        }

        ++retry_count;
        g_usleep (2000);
    } while(retCode < 0);

    pkt = ams_encode_receive_packet(enc->encoder);

    if (pkt == NULL) {
        // need more data
        gst_video_codec_frame_unref(frame);
        GST_TRACE_OBJECT (enc, "get not encode frame");
        return GST_FLOW_OK;
    }

    if(ams_packet_is_empty(pkt)) {
        gst_video_encoder_finish_frame (encoder, frame);
        GST_DEBUG_OBJECT (enc, "get last encode frame, at EOS");
        ams_free_packet(pkt);
        return GST_FLOW_EOS;
    }

    gst_video_codec_frame_unref(frame);
    return gst_ams_enc_proframe(encoder, pkt);
}

static GstFlowReturn gst_ams_enc_getframes (GstVideoEncoder * encoder, int finished) {
    GstAMSEnc *enc = GST_AMS_ENC (encoder);
    GstFlowReturn ret = GST_FLOW_OK;
    ams_frame_t *vpu_frame = NULL;
    ams_packet_t *pkt = NULL;
    int retCode = 0;
    guint retry_count = 0;
    const guint retry_threshold = 1000;

    if (finished) {
        vpu_frame = ams_alloc_frame0(AMS_PIX_FMT_YUV420P);
        do{
            gint gotframe = 0;
            retCode = ams_encode_send_frame(enc->encoder, vpu_frame);
            if (retCode >= 0) {
                break;
            }

            ret = gst_ams_enc_getframe(encoder, &gotframe);
            if ((ret == GST_FLOW_ERROR) || (ret == GST_FLOW_EOS) || (gotframe < 0)) {// data eos or process wrong exit
                ams_free_frame(vpu_frame);
                return ret;
            }

            if ((ret == GST_FLOW_OK) && (gotframe == 1)) {
                continue;
            }

            if (retry_count > retry_threshold) {
                GST_ERROR_OBJECT (encoder, "Give up encode");
                ret = GST_FLOW_ERROR;
                ams_free_frame(vpu_frame);
                break;
            }

            ++retry_count;
            g_usleep (2000);
        } while(retCode < 0);
    }

    retry_count = 0;
    while (1) {
        pkt = ams_encode_receive_packet(enc->encoder);
        if (pkt == NULL) {
            // need more data
            if (retry_count > retry_threshold) {
                GST_TRACE_OBJECT (enc, "get not encode frame");
                ret = GST_FLOW_OK;
                if (finished){
                    ret = GST_FLOW_ERROR;
                }
                break;
            }
            ++retry_count;
            /* Magic number 1ms */
            g_usleep (5000);
            continue;
        }

        retry_count = 0;
        if(ams_packet_is_empty(pkt)) {
            GST_DEBUG_OBJECT (enc, "get last encode frame, at EOS");
            ams_free_packet(pkt);
            return GST_FLOW_EOS;
        }

        ret = gst_ams_enc_proframe(encoder, pkt);
    }

    return ret;
}

static gboolean
gst_ams_enc_sink_event (GstVideoEncoder * encoder, GstEvent * event)
{
    GstAMSEnc *enc = GST_AMS_ENC (encoder);
    GST_DEBUG_OBJECT (enc, "sink_event");
    GST_AMS_LOG("sink_event");
    if (GST_EVENT_TYPE (event) == GST_EVENT_TAG) {
        GstTagList *list;
        GstTagSetter *setter = GST_TAG_SETTER (enc);
        const GstTagMergeMode mode = gst_tag_setter_get_tag_merge_mode (setter);

        gst_event_parse_tag (event, &list);
        gst_tag_setter_merge_tags (setter, list, mode);
    }

    /* just peeked, baseclass handles the rest */
    return GST_VIDEO_ENCODER_CLASS (parent_class)->sink_event (encoder, event);
}

static gboolean
gst_ams_enc_propose_allocation (GstVideoEncoder * encoder, GstQuery * query)
{
    gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);
    GST_AMS_LOG("propose_allocation");
    return GST_VIDEO_ENCODER_CLASS (parent_class)->propose_allocation (encoder, query);
}

static GstFlowReturn gst_ams_enc_getframe (GstVideoEncoder * encoder, gint *gotframe) {
    GstAMSEnc *enc = GST_AMS_ENC (encoder);
    ams_packet_t *pkt = NULL;
    GST_DEBUG_OBJECT (encoder, "getframe");
//   GST_AMS_LOG("getframe");

    *gotframe = 0;
    pkt = ams_encode_receive_packet(enc->encoder);

    if (pkt == NULL) {
        // need more data
        GST_TRACE_OBJECT (enc, "get not encode frame");
        return GST_FLOW_OK;
    }

    *gotframe = 1;
    if(ams_packet_is_empty(pkt)) {
        GST_DEBUG_OBJECT (enc, "get last encode frame, at EOS");
        ams_free_packet(pkt);
        *gotframe = 2;
        return GST_FLOW_EOS;
    }

    return gst_ams_enc_proframe(encoder, pkt);
}

static GstFlowReturn gst_ams_enc_proframe(GstVideoEncoder * encoder, ams_packet_t *pkt) {
    GstAMSEnc *enc = GST_AMS_ENC (encoder);
    GstVideoCodecFrame * frame = NULL;
    GstFlowReturn ret = GST_FLOW_OK;
    if (enc->header.state == 0) {
        enc->header.size = pkt->size;
        memcpy(enc->header.data, pkt->data, pkt->size);
        enc->header.state = 1;
        ams_free_packet(pkt);
        return GST_FLOW_OK;
    }

    frame = gst_video_encoder_get_oldest_frame(encoder);
    if (frame == NULL) {
        GST_ERROR_OBJECT (encoder, "Failed to get video encode frame");
        ams_free_packet(pkt);
        return GST_FLOW_OK;
    }

    if (enc->header.state == 1) {
        ret = gst_video_encoder_allocate_output_frame(encoder, frame, pkt->size + enc->header.size);
    } else {
        ret = gst_video_encoder_allocate_output_frame(encoder, frame, pkt->size);
    }

    if (ret != GST_FLOW_OK) {
        GST_ERROR_OBJECT (encoder, "Failed to get video output frame");
        ams_free_packet(pkt);
        gst_video_codec_frame_unref (frame);
        return ret;
    }

    if (enc->header.state == 1) {
        gst_buffer_fill (frame->output_buffer, 0, enc->header.data, enc->header.size);
        gst_buffer_fill (frame->output_buffer, enc->header.size, pkt->data, pkt->size);
        enc->header.state = 2;
    } else {
        gst_buffer_fill (frame->output_buffer, 0, pkt->data, pkt->size);
    }
    ams_free_packet(pkt);
    //GST_AMS_LOG("finish_frame");
    return gst_video_encoder_finish_frame (encoder, frame);
}