// Switcher
// Copyright (c) 1999-2003 Stas Maximov
// Created by Stas Maximov
// All rights reserved
#ifndef __DeskView__
#define __DeskView__


#include "Settings.h"

#include <StringView.h>


// the dragger part has to be exported
class _EXPORT DeskView;
extern "C" _EXPORT BView *instantiate_deskbar_item();

class DeskView : public BStringView
{
	typedef struct {
		team_id	team;
		int32		keymap;
	} team_keymap;

public:

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
	virtual void MessageReceived(BMessage *);
    void     AttachedToWindow();

private:
	void ShowContextMenu(BPoint where);
	void ChangeKeyMap(int32);
	void ChangeKeyMapSilent(int32);
	void Init();
	int32 FindApp(int32 team);
	void ShowAboutWindow();
	void UpdateViewColors();
	void UpdateText();
private:
	Settings *settings; 
	bool watching;
	BPath cur_map_path;
	int32 active_keymap;
	BList *keymaps;
	BList *app_list;
};

#endif

