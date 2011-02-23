// Switcher
// Copyright © 1999-2001 Stas Maximov
// All rights reserved
// Version 1.0.4


#include "SwitchFilter.h"

#include <stdio.h>
#include <syslog.h>

#include <Entry.h>
#include <InputServerDevice.h>
#include <Locker.h>
#include <NodeMonitor.h>
#include <Roster.h>
#include <View.h>

#include "KeymapSwitcher.h"


const int32 key_table[][2] = {
{41,101},
{42,114},
{43,116},
{44,121},
{45,117},
{46,105},
{47,111},
{48,112},
{49,91},
{50,93},
{60,97},
{61,115},
{62,100},
{63,102},
{64,103},
{65,104},
{66,106},
{67,107},
{68,108},
{69,59},
{70,39},
{76,122},
{77,120},
{78,99},
{79,118},
{80,98},
{81,110},
{82,109},
{83,44},
{84,46},
{85,47}
};

enum __msgs {
	MSG_CHANGEKEYMAP = 0x400, // thats for Indicator, don't change it
}; 

#if 0 //def NDEBUG
#define trace(x...) syslog(LOG_DEBUG, x);
#else
#define trace(s) ((void)0)  
#endif


BInputServerFilter* instantiate_input_filter() {
	openlog("switcher_filter", LOG_CONS, LOG_USER);
	trace("start");
	// this is where it all starts
	// make sure this function is exported!
	trace("filter instantiated");
	SwitchFilter *filter = new SwitchFilter();
	if (filter) {
		return filter;
	}
	return (NULL);
}


SwitchFilter::SwitchFilter() : BInputServerFilter() {

	openlog("KS", LOG_NDELAY, LOG_USER);

	trace("start");

	// register your filter(s) with the input_server
	// the last filter in the list should be NULL
//	trace(Name());
	settings = NULL;
	monitor = NULL;
	switch_on_hold = false;
}


// Destructor
SwitchFilter::~SwitchFilter() {
	trace("start");

	// do some clean-ups here
	if (NULL != monitor)
		monitor->PostMessage(B_QUIT_REQUESTED);
	snooze(1000000); // lets wait until monitor dies
	
	if (NULL != settings) {
		trace("killing settings now");
		settings->Reload(); // we need to load new settings first
		settings->Save();
		delete settings; // cause destructor saves settings here <g>
		settings = 0;
	}
	trace("end");

	closelog();
}


// Init check
status_t SwitchFilter::InitCheck() {
	trace("init check");
	settings = new Settings("Switcher");
	//monitor = new SettingsMonitor(INDICATOR_SIGNATURE, settings);
	monitor = new SettingsMonitor(APP_SIGNATURE, settings);
	monitor->Run();
	
	return B_OK;
}



