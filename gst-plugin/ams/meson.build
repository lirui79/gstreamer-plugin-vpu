plugin_c_args = ['-DHAVE_CONFIG_H']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'gst-amscodec-plugin')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'GStreamer AMSCODEC Plug-ins')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://gstreamer.freedesktop.org')
configure_file(output : 'config.h', configuration : cdata)



# Plugin ams
ams_sources = [
  'gstamsh264dec.c',
  'gstamsh265dec.c',
  'gstamsh264enc.c',
  'gstamsh265enc.c',
  'gstamsdec.c',
  'gstamsenc.c',
  'plugin.c',
]


#ams_option = get_option('libams_codec')
#ams_dep = dependency('libams_codec', version : '>=1.0.0', required : ams_option)
ams_dep = dependency('libams_codec', version : '>=1.0.0')

gstamscodec = library('gstamscodec',
  ams_sources,
  c_args: plugin_c_args,
  dependencies : [gst_dep, gstbase_dep, gstvideo_dep, gstcodecparsers_dep, ams_dep],
  install : true,
  install_dir : plugins_install_dir,
)
