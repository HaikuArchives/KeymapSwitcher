// Switcher
// Copyright © 1999-2001 Stas Maximov
// All rights reserved
// Version 1.0.4
#ifndef __SWITCHFILTER_H
#define __SWITCHFILTER_H


#include <InputServerFilter.h>
#include <Looper.h>

#include "Settings.h"


// export this for the input_server
extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

const int maxCharBytes = 3;

class SwitchFilter: public BInputServerFilter 
{
	class SettingsMonitor : public BLooper
   	{
	public:
		SettingsMonitor(const char *name, SwitchFilter *filter);
		~SettingsMonitor();
		virtual void MessageReceived(BMessage *msg);
	private:
		SwitchFilter *filter;
	};

	friend class SettingsMonitor;

	void MonitorEvent();
	void UpdateRemapTable();

public:
	
	union _key {
		char byte;
		int8 bytes[maxCharBytes + 1];
	};

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

	_key* RemapKey(uint32 index);

	Settings *settings;
	bool switch_on_hold;
	SettingsMonitor *monitor;
	_key *remapKeys;
};

#endif	//	__SWITCHFILTER_H