// Filter key pressed 
filter_result SwitchFilter::Filter(BMessage *message, BList *outList) {
/*	union U{
		int32 long l;
		char b[6];
	} u;
	memset(&u, 0, sizeof(u));
	u.l = message->what;
	trace("Filter:%s\n", u.b);
*/	
	switch (message->what) {
	case B_KEY_MAP_CHANGED:
		trace("key_map_changed");
		break;

	case B_MODIFIERS_CHANGED: {
		uint32 new_modifiers = 0;
		uint32 old_modifiers = 0;
		uint8 states = 0;

////		key_info info;
////		get_key_info(&info);
////		new_modifiers = info.modifiers;
		message->FindInt32("modifiers", (int32 *) &new_modifiers);
		message->FindInt32("be:old_modifiers", (int32 *) &old_modifiers);
		message->FindInt8("states", (int8 *)&states);

		char *buf = new char[128];
		sprintf(buf, "new: %#010lx, old: %#010lx states:%#04x", 
				new_modifiers, old_modifiers, states);
		trace(buf);
		delete buf;

		int32 hotkey;
		settings->FindInt32("hotkey", &hotkey);

		if(!switch_on_hold) {
			// handle one-key switches first ...
			switch (hotkey) {
			case KEY_CAPS_LOCK: 
				// Shift-CapsLock will be used to switch CAPITALIZATION
				//if ((new_modifiers & (B_CAPS_LOCK | B_SHIFT_KEY)) == B_CAPS_LOCK) {
				if (new_modifiers & B_CAPS_LOCK) {
				//	UpdateIndicator();
					// eat the message to prevent CAPITALIZATION ;-)
					return B_SKIP_MESSAGE; 
				}
				if (old_modifiers & B_CAPS_LOCK) {
					UpdateIndicator();
					// eat the message to prevent CAPITALIZATION ;-)
					return B_SKIP_MESSAGE; 
				}
				break;
			case KEY_SCROLL_LOCK:
				if ((new_modifiers & B_SCROLL_LOCK) != 0 || (old_modifiers & B_SCROLL_LOCK) != 0) {
					UpdateIndicator();
					// do not eat the message - to on/off the LED!
					return B_DISPATCH_MESSAGE; 
				}
				break;
			default:
				break;
			}
		}

		old_modifiers &= ~(B_CAPS_LOCK | B_SCROLL_LOCK | B_NUM_LOCK);
		new_modifiers &= ~(B_CAPS_LOCK | B_SCROLL_LOCK | B_NUM_LOCK);
		
		if(switch_on_hold)
			trace("switch is on hold now");					

		if(switch_on_hold && new_modifiers == 0) {
			UpdateIndicator();
			switch_on_hold = false;
			//return B_SKIP_MESSAGE;
			return B_DISPATCH_MESSAGE; 
		}
		
		// handle two-keys switches ...
		switch (hotkey) {
		case KEY_LCTRL_SHIFT: {
			old_modifiers &= 0x0FF; // cut off all RIGHT and LEFT key bits
			uint32 mask1 = B_SHIFT_KEY|B_CONTROL_KEY;
			//uint32 mask2 = B_SHIFT_KEY|B_OPTION_KEY;
			if ((old_modifiers == mask1) /*|| (old_modifiers == mask2)*/)
				switch_on_hold = true;
			//return B_SKIP_MESSAGE;
			return B_DISPATCH_MESSAGE; 
		}
		
		case KEY_OPT_SHIFT:
		case KEY_ALT_SHIFT: {
			old_modifiers &= 0x0FF; // cut off all RIGHT and LEFT key bits
			uint32 mask1 = B_SHIFT_KEY|B_COMMAND_KEY;
			if (old_modifiers == mask1)
				switch_on_hold = true;
			//return B_SKIP_MESSAGE;
			return B_DISPATCH_MESSAGE; 
		} 
		case KEY_SHIFT_SHIFT: {
			uint32 mask1 = B_SHIFT_KEY|B_LEFT_SHIFT_KEY|B_RIGHT_SHIFT_KEY;
			if (old_modifiers == mask1)
				switch_on_hold = true;
			//return B_SKIP_MESSAGE;
			return B_DISPATCH_MESSAGE; 
		}
		default:
			break;
		}
		break; // B_MODIFIERS_CHANGED
	}
/*
	case B_UNMAPPED_KEY_UP:
	case B_UNMAPPED_KEY_DOWN:
							  {
		uint32 key = 0;
		uint32 modifiers = 0;
		uint8 states = 0;

		message->FindInt32("key", (int32 *) &key);
		message->FindInt32("modifiers", (int32 *) &modifiers);
		message->FindInt8("states", (int8 *)&states);

		trace("UKD key:%#010x modif:%#010x states:%#010\n", key, modifiers, states);

		if(modifiers & B_CAPS_LOCK) {
			trace("skipped!\n");
			return B_SKIP_MESSAGE;
		}

							  }
		break;
*/
//	case B_KEY_UP: 
//		trace("Key up:\n");
		break;
	case B_KEY_DOWN: {
		trace("Key down:\n");
		if(switch_on_hold) {
			trace("skipping last modifier change");
			switch_on_hold = false; // skip previous attempt
		}
		
		// check if it is Alt+Key key pressed, we shall put correct value 
		// just as it is with American keymap 
		int32 modifiers = message->FindInt32("modifiers");
		//int32 raw = message->FindInt32("raw_char");

/*		if ((modifiers & B_CONTROL_KEY) && (raw == B_ESCAPE)) {
			trace("Ctrl+Esc found. Generating message...");
			BMessage msg(B_MOUSE_DOWN);
			msg.AddInt32("clicks", 1);
			msg.AddPoint("where", BPoint(790, 5));
			msg.AddInt32("modifiers", modifiers);
			msg.AddInt32("buttons", B_PRIMARY_MOUSE_BUTTON);
			be_roster->Broadcast(&msg);
			return B_SKIP_MESSAGE;		
		} 
*/
		modifiers &= ~(B_CAPS_LOCK | B_SCROLL_LOCK | B_NUM_LOCK);
		if((modifiers != (B_COMMAND_KEY | B_LEFT_COMMAND_KEY)) && 
			(modifiers != (B_COMMAND_KEY | B_RIGHT_COMMAND_KEY))) {
//			trace("quitting because...");
			break; // only Alts needed
		}
			
		int32 key = message->FindInt32("key");
		
/*		if( key == 94 ) { // request for RealClipboard 
			BMessenger *indicator = GetIndicatorMessenger();
			trace("RealClipboard requested.");
			if(indicator)
				indicator->SendMessage(MSG_SHOW_RC_MENU);
			break;
		}
*/		

		// continue processing "modifiered" key
		app_info appinfo;
		if(B_OK != be_roster->GetActiveAppInfo(&appinfo)) 
			break; // no active app found. strange.. :))

			
//		if(raw > 0)
//			break;
			
		for(uint32 i=0; i<sizeof(key_table)/sizeof(key_table[0]);i++) {
			if(key == key_table[i][0]) {
				message->ReplaceInt32("raw_char", key_table[i][1]);
//				raw = key_table[i][1]; // set correct  value
				break;
			}
		}

//		if(raw < 0) // have no raw value for this key
//			break;
			
		break;
	} 
	default: 
		break;
	} // switch
	return B_DISPATCH_MESSAGE; // tell input_server to dispatch message
}


