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

#include "DeskView.h"
#include "KeymapSwitcher.h"
#include "Replicator.h"
#include "Settings.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "SwitcherSettingsWindow"

//#define trace(x...) (void(0)) // syslog(0, x);
#define trace(x...) printf(x);

// the initial dimensions of the window.
const float WINDOW_X      = 100;
const float WINDOW_Y      = 100;
const float WINDOW_WIDTH  = 600;
const float WINDOW_HEIGHT = 600;
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
	settingsOrg = new Settings("Switcher");
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

	BRect rc(RC.LeftTop(), RC.LeftTop());
	// create hot-key selector
	BStringView* selectorLabel = new BStringView(rc, "string1",
								B_TRANSLATE("Hotkey:"), B_FOLLOW_RIGHT);
	box->AddChild(selectorLabel);
	
	float fLineHeight = 0.f;
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
	fMaxButtonsWidth += pt.x + fXSpacing;
	float fSelectorHeight = fmax(fLineHeight, pt.y);

	// create top divider
	BBox* dividerTop = new BBox(rc, B_EMPTY_STRING, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW, B_FANCY_BORDER);
	box->AddChild(dividerTop);

	// create labels
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
	MoveButton* addButton = new MoveButton(rcBtn, "add_keymap_button", 
					R_ResAddButton, R_ResAddButtonPressed, R_ResAddButtonDisabled,
		new BMessage(MSG_BUTTON_ADD_ITEM), B_ONE_STATE_BUTTON, B_FOLLOW_RIGHT);
	box->AddChild(addButton);
	addButton->SetTarget(this);
	
	MoveButton* delButton = new MoveButton(rcBtn, "remove_keymap_button",
					R_ResRemoveButton, R_ResRemoveButtonPressed, R_ResRemoveButtonDisabled, 
		new BMessage(MSG_BUTTON_REMOVE_ITEM), B_ONE_STATE_BUTTON, B_FOLLOW_RIGHT);
	box->AddChild(delButton);
	delButton->SetTarget(this);

	// calculate the layout of "lists column" of controls
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

	fMaxListsWidth = fmax(fMaxListsWidth, fMaxButtonsWidth);

	// create buttons
	struct _button {
		const char* name;
		const char* label;
		int32		message;
		int32		flags;
		BButton*	button;
		BPoint		pt;
	} b[] = {
		{ "save_button", !AlreadyInDeskbar() ? B_TRANSLATE("Install in Deskbar")
									: B_TRANSLATE("Apply"),
					MSG_SAVE_SETTINGS, B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT, NULL },
		{ "close_button", !AlreadyInDeskbar() ? B_TRANSLATE("Exit")
									: B_TRANSLATE("Revert"),
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

	// remember them for later using
	buttonOK = b[0].button;
	buttonCancel = b[1].button;

	buttonOK->SetEnabled(!AlreadyInDeskbar());
	buttonCancel->SetEnabled(!AlreadyInDeskbar());
	
	BString strRemap(B_TRANSLATE("Use %KEYMAP% keymap for shortcuts."));
	strRemap.ReplaceAll("%KEYMAP%", "US-International");
	checkRemap = new RemapCheckBox(rc, "check_remap", strRemap,
			   new BMessage(MSG_CHECK_REMAP), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	box->AddChild(checkRemap);

	BPoint ptRemap;
	checkRemap->GetPreferredSize(&ptRemap.x, &ptRemap.y);
	checkRemap->ResizeToPreferred();

	BBox* dividerBottom = new BBox(rc, B_EMPTY_STRING,
		   B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW, B_FANCY_BORDER);
	box->AddChild(dividerBottom);
	
	fMaxListsWidth = fmax(fMaxListsWidth, ptRemap.x);
	float fMaxWindowWidth =  X_INSET * 2 
					+ fmax(fMaxListsWidth, b[0].pt.x + b[1].pt.x + X_INSET);

	fMaxListsWidth = fMaxWindowWidth - X_INSET * 2;

	// reposition of "lists column" controls
	BPoint ptOrg(RC.LeftTop());
	selectorLabel->ResizeToPreferred();
	menuField->MoveTo(ptOrg.x + selectorLabel->Bounds().Width() + fXSpacing, ptOrg.y);
	selectorLabel->MoveTo(ptOrg.x,
		   ptOrg.y + (fSelectorHeight - selectorLabel->Bounds().Height()) / 2);
	ptOrg.y += fSelectorHeight + Y_INSET/*fYSpacing*/;

	dividerTop->MoveTo(ptOrg);
	dividerTop->ResizeTo(fMaxListsWidth, 1);
	
	ptOrg.y += Y_INSET;

	// other
	selMapsLabel->ResizeToPreferred();
	selMapsLabel->MoveTo(ptOrg);

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

	ptOrg.y += scrollViewAll->Bounds().Height() + fYSpacing;

	// populate selected keymaps list
	selected_list->ResetKeymapsList(settings);

	// populate all available keymaps list - system keymaps
	available_list->PopulateTheTree();

	checkRemap->MoveTo(ptOrg);
	checkRemap->ResizeTo(fMaxListsWidth, ptRemap.y);
	ptOrg.y += checkRemap->Bounds().Height() + fYSpacing;

	// read the remap check state
	int32 index = settings->FindInt32("remap");
	checkRemap->SetIndex(++index);
	AdjustRemapCheck(false);

	dividerBottom->MoveTo(ptOrg);
	dividerBottom->ResizeTo(fMaxListsWidth, 1);

	ptOrg.y += dividerBottom->Bounds().Height() + Y_INSET;

	float fMaxWindowHeight = ptOrg.y + fmax(b[0].pt.y, b[1].pt.y) + Y_INSET;
	
	ptOrg.x = fMaxWindowWidth - X_INSET - b[0].pt.x;
	b[0].button->MoveTo(ptOrg);
	ptOrg.x -= X_INSET + b[1].pt.x;
	b[1].button->MoveTo(ptOrg);
	

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
	delete settings;
	delete settingsOrg;
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
		if(index >= 0) {
			BMessage message(MSG_REMOVE_ACTIVE_ITEM);
			message.AddInt32("index", index);
			PostMessage(&message, selected_list);
		}
	}
	break;
	case MSG_KEYMAPS_CHANGED:
		trace("keymaps changed");
		if (!keymaps_changed) {
			buttonOK->SetEnabled(true);
			keymaps_changed = true;
		}
		AdjustRemapCheck(false);
		break;
	case MSG_HOTKEY_CHANGED: {
		int32 temp = 0;
		if (B_OK==msg->FindInt32("hotkey",&temp))
			settings->SetInt32("hotkey", temp);
		hotkey_changed = true;
		break;
	}
	case MSG_CHECK_REMAP: {
		if (!keymaps_changed) {
			buttonOK->SetEnabled(true);
			keymaps_changed = true;
		}
		AdjustRemapCheck(true);
		break;
	}
	case MSG_SAVE_SETTINGS: {
		if (!AlreadyInDeskbar()) {
			add_system_beep_event(BEEP_NAME);

			BDeskbar deskbar;
			if(deskbar.IsRunning()) {
				entry_ref ref;
				be_roster->FindApp(APP_SIGNATURE, &ref);
				deskbar.AddItem(&ref);

				buttonOK->SetLabel(B_TRANSLATE("Apply"));
				buttonCancel->SetLabel(B_TRANSLATE("Revert"));
				buttonOK->SetEnabled(false);
				buttonCancel->SetEnabled(false);

			} else {
				BAlert* alert = new BAlert("Error",
					   B_TRANSLATE("Unable to install keymap indicator. "
									"Deskbar application is not running."),
				   	   B_TRANSLATE("OK"), 0, 0, B_WIDTH_AS_USUAL, B_STOP_ALERT);
				alert->Go();
			}

		}

		// delete all keymaps from settings
		if(keymaps_changed) {
			selected_list->ReadKeymapsList(settings);
			keymaps_changed = false;

			buttonOK->SetEnabled(false);
			buttonCancel->SetEnabled(true);

			settings->SetInt32("remap", checkRemap->Index() - 1);
		}

		trace("settings saved!");
		settings->Save();
//		delete settings; // we save settings in its destructor

		::UpdateIndicator(MSG_UPDATESETTINGS);
		/*if(!from_deskbar) {
			be_app->PostMessage(B_QUIT_REQUESTED);
		} else
			Close();*/
		break;
	}
	case MSG_CLOSE:
		if(!from_deskbar && !AlreadyInDeskbar()) {
			be_app->PostMessage(B_QUIT_REQUESTED);
		} else {
			//Close();
			// perform "Revert" functionality
			*settings = *settingsOrg;

			selected_list->ResetKeymapsList(settings);

			//while(0 < available_list->CountItems())
			//	delete (dynamic_cast<KeymapItem*> (available_list->RemoveItem(0L)));
			settings->Save();
			::UpdateIndicator(MSG_UPDATESETTINGS);
			
			buttonOK->SetEnabled(false);
			buttonCancel->SetEnabled(false);

			AdjustRemapCheck(false);
		}
		break;
		
	case MSG_ABOUT:
	//	ShowAboutWindow();
		break;
	default:
		break;
	}
	BWindow::MessageReceived(msg);
}

