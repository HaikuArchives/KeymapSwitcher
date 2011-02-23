// Switcher
// Copyright © 1999-2003 by Stas Maximov
// All rights reserved
//
#ifndef __SETTINGSWINDOW_H
#define __SETTINGSWINDOW_H


#include <ListView.h>
#include <OutlineListView.h>
#include <PictureButton.h>
#include <Resources.h>
#include <Window.h>

#include "Settings.h"


class SettingsWindow: public BWindow {

	// some messages
	enum {
		MSG_KEYMAPS_CHANGED = 0x1002, // indexes are backward compatible!
		MSG_HOTKEY_CHANGED = 0x1004,
		MSG_SAVE_SETTINGS,
		MSG_CLOSE,
		MSG_ABOUT,
		MSG_BEEP_SETUP,
		MSG_ACTIVE_ITEM_DRAGGED = 0x100a,
		MSG_ITEM_DRAGGED,
		MSG_REMOVE_ACTIVE_ITEM,
		MSG_BUTTON_ADD_ITEM = 0x100e,
		MSG_BUTTON_REMOVE_ITEM
	};

	class KeymapItem : public BStringItem {
	public:
		KeymapItem(const char *text, const char *real_name, int32 dir);
		KeymapItem(KeymapItem *item);
		int32 Dir() { return dir; };
		const char * RealName() { return real_name.String(); }
	private:
		int32 dir;
		BString real_name;
	};

	class KeymapListView : public BListView {
	public:
		KeymapListView(BRect r, const char *name);
		virtual bool InitiateDrag(BPoint pt, int32 index, bool wasSelected);
		virtual void MessageReceived(BMessage *message);
	};

	class KeymapOutlineListView : public BOutlineListView {
	public:
		KeymapOutlineListView(BRect r, const char *name);
		virtual bool InitiateDrag(BPoint point, int32 index, bool wasSelected);
	};

	class BMoveButton : public BPictureButton {
		BPicture fPicOff;
		BPicture fPicOn;
		BPicture fPicDisabled;
		uint32   fResIdOff;
		uint32   fResIdOn;
		uint32   fResIdDisabled;
		status_t LoadPicture(BResources *resFrom, BPicture *picTo, uint32 resId);
	public:
		BMoveButton(BRect 		frame, const char *name,  
					uint32		resIdOff, uint32 resIdOn, uint32 resIdDisabled,
					BMessage	*message,
					uint32      behaviour		= B_ONE_STATE_BUTTON,
					uint32		resizingMode 	= B_FOLLOW_LEFT	| B_FOLLOW_TOP,
					uint32		flags			= B_WILL_DRAW	| B_NAVIGABLE);		
		~BMoveButton();

		virtual void AttachedToWindow();
		virtual void GetPreferredSize(float *width, float *height);
	};

	void ShowAboutWindow();
	bool AlreadyInDeskbar();

public:
	SettingsWindow(bool fromDeskbar);
	virtual ~SettingsWindow();
	virtual void MessageReceived(BMessage *msg);
	virtual bool QuitRequested();
	
private:
	Settings *settings;
	bool hotkey_changed;
	bool keymaps_changed;
	bool from_deskbar;
	KeymapListView *selected_list;
	KeymapOutlineListView *available_list;
	BView *parent;
};


#endif	//	__SETTINGSWINDOW_H

