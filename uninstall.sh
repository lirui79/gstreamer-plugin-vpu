#!/bin/bash

PLUGIN_FILE=/usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgstamscodec.so
PLUGIN_PKG=/usr/lib/x86_64-linux-gnu/pkgconfig/amscodec.pc

echo "uninstall AMSCODEC Plugins"

if [ -e ${PLUGIN_FILE} ] ; then
   rm ${PLUGIN_FILE} -rf
fi

if [ -e ${PLUGIN_PKG} ] ; then
   rm ${PLUGIN_PKG} -rf
fi
