#!/bin/sh
MODULE_TITLE="Keymap Switcher"
SRC_DIR=`dirname $0`
APP_NAME=KeymapSwitcher
ADDON_NAME=keymap_switcher
OBJ_DIR=$SRC_DIR/objects.x86-gcc4-release
APP_SRC=$OBJ_DIR/$APP_NAME
ADDON_SRC=$OBJ_DIR/$ADDON_NAME
ADDON_DEST=/boot/common/add-ons/input_server/filters/ 
APP_DEST=/boot/common/bin/ 
CATALOGS_DEST=/boot/common/data/locale/catalogs/
APP_MIME_SIG=x-vnd.Nexus-KeymapSwitcher

answer=`alert "Do you really want to install the Keymap Switcher?" "No" "Yes"`
if [ $answer == "No" ]; then
	exit
fi

hey input_server quit
hey Deskbar quit
sleep 1

/boot/system/bin/install $APP_SRC   $APP_DEST/$APP_NAME
/boot/system/bin/install $ADDON_SRC $ADDON_DEST/$ADDON_NAME
mimeset $APP_DEST/$APP_NAME  
mimeset $ADDON_DEST/$ADDON_NAME

/boot/system/Deskbar &
/boot/system/servers/input_server &
sleep 1
$APP_DEST/$APP_NAME --deskbar

