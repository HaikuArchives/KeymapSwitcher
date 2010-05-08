#!/bin/sh
MODULE_TITLE="Keymap Switcher"
SRC_DIR=`dirname $0`
APP_NAME=KeymapSwitcher
ADDON_NAME=keymap_switcher
APP_SRC=$SRC_DIR/obj.x86/$APP_NAME
ADDON_SRC=$SRC_DIR/obj.x86/$ADDON_NAME

answer=`alert "Do you really want to install the Keymap Switcher?" "No" "Yes"`
if [ $answer == "No" ]; then
	exit
fi

/boot/system/bin/install $APP_SRC   /boot/common/bin/$APP_NAME
/boot/system/bin/install $ADDON_SRC /boot/common/add-ons/input_server/filters/$ADDON_NAME
mimeset /boot/common/bin/$APP_NAME  
mimeset /boot/common/add-ons/input_server/filters/$ADDON_NAME

#ln -s /boot/system/apps/$APP_NAME "/boot/home/config/be/Desktop Applets/Keymap Switcher"

