// Switcher
// Copyright © 1999-2003 Stas Maximov
// Created by Stas Maximov
// All rights reserved


#include "DeskView.h"

#include <stdio.h>
#include <syslog.h>

#include <Alert.h>
#include <Beep.h>
#include <Catalog.h>
#include <Deskbar.h>
//#include <Dragger.h>
#include <File.h>
#include <FindDirectory.h>
#include <InputServerDevice.h>
#include <Locale.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <TextView.h>

#include "KeymapSwitcher.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SwitcherDeskView"

#define DELETE(p) {if (0 != p) { delete p; p = 0; } }

extern void _restore_key_map_(); // magic undoc function :))

#if 0
#define trace(x...) syslog(0, x);
#else
#define trace(s) ((void) 0)
#endif

#define TRACE_MODIFIERS(s, m) \
	trace(s); \
	if(m & B_SHIFT_KEY) trace("    B_SHIFT_KEY"); \
	if(m & B_COMMAND_KEY) trace("    B_COMMAND_KEY"); \
	if(m & B_CONTROL_KEY) trace("    B_CONTROL_KEY"); \
	if(m & B_CAPS_LOCK) trace("    B_CAPS_LOCK"); \
	if(m & B_SCROLL_LOCK) trace("    B_SCROLL_LOCK"); \
	if(m & B_NUM_LOCK) trace("    B_NUM_LOCK"); \
	if(m & B_OPTION_KEY) trace("    B_OPTION_KEY"); \
	if(m & B_MENU_KEY) trace("    B_MENU_KEY"); \
	if(m & B_LEFT_SHIFT_KEY) trace("    B_LEFT_SHIFT_KEY"); \
	if(m & B_RIGHT_SHIFT_KEY) trace("    B_RIGHT_SHIFT_KEY"); \
	if(m & B_LEFT_COMMAND_KEY) trace("    B_LEFT_COMMAND_KEY"); \
	if(m & B_RIGHT_COMMAND_KEY) trace("    B_RIGHT_COMMAND_KEY"); \
	if(m & B_LEFT_CONTROL_KEY) trace("    B_LEFT_CONTROL_KEY"); \
	if(m & B_RIGHT_CONTROL_KEY) trace("    B_RIGHT_CONTROL_KEY"); \
	if(m & B_LEFT_OPTION_KEY) trace("    B_LEFT_OPTION_KEY"); \
	if(m & B_RIGHT_OPTION_KEY) trace("    B_RIGHT_OPTION_KEY");

const uint32	kChangeKeymap = 'CChK';
const uint32	kSettings = 'CSet';
const uint32	kAbout = 'CAbt';
const uint32	kUnloadNow = 'CUnl';

/*struct async_menu_info
{
	BPopUpMenu*	menu;
	BPoint		where;
	BRect		bounds;
	async_menu_info() : menu(0) {}
	~async_menu_info() { delete menu; }
}; */


BView *instantiate_deskbar_item(float maxWidth, float maxHeight) {
	return new DeskView(REPLICANT_NAME, BRect(0, 0, maxHeight * 1.44, maxHeight));
}

//
/*
int32 ShowContextMenuAsync(void *pMenuInfo)
{
	async_menu_info *pMI = (async_menu_info*)pMenuInfo;
	pMI->menu->Go(pMI->where, true, true, pMI->bounds, true);
	delete pMI;
	return B_OK;
}
*/

// General constructor
DeskView::DeskView(const char *name, BRect rect,
	uint32 resizeMask, uint32 flags)
		: BView(rect, name, resizeMask, flags)
{
//	AddChild(new BDragger(BRect(-10, -10, - 10, -10), this));
	Init();	// Do prepare...
}

// Archive constructor
DeskView::DeskView(BMessage *message) : 
		BView(message)
{
	BFont font;
//	font.SetSize(12.f);
	SetFont(&font);

	Init();	// Do prepare...
}

