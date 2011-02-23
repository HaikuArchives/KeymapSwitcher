// Switcher
// Copyright © 1999-2003 by Stas Maximov
// All rights reserved

#include <Window.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Box.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <Menu.h>
#include <Directory.h>
#include <Entry.h>
#include <CheckBox.h>
#include <Roster.h>
#include <Button.h>
#include <StringView.h>
#include <ScrollView.h>
#include <TextView.h>
#include <ListView.h>
#include <OutlineListView.h>
#include <Resources.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <Alert.h>
#include <stdio.h>
#include <syslog.h>
#include <Catalog.h>
#include <Locale.h>

#include "Settings.h"
#include "SettingsWindow.h"
#include "DeskView.h"
#include "KeymapSwitcher.h"
#include "Replicator.h"
#include "Resource.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "SwitcherSettingsWindow"

	//CAUTION
#define trace(x...) (void(0)) // syslog(0, x);
	//NO_CAUTION :)

	// the initial dimensions of the window.
	const float WINDOW_X      = 100;
	const float WINDOW_Y      = 100;
	const float WINDOW_WIDTH  = 330 + 60;
	const float WINDOW_HEIGHT = 210;
	const float BUTTON_WIDTH = 140;

	// hot-keys
//	const uint32	KEY_LCTRL_SHIFT = 0x2000;
//	const uint32	KEY_OPT_SHIFT = 0x2001;
//	const uint32	KEY_ALT_SHIFT = 0x2002;
//	const uint32	KEY_SHIFT_SHIFT = 0x2003;

	// some messages
	enum {
		MSG_PRIMARY_MAP = 0x1000,
		MSG_ALT_MAP,
		MSG_KEYMAPS_CHANGED,
		MSG_BEEP_CHECKBOX_TOGGLED,
		MSG_HOTKEY_CHANGED,
		MSG_SAVE_SETTINGS,
		MSG_CLOSE,
		MSG_ABOUT,
		MSG_BEEP_SETUP,
		MSG_SETTINGS_CHANGED,
		MSG_ACTIVE_ITEM_DRAGGED,
		MSG_ITEM_DRAGGED,
		MSG_REMOVE_ACTIVE_ITEM,
		MSG_MOVE_ACTIVE_ITEM,
		MSG_BUTTON_ADD_ITEM,
		MSG_BUTTON_REMOVE_ITEM
	};

	KeymapItem::KeymapItem(KeymapItem *item) : BStringItem(((KeymapItem*) item)->Text()){
		dir = (int32) item->Dir();
		real_name = item->RealName();
	}

	KeymapItem::KeymapItem(const char *text, const char *real_name, int32 dir) : BStringItem(text) {
		this->dir = dir;
		this->real_name = real_name;
	}

	KeymapListView::KeymapListView(BRect r, const char *name) : BListView(r, name) {
	}

	bool KeymapListView::InitiateDrag(BPoint point, int32 index, bool wasSelected) {
		BListView::InitiateDrag(point, index, wasSelected);
		BMessage message(MSG_ACTIVE_ITEM_DRAGGED);
		message.AddInt32("index", index);
		DragMessage(&message, ItemFrame(index));
		return true;
	}

	void KeymapListView::MessageReceived(BMessage *message) {
		switch (message->what) {
			// new item arrived
			case MSG_ITEM_DRAGGED: {
				BMessage notify(MSG_KEYMAPS_CHANGED);
				Window()->PostMessage(&notify);
				KeymapItem *item;
				message->FindPointer("keymap_item", (void **)&item);
				DeselectAll();
				int index = -1;
				index = IndexOf(ConvertFromScreen(message->DropPoint()));
				KeymapItem *new_item = new KeymapItem(item);
				BString name = new_item->Text();
				if(B_BEOS_DATA_DIRECTORY == new_item->Dir())
					name += B_TRANSLATE(" (System)");
				else
					name += B_TRANSLATE(" (User)");
				new_item->SetText(name.String());
				if(0 > index) {
					AddItem(new_item);
					index = IndexOf(new_item);
				} else AddItem(new_item, index);
				Select(index);
				//trace(index);
				ScrollToSelection();
				break;
			}
			// item dropped somewhere out there, remove it 
			case MSG_REMOVE_ACTIVE_ITEM: {
				BMessage notify(MSG_KEYMAPS_CHANGED);
				Window()->PostMessage(&notify);
				int32 index = -1;
				message->FindInt32("index", &index);
				delete (dynamic_cast<KeymapItem*> (RemoveItem(index)));
				break;
			}
			// item dropped inside, rearrange items
			case MSG_ACTIVE_ITEM_DRAGGED: {
				BMessage notify(MSG_KEYMAPS_CHANGED);
				Window()->PostMessage(&notify);
				int32 draggedIndex = -1;
				message->FindInt32("index", &draggedIndex);
				int32 index = -1;
				index = IndexOf(ConvertFromScreen(message->DropPoint()));
				DeselectAll();
				if (0>index) 
					index = CountItems() - 1;
				MoveItem(draggedIndex, index);
				Select(index);
				ScrollToSelection();
				//trace(index);
				return;						
			}
		}
		BListView::MessageReceived(message);
	}
		
	KeymapOutlineListView::KeymapOutlineListView(BRect r, const char *name) : BOutlineListView(r, name) {
	}

	bool KeymapOutlineListView::InitiateDrag(BPoint point, int32 index, bool wasSelected) {
		BOutlineListView::InitiateDrag(point, index, wasSelected);
		BMessage message(MSG_ITEM_DRAGGED);
		message.AddInt32("index", index);
		message.AddPointer("keymap_item", ItemAt(index));
		if (NULL != Superitem(ItemAt(index)))
			DragMessage(&message, ItemFrame(index));
		else return false;
		return true;
	}

	//  construct main window
	SettingsWindow::SettingsWindow() : 
			BWindow(BRect(WINDOW_X, 
				WINDOW_Y, 
								WINDOW_X + WINDOW_WIDTH,
						WINDOW_Y + WINDOW_HEIGHT),
						B_TRANSLATE("Keymap Switcher"),
						B_TITLED_WINDOW, 
						B_NOT_ZOOMABLE | B_NOT_RESIZABLE) {
		trace("read settings");
		settings = new Settings("Switcher");
		keymaps_changed = false;
		Lock();
		
		BBox *box = new BBox(Bounds(),B_EMPTY_STRING, 
					B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, 
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, 
					B_PLAIN_BORDER);
		AddChild(box);

		BRect r = box->Bounds();
		r.InsetBy(10,5);
		r.bottom = r.top + 14;
		r.right = r.left + 200;
		box->AddChild(new BStringView(r, "string0", B_TRANSLATE("Selected keymaps:")));
		r.OffsetBy(0, 18);
		r.bottom = r.top + 50;
		selected_list = new KeymapListView(r, "selected_list");
		box->AddChild(new BScrollView("scroll_selected", selected_list, 
			B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, true));
		
		// fill selected keymaps list
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
			settings->FindInt32(param.String(), &dir);
			find_directory((directory_which)dir, &path);
			if(dir == B_BEOS_DATA_DIRECTORY)
				path.Append("Keymaps");
			else
				path.Append("Keymap");
			path.Append(name.String());

			BString display_name = name;
			if(dir == B_BEOS_DATA_DIRECTORY)
				display_name += B_TRANSLATE(" (System)");
			else	display_name += B_TRANSLATE(" (User)");
			selected_list->AddItem(new KeymapItem(display_name.String(), name.String(), dir));
		}
		
		r.OffsetBy(0, 58);
		r.bottom = r.top + 14;
		r.right  = r.left + 170;
		box->AddChild(new BStringView(r, "string1", B_TRANSLATE("Available keymaps:")));
		
		//sz: 05.05.07: patch: "add" and "remove" keymap buttons added
		BRect rA(r);
		rA.OffsetBy(0, -3);
		rA.left += 170;
		rA.right  = rA.left + 17;
		rA.bottom = rA.top + 16;
		BMoveButton *buttonEx = new BMoveButton(rA, "add_keymap_button", 
						R_ResAddButton, R_ResAddButtonPressed, R_ResAddButtonDisabled,
			new BMessage(MSG_BUTTON_ADD_ITEM));
		box->AddChild(buttonEx);
		buttonEx->SetTarget(this);
		
		rA.OffsetBy(27, 0);
		buttonEx = new BMoveButton(rA, "remove_keymap_button",
						R_ResRemoveButton, R_ResRemoveButtonPressed, R_ResRemoveButtonDisabled, 
			new BMessage(MSG_BUTTON_REMOVE_ITEM));
		box->AddChild(buttonEx);
		buttonEx->SetTarget(this);
		
		r.OffsetBy(0, 18);
		r.bottom = r.top + 100;
		r.right  = r.left + 200;
		available_list = new KeymapOutlineListView(r, "selected_list");
		box->AddChild(new BScrollView("scroll_available", available_list, 
			B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, true));

		// populate available_list
		find_directory(B_BEOS_DATA_DIRECTORY, &path);
		path.Append("Keymaps");
		BDirectory *dir = new BDirectory(path.Path());
		long maps = dir->CountEntries();
		entry_ref ref;
		KeymapItem *keymap_node = new KeymapItem(B_TRANSLATE("System"), NULL, 0);
		available_list->AddItem(keymap_node);
		
		BList temp_list;
		for (int i=0; i<maps; i++) {
			dir->GetNextRef(&ref);
			temp_list.AddItem(new KeymapItem(ref.name, ref.name, (int32) B_BEOS_DATA_DIRECTORY), 0);
		}
		for (int i=0; i<temp_list.CountItems(); i++)
			available_list->AddUnder((KeymapItem*) temp_list.ItemAt(i), keymap_node);
		temp_list.MakeEmpty();	
		delete dir;	

		directory_which userDir = B_USER_SETTINGS_DIRECTORY;