// Gets Indicator's BMessenger
BMessenger* SwitchFilter::GetIndicatorMessenger() {
	//const char *REPLICANT_NAME = "Switcher/Deskbar";
	BMessenger *indicator = 0;

	BMessage	request(B_GET_PROPERTY);
	BMessenger	to;
	BMessenger	status;

	request.AddSpecifier("Messenger");
	request.AddSpecifier("Shelf");
	
	// In the Deskbar the Shelf is in the View "Status" in Window "Deskbar"
//	request.AddSpecifier("View", REPLICANT_NAME);
	request.AddSpecifier("View", "Status");
	request.AddSpecifier("Window", "Deskbar");
	to = BMessenger(DESKBAR_SIGNATURE, -1);
	
	BMessage	reply;
	
	if (to.SendMessage(&request, &reply) == B_OK) {
		if(reply.FindMessenger("result", &status) == B_OK) {

			// enum replicant in Status view
			int32	index = 0;
			int32	uid;
			while ((uid = GetReplicantAt(status, index++)) >= B_OK) {
				BMessage	rep_info;
				if (GetReplicantName(status, uid, &rep_info) != B_OK) {
					continue;
				}
				const char *name;
				if (rep_info.FindString("result", &name) == B_OK) {
					if(strcmp(name, REPLICANT_NAME)==0) {
						BMessage rep_view;
						if (GetReplicantView(status, uid, &rep_view)==0) {
							BMessenger result;
							if (rep_view.FindMessenger("result", &result) == B_OK) {
								indicator = new BMessenger(result);
							}
						} 
					}
				}
			}
		}
	}
	return indicator;
}

//
void SwitchFilter::UpdateIndicator() {
	trace("go");
	// tell Indicator to change icon
	BMessenger *indicator = GetIndicatorMessenger();
	if(indicator)
		indicator->SendMessage(MSG_CHANGEKEYMAP);
}

