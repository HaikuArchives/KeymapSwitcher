/*
 * Copyright (c) 1999-2003 Stas Maximov
 * Copyright 2008, Haiku, Inc. 
 * All Rights Reserved.
 * 
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stas Maximov
 *		Siarzhuk Zharski
 */
#include <Application.h>
#include <Alert.h>
#include <String.h>
#include <FindDirectory.h>
#include <Deskbar.h>
#include <OS.h>
#include <Roster.h>
#include <stdlib.h>
#include <stdio.h>
#include <Beep.h>

#include "KeymapSwitcher.h"
#include "SettingsWindow.h"
#include "DeskView.h"
#include "Resource.h"

int main(int argc, char *argv[])  {
	add_system_beep_event(BEEP_NAME);
	BDeskbar deskbar;
	entry_ref ref;
	be_roster->FindApp(APP_SIGNATURE, &ref);
	deskbar.AddItem(&ref);

	return B_OK;
}

