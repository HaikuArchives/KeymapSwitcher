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
#include <Catalog.h>
#include <Deskbar.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <OS.h>
#include <Roster.h>
#include <String.h>

#include "DeskView.h"
#include "SettingsWindow.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "SwitcherApplication"


class SwitcherApplication : public BApplication
{
  public:
    SwitcherApplication() : BApplication(APP_SIGNATURE)
	{
	}

    virtual void ReadyToRun(void)
	{
		SettingsWindow* win = new SettingsWindow(false);
		if (win == 0) {
			BAlert* alert = new BAlert("Error",
					B_TRANSLATE("Unable to create Settings configuration window."),
					B_TRANSLATE("OK"), 0, 0, B_WIDTH_AS_USUAL, B_STOP_ALERT);
			alert->Go();
			PostMessage(B_QUIT_REQUESTED);
		} else
			win->Show();
	}
};


int main(int argc, char *argv[])
{
	SwitcherApplication* theApp = new SwitcherApplication();
	theApp->Run();
/*
	add_system_beep_event(BEEP_NAME);
	BDeskbar deskbar;
	entry_ref ref;
	be_roster->FindApp(APP_SIGNATURE, &ref);
	deskbar.AddItem(&ref);
*/
	return B_OK;
}