//
int32 SwitchFilter::GetReplicantAt(BMessenger target, int32 index) const
{
	/*
	 So here we want to get the Unique ID of the replicant at the given index
	 in the target Shelf.
	 */
	 
	BMessage	request(B_GET_PROPERTY);		// We're getting the ID property
	BMessage	reply;
	status_t	err;
	
	request.AddSpecifier("ID");					// want the ID
	request.AddSpecifier("Replicant", index);	// of the index'th replicant
	
	if ((err = target.SendMessage(&request, &reply)) != B_OK)
		return err;
	
	int32	uid;
	if ((err = reply.FindInt32("result", &uid)) != B_OK) 
		return err;
	
	return uid;
}


//
status_t SwitchFilter::GetReplicantName(BMessenger target, int32 uid, BMessage *reply) const
{
	/*
	 We send a message to the target shelf, asking it for the Name of the 
	 replicant with the given unique id.
	 */
	 
	BMessage	request(B_GET_PROPERTY);
	BMessage	uid_specifier(B_ID_SPECIFIER);	// specifying via ID
	status_t	err;
	status_t	e;
	
	request.AddSpecifier("Name");		// ask for the Name of the replicant
	
	// IDs are specified using code like the following 3 lines:
	uid_specifier.AddInt32("id", uid);
	uid_specifier.AddString("property", "Replicant");
	request.AddSpecifier(&uid_specifier);
	
	if ((err = target.SendMessage(&request, reply)) != B_OK)
		return err;
	
	if (((err = reply->FindInt32("error", &e)) != B_OK) || (e != B_OK))
		return err ? err : e;
	
	return B_OK;
}

//
status_t SwitchFilter::GetReplicantView(BMessenger target, int32 uid, BMessage *reply) const
{
	/*
	 We send a message to the target shelf, asking it for the Name of the 
	 replicant with the given unique id.
	 */
	 
	BMessage	request(B_GET_PROPERTY);
	BMessage	uid_specifier(B_ID_SPECIFIER);	// specifying via ID
	status_t	err;
	status_t	e;
	
	request.AddSpecifier("View");		// ask for the Name of the replicant
	
	// IDs are specified using code like the following 3 lines:
	uid_specifier.AddInt32("id", uid);
	uid_specifier.AddString("property", "Replicant");
	request.AddSpecifier(&uid_specifier);
	
	if ((err = target.SendMessage(&request, reply)) != B_OK)
		return err;
	
	if (((err = reply->FindInt32("error", &e)) != B_OK) || (e != B_OK))
		return err ? err : e;
	
	return B_OK;
}

SwitchFilter::SettingsMonitor::SettingsMonitor(const char *name, Settings *settings) : BLooper(name) {
	trace("creating monitor");
	this->settings = settings; // copy pointer

	BLocker lock;
	lock.Lock();
	node_ref nref;
	BEntry entry(settings->GetPath().String(), false);
	if(B_OK != entry.InitCheck())
		trace("entry is invalid!");
	entry.GetNodeRef(&nref);
	watch_node(&nref, B_WATCH_STAT | B_WATCH_NAME, this);
	lock.Unlock();
	trace("created");
}

SwitchFilter::SettingsMonitor::~SettingsMonitor() {
	trace("killing");
	BLocker lock;
	lock.Lock();
	node_ref nref;
	BEntry entry(settings->GetPath().String(), false);
	if(B_OK != entry.InitCheck())
		trace("entry is invalid!");
	entry.GetNodeRef(&nref);
	watch_node(&nref, B_STOP_WATCHING, this);
	lock.Unlock();
	trace("killed");
}

void SwitchFilter::SettingsMonitor::MessageReceived(BMessage *message) {
	trace("start");
	switch (message->what) {
		case B_NODE_MONITOR: {
			BLocker lock;
			lock.Lock();
			settings->Reload();
			lock.Unlock();
			return; // message handled
		}
	}
	BLooper::MessageReceived(message);
	trace("end");
}

