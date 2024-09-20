#!/bin/bash

PLUGIN_FILE=build/gst-plugin/ams/libgstamscodec.so
PLUGIN_PKG=gst-plugin/ams/amscodec.pc
GSTREAM_PATH=/usr/lib/x86_64-linux-gnu/

echo "install AMSCODEC Plugins"

if [ -e ${PLUGIN_FILE} ] ; then
   cp ${PLUGIN_FILE} ${GSTREAM_PATH}/gstreamer-1.0/ -rf
fi

if [ -e ${PLUGIN_PKG} ] ; then
   cp ${PLUGIN_PKG} ${GSTREAM_PATH}/pkgconfig/ -rf
fi
