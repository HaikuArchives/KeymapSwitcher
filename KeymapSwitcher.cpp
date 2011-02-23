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
    SwitcherApplication() : BApplication(APP_SIGNATURE),
							fAutoInstallInDeskbar(false),
							fQuitImmediately(false)
	{
	}

    virtual void
	ReadyToRun(void)
	{
		if (fQuitImmediately) {
			Quit();
			return;
		}

		if (fAutoInstallInDeskbar) {
			BDeskbar deskbar;
			
			if (deskbar.HasItem(REPLICANT_NAME)) {
				Quit();
				return;
			}

			if(!deskbar.IsRunning()) {
				fprintf(stderr, B_TRANSLATE("Unable to install keymap indicator. "
									"Deskbar application is not running."));
				Quit();
				return;
			}

			entry_ref ref;
			be_roster->FindApp(APP_SIGNATURE, &ref);
			deskbar.AddItem(&ref);

			Quit();
			return;
		}
		
		
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

	virtual void
	ArgvReceived(int32 argc, char** argv)
	{
		if (argc <= 1)
			return;

		if (strcmp(argv[1], "--help") == 0
			|| strcmp(argv[1], "-h") == 0) {
			const char* str = B_TRANSLATE("KeymapSwitcher options:\n"
					"\t--deskbar\tautomatically add replicant to Deskbar\n"
					"\t--help\t\tprint this info and exit\n");
			printf(str);
			fQuitImmediately = true;
			return;
		}

		if (strcmp(argv[1], "--deskbar") == 0)
			fAutoInstallInDeskbar = true;
	}

  private:
	bool	fAutoInstallInDeskbar;
	bool	fQuitImmediately;
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

