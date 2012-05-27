// Switcher
// Copyright © 1999-2003 Stas Maximov
// Created by Stas Maximov
// All rights reserved
#ifndef __KeymapSwitcher_H
#define __KeymapSwitcher_H 


#define APP_SIGNATURE "application/x-vnd.Nexus-KeymapSwitcher"
#define DESKBAR_SIGNATURE "application/x-vnd.Be-TSKB"
#define REPLICANT_NAME "Switcher/Deskbar"
#define BEEP_NAME "Keymap switch"

enum {
	MSG_CHANGEKEYMAP = 0x400,
	MSG_UPDATESETTINGS,
	MSG_SHOW_RC_MENU = 0x500
}; 

// add/remove keymap buttons ids 
enum {
	R_ResAddButton = 200,
	R_ResAddButtonPressed = 201,
	R_ResAddButtonDisabled = 202,
	R_ResRemoveButton = 203,
	R_ResRemoveButtonPressed = 204,
	R_ResRemoveButtonDisabled = 205
};

// version defines
#define	VersionMajor  1
#define	VersionMiddle 2
#define	VersionMinor  7
#define VersionBuild  3

#define _NUM2STR(_x) #_x
#define NUM2STR(_x) _NUM2STR(_x)

#define VERSION NUM2STR(VersionMajor) "." \
		NUM2STR(VersionMiddle) "." \
		NUM2STR(VersionMinor) "." \
		NUM2STR(VersionBuild)


#endif // __KeymapSwitcher_H

