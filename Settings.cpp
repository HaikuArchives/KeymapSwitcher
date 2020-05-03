
#include "Settings.h"

#include <syslog.h>
#include <stdio.h> //XXX

#include <File.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <FindDirectory.h>

#include "KeymapSwitcher.h"


#if 0 //def NDEBUG
#define trace(x...) syslog(0, x); 
#else
#define trace(x...) 
#endif


Settings::Settings(const char *filename) : BMessage('pref')
{
	status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	trace("constr:%s %08x\n", path.Path(), status);
	if (status != B_OK) {
		return;
	}
	
	path.Append(filename);
	trace("constr2:%s %08x\n", path.Path(), status);
	Reload();
}

status_t Settings::Reload()
{
	dirty = false;
	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	trace("reload:%s %08x\n", path.Path(), status);
	if(status == B_ENTRY_NOT_FOUND) {
		SetDefaults();
		Save();
	} else
	if (status == B_OK) {
		status = Unflatten(&file);
	}

	return status;
}

status_t Settings::SetDefaults()
{
	SetString("version", VERSION);
	SetInt32("hotkey", KEY_LCTRL_SHIFT);
	SetBool("beep", true);
	SetBool("disabled", false);
	SetInt32("active", 0L);
	SetInt32("keymaps", 1L);
	SetString("n0", "US-International");
	SetInt32("d0", B_SYSTEM_DATA_DIRECTORY); 
	SetInt32("system_wide", 0);

	BMessage preferredLanguages;
	BLocaleRoster::Default()->GetPreferredLanguages(&preferredLanguages);
	BString preferredLanguage;
	if (preferredLanguages.FindString("language", &preferredLanguage) != B_OK) {
		preferredLanguage = "en";
	}
	// Add secondary keymap for cyrilic languages by default
	if (preferredLanguage.Compare("ru", 2) == 0) {
		SetString("n1", "Russian");
		SetInt32("remap", 1L);
		SetInt32("keymaps", 2L);
	} else if (preferredLanguage.Compare("uk", 2) == 0) {
		SetString("n1", "Ukainian");
		SetInt32("remap", 1L);
		SetInt32("keymaps", 2L);
	} else if (preferredLanguage.Compare("be", 2) == 0) {
		SetString("n1", "Belarusian");
		SetInt32("remap", 1L);
		SetInt32("keymaps", 2L);
	}

	return status = B_OK;
}

status_t Settings::Save()
{
	BFile file;
	
	status = file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	trace("save:%s %08x\n", path.Path(), status);
	if (status == B_OK) {
		status = Flatten(&file);
	}
	return status;
}

Settings::~Settings()
{
	//Save(); // autosave on destruction
}

status_t Settings::SetBool(const char *name, bool b)
{
	dirty = true;
	if (HasBool(name)) {
		return ReplaceBool(name, 0, b);
	}
	return AddBool(name, b);
}

status_t Settings::SetInt8(const char *name, int8 i)
{
	dirty = true;
	if (HasInt8(name)) {
		return ReplaceInt8(name, 0, i);
	}
	return AddInt8(name, i);
}

status_t Settings::SetInt16(const char *name, int16 i)
{
	dirty = true;
	if (HasInt16(name)) {
		return ReplaceInt16(name, 0, i);
	}
	return AddInt16(name, i);
}

status_t Settings::SetInt32(const char *name, int32 i)
{
	dirty = true;
	fprintf(stderr, "SetInt32(%s):%" B_PRIi32 "d\n", name, i);
	if (HasInt32(name)) {
		return ReplaceInt32(name, 0, i);
	}
	return AddInt32(name, i);
}

status_t Settings::SetInt64(const char *name, int64 i)
{
	dirty = true;
	if (HasInt64(name)) {
		return ReplaceInt64(name, 0, i);
	}
	return AddInt64(name, i);
}

status_t Settings::SetFloat(const char *name, float f)
{
	dirty = true;
	if (HasFloat(name)) {
		return ReplaceFloat(name, 0, f);
	}
	return AddFloat(name, f);
}

status_t Settings::SetDouble(const char *name, double f)
{
	dirty = true;
	if (HasDouble(name)) {
		return ReplaceDouble(name, 0, f);
	}
	return AddDouble(name, f);
}

status_t Settings::SetString(const char *name, const char *s)
{
	dirty = true;
	if (HasString(name)) {
		return ReplaceString(name, 0, s);
	}
	return AddString(name, s);
}

status_t Settings::SetPoint(const char *name, BPoint p)
{
	dirty = true;
	if (HasPoint(name)) {
		return ReplacePoint(name, 0, p);
	}
	return AddPoint(name, p);
}

status_t Settings::SetRect(const char *name, BRect r)
{
	dirty = true;
	if (HasRect(name)) {
		return ReplaceRect(name, 0, r);
	}
	return AddRect(name, r);
}

status_t Settings::SetMessage(const char *name, const BMessage *message)
{
	dirty = true;
	if (HasMessage(name)) {
		return ReplaceMessage(name, 0, message);
	}
	return AddMessage(name, message);
}

status_t Settings::SetFlat(const char *name, const BFlattenable *obj)
{
	dirty = true;
	if (HasFlat(name, obj)) {
		return ReplaceFlat(name, 0, (BFlattenable *) obj);
	}
	return AddFlat(name, (BFlattenable *) obj);
}

status_t Settings::SetColor(const char *name, const rgb_color* color)
{
	dirty = true;
	if (HasData(name, B_RGB_COLOR_TYPE)) {
		return ReplaceData(name, B_RGB_COLOR_TYPE, color, sizeof(rgb_color));
	}
	return AddData(name, B_RGB_COLOR_TYPE, color, sizeof(rgb_color));
}

bool Settings::GetColor(const char *name, rgb_color* color)
{
	if (!HasData(name, B_RGB_COLOR_TYPE)) {
		return false;
	}

	rgb_color *clr = 0;
	ssize_t sz = sizeof(rgb_color);
	if (B_OK == FindData(name, B_RGB_COLOR_TYPE, (const void**)&clr, &sz)
						&& sz == sizeof(rgb_color))
   	{
		*color = *clr;
		return true;
	}

	return false;
}

