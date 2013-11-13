// Switcher
// Copyright © 1999-2003 by Stas Maximov
// All rights reserved
//
#ifndef __SETTINGSWINDOW_H
#define __SETTINGSWINDOW_H


#include <stdio.h>

#include <Box.h>
#include <CheckBox.h>
#include <FindDirectory.h>
#include <ListView.h>
#include <MenuField.h>
#include <OutlineListView.h>
#include <PictureButton.h>
#include <Resources.h>
#include <Window.h>

#include "Settings.h"

class SettingsWindow: public BWindow
{
	// private Settings Window messages
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
		MSG_BUTTON_REMOVE_ITEM,
		MSG_CHECK_REMAP,
		MSG_CHECK_SYSTEM_WIDE,
//		MSG_CHECK_USE_ACTIVE,
		MSG_LIST_SEL_CHANGE
	};

	class KeymapItem : public BStringItem
   	{
	public:
		KeymapItem(const char *text, const char *real_name, int32 dir);
		KeymapItem(KeymapItem *item);
		int32 Dir() { return dir; };
		const char * RealName() { return real_name.String(); }
	private:
		int32 dir;
		BString real_name;
	};

	static int CompareKeymapItems(const void* left, const void* right);

	class KeymapListView : public BListView
   	{
	public:
		KeymapListView(BRect r, const char *name);
		virtual bool InitiateDrag(BPoint pt, int32 index, bool wasSelected);
		virtual void MessageReceived(BMessage *message);

		void ResetKeymapsList(const Settings* settings);
		void ReadKeymapsList(Settings* settings);
		virtual void SelectionChanged();
		virtual void MouseDown(BPoint point);
	};

	class KeymapOutlineListView : public BOutlineListView
   	{
	public:
		KeymapOutlineListView(BRect r, const char *name);
		virtual bool InitiateDrag(BPoint point, int32 index, bool wasSelected);

		void PopulateTheTree();
		virtual void SelectionChanged();
		virtual void MouseDown(BPoint point);
	};

	class MoveButton : public BPictureButton
   	{
		BPicture fPicOff;
		BPicture fPicOn;
		BPicture fPicDisabled;
		uint32   fResIdOff;
		uint32   fResIdOn;
		uint32   fResIdDisabled;
		status_t LoadPicture(BResources *resFrom, BPicture *picTo, uint32 resId);
	public:
		MoveButton(BRect 		frame, const char *name,  
					uint32		resIdOff, uint32 resIdOn, uint32 resIdDisabled,
					BMessage	*message,
					uint32      behaviour		= B_ONE_STATE_BUTTON,
					uint32		resizingMode 	= B_FOLLOW_LEFT	| B_FOLLOW_TOP,
					uint32		flags			= B_WILL_DRAW	| B_NAVIGABLE);		
		~MoveButton();

		virtual void AttachedToWindow();
		virtual void GetPreferredSize(float *width, float *height);
	};

	class RemapCheckBox : public BCheckBox
   	{
		int32 fIndex;
	public:
		RemapCheckBox(BRect frame, const char* name, const char* label,
						BMessage* message, uint32 resizing_mode)
			: BCheckBox(frame, name, label,
						message, resizing_mode), fIndex(0) {}

		virtual void SetIndex(int32 index) { 
			fIndex = index; 
			BCheckBox::SetValue(fIndex > 0 ? 1 : 0);
		}
		int32 Index() const { return fIndex; }
	};

	class ClientBox : public BBox
	{
	public:
		ClientBox(BRect frame, const char* name,
			uint32 resizingMode, uint32 flags, border_style style);

		virtual void Pulse();
	};

	bool AlreadyInDeskbar();
	void AdjustRemapCheck(bool next_index);

public:
	SettingsWindow(bool fromDeskbar);
	virtual ~SettingsWindow();
	virtual void MessageReceived(BMessage *msg);
	virtual bool QuitRequested();
	
private:
	Settings *settings;
	Settings *settingsOrg;
	bool hotkey_changed;
	bool keymaps_changed;
	bool from_deskbar;
	BMenuField* menuField;
	KeymapListView *selected_list;
	KeymapOutlineListView *available_list;
//	BView *parent;
	MoveButton* addButton;
	MoveButton* delButton;
	BButton* buttonOK;
	BButton* buttonCancel;
	RemapCheckBox* checkRemap;
	BCheckBox* checkSystemWideKeymap;
//	BCheckBox* checkUseActiveKeymap;
};


#endif	//	__SETTINGSWINDOW_H

