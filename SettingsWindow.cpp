// Switcher
// Copyright © 1999-2003 by Stas Maximov
// All rights reserved


#include "SettingsWindow.h"

#include <stdio.h>
#include <syslog.h>

#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>

#include "DeskView.h"
#include "KeymapSwitcher.h"
#include "Replicator.h"
#include "Settings.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "SwitcherSettingsWindow"

//CAUTION
//#define trace(x...) (void(0)) // syslog(0, x);
#define trace(x...) printf(x);
//NO_CAUTION :)

// the initial dimensions of the window.
const float WINDOW_X      = 100;
const float WINDOW_Y      = 100;
const float WINDOW_WIDTH  = 600;
const float WINDOW_HEIGHT = 600;
//const float BUTTON_WIDTH  = 140;
const float X_INSET	= 10;
const float Y_INSET	= 10;

const float fYSpacing = 5.;
const float fXSpacing = 5.;
const float kBmpBtnX = 17.;
const float kBmpBtnY = 16.;

//  construct main window
SettingsWindow::SettingsWindow(bool fromDeskbar) 
				: 
				BWindow(BRect(WINDOW_X, WINDOW_Y,
						WINDOW_X + WINDOW_WIDTH, WINDOW_Y + WINDOW_HEIGHT),
						B_TRANSLATE("Keymap Switcher"),
						B_TITLED_WINDOW, 0) 
{
	trace("read settings");
	settings = new Settings("Switcher");
	keymaps_changed = false;
	from_deskbar = fromDeskbar;

	Lock();

	// "client" area view
	BBox *box = new BBox(Bounds(), B_EMPTY_STRING,
						B_FOLLOW_NONE,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
						B_PLAIN_BORDER);
	AddChild(box);

	BRect RC = box->Bounds();
	RC.InsetBy(X_INSET, Y_INSET);

	// create labels
	BRect rc(RC.LeftTop(), RC.LeftTop());
	BStringView* selMapsLabel = new BStringView(rc, "string0",
										B_TRANSLATE("Selected keymaps:"));
	box->AddChild(selMapsLabel);
	selMapsLabel->ResizeToPreferred();

	BStringView* allMapsLabel = new BStringView(rc, "string1",
										B_TRANSLATE("Available keymaps:"));
	box->AddChild(allMapsLabel);
	allMapsLabel->ResizeToPreferred();

	// create add/del button
	BRect rcBtn(RC);
	rcBtn.right  = rcBtn.left + kBmpBtnX;
	rcBtn.bottom = rcBtn.top  + kBmpBtnY;
	BMoveButton* addButton = new BMoveButton(rcBtn, "add_keymap_button", 
					R_ResAddButton, R_ResAddButtonPressed, R_ResAddButtonDisabled,
		new BMessage(MSG_BUTTON_ADD_ITEM), B_ONE_STATE_BUTTON, B_FOLLOW_RIGHT);
	box->AddChild(addButton);
	addButton->SetTarget(this);
	
	BMoveButton* delButton = new BMoveButton(rcBtn, "remove_keymap_button",
					R_ResRemoveButton, R_ResRemoveButtonPressed, R_ResRemoveButtonDisabled, 
		new BMessage(MSG_BUTTON_REMOVE_ITEM), B_ONE_STATE_BUTTON, B_FOLLOW_RIGHT);
	box->AddChild(delButton);
	delButton->SetTarget(this);

	// calculate the layout of "lists column" of controls
	float fLineHeight = 0.f;
	float fMaxListsWidth = 0.f;
	selMapsLabel->GetPreferredSize(&fMaxListsWidth, &fLineHeight);
	
	float fAllLabelWidth = 0.;
	allMapsLabel->GetPreferredSize(&fAllLabelWidth, &fLineHeight);
	fMaxListsWidth = fmax(fMaxListsWidth, fAllLabelWidth + (kBmpBtnX + fYSpacing) * 2);

	selected_list = new KeymapListView(rc, "selected_list");
	BScrollView* scrollViewSel =  new BScrollView("scroll_selected", selected_list, 
		B_FOLLOW_LEFT_RIGHT, 0, false, true);
	box->AddChild(scrollViewSel);
	
	float fScrollBarWidth = scrollViewSel->Bounds().right - selected_list->Bounds().right;
	float fScrollYInset = scrollViewSel->Bounds().Height() - selected_list->Bounds().Height();

	fMaxListsWidth += fScrollBarWidth;

	available_list = new KeymapOutlineListView(rc, "selected_list");
	BScrollView* scrollViewAll = new BScrollView("scroll_available", available_list, 
		B_FOLLOW_ALL_SIDES, 0, false, true);
	box->AddChild(scrollViewAll);
	
	// reposition of "lists column" controls
	BPoint ptOrg(RC.LeftTop());
	selMapsLabel->ResizeToPreferred();
	selMapsLabel->MoveTo(ptOrg.x, ptOrg.y);

	ptOrg.y += selMapsLabel->Bounds().Height() + fYSpacing;
	scrollViewSel->MoveTo(ptOrg.x, ptOrg.y);
	scrollViewSel->ResizeTo(fMaxListsWidth, fLineHeight * 4 + fScrollYInset);

	ptOrg.y += scrollViewSel->Bounds().Height() + fYSpacing;
	BPoint ptBtnOrg(ptOrg);
	allMapsLabel->ResizeToPreferred();
	ptOrg.y += kBmpBtnY - allMapsLabel->Bounds().Height();
	allMapsLabel->MoveTo(ptOrg);

	// buttons
	ptBtnOrg.x += fAllLabelWidth + fXSpacing;
	addButton->MoveTo(ptBtnOrg);
	ptBtnOrg.x += kBmpBtnX + fXSpacing;
	delButton->MoveTo(ptBtnOrg);

	// and finally the all keymaps list
	ptOrg.y += allMapsLabel->Bounds().Height() + fYSpacing;
	scrollViewAll->MoveTo(ptOrg.x, ptOrg.y);
	scrollViewAll->ResizeTo(fMaxListsWidth, fLineHeight * 8 + fScrollYInset);

	float fMaxWindowHeight = ptOrg.y + scrollViewAll->Bounds().Height() + Y_INSET;

	// populate selected keymaps list
	int32 count = 0;
	settings->FindInt32("keymaps", &count); // retrieve keymaps number
	// read all the keymaps
	for (int32 i = 0; i<count; i++) {
		BString param("n");
		param << i;
		
		BString name;
		settings->FindString(param.String(), &name);

		param = "d";
	   	param << i;
		int32 dir = 0;
		settings->FindInt32(param.String(), &dir);

		BPath path;
		find_directory((directory_which)dir, &path);
		if(dir == B_BEOS_DATA_DIRECTORY)
			path.Append("Keymaps");
		else
			path.Append("Keymap");
		path.Append(name.String());

		BString display_name(name);
		if(dir == B_BEOS_DATA_DIRECTORY)
			display_name += B_TRANSLATE(" (system)");
		else
			display_name += B_TRANSLATE(" (user)");
		selected_list->AddItem(new KeymapItem(display_name.String(), name.String(), dir));
	}

	// populate all available keymaps list - system keymaps
	struct _d {
		directory_which which;
		const char* subDir;
		const char* name;
	} d[] = {
		{ B_BEOS_DATA_DIRECTORY,	 "Keymaps", B_TRANSLATE("System") },
		{ B_USER_SETTINGS_DIRECTORY, "Keymap",  B_TRANSLATE("User")   }
	};

	for (size_t i = 0; i < sizeof(d)/sizeof(d[0]); i++) {
		BPath path;
		find_directory(d[i].which, &path);
		path.Append(d[i].subDir);

		BDirectory dir(path.Path());
		long count = dir.CountEntries();
		if(count <= 0)
			continue;

		KeymapItem *keymap_node = new KeymapItem(d[i].name, NULL, 0);
		available_list->AddItem(keymap_node);

		BEntry entry;
		BList list;
		for (int i = 0; i < count; i++) {
			dir.GetNextEntry(&entry);
			struct stat st;
			entry.GetStat(&st);
			if(S_ISDIR(st.st_mode))
				continue;

			entry_ref ref;
			entry.GetRef(&ref);
			list.AddItem(new KeymapItem(ref.name, ref.name, (int32)d[i].which ), 0);
		}

		for (int i = 0; i < list.CountItems(); i++) {
			available_list->AddUnder((KeymapItem*) list.ItemAt(i), keymap_node);
		}
	}

	// create hot-key selector
	BStringView* selectorLabel = new BStringView(rc, "string1",
								B_TRANSLATE("Hotkey:"), B_FOLLOW_RIGHT);
	box->AddChild(selectorLabel);
	
	float fMaxButtonsWidth = 0.f;
	selectorLabel->GetPreferredSize(&fMaxButtonsWidth, &fLineHeight);

	// create and populate selector menu
	int32 hotkey = 0;
	settings->FindInt32("hotkey", &hotkey);

	struct _pair {
		int32		hotkey;
		const char*	name;
	} a[] = {
		{ KEY_LCTRL_SHIFT,	"Ctrl+Shift" },
		{ KEY_ALT_SHIFT,	"Alt+Shift" },
		{ KEY_SHIFT_SHIFT,	"Shift+Shift" },
		{ KEY_CAPS_LOCK,	"Caps Lock" },
		{ KEY_SCROLL_LOCK,	"Scroll Lock" }
	};

	const char* menuName = "none";
	for (size_t i = 0; i < sizeof(a)/sizeof(a[0]); i++)
		if (a[i].hotkey == hotkey)
			menuName = a[i].name;

	BPopUpMenu *pop_key = new BPopUpMenu(menuName);

	for (size_t i = 0; i < sizeof(a)/sizeof(a[0]); i++) {
		BMessage *msg = new BMessage(MSG_HOTKEY_CHANGED);
		msg->AddInt32("hotkey", a[i].hotkey);
		pop_key->AddItem(new BMenuItem(a[i].name, msg));
	}
	
	BMenuField * menuField = new BMenuField(rc, "HotKey", NULL, pop_key, B_FOLLOW_RIGHT);
	menuField->SetDivider(0);
	
	box->AddChild(menuField);

	BPoint pt(0, 0);
	menuField->GetPreferredSize(&pt.x, &pt.y);
	fMaxButtonsWidth = fmax(fMaxButtonsWidth, pt.x);

	// create buttons
	struct _button {
		const char* name;
		const char* label;
		int32		message;
		int32		flags;
		BButton*	button;
		BPoint		pt;
	} b[] = {
		{ "beep_button", B_TRANSLATE("Beep setup" B_UTF8_ELLIPSIS),
					MSG_BEEP_SETUP, B_FOLLOW_RIGHT, NULL },
		{ "about_button", B_TRANSLATE("About" B_UTF8_ELLIPSIS),
					MSG_ABOUT, B_FOLLOW_RIGHT, NULL },
		{ "save_button", !AlreadyInDeskbar() ? B_TRANSLATE("Install in Deskbar")
									: B_TRANSLATE("Apply"),
					MSG_SAVE_SETTINGS, B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT, NULL },
		{ "close_button", !AlreadyInDeskbar() ? B_TRANSLATE("No, thanks. Exit")
									: (from_deskbar) ? B_TRANSLATE("Cancel")
												: B_TRANSLATE("Exit"),
					MSG_CLOSE, B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT, NULL },
	};

	for (size_t i = 0; i < sizeof(b)/sizeof(b[0]); i++) {
		b[i].button = new BButton(rc, b[i].name, b[i].label, 
							new BMessage(b[i].message), b[i].flags);
		box->AddChild(b[i].button);
		b[i].button->ResizeToPreferred();
		b[i].button->GetPreferredSize(&b[i].pt.x, &b[i].pt.y);
		fMaxButtonsWidth = fmax(fMaxButtonsWidth, b[i].pt.x);
	}

	b[2].button->MakeDefault(TRUE);
	b[2].pt.y += fYSpacing;

	ptOrg = RC.LeftTop();
	ptOrg.x += fMaxListsWidth + X_INSET;
	selectorLabel->ResizeToPreferred();
	selectorLabel->MoveTo(ptOrg);

	ptOrg.y += selectorLabel->Bounds().Height() + fYSpacing;
	BPoint ptSelector(ptOrg);
	menuField->ResizeToPreferred();
	menuField->MoveTo(ptSelector);
	
	ptOrg.y += menuField->Bounds().Height() + fYSpacing * 5;

	b[0].button->MoveTo(ptOrg);
	ptOrg.y += b[0].pt.y + fYSpacing;
	b[1].button->MoveTo(ptOrg);
	ptOrg.y += b[1].pt.y + fYSpacing;
	
	fMaxWindowHeight = fmax(fMaxWindowHeight,
							Y_INSET + fYSpacing * 2 + b[2].pt.y + b[3].pt.y);

	ptOrg.y = fMaxWindowHeight - Y_INSET - b[3].pt.y;
	b[3].button->MoveTo(ptOrg);
	ptOrg.y -= b[2].pt.y + fYSpacing;
	b[2].button->MoveTo(ptOrg);

	for (size_t i = 0; i < sizeof(b)/sizeof(b[0]); i++)
		b[i].button->ResizeTo(fMaxButtonsWidth, b[i].pt.y); 

	ptSelector.x += fMaxButtonsWidth - menuField->Bounds().Width();
	menuField->MoveTo(ptSelector);

	float fMaxWindowWidth = fMaxListsWidth + fMaxButtonsWidth + X_INSET * 3;

	// set resize/zoom policy
	float fwMin, fwMax, fhMin, fhMax;
	GetSizeLimits(&fwMin, &fwMax, &fhMin, &fhMax);

	fwMin = fMaxWindowWidth;
	fhMin = fMaxWindowHeight;
	fwMax = fMaxWindowWidth * 1.5f;

	SetSizeLimits(fMaxWindowWidth, fMaxWindowWidth * 1.5f,
					fMaxWindowHeight, fMaxWindowHeight * 3.f);
	SetZoomLimits(fwMax, fhMax);

	// set the size
	ResizeTo(fMaxWindowWidth, fMaxWindowHeight);

	box->SetResizingMode(B_FOLLOW_ALL_SIDES);
	
	// resize a bit from minimal layout - controls will follow!
	ResizeTo(fMaxWindowWidth * 1.1f, fMaxWindowHeight * 1.1f);

	BScreen screen(this);
	BRect rcScreen(0, 0, 640, 480);
	if(screen.IsValid())
		rcScreen = screen.Frame();

	ptOrg.x = rcScreen.left + (rcScreen.Width() - Bounds().Width()) / 2;
	ptOrg.y = rcScreen.top + (rcScreen.Height() - Bounds().Height()) / 3;
	MoveTo(ptOrg);

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
	/*case MSG_BEEP_CHECKBOX_TOGGLED: {
		bool beep;		
		settings->FindBool("beep", &beep);
			settings->SetBool("beep", !beep);
		break;
	}*/
	case MSG_HOTKEY_CHANGED: {
		int32 temp = 0;
		if (B_OK==msg->FindInt32("hotkey",&temp))
			settings->SetInt32("hotkey", temp);
		hotkey_changed = true;
		break;
	}
	case MSG_BEEP_SETUP: {
		be_roster->Launch(SOUNDS_PREF_SIGNATURE);
		break;
	}
	case MSG_SAVE_SETTINGS: {
		if (!AlreadyInDeskbar()) {
			add_system_beep_event(BEEP_NAME);

			BDeskbar deskbar;
			entry_ref ref;
			be_roster->FindApp(APP_SIGNATURE, &ref);
			deskbar.AddItem(&ref);
		}
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
		::UpdateIndicator(DeskView::MSG_UPDATESETTINGS);
		if(!from_deskbar) {
			be_app->PostMessage(B_QUIT_REQUESTED);
		} else
			Close();
		break;
	}
	case MSG_CLOSE:
		if(!from_deskbar) {
			be_app->PostMessage(B_QUIT_REQUESTED);
		} else
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
	BString str(B_TRANSLATE("Keymap Switcher\n\n"));
	int nameLen = str.Length();
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
	view->SetFontAndColor(0, nameLen, &font);

	alert->Go();
}

