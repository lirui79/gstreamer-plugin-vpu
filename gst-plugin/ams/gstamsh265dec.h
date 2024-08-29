

#ifndef __GST_AMSH265_DEC_H__
#define __GST_AMSH265_DEC_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif



#include <gst/gst.h>
#include <gst/video/gstvideodecoder.h>
#include <gst/codecparsers/gsth265parser.h>
#include <gstamsdec.h>


#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif


G_BEGIN_DECLS

#define GST_TYPE_AMSH265_DEC (gst_amsh265_dec_get_type())
G_DECLARE_FINAL_TYPE (GstAMSH265Dec, gst_amsh265_dec, GST, AMSH265_DEC, GstAMSDec)

struct _GstAMSH265Dec
{
  GstAMSDec                  base_ams_decoder;
  GstH265Parser             *parser;
};

G_END_DECLS



#endif /* __GST_AMSH265_DEC_H__ */