// Prepare 
void DeskView::Init()
{
	keymap_text.SetTo(":)");
	keymaps = new BList();
	settings = new Settings("Switcher");
	if (settings->InitCheck() != B_OK) {
		// settings do not exist, populate default values
		settings->SetDefaults();
		settings->Save(); // let the Filter know about new settings
	}
	active_keymap = 0;
	settings->FindInt32("active", &active_keymap);
	watching = false;	

	active_team = -1; 
	find_directory(B_USER_SETTINGS_DIRECTORY, &cur_map_path);
	cur_map_path.Append("Key_map");
	trace(cur_map_path.Path());

	int32 count = 0;
	settings->FindInt32("keymaps", &count); // retrieve keymaps number
	BString param, name;
	BPath path;

	// read all the keymaps
	for (int32 i = 0; i<count; i++) {
		param = "";
		param << "n" << i;
		settings->FindString(param.String(), &name);
		param = "";
		param << "d" << i;
		int32 dir;
		trace(param.String());
		settings->FindInt32(param.String(), &dir);
		find_directory((directory_which)dir, &path);
		path.Append(name.String());
		trace(path.Path());
		keymaps->AddItem((void *)new BString(path.Path()));
	}

	// initialize app list
	app_list = new BList;
	BList *teams = new BList;
	be_roster->GetAppList(teams);
	int32 teams_count = teams->CountItems();
	for (int i=0; i<teams_count; i++) {
		team_id team = team_id((addr_t)teams->ItemAt(i));
		app_info info;
		be_roster->GetRunningAppInfo(team, &info);

		if((info.flags & B_BACKGROUND_APP) || (0 == strcmp(info.signature, DESKBAR_SIGNATURE))){
			continue; // we don't need guys like input_server to appear in our list
		}
		team_keymap *item = new team_keymap;
		item->team = team;
		item->keymap = 0;
		app_list->AddItem((void *)item);
	}
	DELETE(teams);
}

//
DeskView::~DeskView()
{
	be_roster->StopWatching(this);
	settings->SetInt32("active", active_keymap);
	if (NULL != settings) 
		DELETE(settings);
	if (NULL != app_list)
		while (!app_list->IsEmpty())
			delete (static_cast<team_keymap*> (app_list->RemoveItem((int32)0))); 
	DELETE(app_list);
	if (NULL != keymaps) {
		while (!keymaps->IsEmpty())
			delete (static_cast<BString*> (keymaps->RemoveItem((int32)0)));
		DELETE(keymaps);
	}
}


// archiving overrides
DeskView *DeskView::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "DeskView")) {
		return new DeskView(data);
	}

	return NULL;
}

//
status_t DeskView::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);
	data->AddString("add_on", APP_SIGNATURE);
	data->AddString("class", REPLICANT_NAME);
	return B_NO_ERROR;
}

//
void DeskView::AttachedToWindow(void)
{
	BView::AttachedToWindow();

	UpdateViewColors();
	UpdateText(true);

	// force Key_map file overwriting to first one 
	ChangeKeyMapSilent(0, true);
}

void DeskView::UpdateViewColors()
{
	background_color = ui_color(B_DESKTOP_COLOR);
	text_color = make_color(255, 255, 255, 255);

	if (!settings->GetColor("low", &background_color))
		background_color = ui_color(B_DESKTOP_COLOR);
	if (!settings->GetColor("high", &text_color))
		text_color = make_color(255, 255, 255, 255);

	SetViewColor(Parent()->ViewColor());
	SetLowColor(background_color);
	SetHighColor(text_color);
}

BRect DeskView::GetTextRect(const char *text)
{
	BFont font;
	GetFont(&font);

	const char* stringArray[1];
	stringArray[0] = text;
	BRect rectArray[1];
	escapement_delta delta = { 0.0, 0.0 };
	font.GetBoundingBoxesForStrings(stringArray, 1, B_SCREEN_METRIC, &delta, rectArray);

	return rectArray[0];
}