bool SettingsWindow::AlreadyInDeskbar()
{
	if (from_deskbar)
		return true; // in any case - avoid creating BDeskbar from Deskbar! :-D

	BDeskbar deskbar;
	return deskbar.HasItem(REPLICANT_NAME);
}

// process quit message
bool
SettingsWindow::QuitRequested()
{
	if(!from_deskbar) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	return BWindow::QuitRequested(); // cause otherwise it will kill Deskbar
}

SettingsWindow::KeymapItem::KeymapItem(KeymapItem *item) : BStringItem(((KeymapItem*) item)->Text())
{
	dir = (int32) item->Dir();
	real_name = item->RealName();
}

SettingsWindow::KeymapItem::KeymapItem(const char *text, const char *real_name, int32 dir) : BStringItem(text)
{
	this->dir = dir;
	this->real_name = real_name;
}

SettingsWindow::KeymapListView::KeymapListView(BRect r, const char *name)
				: 
				BListView(r, name, B_SINGLE_SELECTION_LIST,
					   	B_FOLLOW_ALL_SIDES/* | B_FOLLOW_TOP | B_FOLLOW_RIGHT*/) 
{
}

bool
SettingsWindow::KeymapListView::InitiateDrag(BPoint point, int32 index, bool wasSelected)
{
	BListView::InitiateDrag(point, index, wasSelected);
	BMessage message(MSG_ACTIVE_ITEM_DRAGGED);
	message.AddInt32("index", index);
	DragMessage(&message, ItemFrame(index));
	return true;
}

