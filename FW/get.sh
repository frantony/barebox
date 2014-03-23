#!/bin/sh

OWRTPREF=https://raw.githubusercontent.com/mirrors/openwrt/master

curl -OL $OWRTPREF/tools/firmware-utils/src/mktplinkfw.c \
	-OL $OWRTPREF/tools/firmware-utils/src/md5.c \
	-OL $OWRTPREF/tools/firmware-utils/src/md5.h

cc -o mktplinkfw mktplinkfw.c md5.c