void DeskView::Draw(BRect r)
{
	BRect rect(Bounds());
	rect.bottom--;

	SetDrawingMode(B_OP_COPY);

	SetHighColor(Parent()->ViewColor());
	FillRect(Bounds());

	SetDrawingMode(B_OP_ALPHA);
	SetHighColor(background_color);
	float radius = rect.Height() * 0.2;
	FillRoundRect(rect, radius, radius);

	rgb_color frame_color = tint_color(background_color, 1.5);
	SetHighColor(frame_color);
	StrokeRoundRect(rect, radius, radius);

	BRect textRect = GetTextRect(keymap_text.String());
	BPoint point(ceilf((rect.Width() - textRect.Width() + 1.0) / 2.0 - textRect.left),
		ceilf((rect.Height() - textRect.Height() + 1.0) / 2.0 - textRect.top));
	SetLowColor(background_color);
	SetHighColor(text_color);
	DrawString(keymap_text.String(), point);
}

void DeskView::UpdateText(bool init)
{
	BString map_name;
	// get keymap name from the keymap path
	BString *keymap = (BString *)keymaps->ItemAt(active_keymap); 
	if(keymap == 0) {
		keymap = (BString *)keymaps->ItemAt(0);
	}

	if (keymap != 0) {
		BPath path(keymap->String());
		map_name = path.Leaf();
	}

	if (map_name.Length() == 0)
		map_name << ":(";

	map_name.Truncate(2, true);

	if(0 != map_name.Compare(keymap_text))
		keymap_text = map_name;

	// handle case of too narrow language-codes ("It" for example)
	BRect textRect = GetTextRect(keymap_text.String());
	if (init || ((textRect.Width() + 5) > Bounds().Width())) {
		ResizeTo(textRect.Width() + 5, Bounds().Height()); // let some place for better look
	}
	Invalidate();
}

