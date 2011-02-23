// Switcher
// Copyright © 1999-2001 Stas Maximov
// All rights reserved
// Version 1.0.4


#include "SwitchFilter.h"

#include <stdio.h>
#include <syslog.h>

#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <InputServerDevice.h>
#include <Locker.h>
#include <NodeMonitor.h>
#include <Roster.h>
#include <View.h>

#include "KeymapSwitcher.h"

//
//	Raw key numbering for 101 keyboard...
//                                                                                        [sys]       [brk]
//                                                                                         0x7e        0x7f
// [esc]       [ f1] [ f2] [ f3] [ f4] [ f5] [ f6] [ f7] [ f8] [ f9] [f10] [f11] [f12]    [prn] [scr] [pau]
//  0x01        0x02  0x03  0x04  0x05  0x06  0x07  0x08  0x09  0x0a  0x0b  0x0c  0x0d     0x0e  0x0f  0x10     K E Y P A D   K E Y S
//
// [ ` ] [ 1 ] [ 2 ] [ 3 ] [ 4 ] [ 5 ] [ 6 ] [ 7 ] [ 8 ] [ 9 ] [ 0 ] [ - ] [ = ] [bck]    [ins] [hme] [pup]    [num] [ / ] [ * ] [ - ]
//  0x11  0x12  0x13  0x14  0x15  0x16  0x17  0x18  0x19  0x1a  0x1b  0x1c  0x1d  0x1e     0x1f  0x20  0x21     0x22  0x23  0x24  0x25
//
// [tab] [ q ] [ w ] [ e ] [ r ] [ t ] [ y ] [ u ] [ i ] [ o ] [ p ] [ [ ] [ ] ] [ \ ]    [del] [end] [pdn]    [ 7 ] [ 8 ] [ 9 ] [ + ]
//  0x26  0x27  0x28  0x29  0x2a  0x2b  0x2c  0x2d  0x2e  0x2f  0x30  0x31  0x32  0x33     0x34  0x35  0x36     0x37  0x38  0x39  0x3a
//
// [cap] [ a ] [ s ] [ d ] [ f ] [ g ] [ h ] [ j ] [ k ] [ l ] [ ; ] [ ' ] [  enter  ]                         [ 4 ] [ 5 ] [ 6 ]
//  0x3b  0x3c  0x3d  0x3e  0x3f  0x40  0x41  0x42  0x43  0x44  0x45  0x46     0x47                             0x48  0x49  0x4a
//
// [shift]     [ z ] [ x ] [ c ] [ v ] [ b ] [ n ] [ m ] [ , ] [ . ] [ / ]     [shift]          [ up]          [ 1 ] [ 2 ] [ 3 ] [ent]
//   0x4b       0x4c  0x4d  0x4e  0x4f  0x50  0x51  0x52  0x53  0x54  0x55       0x56            0x57           0x58  0x59  0x5a  0x5b
//
// [ctr]             [cmd]             [  space  ]             [cmd]             [ctr]    [lft] [dwn] [rgt]    [ 0 ] [ . ]
//  0x5c              0x5d                 0x5e                 0x5f              0x60     0x61  0x62  0x63     0x64  0x65
//
//	NOTE: On a Microsoft Natural Keyboard:
//			left option  = 0x66
//			right option = 0x67
//			menu key     = 0x68
//	NOTE: On an Apple Extended Keyboard:
//			left option  = 0x66
//			right option = 0x67
//			keypad '='   = 0x6a
//			power key    = 0x6b