//		directory_which userDir = B_USER_DATA_DIRECTORY;

		find_directory(userDir, &path);

		path.Append("Keymap");
		dir = new BDirectory(path.Path());
		maps = dir->CountEntries();
		if(0 != maps) {
			keymap_node = new KeymapItem(B_TRANSLATE("User"), NULL, 0);
			available_list->AddItem(keymap_node);
		}
			
		for (int i=0; i<maps; i++) {
			dir->GetNextRef(&ref);
			temp_list.AddItem(new KeymapItem(ref.name, ref.name, (int32) userDir), 0);
		}
		for (int i=0; i<temp_list.CountItems(); i++)
			available_list->AddUnder((KeymapItem*) temp_list.ItemAt(i), keymap_node);
		delete dir;
		
		// add hotkey and beep
		int32 hotkey;
		bool beep;
		settings->FindInt32("hotkey", &hotkey);
		settings->FindBool("beep", &beep);

		r.OffsetTo((WINDOW_WIDTH-BUTTON_WIDTH)-12, 5);
		r.bottom = r.top + 14;
		r.right = r.left + BUTTON_WIDTH;
		box->AddChild(new BStringView(r, "string1", B_TRANSLATE("Hotkey:")));

		r.OffsetBy(0, 14);
		BPopUpMenu *pop_key;
		BString temp = "none";
		if(hotkey == KEY_LCTRL_SHIFT)
			temp = "Ctrl+Shift";
		if(hotkey == KEY_ALT_SHIFT)
			temp = "Alt+Shift";
		if(hotkey == KEY_SHIFT_SHIFT)
			temp = "Shift+Shift";
