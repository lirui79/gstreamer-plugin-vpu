#!/bin/bash

PNUM=$#

if [ $PNUM -lt 2 ] ;then
   printf "Usage 0:\n\r$0 INPUTFILE CHNUM\n"
   printf "Example 0:\n\r$0 /home/stone/1080.h264 8\n"
   printf "Usage 1:\n\r$0 INPUTFILE OUTPUTDIR CHNUM\n"
   printf "Example 1:\n\r$0 /home/stone/1080.h264 /tmp/ 8\n"
   exit -1   
fi

export GST_PLUGIN_PATH=$GST_PLUGIN_PATH:/home/stone/gstreamer-vpu/build/gst-plugin/ams/

if [ $PNUM -eq 2 ] ;then
    INPUTFILE=$1
    CHNUM=$2
    for ((CHIDX=0; CHIDX < CHNUM; ++CHIDX))
    do
        gst-launch-1.0 filesrc location=${INPUTFILE} ! parsebin ! amsh264dec ! fakesink  > ${CHIDX}.log 2>&1 &
    done
    exit 1
fi

INPUTFILE=$1
OUTPUTDIR=$2
CHNUM=$3

for ((CHIDX=0; CHIDX < CHNUM; ++CHIDX))
do
  nohup gst-launch-1.0 filesrc location=${INPUTFILE} ! parsebin ! amsh264dec ! filesink location=${OUTPUTDIR}/${CHIDX}.yuv > ${OUTPUTDIR}/${CHIDX}.log 2>&1 &
done