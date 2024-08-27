project('gst-template', 'c', version : '1.20.3.0', license : 'LGPL')

plugins_install_dir = join_paths(get_option('libdir'), 'gstreamer-1.0')

cc = meson.get_compiler('c')

gst_version = meson.project_version()

api_version = '1.0'

gst_dep = dependency('gstreamer-1.0', version : '>=1.20',
    required : true, fallback : ['gstreamer', 'gst_dep'])
gstbase_dep = dependency('gstreamer-base-1.0', version : '>=1.20',
  fallback : ['gstreamer', 'gst_base_dep'])

gstvideo_dep = dependency('gstreamer-video-1.0', version : '>=1.20',
    fallback : ['gst-plugins-base', 'video_dep'])
    
gstcodecparsers_dep = dependency('gstreamer-codecparsers-1.0', version : '>=1.20',
    fallback : ['gst-plugins-bad', 'gstcodecparsers_dep'])

subdir('gst-app')
subdir('gst-plugin')
