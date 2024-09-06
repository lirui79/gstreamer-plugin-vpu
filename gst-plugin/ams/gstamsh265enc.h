
#ifndef __GST_AMSH265_ENC_H__
#define __GST_AMSH265_ENC_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <gstamsenc.h>

#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif

G_BEGIN_DECLS

#define GST_TYPE_AMSH265_ENC (gst_amsh265_enc_get_type())
G_DECLARE_FINAL_TYPE (GstAMSH265Enc, gst_amsh265_enc, GST, AMSH265_ENC, GstAMSEnc)

struct _GstAMSH265Enc
{
    GstAMSEnc base_ams_encoder;
};

G_END_DECLS


#endif /* __GST_AMSH265_ENC_H__ */