union SwitchFilter::_key defaultRemapKeys[] = {
/*x00*/ {   0 }, {   0 },  {   0 },  {   0 },  {   0 },  {   0 },  {   0 },  {   0 }, 
/*x08*/ {   0 }, {   0 },  {   0 },  {   0 },  {   0 },  {   0 },  {   0 },  {   0 }, 
/*x10*/ {   0 }, { '`' },  { '1' },  { '2' },  { '3' },  { '4' },  { '5' },  { '6' }, 
/*x18*/ { '7' }, { '8' },  { '9' },  { '0' },  { '-' },  { '=' },  {   0 },  {   0 }, 
/*x20*/ {   0 }, {   0 },  {   0 },  {   0 },  {   0 },  {   0 },  {   0 },  { 'q' }, 
/*x28*/ { 'w' }, { 'e' },  { 'r' },  { 't' },  { 'y' },  { 'u' },  { 'i' },  { 'o' }, 
/*x30*/ { 'p' }, { '[' },  { ']' },  { '\\'},  {   0 },  {   0 },  {   0 },  {   0 }, 
/*x38*/ {   0 }, {   0 },  {   0 },  {   0 },  { 'a' },  { 's' },  { 'd' },  { 'f' }, 
/*x40*/ { 'g' }, { 'h' },  { 'j' },  { 'k' },  { 'l' },  { ';' },  { '\''},  {   0 }, 
/*x48*/ {   0 }, {   0 },  {   0 },  {   0 },  { 'z' },  { 'x' },  { 'c' },  { 'v' }, 
/*x50*/ { 'b' }, { 'n' },  { 'm' },  { ',' },  { '.' },  { '/' },  {   0 },  {   0 }, 
/*x58*/ {   0 }, {   0 },  {   0 },  {   0 },  {   0 },  { ' ' },  {   0 },  {   0 }, 
/*x60*/ {   0 }, {   0 },  {   0 },  {   0 },  {   0 },  {   0 },  {   0 },  {   0 }, 
/*x68*/ {   0 }, {   0 },  {   0 },  {   0 },  {   0 },  {   0 },  {   0 },  {   0 }, 
};

const uint32 remapKeysCount = sizeof(defaultRemapKeys)/sizeof(defaultRemapKeys[0]);

/*		
enum __msgs {
	MSG_CHANGEKEYMAP = 0x400, // thats for Indicator, don't change it
}; */

#if 0 //def NDEBUG
#define trace(x...) syslog(LOG_DEBUG, x);
#else
#define trace(x...) ((void)0)  
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


SwitchFilter::SwitchFilter() : BInputServerFilter()
{
	remapKeys = NULL;

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

	delete[] remapKeys;

	closelog();
}


// Init check
status_t SwitchFilter::InitCheck() {
	trace("init check");
	settings = new Settings("Switcher");

	UpdateRemapTable();

	//monitor = new SettingsMonitor(INDICATOR_SIGNATURE, settings);
	monitor = new SettingsMonitor(APP_SIGNATURE, this);
	monitor->Run();
	
	return B_OK;
}



// Filter key pressed 
filter_result SwitchFilter::Filter(BMessage *message, BList *outList) {
#if 0	
	union U{
		int32 long l;
		char b[6];
	} u;
	memset(&u, 0, sizeof(u));
	u.l = message->what;
	trace("Filter:%s\n", u.b);
#endif

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
	case B_KEY_DOWN: {
		trace("Key down:\n");
		if(switch_on_hold) {
			trace("skipping last modifier change");
			switch_on_hold = false; // skip previous attempt
		}
		
		int32 index = 0;
		settings->FindInt32("remap", &index);
		if(--index < 0)
			break; // no remap option was configured

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
//		modifiers &= ~(B_CAPS_LOCK | B_SCROLL_LOCK | B_NUM_LOCK);
//		if((modifiers != (B_COMMAND_KEY | B_LEFT_COMMAND_KEY)) && 
//			(modifiers != (B_COMMAND_KEY | B_RIGHT_COMMAND_KEY)))
		if(!(modifiers & (B_COMMAND_KEY | B_LEFT_COMMAND_KEY | B_RIGHT_COMMAND_KEY)))
		{
			trace("Do not remap. Cmd- is not pressed(%#x)", modifiers);
			break; // only Cmd-based are mapped
		}
			
		uint32 key = message->FindInt32("key");
		
		// continue processing "modifiered" key
		app_info appinfo;
		if(B_OK != be_roster->GetActiveAppInfo(&appinfo)) 
			break; // no active app found. strange.. :)) //XXX ???

		if(key > 0 && key <= remapKeysCount
			&& 0 != RemapKey(key) && RemapKey(key)->byte != 0)
	   	{
			_key* mappedKey = RemapKey(key);
			message->ReplaceInt32("raw_char", mappedKey->byte);
			message->RemoveName("bytes");
			message->AddString("bytes", (const char *)(mappedKey->bytes));
			for (int i = 0; i < maxCharBytes; i++)
				message->ReplaceInt8("byte", i, mappedKey->bytes[i]);
		}

		break;
	} 

	default: 
		break;
	} // switch
	return B_DISPATCH_MESSAGE; // tell input_server to dispatch message
}


