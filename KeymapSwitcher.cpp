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


#include "KeymapSwitcher.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Deskbar.h>
#include <FindDirectory.h>
#include <OS.h>
#include <Roster.h>
#include <String.h>

#include "DeskView.h"
#include "SettingsWindow.h"


int main(int argc, char *argv[])
{
	add_system_beep_event(BEEP_NAME);
	BDeskbar deskbar;
	entry_ref ref;
	be_roster->FindApp(APP_SIGNATURE, &ref);
	deskbar.AddItem(&ref);

	return B_OK;
}