void
SettingsWindow::KeymapListView::MessageReceived(BMessage *message)
{
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
				name += B_TRANSLATE(" (system)");
			else
				name += B_TRANSLATE(" (user)");
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
	
SettingsWindow::KeymapOutlineListView::KeymapOutlineListView(BRect r, const char *name) 
				:
				BOutlineListView(r, name, B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES) 
{
}

bool
SettingsWindow::KeymapOutlineListView::InitiateDrag(BPoint point, int32 index, bool wasSelected) {
	BOutlineListView::InitiateDrag(point, index, wasSelected);
	BMessage message(MSG_ITEM_DRAGGED);
	message.AddInt32("index", index);
	message.AddPointer("keymap_item", ItemAt(index));
	if (NULL != Superitem(ItemAt(index)))
		DragMessage(&message, ItemFrame(index));
	else return false;
	return true;
}

static BPicture sPicture;

SettingsWindow::BMoveButton::BMoveButton(BRect 		frame, const char *name,  
						uint32		resIdOff, uint32 resIdOn, uint32 resIdDisabled,
						BMessage	*message,
						uint32		behavior		/*= B_ONE_STATE_BUTTON*/,
						uint32		resizingMode 	/*= B_FOLLOW_LEFT| B_FOLLOW_TOP*/,
						uint32		flags			/*= B_WILL_DRAW	 | B_NAVIGABLE*/):
		BPictureButton(frame, name, &sPicture, &sPicture, message, behavior, resizingMode, flags),
		fResIdOff(resIdOff), fResIdOn(resIdOn), fResIdDisabled(resIdDisabled)
{
}

SettingsWindow::BMoveButton::~BMoveButton()
{
}

void
SettingsWindow::BMoveButton::AttachedToWindow()
{
	//	debugger("app");
	entry_ref ref;
	be_roster->FindApp(APP_SIGNATURE, &ref);
	BEntry entry(&ref);
	BFile appFile;
	appFile.SetTo(&entry, B_READ_ONLY);
	BResources appResources;
	appResources.SetTo(&appFile);
	
	LoadPicture(&appResources, &fPicOff,      fResIdOff);
	LoadPicture(&appResources, &fPicOn,       fResIdOn);
	LoadPicture(&appResources, &fPicDisabled, fResIdDisabled);

	SetEnabledOff(&fPicOff);
	SetEnabledOn(&fPicOn);
	SetDisabledOff(&fPicDisabled);
	SetDisabledOn(&fPicDisabled);
}


void
SettingsWindow::BMoveButton::GetPreferredSize(float *width, float *height)
{
	*width  = 17.;
	*height = 16.;
}

status_t
SettingsWindow::BMoveButton::LoadPicture(BResources *resFrom, BPicture *picTo, uint32 resId)
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

