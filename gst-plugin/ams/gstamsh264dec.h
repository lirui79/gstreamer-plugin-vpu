

#ifndef __GST_AMSH264_DEC_H__
#define __GST_AMSH264_DEC_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif



#include <gst/gst.h>
#include <gst/video/gstvideodecoder.h>
#include <gst/codecparsers/gsth264parser.h>
#include <gstamsdec.h>


#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif


G_BEGIN_DECLS

#define GST_TYPE_AMSH264_DEC (gst_amsh264_dec_get_type())
G_DECLARE_FINAL_TYPE (GstAMSH264Dec, gst_amsh264_dec, GST, AMSH264_DEC, GstAMSDec)

struct _GstAMSH264Dec
{
  GstAMSDec                  base_ams_decoder;
  GstH264NalParser          *parser;
};

G_END_DECLS



#endif /* __GST_AMSH264_DEC_H__ */