void SettingsWindow::AdjustRemapCheck(bool next_index)
{
	int32 index = checkRemap->Index();
	if (next_index)
		index++;

	index %= selected_list->CountItems() + 1;
	checkRemap->SetIndex(index);
	BString str(index <= 0 ? B_TRANSLATE("Use shortcuts remapping.")
			: B_TRANSLATE("Use %KEYMAP% keymap for shortcuts."));
	if (index > 0) {
		KeymapItem *item = (KeymapItem*)selected_list->ItemAt(index - 1);
		str.ReplaceAll("%KEYMAP%", item->RealName());
	}
	checkRemap->SetLabel(str);

	fprintf(stderr, "adj:%ld\n", checkRemap->Index());
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
SettingsWindow::KeymapListView::ResetKeymapsList(const Settings* settings)
{
	if(!IsEmpty()) {
		// first clean the list
		while(0 < CountItems())
			delete (dynamic_cast<KeymapItem*> (RemoveItem(0L)));
		MakeEmpty();
	}

	// now populate it with settings
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
		AddItem(new KeymapItem(display_name.String(), name.String(), dir));
	}
}

void 
SettingsWindow::KeymapListView::ReadKeymapsList(Settings* settings)
{
	// remove previous keymaps from settings
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
	// after this save new set of keymaps
	settings->SetInt32("keymaps", CountItems());
	for (int i = 0; i < CountItems(); i++) {
		KeymapItem *item = (KeymapItem*)ItemAt(i);
		trace((char*)(item->RealName()));
		param = "";
		param << "d" << i;
		settings->SetInt32(param.String(), item->Dir());
		param = "";
		param << "n" << i;
		settings->SetString(param.String(), item->RealName());	
	}
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
			} else 
				AddItem(new_item, index);

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

