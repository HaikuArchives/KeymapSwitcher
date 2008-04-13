#!/bin/sh
MODULE_TITLE="Keymap Switcher"
SRC_DIR=`dirname $0`
APP_NAME=KeymapSwitcher
ADDON_NAME=keymap_switcher
APP_SRC=$SRC_DIR/$APP_NAME
ADDON_SRC=$SRC_DIR/$ADDON_NAME

answer=`alert "Do you really want to install the Keymap Switcher?" "No" "Yes"`
if [ $answer == "No" ]; then
	exit
fi

/boot/beos/bin/install $APP_SRC   /boot/beos/apps/$APP_NAME
/boot/beos/bin/install $ADDON_SRC /boot/beos/apps/$ADDON_NAME
mimeset /boot/beos/apps/$APP_NAME  
mimeset /boot/beos/apps/$ADDON_NAME

ln -s /boot/beos/apps/$APP_NAME "/boot/home/config/be/Desktop Applets/Keymap Switcher"

