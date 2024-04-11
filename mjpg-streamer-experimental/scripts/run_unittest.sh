#!/bin/sh
export UPL_PATH=/opt/upl
export LD_LIBRARY_PATH=$UPL_PATH/lib:$LD_LIBRARY_PATH

export TSLIB_CALIBFILE=/etc/pointercal
export TSLIB_CONFFILE='/etc/ts.conf'
export TSLIB_CONSOLEDEVICE='none'
export TSLIB_FBDEVICE='/dev/fb1'
export TSLIB_PLUGINDIR='/usr/lib/ts'
export TSLIB_TSDEVICE="/dev/input/event"$(ls /sys/devices/platform/soc/3f204000.spi/spi_master/spi0/spi0.1/input | awk -F "input" '{print $2}')
export QT_QPA_PLATFORM="linuxfb:fb=/dev/fb1:size=480x320"
#export QT_DEBUG_PLUGINS=1
export QT_QPA_GENERIC_PLUGINS=Tslib
export QT_QPA_FONTDIR=/opt/upl/font

$UPL_PATH/mjpg_streamer -i "input_uvc_upl.so -f 15 -r 1280x720 -y" -o "output_gauger.so" -o "test_gauger.so"