//		if(hotkey == KEY_CAPS_LOCK)
//			temp = "Caps Lock";
		if(hotkey == KEY_SCROLL_LOCK)
			temp = "Scroll Lock";
		pop_key = new BPopUpMenu(temp.String());

		BMessage *msg = new BMessage(MSG_HOTKEY_CHANGED);
		msg->AddInt32("hotkey", KEY_LCTRL_SHIFT);
		pop_key->AddItem(new BMenuItem("Ctrl+Shift", msg));

		msg = new BMessage(MSG_HOTKEY_CHANGED);
		msg->AddInt32("hotkey", KEY_ALT_SHIFT);
		pop_key->AddItem(new BMenuItem("Alt+Shift", msg));

		msg = new BMessage(MSG_HOTKEY_CHANGED);
		msg->AddInt32("hotkey", KEY_SHIFT_SHIFT);
		pop_key->AddItem(new BMenuItem("Shift+Shift", msg));
		
//		msg = new BMessage(MSG_HOTKEY_CHANGED);
//		msg->AddInt32("hotkey", KEY_CAPS_LOCK);
//		pop_key->AddItem(new BMenuItem("Caps Lock", msg));
		
		msg = new BMessage(MSG_HOTKEY_CHANGED);
		msg->AddInt32("hotkey", KEY_SCROLL_LOCK);
		pop_key->AddItem(new BMenuItem("Scroll Lock", msg));
		
		BMenuField * menu_field = new BMenuField(r, "HotKey", NULL, pop_key);
		menu_field->SetDivider(0);
		
		box->AddChild(menu_field);

		r.OffsetBy(0, 39);
	/*	BCheckBox *will_beep = new BCheckBox(r,
					"willbeep", "Beep",
					new BMessage(MSG_BEEP_CHECKBOX_TOGGLED), B_FOLLOW_LEFT);
			  
		will_beep->SetValue((beep?B_CONTROL_ON:B_CONTROL_OFF));
		box->AddChild(will_beep);
	*/
		long lTop = 46;
		BButton *button = new BButton(BRect((WINDOW_WIDTH-BUTTON_WIDTH)-12,lTop,(WINDOW_WIDTH)-12,1),
			"beep_button",B_TRANSLATE("Beep setup" B_UTF8_ELLIPSIS),
			new BMessage(MSG_BEEP_SETUP), B_FOLLOW_RIGHT);
		box->AddChild(button);
		
		
		lTop += 31;
		/*BButton **/button = new BButton(BRect((WINDOW_WIDTH-BUTTON_WIDTH)-12,lTop,(WINDOW_WIDTH)-12,1),
			"about_button",B_TRANSLATE("About" B_UTF8_ELLIPSIS),
			new BMessage(MSG_ABOUT), B_FOLLOW_RIGHT);
		box->AddChild(button);

		lTop += 68;
		button = new BButton(BRect((WINDOW_WIDTH-BUTTON_WIDTH)-12,lTop, (WINDOW_WIDTH)-12,1),
			"save_button",B_TRANSLATE("Apply"),
			new BMessage(MSG_SAVE_SETTINGS), B_FOLLOW_RIGHT);
		box->AddChild(button);
		button->MakeDefault(TRUE); 
		
		lTop += 31;
		button = new BButton(BRect((WINDOW_WIDTH-BUTTON_WIDTH)-12,lTop,(WINDOW_WIDTH)-12,1),
			"close_button",B_TRANSLATE("Revert"),
			new BMessage(MSG_CLOSE), B_FOLLOW_RIGHT);
		box->AddChild(button);

		hotkey_changed = false;
		Unlock();
		Show();
	}

	// destructor
	SettingsWindow::~SettingsWindow() {
		while(0 < available_list->CountItems())
			delete (dynamic_cast<KeymapItem*> (available_list->RemoveItem(0L)));
		while(0 < selected_list->CountItems())
			delete (dynamic_cast<KeymapItem*> (selected_list->RemoveItem(0L)));
	}

	// process message
	void SettingsWindow::MessageReceived(BMessage *msg) {
		switch (msg->what) {
		case MSG_ACTIVE_ITEM_DRAGGED: {
			BMessage reply(*msg);
			reply.what = MSG_REMOVE_ACTIVE_ITEM;
			msg->ReturnAddress().SendMessage(&reply);
			break;
		}
		case MSG_BUTTON_ADD_ITEM: {
			int32 index = available_list->CurrentSelection(0);
			//int32 i = 0, z = 6;
			//index *= z/i;
			if(index >= 0) {
				BMessage message(MSG_ITEM_DRAGGED);
				message.AddInt32("index", index);
				message.AddPointer("keymap_item", available_list->ItemAt(index));
				PostMessage(&message, selected_list);
			}
		}
		break;
		case MSG_BUTTON_REMOVE_ITEM: {
			int32 index = selected_list->CurrentSelection(0);
			//int32 i = 0, z = 6;
			//index *= z/i;
			if(index >= 0) {
				BMessage message(MSG_REMOVE_ACTIVE_ITEM);
				message.AddInt32("index", index);
			//	message.AddPointer("keymap_item", selected_list->ItemAt(index));
				PostMessage(&message, selected_list);
			}
		}
		break;
		case MSG_KEYMAPS_CHANGED:
			trace("keymaps changed");
			keymaps_changed = true;
			break;
		case MSG_BEEP_CHECKBOX_TOGGLED: {
			bool beep;		
			settings->FindBool("beep", &beep);
				settings->SetBool("beep", !beep);
			break;
		}
		case MSG_HOTKEY_CHANGED: {
			int32 temp = 0;
			if (B_OK==msg->FindInt32("hotkey",&temp))
				settings->SetInt32("hotkey", temp);
			hotkey_changed = true;
			break;
		}
		case MSG_BEEP_SETUP: {
			be_roster->Launch("application/x-vnd.Haiku-Sounds");
			break;
		}
		case MSG_SAVE_SETTINGS: {
			// delete all keymaps from settings
			if(keymaps_changed) {
				int32 keymaps = 0;
				settings->FindInt32("keymaps", &keymaps);
				BString param;
				for (int i=0; i<keymaps; i++) {
					param = "";
					param << "d" << i;
					settings->RemoveName(param.String());
					param = "";
					param << "n" << i;
					settings->RemoveName(param.String());
				}
				// now save keymaps
				settings->SetInt32("keymaps", selected_list->CountItems());
				for (int i=0; i<selected_list->CountItems(); i++) {
					KeymapItem *item = (KeymapItem*) selected_list->ItemAt(i);
					trace((char*)(item->RealName()));
					param = "";
					param << "d" << i;
					settings->SetInt32(param.String(), item->Dir());
					param = "";
					param << "n" << i;
					settings->SetString(param.String(), item->RealName());	
				}
			}
			trace("settings saved!");
			settings->Save();
			delete settings; // we save settings in its destructor
			::UpdateIndicator(MSG_UPDATESETTINGS);
			Close();
			break;
		}
		case MSG_CLOSE:
			Close();
			break;
			
		case MSG_ABOUT:
			ShowAboutWindow();
			break;
		default:
			break;
		}
		BWindow::MessageReceived(msg);
	}

	void SettingsWindow::ShowAboutWindow()  {
		//BString strName = "Keymap Switcher";
		BString str(B_TRANSLATE("Keymap Switcher\n\n"));
		str << B_TRANSLATE("Copyright " B_UTF8_COPYRIGHT " 1999-2003 Stas Maximov.\n");
		str << B_TRANSLATE("Copyright " B_UTF8_COPYRIGHT " 2008-2010 Haiku, Inc.\n");
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
		BAlert *alert = new BAlert(B_TRANSLATE("About"), str,
					B_TRANSLATE("Okay"), 0, 0, B_WIDTH_AS_USUAL, B_IDEA_ALERT);
	
	BTextView *view = alert->TextView();
	view->SetStylable(true);
	BFont font;
	view->GetFont(&font);
	font.SetSize(font.Size() * 1.5);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 15/*strName.Length()*/, &font);
	
	alert->Go();
}

