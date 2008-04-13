// Switcher
// Copyright © 1999-2001 Stas Maximov
// All rights reserved
// Version 1.0.4

#ifndef __SWITCHFILTER_H
#define __SWITCHFILTER_H

#include <add-ons/input_server/InputServerFilter.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Looper.h>

#include "Settings.h" // from /Common

// export this for the input_server
extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

class SettingsMonitor : public BLooper {
public:
	SettingsMonitor(const char *name, Settings *settings);
	~SettingsMonitor();
	virtual void MessageReceived(BMessage *msg);
private:
	Settings *settings;
};

class SwitchFilter: public BInputServerFilter {
public:
	SwitchFilter();
	virtual ~SwitchFilter();
	virtual status_t InitCheck();
	virtual filter_result Filter(BMessage *message, BList *outList);
	status_t GetScreenRegion(BRegion *region) const;
private:
	void UpdateIndicator();
	void ShowRCMenu(BMessage msg);
	status_t GetReplicantName(BMessenger target,  int32 uid, BMessage *reply) const;
	status_t GetReplicantView(BMessenger target,  int32 uid, BMessage *reply) const;
	int32 GetReplicantAt(BMessenger target, int32 index) const;
	BMessenger *GetIndicatorMessenger();	
private:
	Settings *settings;
	bool switch_on_hold;
	SettingsMonitor *monitor;
};

#endif	//	__SWITCHFILTER_H
