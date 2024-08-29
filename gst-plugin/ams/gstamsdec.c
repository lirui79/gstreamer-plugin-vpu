
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <string.h>

#include "gstamselements.h"
#include "gstamsdec.h"

#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

GST_DEBUG_CATEGORY_STATIC (gst_amsdec_debug);
#define GST_CAT_DEFAULT gst_amsdec_debug

enum
{
  PROP_0,
  PROP_VPU_BOARD_IDX,
  PROP_VPU_CORE_IDX,
  PROP_VPU_SKIP_FRAMES,
  PROP_VPU_TRANSCODE,
  PROP_VPU_BITCONVER,
  PROP_VPU_BITSEGMENT,
  PROP_VPU_YUV_BACK,
  PROP_VPU_WIDTH,
  PROP_VPU_HEIGHT
};

static void gst_ams_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_ams_dec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);


static gboolean gst_ams_dec_start (GstVideoDecoder * decoder);

static gboolean gst_ams_dec_stop (GstVideoDecoder * decoder);

static gboolean gst_ams_dec_set_format (GstVideoDecoder * decoder,
    GstVideoCodecState * state);

  
static GstFlowReturn gst_ams_dec_finish (GstVideoDecoder * decoder);

static GstFlowReturn gst_ams_dec_drain (GstVideoDecoder * decoder);

static gboolean gst_ams_dec_flush (GstVideoDecoder * decoder);

static GstFlowReturn gst_ams_dec_getframes (GstVideoDecoder * decoder, int finished);

static GstFlowReturn gst_ams_dec_getframe (GstVideoDecoder * decoder, gint *gotframe);

static GstFlowReturn
gst_ams_dec_handle_frame (GstVideoDecoder * decoder,
    GstVideoCodecFrame * frame);

static gboolean gst_ams_dec_decide_allocation (GstVideoDecoder * decoder,
    GstQuery * query);


static GstFlowReturn gst_ams_dec_handle_resolution (GstVideoDecoder * decoder, ams_frame_t *vpu_frame);

#if 0
static GstFlowReturn gst_ams_dec_handle_resolution_change (GstVideoDecoder * decoder, ams_frame_t *vpu_frame);
#endif

static GstFlowReturn gst_ams_dec_frame_to_buffer (GstAMSDec *dec, const ams_frame_t * frame, GstBuffer * buffer);

#define parent_class gst_ams_dec_parent_class
G_DEFINE_TYPE (GstAMSDec, gst_ams_dec, GST_TYPE_VIDEO_DECODER);