SwitchFilter::_key* SwitchFilter::RemapKey(uint32 index)
{
	if (index < 0 || index > remapKeysCount)
		return 0;

	if (0 != remapKeys) {
		return &remapKeys[index];
	}

	return &defaultRemapKeys[index];
}

void
SwitchFilter::UpdateRemapTable()
{
	// clean existing table if exists...
	delete[] remapKeys;
	remapKeys = 0;

	int32 keymaps = 0;
	settings->FindInt32("keymaps", &keymaps);
	if(keymaps <= 0)
		return; // settings are empty - default mapping will be used.

	int32 index = 0;
	settings->FindInt32("remap", &index);
	if(--index < 0)
		return; //  -1 means no remap option was configured

	status_t st = B_OK;
	directory_which dir;
	BString str("d");
	str << index;
	if((st = settings->FindInt32(str, (int32*)&dir)) != B_OK) {
		trace("attr %s (dir) not found: %#x (%s)", str.String(), st, strerror(st));
		return;
	}

	BString strRealName;
	str = "n";
	str << index;
	if((st = settings->FindString(str, &strRealName)) != B_OK) {
		trace("attr %s (name) not found: %#x (%s)", str.String(), st, strerror(st));
		return;
	}

	key_map KM = { 0 };

	BPath path;
	find_directory((directory_which)dir, &path);
	if(dir == B_BEOS_DATA_DIRECTORY)
		path.Append("Keymaps");
	else
		path.Append("Keymap");
	path.Append(strRealName);

	BFile file(path.Path(), B_READ_ONLY);
	if(file.Read(&KM, sizeof(key_map)) < (ssize_t)sizeof(key_map)) {
		trace("Cannot read the key_map header.");
		return;
	}

	for (size_t i = 0; i < sizeof(key_map); i += sizeof(uint32)) {
		uint32* p = (uint32*)(((uint8*)&KM) + i);
		*p = B_BENDIAN_TO_HOST_INT32(*p);
	}

	if(KM.version != 3) {
		trace("Keympa version mismatch:%d", KM.version);
		return;
	}

	uint32 charsSize = 0;
	if(file.Read(&charsSize, sizeof(uint32)) < (ssize_t)sizeof(uint32)) {
		trace("Cannot read size of chars buffer.");
		return;
	}

	charsSize = B_BENDIAN_TO_HOST_INT32(charsSize);

	char* chars = new char[charsSize];

	if (file.Read(chars, charsSize) >= (ssize_t)charsSize) {

		remapKeys = new _key[remapKeysCount]; 
		memset(remapKeys, 0, remapKeysCount * sizeof(_key));

		size_t count = min_c(sizeof(KM.normal_map)/sizeof(KM.normal_map[0]), remapKeysCount);
		for(size_t i = 0; i < count; i++) {

			if(0 == defaultRemapKeys[i].byte)
				continue; // ignore not-remapped in default table.

			int offset = KM.normal_map[i];
			char bytes = chars[offset];
			switch(bytes) {
				case 3: remapKeys[i].bytes[2] = chars[offset + 3]; // fall through
				case 2: remapKeys[i].bytes[1] = chars[offset + 2]; // fall through
				case 1: remapKeys[i].bytes[0] = chars[offset + 1];
					trace("off:%#x -> %s", i, remapKeys[i].bytes);
					break;
				case 0:
					break; // not mapped at all - silently skip
				default:
					{
						char* b = new char[bytes + 1];
						memcpy(b, &chars[offset + 1], bytes);
						b[int(bytes)] = '\0';
						trace("too long (%d bytes) char ignored:%s", bytes, b);
						delete[] b;
					}
					break;
			}
		}
	}

	delete[] chars;
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
	// tell Indicator to change icon
	BMessenger *indicator = GetIndicatorMessenger();
	trace("go:%#x", indicator);
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

void SwitchFilter::MonitorEvent()
{
	settings->Reload();
	UpdateRemapTable();
	trace("monitor event!");
}

SwitchFilter::SettingsMonitor::SettingsMonitor(const char *name, SwitchFilter* filter)
							   	: BLooper(name) 
{
	trace("creating monitor");
	this->filter = filter; // copy pointer

	BLocker lock;
	lock.Lock();
	node_ref nref;
	BEntry entry(filter->settings->GetPath().String(), false);
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
	BEntry entry(filter->settings->GetPath().String(), false);
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
			filter->MonitorEvent();
			lock.Unlock();
			return; // message handled
		}
	}
	BLooper::MessageReceived(message);
	trace("end");
}

