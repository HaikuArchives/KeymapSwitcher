// Switcher
// Copyright © 1999-2003 by Stas Maximov
// All rights reserved


#include "SettingsWindow.h"

#include <new>
#include <stdio.h>
#include <syslog.h>

#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Entry.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MessageFilter.h>
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

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SwitcherSettingsWindow"

#if 0
#define trace(x...) (void(0))
#else
#define trace(x...) (void(0))//printf(x);
#endif

// the initial dimensions of the window.
const float WINDOW_X      = 100;
const float WINDOW_Y      = 100;
const float WINDOW_WIDTH  = 800;
const float WINDOW_HEIGHT = 800;
const float X_INSET	= 10;
const float Y_INSET	= 10;

const float fYSpacing = 5.;
const float fXSpacing = 5.;
const float kBmpBtnX = 17.;
const float kBmpBtnY = 24.;

const char* remapLabel0 = B_TRANSLATE("Activate shortcuts substitution");
const char* remapLabel1	= B_TRANSLATE("Substitute shortcuts with %KEYMAP%'s ones");

class _KeysFilter_ : public BMessageFilter {
public:
	_KeysFilter_() : BMessageFilter(B_KEY_DOWN) {}  
	~_KeysFilter_() {}

    virtual filter_result Filter(BMessage* msg, BHandler** target) {
		int32 rawChar = 0;
		if (msg->what == B_KEY_DOWN
			&& msg->FindInt32("raw_char", &rawChar) == B_OK
			&& rawChar == B_ESCAPE)
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
		}
		return B_DISPATCH_MESSAGE;
	}
};

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
	ClientBox *box = new ClientBox(Bounds(), B_EMPTY_STRING,
						B_FOLLOW_NONE,
						B_WILL_DRAW | B_FRAME_EVENTS
						| B_NAVIGABLE_JUMP | B_PULSE_NEEDED,
						B_PLAIN_BORDER);
	AddChild(box);

	BRect RC = box->Bounds();
	RC.InsetBy(X_INSET, Y_INSET);

	BRect rc(RC.LeftTop(), RC.LeftTop());
	// create hot-key selector
	BStringView* selectorLabel = new BStringView(rc, "string1",
								B_TRANSLATE("Shortcut:"));
	box->AddChild(selectorLabel);
	
	float fLineHeight = 0.f;
	float fMaxButtonsWidth = 0.f;
	selectorLabel->GetPreferredSize(&fMaxButtonsWidth, &fLineHeight);
	float fSelectorLabelWidth = fMaxButtonsWidth + 5.f;

	// create and populate selector menu
	int32 hotkey = 0;
	settings->FindInt32("hotkey", &hotkey);

	struct _pair {
		int32		hotkey;
		const char*	name;
	} a[] = {
		{ KEY_LCTRL_SHIFT,	B_TRANSLATE("Ctrl+Shift") },
		{ KEY_ALT_SHIFT,	B_TRANSLATE("Cmd+Shift") },
		{ KEY_SHIFT_SHIFT,	B_TRANSLATE("Shift+Shift") },
		{ KEY_CAPS_LOCK,	B_TRANSLATE("Caps Lock") },
		{ KEY_SCROLL_LOCK,	B_TRANSLATE("Scroll Lock") },
		{ KEY_OPT_SPACE,	B_TRANSLATE("Opt+Space") }
	};

	const char* menuName = "none";
	for (size_t i = 0; i < sizeof(a)/sizeof(a[0]); i++)
		if (a[i].hotkey == hotkey)
			menuName = a[i].name;

	BPopUpMenu *pop_key = new BPopUpMenu(menuName);

	for (size_t i = 0; i < sizeof(a)/sizeof(a[0]); i++) {
		BMessage *msg = new BMessage(MSG_HOTKEY_CHANGED + a[i].hotkey);
		msg->AddInt32("hotkey", a[i].hotkey);
		BMenuItem* item = new BMenuItem(a[i].name, msg);
		item->SetMarked(a[i].hotkey == hotkey);
		pop_key->AddItem(item);
	}
	
	menuField = new BMenuField(rc, "HotKey", NULL, pop_key);
	menuField->SetDivider(0);
	
	box->AddChild(menuField);

	BPoint pt(0, 0);
	menuField->GetPreferredSize(&pt.x, &pt.y);
	fMaxButtonsWidth += pt.x + fXSpacing;
	float fSelectorHeight = fmax(fLineHeight, pt.y);

	menuField->ResizeToPreferred();
	menuField->SetDivider(fSelectorLabelWidth);
	menuField->ResizeBy(fSelectorLabelWidth, 0.f);
	menuField->SetLabel(B_TRANSLATE("Shortcut:"));

	// create top divider
	BBox* dividerTop = new BBox(rc, B_EMPTY_STRING, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW, B_FANCY_BORDER);
	box->AddChild(dividerTop);

	// create labels
	BStringView* selMapsLabel = new BStringView(rc, "string0",
										B_TRANSLATE("Selected keymaps:"));
	box->AddChild(selMapsLabel);
	selMapsLabel->ResizeToPreferred();

	BStringView* allMapsLabel = new BStringView(rc, "string2",
										B_TRANSLATE("Available keymaps:"));
	box->AddChild(allMapsLabel);
	allMapsLabel->ResizeToPreferred();

	// create add/del button
	BRect rcBtn(RC);
	rcBtn.right  = rcBtn.left + kBmpBtnX;
	rcBtn.bottom = rcBtn.top  + kBmpBtnY;
	addButton = new MoveButton(rcBtn, "add_keymap_button", 
					R_ResAddButton, R_ResAddButtonPressed, R_ResAddButtonDisabled,
		new BMessage(MSG_BUTTON_ADD_ITEM), B_ONE_STATE_BUTTON, B_FOLLOW_RIGHT);
	box->AddChild(addButton);
	addButton->SetTarget(this);
	
	delButton = new MoveButton(rcBtn, "remove_keymap_button",
					R_ResRemoveButton, R_ResRemoveButtonPressed, R_ResRemoveButtonDisabled, 
		new BMessage(MSG_BUTTON_REMOVE_ITEM), B_ONE_STATE_BUTTON, B_FOLLOW_RIGHT);
	box->AddChild(delButton);
	delButton->SetTarget(this);

	addButton->SetEnabled(false);
	delButton->SetEnabled(false);

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
		uint32		message;
		uint32		flags;
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
	
	BString strRemap(remapLabel1);
	strRemap.ReplaceAll("%KEYMAP%", "US-International");
	checkRemap = new RemapCheckBox(rc, "check_remap", strRemap,
			   new BMessage(MSG_CHECK_REMAP), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	box->AddChild(checkRemap);

	BPoint ptRemap;
	checkRemap->GetPreferredSize(&ptRemap.x, &ptRemap.y);
	checkRemap->ResizeToPreferred();

	checkSystemWideKeymap = new BCheckBox(rc, "check_system_wide", 
			B_TRANSLATE("Use the separate keymap in each application"),
			   new BMessage(MSG_CHECK_SYSTEM_WIDE), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	box->AddChild(checkSystemWideKeymap);

	BPoint ptSystemWide;
	checkSystemWideKeymap->GetPreferredSize(&ptSystemWide.x, &ptSystemWide.y);
	checkSystemWideKeymap->ResizeToPreferred();

	//checkUseActiveKeymap = new BCheckBox(rc, "check_use_active", 
	//		B_TRANSLATE("Launch application using active keymap"),
	//		   new BMessage(MSG_CHECK_USE_ACTIVE), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	//box->AddChild(checkUseActiveKeymap);

	//BPoint ptActiveKeymap;
	//checkUseActiveKeymap->GetPreferredSize(&ptActiveKeymap.x, &ptActiveKeymap.y);
	//checkUseActiveKeymap->ResizeToPreferred();

	BBox* dividerBottom = new BBox(rc, B_EMPTY_STRING,
		   B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW, B_FANCY_BORDER);
	box->AddChild(dividerBottom);
	
	fMaxListsWidth = fmax(fMaxListsWidth, ptRemap.x);
	fMaxListsWidth = fmax(fMaxListsWidth, ptSystemWide.x);
//	fMaxListsWidth = fmax(fMaxListsWidth, ptActiveKeymap.x);
	float fMaxWindowWidth =  X_INSET * 2 
					+ fmax(fMaxListsWidth, b[0].pt.x + b[1].pt.x + X_INSET);

	fMaxListsWidth = fMaxWindowWidth - X_INSET * 2;

	// reposition of "lists column" controls
	BPoint ptOrg(RC.LeftTop());
	selectorLabel->ResizeToPreferred();
	
	//selectorLabel->MoveTo(ptOrg.x,
	//	   ptOrg.y + (fSelectorHeight - selectorLabel->Bounds().Height()) / 2);
	selectorLabel->MoveTo(-1000.f, -1000.f);
	menuField->MoveTo(ptOrg.x /*+ selectorLabel->Bounds().Width() + fXSpacing*/, ptOrg.y);
	ptOrg.y += fSelectorHeight + Y_INSET;

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
	ptBtnOrg.x = fMaxWindowWidth - X_INSET - kBmpBtnX - fScrollBarWidth;
	delButton->MoveTo(ptBtnOrg);
	ptBtnOrg.x -= kBmpBtnX + fXSpacing;
	addButton->MoveTo(ptBtnOrg);

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
	fprintf(stderr, "init:%" B_PRIi32 "d\n", index);
	checkRemap->SetIndex(index);
	AdjustRemapCheck(false);

	checkSystemWideKeymap->MoveTo(ptOrg);
	checkSystemWideKeymap->ResizeTo(fMaxListsWidth, ptSystemWide.y);
	ptOrg.y += checkSystemWideKeymap->Bounds().Height() + fYSpacing;

	int32 nSystemWide = 0;
	if (B_OK == settings->FindInt32("system_wide", &nSystemWide))
		checkSystemWideKeymap->SetValue(!nSystemWide);

	//checkUseActiveKeymap->MoveTo(ptOrg);
	//checkUseActiveKeymap->ResizeTo(fMaxListsWidth, ptActiveKeymap.y);
	//ptOrg.y += checkUseActiveKeymap->Bounds().Height() + fYSpacing;

	//int32 nUseActiveKeymap = 0;
	//if (B_OK == settings->FindInt32("use_active_keymap", &nUseActiveKeymap)) {
	//	checkUseActiveKeymap->SetValue(nUseActiveKeymap);
	//	if (nSystemWide != 0) {
	//		checkUseActiveKeymap->SetValue(1);
	//		checkUseActiveKeymap->SetEnabled(false);
	//	}
	//}
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
	//ResizeTo(fMaxWindowWidth * 1.1f, fMaxWindowHeight * 1.1f);

	BScreen screen(this);
	BRect rcScreen(0, 0, 640, 480);
	if(screen.IsValid())
		rcScreen = screen.Frame();

	ptOrg.x = rcScreen.left + (rcScreen.Width() - Bounds().Width()) / 2;
	ptOrg.y = rcScreen.top + (rcScreen.Height() - Bounds().Height()) / 3;
	MoveTo(ptOrg);

	AddCommonFilter(new (std::nothrow) _KeysFilter_);
	hotkey_changed = false;
	Unlock();
	Show();
}

// destructor
SettingsWindow::~SettingsWindow()
{
	while(0 < available_list->CountItems())
		delete (dynamic_cast<KeymapItem*> (available_list->RemoveItem((int32)0)));
	while(0 < selected_list->CountItems())
		delete (dynamic_cast<KeymapItem*> (selected_list->RemoveItem((int32)0)));
	delete settings;
	delete settingsOrg;
}

// process message
void SettingsWindow::MessageReceived(BMessage *msg)
{
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
	case MSG_HOTKEY_CHANGED + KEY_LCTRL_SHIFT:
	case MSG_HOTKEY_CHANGED + KEY_ALT_SHIFT:
	case MSG_HOTKEY_CHANGED + KEY_SHIFT_SHIFT:
	case MSG_HOTKEY_CHANGED + KEY_CAPS_LOCK:
	case MSG_HOTKEY_CHANGED + KEY_SCROLL_LOCK:
	case MSG_HOTKEY_CHANGED + KEY_OPT_SPACE: {
		int32 temp = 0;
		if (B_OK==msg->FindInt32("hotkey",&temp))
			settings->SetInt32("hotkey", temp);
		hotkey_changed = true;
		buttonOK->SetEnabled(true);
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
	case MSG_CHECK_SYSTEM_WIDE: /*{
			int32 on = 0;
			if (B_OK == msg->FindInt32("be:value", &on)) {
				if (on == 0)
					checkUseActiveKeymap->SetValue(!on);
				checkUseActiveKeymap->SetEnabled(on != 0);
			}
		} 
	case MSG_CHECK_USE_ACTIVE:*/ {
		if (!keymaps_changed) {
			buttonOK->SetEnabled(true);
			keymaps_changed = true;
		}
		AdjustRemapCheck(false);
		break;
	}
	case MSG_LIST_SEL_CHANGE: {
		int32 index = available_list->CurrentSelection();
		addButton->SetEnabled(index != -1
				&& available_list->ItemAt(index) != NULL
				&& available_list->Superitem(available_list->ItemAt(index)) != NULL);
		delButton->SetEnabled(selected_list->CurrentSelection() != -1);
		break;
	}
	case MSG_SAVE_SETTINGS: {
		if (!AlreadyInDeskbar()) {
			add_system_beep_event(BEEP_NAME);

			bool isRunning = true;
			{
				BDeskbar deskbar;
				isRunning = deskbar.IsRunning();
				if (!isRunning) {
					BAlert* alert = new BAlert("Error",
					   B_TRANSLATE("Unable to install keymap indicator. "
									"Deskbar application is not running."),
				   	   B_TRANSLATE("Start Deskbar"), B_TRANSLATE("OK"),
					   0, B_WIDTH_AS_USUAL, B_STOP_ALERT);

					if (0 == alert->Go()) {
						be_roster->Launch(DESKBAR_SIGNATURE);
					}
				}
			}

			int tries = 10;
			while (!isRunning && --tries) {
				BDeskbar deskbar;
				isRunning = deskbar.IsRunning();
				if (!isRunning) {
					snooze(1000000);
				}
			}

			if (isRunning) {
				BDeskbar deskbar;

				entry_ref ref;
				be_roster->FindApp(APP_SIGNATURE, &ref);
				deskbar.AddItem(&ref);

				buttonOK->SetLabel(B_TRANSLATE("Apply"));
				buttonCancel->SetLabel(B_TRANSLATE("Revert"));
				buttonOK->SetEnabled(false);
				buttonCancel->SetEnabled(false);
			} else {
				BAlert* alert = new BAlert("Error",
				   B_TRANSLATE("Unable to start Deskbar application."
								" Keymap indicator was not installed."),
				   B_TRANSLATE("OK"), 0, 0, B_WIDTH_AS_USUAL, B_STOP_ALERT);
				alert->Go();
			}
		}

		// delete all keymaps from settings
		if(keymaps_changed || hotkey_changed) {
			selected_list->ReadKeymapsList(settings);
			keymaps_changed = false;
			hotkey_changed = false;

			buttonOK->SetEnabled(false);
			buttonCancel->SetEnabled(true);

			settings->SetInt32("remap", checkRemap->Index());
			settings->SetInt32("system_wide", !checkSystemWideKeymap->Value());
			//settings->SetInt32("use_active_keymap", checkUseActiveKeymap->Value());
		}
		trace("settings saved!");
		settings->Save();
		::UpdateIndicator(MSG_UPDATESETTINGS);
		break;
	}
	case MSG_CLOSE:
		if(!from_deskbar && !AlreadyInDeskbar()) {
			be_app->PostMessage(B_QUIT_REQUESTED);
		} else {
			// perform "Revert" functionality
			*settings = *settingsOrg;

			selected_list->ResetKeymapsList(settings);

			settings->Save();
			::UpdateIndicator(MSG_UPDATESETTINGS);
			
			buttonOK->SetEnabled(false);
			buttonCancel->SetEnabled(false);

			checkRemap->SetIndex(settings->FindInt32("remap"));
			AdjustRemapCheck(false);

			int32 hotkey = 0;
			settings->FindInt32("hotkey", &hotkey);
			BMenuItem* item = menuField->Menu()->FindItem(MSG_HOTKEY_CHANGED + hotkey);
			if (item != 0) 
				item->SetMarked(true);
		break;
		}
/*	case B_KEY_DOWN:
		if (msg->FindInt32("raw_char") == B_ESCAPE)
			be_app->PostMessage(B_QUIT_REQUESTED); 
		break; */
	case MSG_ABOUT:
	//	ShowAboutWindow();
		break;
	case MSG_PULSE_RESEIVED:
		{	
			bool isInDeskbar = AlreadyInDeskbar();
			const char* labelOK = isInDeskbar
				? B_TRANSLATE("Apply") : B_TRANSLATE("Install in Deskbar");
			const char* labelCancel = isInDeskbar
				? B_TRANSLATE("Revert") : B_TRANSLATE("Exit");

			float w0 = buttonOK->Bounds().Width();
			buttonOK->SetLabel(labelOK);
			float w1 = 0.f, h0 = 0.f;
			buttonOK->GetPreferredSize(&w1, &h0);
			float deltaOK = min_c(w0 - w1, 0.f);
			buttonOK->MoveBy(deltaOK, 0.f);
			if (deltaOK < 0.f)
				buttonOK->ResizeToPreferred();

			w0 = buttonCancel->Bounds().Width();
			buttonCancel->SetLabel(labelCancel);
			buttonCancel->GetPreferredSize(&w1, &h0);
			float deltaCancel = min_c(w0 - w1, 0.f);
			buttonCancel->MoveBy(deltaOK + deltaCancel, 0.f);
			if (deltaCancel < 0.f)
				buttonCancel->ResizeToPreferred();

			if (!isInDeskbar) {
				buttonOK->SetEnabled(true);
				buttonCancel->SetEnabled(true);
			}

			break;
		}

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
	BString str(index <= 0 ? remapLabel0 : remapLabel1 );
	if (index > 0) {
		KeymapItem *item = (KeymapItem*)selected_list->ItemAt(index - 1);
		str.ReplaceAll("%KEYMAP%", item->RealName());
	}
	checkRemap->SetLabel(str);

	fprintf(stderr, "adj:%" B_PRIi32 "d\n", checkRemap->Index());
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
			delete (dynamic_cast<KeymapItem*> (RemoveItem((int32)0)));
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
		if(dir == B_SYSTEM_DATA_DIRECTORY)
			path.Append("Keymaps");
		else
			path.Append("Keymap");
		path.Append(name.String());

		BString display_name(name);
		if(dir == B_SYSTEM_DATA_DIRECTORY)
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
			if(B_SYSTEM_DATA_DIRECTORY == new_item->Dir())
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

			// restore selection after delete
			int32 count = CountItems();
			int32 new_index = (index >= count) ? count - 1 : index ;
			Select(max_c(0, new_index));
			break;
		}

		// item dropped inside, rearrange items
		case MSG_ACTIVE_ITEM_DRAGGED: {
			int32 draggedIndex = -1;
			message->FindInt32("index", &draggedIndex);
			int32 index = -1;
			index = IndexOf(ConvertFromScreen(message->DropPoint()));
			if (0 > index) 
				index = CountItems() - 1;

			if (index == draggedIndex)
				return;
			
			DeselectAll();
			MoveItem(draggedIndex, index);
			Select(index);
			ScrollToSelection();
			
			BMessage notify(MSG_KEYMAPS_CHANGED);
			Window()->PostMessage(&notify);
			return;						
		}
	}
	BListView::MessageReceived(message);
}

