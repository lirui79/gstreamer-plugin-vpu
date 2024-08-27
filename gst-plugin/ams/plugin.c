
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "ams_api_board.h"
#include "gstamselements.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean ret = FALSE;
  if (ams_codec_dev_init(-1) < 0) {
    GST_ERROR ("Failed to init ams codec library");
    return TRUE;
  }
  
  ret |= GST_ELEMENT_REGISTER (amsh264dec, plugin);

//  ret |= GST_ELEMENT_REGISTER (amsh264enc, plugin);

//  ret |= GST_ELEMENT_REGISTER (amsh265dec, plugin);

//  ret |= GST_ELEMENT_REGISTER (amsh265enc, plugin);

   

  return ret;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    amscodec,
    "GStreamer AMSCODEC plugin",
    plugin_init, PACKAGE_VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
