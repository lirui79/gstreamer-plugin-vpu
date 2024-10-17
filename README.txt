1. GStreamer 安装
请在要调试GStreamer机器上 安装好 1.20.3
a 可采用软件包安装方法安装
   请在相应软件系统平台获取GStreamer 1.20.3 软件包进行安装
   采用 apt list --installed | grep gstreamer* 查看系统已安装GStreamer软件包版本

b 可采用源码编译安装 GStreamer 1.20.3
  本人采用源码编译安装GStreamer 1.20.3
  采用源码编译安装，最好采用 apt list --installed | grep gstreamer* 查看系统已安装GStreamer软件包版本，如需要确保与GStreamer 1.20.3 完全一致可以采用如下指令卸载软件包
      apt-get purge [package]
      apt-get autoremove [package]

  请执行如下指令安装编译相应软件包，可能还有其它所需软件包，请自行安装
  apt install ssh  vim gedit g++ gcc cmake tig git  curl flex bison

  下载 源码 
    git clone https://github.com/GStreamer/gstreamer.git  或    git clone https://gitlab.freedesktop.org/gstreamer/gstreamer.git
  查看tag
   git tag
  切换 1.20.3
   git checkout 1.20.3
  环境设置
    meson setup --prefix=/usr build
    如需要重新设置编译环境，则  rm build -rf，再执行环境设置，后执行编译即可
  编译
    ninja -C build
  安装
    meson install -C build
    注：安装路径 ubuntu  一般在/usr下 头文件在 /usr/include/gstreamer-1.0  库在/usr/lib/x86_64-linux-gnu/  工具在 /usr/bin/

2. 安装VPU 软件包
   依据软件包提示操作完成VPU软件包安装

3. GStreamer VPU 插件编译
   tar -vxf  gstreamer-plugin.tar.gz
   源码
      gstreamer-plugin
        | -----  gst-app
        |          |
        |          | ------  ams  中为测试demo源码
        |
        |
        | -----  gst-plugin 
        |          |
        |          | ------  ams  中VPU为插件库源码

   cd gstreamer-plugin
   

   gstreamer-plugin此目录下有 README.md 可查看此文件已此文件进行编译
   a 准备环境
     请/etc/bash.bashrc   末尾添加
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/SmarHTC_V6624/lib/pkgconfig/
export PKG_CONFIG_PATH
   b 请在编译目录下执行如重启机器在无需执行如下指令
     source /etc/bash.bashrc
   c 环境设置
       meson setup build
       如需要重新设置编译环境，则   rm build -rf，再执行环境设置，后执行编译即可
   d 执行编译
       ninja -C build
       会在当前目录下 build 目录中生成 gst-app 和 gst-plugin 两个目录如下所示
       build
        | -----  gst-app
        |          |
        |          | ------  ams  中ams-app 为测试demo
        |
        |
        | -----  gst-plugin 
        |          |
        |          | ------  ams  中libgstamscodec.so 为插件库

3. 测试 GStreamer VPU 插件
   请在测试前执行如下指令，假设VPU插件库在/home/stone/gstreamer-vpu/build/gst-plugin/ams/ 目录中
      export GST_PLUGIN_PATH=$GST_PLUGIN_PATH:/home/stone/gstreamer-vpu/build/gst-plugin/ams/
   gstreamer-plugin此目录下有 README.md 可查看此文件已此文件进行测试
   测试会有两种：采用GStreamer tool工具和采用Demo(即上面源码测试demo源码，可以修改重新编译）
   a 采用GStreamer tool工具

gst-launch-1.0 filesrc location=/home/stone/1080.h264 ! parsebin ! amsh264dec ! filesink location=/home/stone/3.yuv
gst-launch-1.0 filesrc location=/home/stone/we_8bit_1920x1080.mp4 ! parsebin ! amsh264dec ! filesink location=/home/stone/4.yuv
gst-launch-1.0 filesrc location=/home/stone/264_352x288_B10_IBPBP_F100.264 ! parsebin ! amsh264dec ! filesink location=/home/stone/5.yuv

GST_DEBUG=4 gst-launch-1.0 filesrc location=/home/stone/264_352x288_B10_IBPBP_F100.264 ! parsebin ! amsh264dec ! filesink location=/home/stone/6.yuv

  抽取关键帧
     gst-launch-1.0 filesrc location=/home/stone/1080.h264 ! parsebin ! amsh264dec skip-frames=0 ! filesink location=/home/stone/7.yuv
  每20帧抽取一帧，且保存关键帧 I帧
     gst-launch-1.0 filesrc location=/home/stone/1080.h264 ! parsebin ! amsh264dec skip-frames=20 ! filesink location=/home/stone/8.yuv

  不保存文件测试
gst-launch-1.0 filesrc location=/home/stone/we_8bit_1920x1080.mp4 ! parsebin ! amsh264dec ! fakesink

   b 采用Demo 测试
      查看帮助
  ./build/gst-app/ams/ams-app -h

  打开GStreamer 日志，
 GST_DEBUG=4 ./build/gst-app/ams/ams-app -b 0 -i /home/stone/1080.h264 -o /home/stone/1.yuv

 抽取关键帧
 ./build/gst-app/ams/ams-app -b 0 -i /home/stone//home/stone/264_352x288_B10_IBPBP_F100.264 -o /home/stone/2.yuv -k

  每20帧抽取一帧，且保存关键帧 I帧
 ./build/gst-app/ams/ams-app -b 0 -i /home/stone/we_8bit_1920x1080.mp4 -o /home/stone/3.yuv -s 30

 不保存文件测试
 ./build/gst-app/ams/ams-app -b 0 -i /home/stone/we_8bit_1920x1080.mp4