void
SettingsWindow::KeymapListView::SelectionChanged()
{
	BMessage message(MSG_LIST_SEL_CHANGE);
	Window()->PostMessage(&message);
}
	
void
SettingsWindow::KeymapListView::MouseDown(BPoint point)
{
	int32 clicks = 0;
	if (Window()->CurrentMessage()->FindInt32("clicks", &clicks) == B_OK
			&& clicks > 1)
	{
		BMessage message(MSG_BUTTON_REMOVE_ITEM);
		Window()->PostMessage(&message);
	}

	BListView::MouseDown(point);
}

SettingsWindow::KeymapOutlineListView::KeymapOutlineListView(BRect r, const char *name) 
				:
				BOutlineListView(r, name, B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES) 
{
}

int
SettingsWindow::CompareKeymapItems(const void* left, const void* right)
{
	KeymapItem* leftItem = *(KeymapItem**)left;
	KeymapItem* rightItem = *(KeymapItem**)right;
	return -strcmp(leftItem->Text(), rightItem->Text());
}

void
SettingsWindow::KeymapOutlineListView::PopulateTheTree()
{
	struct _d {
		directory_which which;
		const char* subDir;
		const char* name;
	} ad[] = {
		{ B_SYSTEM_DATA_DIRECTORY,	 "Keymaps", B_TRANSLATE("System") },
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

		list.SortItems(&CompareKeymapItems);

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
	else
		return false;

	return true;
}

void
SettingsWindow::KeymapOutlineListView::SelectionChanged()
{
	BMessage message(MSG_LIST_SEL_CHANGE);
	Window()->PostMessage(&message);
}

void
SettingsWindow::KeymapOutlineListView::MouseDown(BPoint point)
{
	int32 clicks = 0;
	if (Window()->CurrentMessage()->FindInt32("clicks", &clicks) == B_OK
			&& clicks > 1)
	{
		int32 index = CurrentSelection();
		if (index != -1 && NULL != Superitem(ItemAt(index))) {
			BMessage message(MSG_BUTTON_ADD_ITEM);
			Window()->PostMessage(&message);
		}
	}

	BOutlineListView::MouseDown(point);
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
	rgb_color color = ui_color(B_PANEL_BACKGROUND_COLOR);
	view.SetHighColor(color);
	view.FillRect(view.Bounds());
	view.SetDrawingMode(B_OP_OVER);
	view.DrawBitmap(bmp, BPoint(0, 0));
	view.EndPicture();
	RemoveChild(&view);
		
	return B_OK;
}


SettingsWindow::ClientBox::ClientBox(BRect frame, const char* name,
				uint32 resizingMode, uint32 flags, border_style border)
		: BBox(frame, name, resizingMode, flags, border)
{
}


void SettingsWindow::ClientBox::Pulse()
{
	BBox::Pulse();

	static int pulsesCount = 0;
	if (((pulsesCount++ % 4) == 0) && Window() != 0)
		Window()->PostMessage(MSG_PULSE_RESEIVED);
}