//
void DeskView::MessageReceived(BMessage *message)
{
	if (message->WasDropped()) {
		message->PrintToStream();
		rgb_color *color = 0;
		ssize_t sz = 0;
		if (message->FindData("RGBColor", B_RGB_COLOR_TYPE, 0,
			(const void**)&color, &sz) == B_OK && sz == sizeof(rgb_color))
		{
			int32 buttons = B_PRIMARY_MOUSE_BUTTON;
			if (message->FindInt32("buttons", &buttons) == B_OK
					&& (buttons & B_PRIMARY_MOUSE_BUTTON))
			{
				settings->SetColor("low", color);
				background_color = *color;
				SetLowColor(*color);

			} else if (buttons & B_TERTIARY_MOUSE_BUTTON) {
				background_color = ui_color(B_DESKTOP_COLOR);
				text_color = make_color(255, 255, 255, 255);

				settings->SetColor("low",  &background_color);
				settings->SetColor("high", &text_color);

				SetLowColor(background_color);
				SetHighColor(text_color);

			} else {
				settings->SetColor("high", color);
				text_color = *color;
				SetHighColor(*color);
			}
			Invalidate();
			settings->Save();
			return;
		}
	}
	switch(message->what) {
	case B_KEY_MAP_CHANGED:
		break;
	case kChangeKeymap: {
		int32 change_to = 0;
		message->FindInt32("change_to", &change_to);
		ChangeKeyMap(change_to);
		break;
	}
	case kSettings: {
		be_roster->Launch(APP_SIGNATURE);
		break;
	}
	case kAbout: {
		ShowAboutWindow();
		break;
	}
	case kUnloadNow: {
		BDeskbar deskbar;
		deskbar.RemoveItem(REPLICANT_NAME);
		break;
	}
	case MSG_CHANGEKEYMAP:
		// message from Switcher, change Indicator now!
		SwitchToNextKeyMap();
		break;

	case MSG_UPDATESETTINGS: {
		if(settings == NULL)
			settings = new Settings("Switcher");
		else settings->Reload();

		int32 count = 0;
		settings->FindInt32("keymaps", &count); // retrieve keymaps number
		BString param, name;
		BPath path;
		keymaps->MakeEmpty();
		// read all the keymaps
		for (int32 i = 0; i<count; i++) {
			param = "";
			param << "n" << i;
			settings->FindString(param.String(), &name);
			param = "";
			param << "d" << i;
			int32 dir;
			settings->FindInt32(param.String(), &dir);
			find_directory((directory_which)dir, &path);
			path.Append("Keymap/");
			path.Append(name.String());
			trace(path.Path());
			keymaps->AddItem((void *) new BString(path.Path()));
		}
		Pulse();			
		break;
	}
	case MSG_SHOW_RC_MENU: {
		app_info info;
		if(B_OK == be_roster->GetActiveAppInfo(&info)) {
			BMessage request(B_GET_PROPERTY);
			request.AddSpecifier("Name");
			BMessenger app(info.signature, -1);
			BMessage reply;
			if(B_OK == app.SendMessage(&request, &reply)) {
				BString bstr;
				reply.FindString("result", &bstr);
				trace(bstr.String());
			}
			request = BMessage(B_GET_PROPERTY);
			request.AddSpecifier("Messenger");
			request.AddSpecifier("Window");
			if(B_OK == app.SendMessage(&request, &reply)) {
				trace("Got window");
				BMessenger window;				
				if(B_OK == reply.FindMessenger("result", &window))
					trace("Window's messenger found!!!");
			}
		}

		break;
	}			
	case B_SOME_APP_LAUNCHED: {
		int32 team = 0;
		if(B_OK == message->FindInt32("be:team", &team)) {
			app_info info;
			be_roster->GetRunningAppInfo(team, &info);

			if((info.flags & B_BACKGROUND_APP)
					|| (0 == strcmp(info.signature, DESKBAR_SIGNATURE)))
			{
				break; // we don't need any background apps here
			}
			team_keymap *item = new team_keymap;
			item->team = team;
			//int32 nUseActive = 0;
			//if (B_OK != settings->FindInt32("use_active_keymap", &nUseActive))
			//	nUseActive = 0;
			item->keymap = /*nUseActive != 0 ? active_keymap :*/ 0;
			app_list->AddItem((void *)item);
			// will be handled in _ACTIVATED?
			// ChangeKeyMapSilent(item->keymap);
		}
		break;
	}
	case B_SOME_APP_QUIT: {
		int32 team = 0;
		if(B_OK == message->FindInt32("be:team", &team)) {
			int32 index = FindApp(team);
			if (index != -1)
				app_list->RemoveItem(index);
		}
		break;
	}
	case B_SOME_APP_ACTIVATED: {
		int32 nSystemWide = 0;
		if (B_OK == settings->FindInt32("system_wide", &nSystemWide))
			if (nSystemWide != 0)
				break; // do not change keymap

		app_info info;
		if(B_OK == be_roster->GetActiveAppInfo(&info)) {
			if((info.flags & B_BACKGROUND_APP)
				|| (0 == strcmp(info.signature, DESKBAR_SIGNATURE)))
			{
				break; // we don't need any background apps here
			}
			active_team = info.team;
			int32 index = FindApp(info.team);
			if (index != -1) {
				team_keymap *item = (team_keymap *)app_list->ItemAt(index);
				ChangeKeyMapSilent(item->keymap);
			} else {
				team_keymap *item = new team_keymap;
				item->team = info.team;
				item->keymap = 0;
				app_list->AddItem((void *)item);
				ChangeKeyMapSilent(item->keymap);
			}
		}
		break;
	}
		
	}
	BView::MessageReceived(message);
}

int32 DeskView::FindApp(int32 team)
{
	int32 count = app_list->CountItems();
	for(int i=0; i<count; i++) {
		if(((team_keymap*)app_list->ItemAt(i))->team == team)
			return i;
	} 
	return -1;
}

//
void DeskView::Pulse()
{
	UpdateText(false);

	if(!watching) {
		if(B_OK == be_roster->StartWatching(this, B_REQUEST_LAUNCHED | B_REQUEST_QUIT | B_REQUEST_ACTIVATED))
			trace("start watching");
		watching = true;
	}
}