// process quit message
bool SettingsWindow::QuitRequested() {
	return BWindow::QuitRequested(); // cause otherwise it will kill Deskbar
}

static BPicture sPicture;

BMoveButton::BMoveButton(BRect 		frame, const char *name,  
						uint32		resIdOff, uint32 resIdOn, uint32 resIdDisabled,
						BMessage	*message,
						uint32		behavior		/*= B_ONE_STATE_BUTTON*/,
						uint32		resizingMode 	/*= B_FOLLOW_LEFT| B_FOLLOW_TOP*/,
						uint32		flags			/*= B_WILL_DRAW	 | B_NAVIGABLE*/):
		BPictureButton(frame, name, &sPicture, &sPicture, message, behavior, resizingMode, flags),
		fResIdOff(resIdOff), fResIdOn(resIdOn), fResIdDisabled(resIdDisabled)
{
}

BMoveButton::~BMoveButton()
{
}

void BMoveButton::AttachedToWindow()
{
 	//app_info appInfo;
	//be_app->GetAppInfo(&appInfo);
	//BFile appFile;
	//appFile.SetTo(&appInfo.ref, B_READ_ONLY);
	//	debugger("app");
	entry_ref ref;
	be_roster->FindApp(APP_SIGNATURE, &ref);
	BEntry entry(&ref);
	BFile appFile;
	appFile.SetTo(&entry, B_READ_ONLY);
	BResources appResources;
	appResources.SetTo(&appFile);
	
	//BResources *appResources = be_app->AppResources();

	LoadPicture(&appResources, &fPicOff,      fResIdOff);
	LoadPicture(&appResources, &fPicOn,       fResIdOn);
	LoadPicture(&appResources, &fPicDisabled, fResIdDisabled);

	SetEnabledOff(&fPicOff);
	SetEnabledOn(&fPicOn);
	SetDisabledOff(&fPicDisabled);
	SetDisabledOn(&fPicDisabled);
}


void BMoveButton::GetPreferredSize(float *width, float *height)
{
	*width  = 17.;
	*height = 16.;
}

status_t BMoveButton::LoadPicture(BResources *resFrom, BPicture *picTo, uint32 resId)
{
	size_t size = 0;
	const void *data = resFrom->LoadResource(B_MESSAGE_TYPE, resId, &size);
	BMemoryIO stream(data, size);
	stream.Seek(0, SEEK_SET);
	BMessage archive;
	archive.Unflatten(&stream);

	BBitmap *bmp = new BBitmap(&archive);

	BView view(bmp->Bounds(), "", 0, 0);
	AddChild(&view);
	view.BeginPicture(picTo);
	rgb_color color= {220,220,220, 255};
	view.SetHighColor(color);
	view.FillRect(view.Bounds());
	view.SetDrawingMode(B_OP_OVER);
	view.DrawBitmap(bmp, BPoint(0, 0));
	view.EndPicture();
	RemoveChild(&view);
		
	return B_OK;
}