void
SettingsWindow::KeymapOutlineListView::PopulateTheTree()
{
	struct _d {
		directory_which which;
		const char* subDir;
		const char* name;
	} ad[] = {
		{ B_BEOS_DATA_DIRECTORY,	 "Keymaps", B_TRANSLATE("System") },
		{ B_USER_SETTINGS_DIRECTORY, "Keymap",  B_TRANSLATE("User")   }
	};

	for (size_t d = 0; d < sizeof(ad)/sizeof(ad[0]); d++) {
		BPath path;
		find_directory(ad[d].which, &path);
		path.Append(ad[d].subDir);

		BDirectory dir(path.Path());
		long count = dir.CountEntries();
		if(count <= 0)
			continue;

		KeymapItem *keymap_node = new KeymapItem(ad[d].name, NULL, 0);
		/*available_list->*/AddItem(keymap_node);

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
			list.AddItem(new KeymapItem(ref.name, ref.name, (int32)ad[d].which ), 0);
		}

		for (int i = 0; i < list.CountItems(); i++) {
			/*available_list->*/AddUnder((KeymapItem*) list.ItemAt(i), keymap_node);
		}
	}
}

bool
SettingsWindow::KeymapOutlineListView::InitiateDrag(BPoint point, int32 index, bool wasSelected)
{
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

SettingsWindow::MoveButton::MoveButton(BRect 		frame, const char *name,  
						uint32		resIdOff, uint32 resIdOn, uint32 resIdDisabled,
						BMessage	*message,
						uint32		behavior		/*= B_ONE_STATE_BUTTON*/,
						uint32		resizingMode 	/*= B_FOLLOW_LEFT| B_FOLLOW_TOP*/,
						uint32		flags			/*= B_WILL_DRAW	 | B_NAVIGABLE*/):
		BPictureButton(frame, name, &sPicture, &sPicture, message, behavior, resizingMode, flags),
		fResIdOff(resIdOff), fResIdOn(resIdOn), fResIdDisabled(resIdDisabled)
{
}

SettingsWindow::MoveButton::~MoveButton()
{
}

void
SettingsWindow::MoveButton::AttachedToWindow()
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
SettingsWindow::MoveButton::GetPreferredSize(float *width, float *height)
{
	*width  = 17.;
	*height = 16.;
}

status_t
SettingsWindow::MoveButton::LoadPicture(BResources *resFrom, BPicture *picTo, uint32 resId)
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

