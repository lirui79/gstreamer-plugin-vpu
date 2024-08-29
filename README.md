# GStreamer-VPU

VPU GStreamer plugin

meson setup build
ninja -C build


pkg-config --list-all
if no find libams_codec
please   add  
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/SmarHTC_V6624/lib/pkgconfig/
export PKG_CONFIG_PATH
into /etc/bash.bashrc
and 
source /etc/bash.bashrc 

test before, please run cmd
export GST_PLUGIN_PATH=$GST_PLUGIN_PATH:/home/stone/gstreamer-vpu/build/gst-plugin/ams/

gst tool test:

gst-launch-1.0 filesrc location=/home/stone/1080.h264 ! h264parse ! openh264dec ! filesink location=/home/stone/1.yuv

gst-launch-1.0 filesrc location=/home/stone/1080.h264 ! h264parse ! amsh264dec ! filesink location=/home/stone/2.yuv
gst-launch-1.0 filesrc location=/home/stone/1080.h264 ! parsebin ! amsh264dec ! filesink location=/home/stone/3.yuv
gst-launch-1.0 filesrc location=/home/stone/we_8bit_1920x1080.mp4 ! parsebin ! amsh264dec ! filesink location=/home/stone/4.yuv
gst-launch-1.0 filesrc location=/home/stone/264_352x288_B10_IBPBP_F100.264 ! parsebin ! amsh264dec ! filesink location=/home/stone/5.yuv

GST_DEBUG=4 gst-launch-1.0 filesrc location=/home/stone/264_352x288_B10_IBPBP_F100.264 ! parsebin ! amsh264dec ! filesink location=/home/stone/6.yuv

  KEYFRAME is IDR saved
     gst-launch-1.0 filesrc location=/home/stone/1080.h264 ! parsebin ! amsh264dec skip-frames=0 ! filesink location=/home/stone/7.yuv
  SKIP FRAME  is 20
     gst-launch-1.0 filesrc location=/home/stone/1080.h264 ! parsebin ! amsh264dec skip-frames=20 ! filesink location=/home/stone/8.yuv

demo test:
   Help
  ./build/gst-app/ams/ams-app -h

  Enable log
   GST_DEBUG=4 ./build/gst-app/ams/ams-app -b 0 -i /home/stone/1080.h264 -o /home/stone/1.yuv

  KEYFRAME is IDR saved
 ./build/gst-app/ams/ams-app -b 0 -i /home/stone/264_352x288_B10_IBPBP_F100.264 -o /home/stone/2.yuv -k

  SKIP FRAME  is 30
 ./build/gst-app/ams/ams-app -b 0 -i /home/stone/we_8bit_1920x1080.mp4 -o /home/stone/3.yuv -s 30


not save yuv file
   gst-launch-1.0 filesrc location=/home/stone/we_8bit_1920x1080.mp4 ! parsebin ! amsh264dec ! fakesink
   ./build/gst-app/ams/ams-app -b 0 -i /home/stone/1080.h264