//
void DeskView::MouseDown(BPoint where)
{
	BWindow *window = Window();
	if (window == NULL)
		return;

	BMessage *currentMsg = window->CurrentMessage();
	if (currentMsg == NULL)
		return;

	if (currentMsg->what == B_MOUSE_DOWN) {
		uint32 buttons = 0;
		currentMsg->FindInt32("buttons", (int32 *)&buttons);

		uint32 modifiers = 0;
		currentMsg->FindInt32("modifiers", (int32 *)&modifiers);

		if ((buttons & B_SECONDARY_MOUSE_BUTTON) || (modifiers & B_CONTROL_KEY)) {
			// secondary button was clicked or control key was down, show menu and return
			ShowContextMenu(where);
		} else if (buttons & B_PRIMARY_MOUSE_BUTTON /*&& !disabled*/) {
			SwitchToNextKeyMap();
		} else if (buttons & B_TERTIARY_MOUSE_BUTTON) {
			be_roster->Launch(APP_SIGNATURE);
		}
	}
}

//
void DeskView::ShowContextMenu(BPoint where)
{
	BMessenger messenger(this);
	BPopUpMenu *menu = new BPopUpMenu("Menu", false, false);

	// identical to code in pppdeskbar.cpp
 	BMenuItem *item = 0;

	for (int i=0; i<keymaps->CountItems(); i++) {
		BPath path(((BString *) keymaps->ItemAt(i))->String());
		BMessage *msg = new BMessage(kChangeKeymap);
		msg->AddInt32("change_to", i);
		item = new BMenuItem(path.Leaf(), msg);
		item->SetTarget(this);
		item->SetMarked(i == active_keymap);
		menu->AddItem(item);
	}
	
	menu->AddSeparatorItem();

	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS), new BMessage(kSettings)));
	item->SetTarget(this);

	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS), new BMessage(kAbout)));
	item->SetTarget(this);

	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Quit"), new BMessage(kUnloadNow)));
	item->SetTarget(this);
//	BRect bounds = Bounds();

	menu->Go(ConvertToScreen(where), true, true, true);

/*	
	async_menu_info *pMI = new async_menu_info();
	pMI->menu = menu;
	pMI->where = ConvertToScreen(where);
	pMI->bounds = ConvertToScreen(bounds);
	resume_thread(spawn_thread(ShowContextMenuAsync, "Switcher Menu", B_NORMAL_PRIORITY, pMI) ); 
*/	
}

// Switch to the next keymap in the list
void DeskView::SwitchToNextKeyMap()
{
	int32 keymapsCount = keymaps->CountItems();
	if (keymapsCount > 0)
		ChangeKeyMap((active_keymap + 1) % keymapsCount);
}

// change keymap, when user changed keymap with mouse on deskbar or via hotkey.
void DeskView::ChangeKeyMap(int32 change_to)
{
//	fprintf(stderr, "change to:%" B_PRIi32 "d\n", change_to);
	ChangeKeyMapSilent(change_to);
	bool should_beep = false;
	settings->FindBool("beep", &should_beep);
	if(should_beep)
		system_beep(BEEP_NAME);
}

