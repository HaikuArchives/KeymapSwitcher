// Switcher
// Copyright © 1999-2003 Stas Maximov
// Created by Stas Maximov
// All rights reserved
#ifndef __KeymapSwitcher_H
#define __KeymapSwitcher_H 


#define APP_SIGNATURE "application/x-vnd.KeymapSwitcher"
#define DESKBAR_SIGNATURE "application/x-vnd.Be-TSKB"
#define REPLICANT_NAME "Switcher/Deskbar"
#define BEEP_NAME "Keymap Switch"
#define SOUNDS_PREF_SIGNATURE "application/x-vnd.Haiku-Sounds"
#define INDICATOR_SIGNATURE "application/x-vnd.KeymapSwitcher"

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
#define	VersionMinor  6
#define VersionBuild  0

#define _NUM2STR(_x) #_x
#define NUM2STR(_x) _NUM2STR(_x)

#define VERSION NUM2STR(VersionMajor) "." \
		NUM2STR(VersionMiddle) "." \
		NUM2STR(VersionMinor)


#endif // __KeymapSwitcher_H

