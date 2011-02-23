// Switcher
// Copyright (c) 1999-2003 Stas Maximov
// Created by Stas Maximov
// All rights reserved
#ifndef __DeskView__
#define __DeskView__


#include <View.h>

#include "Settings.h"


// the dragger part has to be exported
class _EXPORT DeskView;
extern "C" _EXPORT BView *instantiate_deskbar_item();

class DeskView : public BView
{
	typedef struct {
		team_id	team;
		int32		keymap;
	} team_keymap;

public:
	enum __msgs {
		MSG_CHANGEKEYMAP = 0x400,
		MSG_UPDATESETTINGS,
		MSG_SHOW_RC_MENU = 0x500
	}; 

	DeskView(const char *name,
		uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_PULSE_NEEDED);
		
	DeskView(BMessage *); // BMessage* based constructor needed to support archiving
	virtual ~DeskView();

	// archiving overrides
	static DeskView *Instantiate(BMessage *data);
	virtual	status_t Archive(BMessage *data, bool deep = true) const;

	virtual void MouseDown(BPoint);
	virtual void Pulse();
	virtual void Draw(BRect );
	virtual void MessageReceived(BMessage *);
    void     AttachedToWindow();
    void     DetachedFromWindow();

private:
	void ShowContextMenu(BPoint where);
	void ChangeKeyMap(int32);
	void ChangeKeyMapSilent(int32);
	void Init();
	int32 FindApp(int32 team);
	void ShowAboutWindow();
private:
	Settings *settings; 
	bool disabled;
	bool watching;
	BPath cur_map_path;
	int32 active_keymap;
	BList *keymaps;
	BList *app_list;
};

#endif

