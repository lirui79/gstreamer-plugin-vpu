
#ifndef __GST_AMSH264_ENC_H__
#define __GST_AMSH264_ENC_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <gstamsenc.h>

#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif

G_BEGIN_DECLS

#define GST_TYPE_AMSH264_ENC (gst_amsh264_enc_get_type())
G_DECLARE_FINAL_TYPE (GstAMSH264Enc, gst_amsh264_enc, GST, AMSH264_ENC, GstAMSEnc)

struct _GstAMSH264Enc
{
    GstAMSEnc base_ams_encoder;
};

G_END_DECLS


#endif /* __GST_AMSH264_ENC_H__ */
