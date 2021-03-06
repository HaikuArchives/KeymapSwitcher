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
extern "C" _EXPORT BView *instantiate_deskbar_item(float maxWidth, float maxHeight);

class DeskView : public BView
{
	typedef struct {
		team_id	team;
		int32	keymap;
	} team_keymap;

public:

	DeskView(const char *name, BRect rect,
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
	void     Draw(BRect r);
    void     AttachedToWindow();

private:
	void ShowContextMenu(BPoint where);
	void SwitchToNextKeyMap();
	void ChangeKeyMap(int32);
	void ChangeKeyMapSilent(int32, bool force = false);
	void Init();
	int32 FindApp(int32 team);
	void ShowAboutWindow();
	void UpdateViewColors();
	void UpdateText(bool init);
	BRect GetTextRect(const char *text);
private:
	BString keymap_text;
	Settings *settings; 
	bool watching;
	BPath cur_map_path;
	int32 active_keymap;
	BList *keymaps;
	BList *app_list;
	rgb_color background_color;
	rgb_color text_color;
	team_id active_team; // tracked to ignore Deskbar activation
};

#endif