// silently changes keymap.
void DeskView::ChangeKeyMapSilent(int32 change_to, bool force /*= false*/)
{
	int32 nSystemWide = 0;
	if (B_OK != settings->FindInt32("system_wide", &nSystemWide))
		nSystemWide = 0;

	bool need_switch = force;
	app_info info;
	if(!nSystemWide && B_OK == be_roster->GetActiveAppInfo(&info)) {
		team_keymap *item;
		team_id the_team = info.team;
		if (0 == strcmp(info.signature, DESKBAR_SIGNATURE) && active_team != -1)
			the_team = active_team;
		int32 index = FindApp(the_team);
		if (index != -1) {
			item = (team_keymap *)app_list->ItemAt(index);
			if(item->keymap != change_to) {
				app_list->RemoveItem(index);
				item->keymap = change_to;
				app_list->AddItem(item);
				need_switch = true;
			}
		} else {
			item = new team_keymap;
			item->team = the_team;
			item->keymap = change_to;
			app_list->AddItem((void *)item);
		}
	}
	
	if(change_to != active_keymap)
		need_switch = true;

	if (!need_switch)
		return;

	active_keymap = change_to;
	BString param, name;
	BPath path;
	int32 dir;
	param = "";
	param << "n" << active_keymap;
	settings->FindString(param.String(), &name);
	param = "";
	param << "d" << active_keymap;
	settings->FindInt32(param.String(), &dir);
	find_directory((directory_which)dir, &path);
	if(dir == B_SYSTEM_DATA_DIRECTORY)
		path.Append("Keymaps");
	else	
		path.Append("Keymap");
	path.Append(name.String());
	// open source keymap file
	BFile source_file(path.Path(), B_READ_ONLY);
	off_t size = 0;
	source_file.GetSize(&size);
	char *buf = new char[size];
	// read source keymap to buffer
	source_file.Read((void *)buf, size);
	source_file.Unset();
/*
struct key_map {
	uint32  version;  // compatible version must be 3
	uint32  caps_key;
	uint32  scroll_key;
	uint32  num_key;
	uint32  left_shift_key;
	uint32  right_shift_key;
	uint32  left_command_key;	// from offset 24
	uint32  right_command_key;
	uint32  left_control_key;
	uint32  right_control_key;	// up to 32 bytes
*/
	uint32 keymap_header[10] = { 0 };

	// open target keymap file	
	uint32 om = B_READ_WRITE; 
	BFile target_file(cur_map_path.Path(), om);
	if (sizeof(keymap_header)
			== target_file.ReadAt(0, keymap_header, sizeof(keymap_header))
		&& keymap_header[0] ==  B_BENDIAN_TO_HOST_INT32(3)
		&& size > sizeof(keymap_header))
	{
		// preserve Cmd-switch settings from source file
		memcpy(buf + 24, &keymap_header[6], sizeof(uint32) * 4);
	}

	// write buffer to target keymap file
	if (-1 == target_file.WriteAt(0, (void *)buf, size))
		trace("Target file is NOT okay");
	target_file.Unset();
	DELETE(buf);
	
	// tell system to reload keymap
	uint32 locks = modifiers() & (B_CAPS_LOCK | B_SCROLL_LOCK | B_NUM_LOCK);
	_restore_key_map_();
	set_keyboard_locks(locks);
	Pulse();
}

void DeskView::ShowAboutWindow()
{
	BString str(B_TRANSLATE("Keymap Switcher\n\n"));
	int nameLen = str.Length();
	str << B_TRANSLATE("Copyright " B_UTF8_COPYRIGHT " 1999-2003 Stas Maximov.\n");
	str << B_TRANSLATE("Copyright " B_UTF8_COPYRIGHT " 2008-%YEAR Haiku, Inc.\n");
	str << B_TRANSLATE("Version  %VERSION \n\n");
	str << B_TRANSLATE("Original notice from Stas Maximov:\n");
	str << B_TRANSLATE("Tested and inspired by"
			"\n\tSergey \"fyysik\" Dolgov,"
			"\n\tMaxim \"baza\" Bazarov,"
			"\n\tDenis Korotkov,"
			"\n\tNur Nazmetdinov and others.\n\n");
	str << B_TRANSLATE("Thanks to Pierre Raynaud-Richard and "
			"Robert Polic for BeOS tips.\n\n");
	str << B_TRANSLATE("Special thanks to all BeOS users, "
			"whether they use this app or not"
			" - they're keeping BeOS alive!\n\n");
	str.ReplaceAll("%VERSION", VERSION);
	str.ReplaceAll("%YEAR", "2013");
	BAlert *alert = new BAlert(B_TRANSLATE("About"), str,
			B_TRANSLATE("Okay"), 0, 0, B_WIDTH_AS_USUAL, B_IDEA_ALERT);

	BTextView *view = alert->TextView();
	view->SetStylable(true);
	BFont font;
	view->GetFont(&font);
	font.SetSize(font.Size() * 1.5);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, nameLen, &font);

	alert->Go();
}