static void
gst_ams_dec_class_init (GstAMSDecClass * klass)
{
  GObjectClass *gobject_class;
  GstVideoDecoderClass *base_video_decoder_class;

  gobject_class = G_OBJECT_CLASS (klass);
  base_video_decoder_class = GST_VIDEO_DECODER_CLASS (klass);

  gobject_class->set_property = gst_ams_dec_set_property;
  gobject_class->get_property = gst_ams_dec_get_property;

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

  g_object_class_install_property (gobject_class, PROP_VPU_SKIP_FRAMES,
      g_param_spec_int ("skip-frames", "number of skip frames",
          "designate save one frame of every skip frames(-1:all, 0:IDR, >0:skipframes and IDR), default (-1)",
          -1, 128, -1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VPU_WIDTH,
      g_param_spec_uint ("width", "Picture width",
          "downscaler Picture width",
          0, 8192, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VPU_HEIGHT,
      g_param_spec_uint ("height", "Picture height",
          "downscaler Picture height",
          0, 8192, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VPU_TRANSCODE,
      g_param_spec_boolean ("transcode", "decoder part of transcode",
          "Enable transcode", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VPU_BITCONVER,
      g_param_spec_boolean ("bit-conver", "bit convert from 10 to 8",
          "Enable bitconver", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VPU_BITSEGMENT,
      g_param_spec_boolean ("bit-segment", "bit stream use segment mode",
          "Enable bitsegment", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VPU_YUV_BACK,
      g_param_spec_boolean ("yuv-back", "read yuv data back from vpu",
          "Enable yuvback", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  base_video_decoder_class->start = GST_DEBUG_FUNCPTR (gst_ams_dec_start);
  base_video_decoder_class->stop = GST_DEBUG_FUNCPTR (gst_ams_dec_stop);
  base_video_decoder_class->finish = GST_DEBUG_FUNCPTR (gst_ams_dec_finish);
  base_video_decoder_class->drain = GST_DEBUG_FUNCPTR (gst_ams_dec_drain);
  base_video_decoder_class->flush = GST_DEBUG_FUNCPTR (gst_ams_dec_flush);
  base_video_decoder_class->set_format =
      GST_DEBUG_FUNCPTR (gst_ams_dec_set_format);
  base_video_decoder_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_ams_dec_handle_frame);;
  base_video_decoder_class->decide_allocation =
      GST_DEBUG_FUNCPTR (gst_ams_dec_decide_allocation);

  klass->open  = NULL;
  klass->close = NULL;
  klass->predecode = NULL;

  GST_DEBUG_CATEGORY_INIT (gst_amsdec_debug, "amsdec", 0, "AMS Decoder");
  gst_type_mark_as_plugin_api (GST_TYPE_AMS_DEC, 0);
}

static void
gst_ams_dec_init (GstAMSDec * dec)
{
    GstVideoDecoder *decoder = GST_VIDEO_DECODER (dec);
    GST_DEBUG_OBJECT (dec, "init");
    dec->decoder      = ams_create_codec_context0(AMS_CODEC_ID_H264);
    dec->decoder->board_id = -1;
    dec->decoder->coreidx  = -1;
    dec->decoder->width    = 0;
    dec->decoder->height   = 0;
    dec->decoder->transcode_flag = 0;
    dec->decoder->bitConver = 0;
    dec->decoder->bitstreamMode = 2;
    dec->decoder->decode_return_yuv_flag = 0;

    dec->input_state  = NULL;
    dec->output_state = NULL;
    dec->skipframes   = -1;
    dec->header.capacity = 512;
    dec->header.size     = 0;
    dec->header.data     = NULL;
    gst_video_decoder_set_packetized (decoder, TRUE);

    gst_video_decoder_set_needs_format (decoder, TRUE);
    gst_video_decoder_set_use_default_pad_acceptcaps (decoder, TRUE);
    GST_PAD_SET_ACCEPT_TEMPLATE (GST_VIDEO_DECODER_SINK_PAD (decoder));
}

static void
gst_ams_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAMSDec *dec = NULL;

  g_return_if_fail (GST_IS_AMS_DEC (object));
  dec = GST_AMS_DEC (object);

  GST_DEBUG_OBJECT (object, "set_property");

  switch (prop_id) {
    case PROP_VPU_BOARD_IDX:
      dec->decoder->board_id =  g_value_get_int(value);
      break;
    case PROP_VPU_CORE_IDX:
      dec->decoder->coreidx =  g_value_get_int(value);
      break;
    case PROP_VPU_SKIP_FRAMES:
      dec->skipframes       =  g_value_get_int(value);
      break;
    case PROP_VPU_WIDTH:
      dec->decoder->width =  g_value_get_uint(value);
      break;
    case PROP_VPU_HEIGHT:
      dec->decoder->height =  g_value_get_uint(value);
      break;
    case PROP_VPU_TRANSCODE:
      dec->decoder->transcode_flag = g_value_get_boolean (value);//(g_value_get_boolean (value) ? 1 : 0);
      break;
    case PROP_VPU_BITCONVER:
      dec->decoder->bitConver = g_value_get_boolean (value);
      break;
    case PROP_VPU_BITSEGMENT:
      dec->decoder->bitstreamMode = (g_value_get_boolean (value) ? 0 : 2);
      break;
    case PROP_VPU_YUV_BACK:
      dec->decoder->decode_return_yuv_flag = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_ams_dec_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstAMSDec *dec = NULL;

  g_return_if_fail (GST_IS_AMS_DEC (object));
  dec = GST_AMS_DEC (object);

  GST_DEBUG_OBJECT (object, "get_property");

  switch (prop_id) {
    case PROP_VPU_BOARD_IDX:
      g_value_set_int (value, dec->decoder->board_id);
      break;
    case PROP_VPU_CORE_IDX:
      g_value_set_int (value, dec->decoder->coreidx);
      break;
    case PROP_VPU_SKIP_FRAMES:
      g_value_set_int (value, dec->skipframes);
      break;
    case PROP_VPU_WIDTH:
      g_value_set_uint (value, dec->decoder->width);
      break;
    case PROP_VPU_HEIGHT:
      g_value_set_uint (value, dec->decoder->height);
      break;
    case PROP_VPU_TRANSCODE:
      g_value_set_boolean (value, dec->decoder->transcode_flag);
      break;
    case PROP_VPU_BITCONVER:
      g_value_set_boolean (value, dec->decoder->bitConver);
      break;
    case PROP_VPU_BITSEGMENT:
      g_value_set_boolean (value, dec->decoder->bitstreamMode == 0);
      break;
    case PROP_VPU_YUV_BACK:
      g_value_set_boolean (value, dec->decoder->decode_return_yuv_flag);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_ams_dec_start (GstVideoDecoder * decoder)
{
  GstAMSDec *dec = GST_AMS_DEC (decoder);
  GstAMSDecClass *klass = GST_AMS_DEC_GET_CLASS (dec);

  GST_DEBUG_OBJECT (dec, "start");
  if (dec->header.data == NULL) {
      dec->header.data = g_slice_alloc(dec->header.capacity);
  }
  if (klass->open != NULL) {
     return klass->open (dec);
  }

  return TRUE;
}

static gboolean
gst_ams_dec_stop (GstVideoDecoder * base_video_decoder)
{
  GstAMSDec *dec = GST_AMS_DEC (base_video_decoder);
  GstAMSDecClass *klass = GST_AMS_DEC_GET_CLASS (dec);

  GST_DEBUG_OBJECT (dec, "stop");

  if (dec->input_state) {
    gst_video_codec_state_unref (dec->input_state);
    dec->input_state = NULL;
  }

  if (dec->output_state) {
    gst_video_codec_state_unref (dec->output_state);
    dec->output_state = NULL;
  }

  dec->header.size     = 0;
  if (dec->header.data != NULL)
      g_slice_free1(dec->header.capacity, dec->header.data);
  dec->header.data     = NULL;

  if (dec->decoder != NULL) {
      ams_decode_close(dec->decoder);
      ams_free_codec_context(dec->decoder);
      dec->decoder = NULL;
  }

  if (klass->close != NULL) {
     return klass->close (dec);
  }
  return TRUE;
}

static gboolean
gst_ams_dec_set_format (GstVideoDecoder * decoder, GstVideoCodecState * state)
{
  GstAMSDec *dec = GST_AMS_DEC (decoder);

  GST_DEBUG_OBJECT (dec, "set_format");

  GST_DEBUG_OBJECT (dec, "input caps: %" GST_PTR_FORMAT, state->caps);

  if (dec->input_state) {
    gst_video_codec_state_unref (dec->input_state);
    dec->input_state = NULL;
  }
  dec->input_state = gst_video_codec_state_ref (state);

  if (dec->output_state) {
    gst_video_codec_state_unref (dec->output_state);
    dec->output_state = NULL;
  }

  return TRUE;
}
  
static GstFlowReturn gst_ams_dec_finish (GstVideoDecoder * decoder) {
    GstAMSDec *dec = GST_AMS_DEC (decoder);
    GST_DEBUG_OBJECT (dec, "finish");
    //printf("(%s:%s:%d) finish\n", __FILE__,__FUNCTION__,__LINE__);
    return gst_ams_dec_getframes(decoder, 1);
}

static GstFlowReturn gst_ams_dec_drain (GstVideoDecoder * decoder) {
    GstAMSDec *dec = GST_AMS_DEC (decoder);
    GST_DEBUG_OBJECT (dec, "drain");
    //printf("(%s:%s:%d) drain\n", __FILE__,__FUNCTION__,__LINE__);
    if (dec->output_state == NULL) {
        return GST_FLOW_OK;
    }
    return gst_ams_dec_getframes(decoder, 0);
}

static gboolean
gst_ams_dec_flush (GstVideoDecoder * decoder) {
    GstAMSDec *dec = GST_AMS_DEC (decoder);
    GST_DEBUG_OBJECT (dec, "flush");
    //printf("(%s:%s:%d) flush\n", __FILE__,__FUNCTION__,__LINE__);
    if (gst_ams_dec_getframes(decoder, 1) == GST_FLOW_OK) 
      return TRUE;
    return FALSE;
}

static GstFlowReturn gst_ams_dec_frame_to_buffer (GstAMSDec *dec, const ams_frame_t * img, GstBuffer * buffer) {
  int deststride, srcstride, height, width, line, comp;
  guint8 *dest = NULL, *src = NULL;
  GstVideoFrame frame;
  GstVideoInfo *info = &dec->output_state->info;

  if (!gst_video_frame_map (&frame, info, buffer, GST_MAP_WRITE)) {
    GST_ERROR_OBJECT (dec, "Could not map video buffer");
    return GST_FLOW_ERROR;
  }

  for (comp = 0; comp < 3; comp++) {
    dest = GST_VIDEO_FRAME_COMP_DATA (&frame, comp);
    src = img->data[comp];
    width = GST_VIDEO_FRAME_COMP_WIDTH (&frame, comp)
        * GST_VIDEO_FRAME_COMP_PSTRIDE (&frame, comp);
    height = GST_VIDEO_FRAME_COMP_HEIGHT (&frame, comp);
    deststride = GST_VIDEO_FRAME_COMP_STRIDE (&frame, comp);
    //srcstride = img->linesize[comp];
    srcstride = GST_AMS_ALIGN(img->width, 32);

    if (img->pix_fmt == AMS_PIX_FMT_YUV420P10LE) {
        srcstride = 2 * srcstride;
    }

    if(comp != 0) {
       srcstride = srcstride / 2;
    }

    GST_DEBUG_OBJECT (dec, "dest(width:height:stride)->(%d:%d:%d) - src(width:height:stride)->(%d:%d:%d)",
       width, height, deststride, img->width, img->height, srcstride);

    if (srcstride == deststride) {
      GST_TRACE_OBJECT (dec, "Stride matches. Comp %d: %d, copying full plane", comp, srcstride);
      memcpy (dest, src, srcstride * height);
      continue;
    }

    GST_TRACE_OBJECT (dec, "Stride mismatch. Comp %d: %d != %d, copying line by line.", comp, srcstride, deststride);
    for (line = 0; line < height; line++) {
        memcpy (dest, src, width);
        dest += deststride;
        src += srcstride;
    }
  }

  gst_video_frame_unmap (&frame);
  return GST_FLOW_OK;
}

static GstFlowReturn
gst_ams_dec_handle_frame (GstVideoDecoder * decoder, GstVideoCodecFrame * frame) {
  GstAMSDec *dec = GST_AMS_DEC (decoder);
  GstFlowReturn ret = GST_FLOW_OK;
  GstMapInfo minfo;
  GstAMSDecClass *klass = GST_AMS_DEC_GET_CLASS (dec);
  ams_packet_t packet;
  ams_frame_t* vpu_frame = NULL;
  int retCode = 0;
  gint preCode = -10;
  guint retry_count = 0;
  const guint retry_threshold = 1000;
  GST_DEBUG_OBJECT (dec, "handle_frame");

  if (!gst_buffer_map (frame->input_buffer, &minfo, GST_MAP_READ)) {
        GST_ERROR_OBJECT (decoder, "Failed to map input buffer");
        gst_video_codec_frame_unref (frame);
        return GST_FLOW_ERROR;
  }

  GST_DEBUG_OBJECT (dec, "NALU:%02x %02x %02x %02x %02x %02x",
   minfo.data[0], minfo.data[1], minfo.data[2], minfo.data[3], minfo.data[4], minfo.data[5]);
  
  if (klass->predecode) {
      preCode = klass->predecode(dec, minfo.data, minfo.size);
      if (preCode < 0) {
          gst_buffer_unmap (frame->input_buffer, &minfo);
          gst_video_decoder_drop_frame (decoder, frame);
          return GST_FLOW_OK;
      }

      if (preCode == 0) {// IDR frame
          packet.size = 0;
          packet.data = g_slice_alloc(dec->header.size + minfo.size);
          memcpy(packet.data + packet.size, dec->header.data, dec->header.size);
          packet.size += dec->header.size;
          memcpy(packet.data + packet.size, minfo.data, minfo.size);
          packet.size += minfo.size;
      } else {
          packet.data = minfo.data;
          packet.size = minfo.size;
      }
  } else {
    packet.data = minfo.data;
    packet.size = minfo.size;
  }
  
  GST_DEBUG_OBJECT (dec, "NALU send:%02x %02x %02x %02x %02x %02x %d",
   packet.data[0], packet.data[1], packet.data[2], packet.data[3], packet.data[4], packet.data[5], preCode);

  do {
    gint gotframe = 0;
    retCode = ams_decode_send_packet(dec->decoder, &packet);
    if(retCode >= 0){
        break;
    }

    ret = gst_ams_dec_getframe(decoder, &gotframe);
    if ((ret == GST_FLOW_ERROR) || (ret == GST_FLOW_EOS) || (gotframe < 0)) {// data eos or process wrong exit
        if (preCode == 0) {
          if (packet.data != NULL) {
             g_slice_free1(packet.size, packet.data);
          }
          dec->header.size = 0;
        }
        gst_buffer_unmap (frame->input_buffer, &minfo);
        gst_video_codec_frame_unref (frame);
        return ret;
    }
    
    if ((ret == GST_FLOW_OK) && (gotframe == 1)) {
        continue;
    }

    if (retry_count > retry_threshold) {
      GST_ERROR_OBJECT (decoder, "Give up");
      ret = GST_FLOW_ERROR;
      break;
    }
    ++retry_count;
    /* Magic number 1ms */
    g_usleep (5000);
  } while(retCode < 0);
  
  if (preCode == 0) {
      if (packet.data != NULL) {
         g_slice_free1(packet.size, packet.data);
      }
  }

  gst_buffer_unmap (frame->input_buffer, &minfo);
  vpu_frame = ams_decode_receive_frame(dec->decoder);
  if (vpu_frame == NULL) {
    // need more data
    gst_video_codec_frame_unref (frame);
    GST_TRACE_OBJECT (dec, "ams_decode_receive_frame get no frame");
    return GST_FLOW_OK;
  }
  
  if (vpu_frame->last_frame_flag) {
      gst_video_decoder_drop_frame (decoder, frame);
      GST_DEBUG_OBJECT (dec, "get last frame, at EOS");
      ams_free_frame(vpu_frame);
      return GST_FLOW_EOS;
  }

  if (dec->skipframes > 0) {
    if (((vpu_frame->idx % dec->skipframes) == 0)
        || ((vpu_frame->frame_type == FRAME_TYPE_I)
        || (vpu_frame->frame_type == FRAME_TYPE_IDR))) {
        GST_DEBUG_OBJECT (dec, "save %d frame when enable skip frame", vpu_frame->idx);
    } else {
        gst_video_decoder_drop_frame (decoder, frame);
        GST_TRACE_OBJECT (dec, "drop %d frame when enable skip frame", vpu_frame->idx);
        ams_free_frame(vpu_frame);
        return GST_FLOW_OK;
    }
  }

  if (ams_frame_is_hwframe(vpu_frame)) {
      vpu_frame = ams_decode_retrieve_frame(dec->decoder, vpu_frame);
      if (vpu_frame == NULL) {
        gst_video_decoder_drop_frame (decoder, frame);
        GST_ERROR_OBJECT (decoder, "ams_decode_retrieve_frame get no frame");
        return GST_FLOW_ERROR;// get frame, but get yuv wrong
      }
  }

  GST_DEBUG_OBJECT (dec, "get %d frame yuv", vpu_frame->idx);
  gst_video_codec_frame_unref (frame);//  frame = gst_video_decoder_get_frame (decoder, vpu_frame->idx);
  frame = gst_video_decoder_get_oldest_frame(decoder);
  if (frame == NULL) {
    GST_ERROR_OBJECT (decoder, "Failed to get %d video frame", vpu_frame->idx);
    ams_free_frame(vpu_frame);
    return GST_FLOW_OK;
  }

  ret = gst_ams_dec_handle_resolution(decoder, vpu_frame);
  if (ret != GST_FLOW_OK) {
      ams_free_frame(vpu_frame);
      gst_video_codec_frame_unref (frame);
      return ret;
  }

  ret = gst_video_decoder_allocate_output_frame (decoder, frame);
  if (ret != GST_FLOW_OK) {
      GST_ERROR_OBJECT (decoder, "Failed to get %d video output frame", vpu_frame->idx);
      ams_free_frame(vpu_frame);
      gst_video_codec_frame_unref (frame);
      return ret;
  }

  ret = gst_ams_dec_frame_to_buffer (dec, vpu_frame, frame->output_buffer);
  ams_free_frame(vpu_frame);
  if (ret != GST_FLOW_OK) {
      gst_video_codec_frame_unref (frame);
      return ret;
  }

  GST_TRACE_OBJECT (dec, "finish %d frame", vpu_frame->idx);
  return  gst_video_decoder_finish_frame (decoder, frame);
}

static gboolean
gst_ams_dec_decide_allocation (GstVideoDecoder * decoder, GstQuery * query) {
  GstAMSDec *dec = GST_AMS_DEC (decoder);
  GstBufferPool *pool = NULL;
  GstStructure *config = NULL;

  GST_DEBUG_OBJECT (dec, "decide_allocation");
  if (!GST_VIDEO_DECODER_CLASS (parent_class)->decide_allocation (decoder, query))
    return FALSE;

  g_assert (gst_query_get_n_allocation_pools (query) > 0);
  gst_query_parse_nth_allocation_pool (query, 0, &pool, NULL, NULL, NULL);
  g_assert (pool != NULL);

  config = gst_buffer_pool_get_config (pool);
  if (gst_query_find_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL)) {
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_VIDEO_META);
  }
  gst_buffer_pool_set_config (pool, config);
  gst_object_unref (pool);

  return TRUE;
}

static GstFlowReturn gst_ams_dec_handle_resolution(GstVideoDecoder * decoder, ams_frame_t *vpu_frame) {
  GstAMSDec *dec = GST_AMS_DEC (decoder);
  GstVideoInfo *info = NULL;
  GstVideoCodecState *new_output_state = NULL;
  GstVideoFormat fmt = GST_VIDEO_FORMAT_I420;
  if (vpu_frame->pix_fmt == AMS_PIX_FMT_YUV420P10LE) {
      fmt = GST_VIDEO_FORMAT_I420_10LE;
  }
  if(dec->output_state == NULL) {
    dec->output_state = gst_video_decoder_set_output_state (decoder, fmt, vpu_frame->width, vpu_frame->height, dec->input_state);
    if (!gst_video_decoder_negotiate (decoder)) {
      GST_ERROR_OBJECT (dec, "Failed to negotiate with downstream elements");
      gst_video_codec_state_unref (dec->output_state);
      dec->output_state = NULL;
      return GST_FLOW_NOT_NEGOTIATED;
    }
    return GST_FLOW_OK;
  }

  info = &dec->output_state->info;
  if (GST_VIDEO_INFO_WIDTH (info) != vpu_frame->width
      || GST_VIDEO_INFO_HEIGHT (info) != vpu_frame->height) {
    GST_DEBUG_OBJECT (dec,
        "Changed output resolution was %d x %d now is got %u x %u (display %u x %u)",
        GST_VIDEO_INFO_WIDTH (info), GST_VIDEO_INFO_HEIGHT (info), vpu_frame->coded_width,
        vpu_frame->coded_height, vpu_frame->width, vpu_frame->height);

    new_output_state =
        gst_video_decoder_set_output_state (decoder, fmt, vpu_frame->width, vpu_frame->height, dec->output_state);
    if (dec->output_state) {
      gst_video_codec_state_unref (dec->output_state);
    }

    dec->output_state = new_output_state;
  }
  
  return GST_FLOW_OK;
}

#if 0
static GstFlowReturn gst_ams_dec_handle_resolution_change (GstVideoDecoder * decoder, ams_frame_t *vpu_frame) {
  GstAMSDec *dec = GST_AMS_DEC (decoder);
  GstVideoInfo *info;
  GstVideoCodecState *new_output_state;
  GstVideoAlignment alig;
  GstVideoFormat fmt = GST_VIDEO_FORMAT_I420;
  if (vpu_frame->pix_fmt == AMS_PIX_FMT_YUV420P10LE) {
      fmt = GST_VIDEO_FORMAT_I420_10LE;
  }
  if(dec->output_state == NULL) {
    dec->output_state = gst_video_decoder_set_output_state (decoder, fmt, vpu_frame->width, vpu_frame->height, dec->input_state);
    info = &(dec->output_state->info);
    gst_video_alignment_reset (&alig);
    //alig.padding_left = 0;
    alig.padding_right = vpu_frame->coded_width - vpu_frame->width;
    //alig.padding_top = 0;
    alig.padding_bottom = vpu_frame->coded_height - vpu_frame->height;
    gst_video_info_align (info, &alig);
    if (!gst_video_decoder_negotiate (decoder)) {
      GST_ERROR_OBJECT (dec,
          "Failed to negotiate with downstream elements");
      gst_video_codec_state_unref (dec->output_state);
      dec->output_state = NULL;
      return GST_FLOW_NOT_NEGOTIATED;
    }
    return GST_FLOW_OK;
  }

  info = &dec->output_state->info;
  if (GST_VIDEO_INFO_WIDTH (info) != vpu_frame->width
      || GST_VIDEO_INFO_HEIGHT (info) != vpu_frame->height) {
    GST_DEBUG_OBJECT (dec,
        "Changed output resolution was %d x %d now is got %u x %u (display %u x %u)",
        GST_VIDEO_INFO_WIDTH (info), GST_VIDEO_INFO_HEIGHT (info), vpu_frame->coded_width,
        vpu_frame->coded_height, vpu_frame->width, vpu_frame->height);

    new_output_state =
        gst_video_decoder_set_output_state (decoder, fmt, vpu_frame->width, vpu_frame->height, dec->output_state);
    if (dec->output_state) {
      gst_video_codec_state_unref (dec->output_state);
    }

    dec->output_state = new_output_state;
    info = &(dec->output_state->info);
    gst_video_alignment_reset (&alig);
    //alig.padding_left = 0;
    alig.padding_right = vpu_frame->coded_width - vpu_frame->width;
    //alig.padding_top = 0;
    alig.padding_bottom = vpu_frame->coded_height - vpu_frame->height;
    gst_video_info_align (info, &alig);
  }
  
  return GST_FLOW_OK;
}
#endif

static GstFlowReturn gst_ams_dec_getframes (GstVideoDecoder * decoder, int finished) {
  GstAMSDec *dec = GST_AMS_DEC (decoder);
  GstFlowReturn ret = GST_FLOW_OK;
  ams_frame_t* vpu_frame = NULL;
  GstVideoCodecFrame * frame = NULL;
  guint retry_count = 0;
  const guint retry_threshold = 2000;

  if (finished) {
    ams_packet_t packet;
    int retCode = 0;
    packet.data = 0;
    packet.size = 0;
    //printf("(%s:%s:%d)\n", __FILE__,__FUNCTION__,__LINE__);
    do {
        gint gotframe = 0;
        retCode = ams_decode_send_packet(dec->decoder, &packet);
        if (retCode >= 0)
            break;
        ret = gst_ams_dec_getframe(decoder, &gotframe);
        if ((ret == GST_FLOW_ERROR) || (ret == GST_FLOW_EOS) || (gotframe < 0)) {// data eos or process wrong exit
            return ret;
        }
        
        if ((ret == GST_FLOW_OK) && (gotframe == 1)) {
            continue;
        }

        if (retry_count > retry_threshold) {
          GST_ERROR_OBJECT (dec, "Give up");
          ret = GST_FLOW_ERROR;
          break;
        }
        ++retry_count;
        /* Magic number 1ms */
        g_usleep (5000);
    }while(retCode < 0);
  }

  retry_count = 0;
  while (1) {
    vpu_frame = ams_decode_receive_frame(dec->decoder);
    if (vpu_frame == NULL) {
        // need more data
        if (retry_count > retry_threshold) {
           GST_ERROR_OBJECT (dec, "Give up");
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
    if (vpu_frame->last_frame_flag) {
      GST_DEBUG_OBJECT (dec, "get last frame, at EOS");
      //printf("(%s:%s:%d) get last frame, at EOS\n", __FILE__,__FUNCTION__,__LINE__);
      ams_free_frame(vpu_frame);
      return GST_FLOW_OK;
    }

    if (dec->skipframes > 0) {
        if (((vpu_frame->idx % dec->skipframes) == 0) || ((vpu_frame->frame_type == FRAME_TYPE_I) || (vpu_frame->frame_type == FRAME_TYPE_IDR))) {
            GST_DEBUG_OBJECT (dec, "save %d frame when enable skip frame", vpu_frame->idx);
        } else {
            GST_TRACE_OBJECT (dec, "drop %d frame when enable skip frame", vpu_frame->idx);
            ams_free_frame(vpu_frame);
            continue;
        }
    }

    if (ams_frame_is_hwframe(vpu_frame)) {
        vpu_frame = ams_decode_retrieve_frame(dec->decoder, vpu_frame);
        if (vpu_frame == NULL) {
            GST_ERROR_OBJECT (decoder, "ams_decode_retrieve_frame get no frame");
            return GST_FLOW_ERROR;
        }
    }

    GST_DEBUG_OBJECT (dec, "get %d frame yuv", vpu_frame->idx);
    frame = gst_video_decoder_get_oldest_frame(decoder);
    if (frame == NULL) {
        GST_ERROR_OBJECT (decoder, "Failed to get %d video frame", vpu_frame->idx);
        ams_free_frame(vpu_frame);
        continue;
    }

    ret = gst_ams_dec_handle_resolution(decoder, vpu_frame);
    if (ret != GST_FLOW_OK) {
      ams_free_frame(vpu_frame);
      gst_video_codec_frame_unref (frame);
      continue;
    }

    ret = gst_video_decoder_allocate_output_frame (decoder, frame);
    if (ret != GST_FLOW_OK) {
      GST_ERROR_OBJECT (decoder, "Failed to get %d video output frame", vpu_frame->idx);
      ams_free_frame(vpu_frame);
      gst_video_codec_frame_unref (frame);
      continue;
    }

    ret = gst_ams_dec_frame_to_buffer (dec, vpu_frame, frame->output_buffer);
    ams_free_frame(vpu_frame);
    if (ret != GST_FLOW_OK) {
      gst_video_codec_frame_unref (frame);
      continue;
    }

    GST_TRACE_OBJECT (dec, "finish %d frame", vpu_frame->idx);
    gst_video_decoder_finish_frame (decoder, frame);
  }
  return ret;
}

static GstFlowReturn gst_ams_dec_getframe (GstVideoDecoder * decoder, gint *gotframe) {
    GstAMSDec *dec = GST_AMS_DEC (decoder);
    ams_frame_t* vpu_frame = NULL;
    GstVideoCodecFrame * frame = NULL;
    GstFlowReturn ret = GST_FLOW_OK;
    *gotframe = 0;
    vpu_frame = ams_decode_receive_frame(dec->decoder);
    if (vpu_frame == NULL) {
        // need more data
        GST_TRACE_OBJECT (dec, "need wait for frame");
        return GST_FLOW_OK;// continue no get frame
    }

    *gotframe = 1;
    if (vpu_frame->last_frame_flag) {
      GST_DEBUG_OBJECT (dec, "get last frame, at EOS");
      //printf("(%s:%s:%d) get last frame, at EOS\n", __FILE__,__FUNCTION__,__LINE__);
      ams_free_frame(vpu_frame);
      *gotframe = 2;
      return GST_FLOW_EOS;// get last frame EOS
    }

    if (dec->skipframes > 0) {
        if (((vpu_frame->idx % dec->skipframes) == 0) || ((vpu_frame->frame_type == FRAME_TYPE_I) || (vpu_frame->frame_type == FRAME_TYPE_IDR))) {
            GST_DEBUG_OBJECT (dec, "save %d frame when enable skip frame", vpu_frame->idx);
        } else {
            GST_TRACE_OBJECT (dec, "drop %d frame when enable skip frame", vpu_frame->idx);
            ams_free_frame(vpu_frame);
            return GST_FLOW_OK;
        }
    }

    if (ams_frame_is_hwframe(vpu_frame)) {
      vpu_frame = ams_decode_retrieve_frame(dec->decoder, vpu_frame);
      if (vpu_frame == NULL) {
        GST_ERROR_OBJECT (decoder, "ams_decode_retrieve_frame get no frame");
        *gotframe = -1;
        return GST_FLOW_ERROR;
      }
    }

    GST_DEBUG_OBJECT (dec, "get %d frame yuv", vpu_frame->idx);
    frame = gst_video_decoder_get_oldest_frame(decoder);
    if (frame == NULL) {
        GST_ERROR_OBJECT (decoder, "Failed to get %d video frame", vpu_frame->idx);
        ams_free_frame(vpu_frame);
        *gotframe = -2;
        return GST_FLOW_ERROR;// get frame, but get video frame wrong
    }

    ret = gst_ams_dec_handle_resolution(decoder, vpu_frame);
    if (ret != GST_FLOW_OK) {
      ams_free_frame(vpu_frame);
      gst_video_codec_frame_unref (frame);
      *gotframe = -3;
      return GST_FLOW_ERROR;// get frame, get video frame, but resolution wrong
    }

    ret = gst_video_decoder_allocate_output_frame (decoder, frame);
    if (ret != GST_FLOW_OK) {
      GST_ERROR_OBJECT (decoder, "Failed to get %d video output frame", vpu_frame->idx);
      ams_free_frame(vpu_frame);
      gst_video_codec_frame_unref (frame);
      *gotframe = -4;
      return GST_FLOW_ERROR;// get frame, get video frame, but allocate output frame wrong
    }

    ret = gst_ams_dec_frame_to_buffer (dec, vpu_frame, frame->output_buffer);
    ams_free_frame(vpu_frame);
    if (ret != GST_FLOW_OK) {
      gst_video_codec_frame_unref (frame);
      *gotframe = -5;
      return ret;// get frame, get video frame, but copy frame wrong
    }

    GST_TRACE_OBJECT (dec, "finish %d frame", vpu_frame->idx);
    return gst_video_decoder_finish_frame (decoder, frame);
}