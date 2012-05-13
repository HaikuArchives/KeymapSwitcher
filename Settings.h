#ifndef __Settings_h
#define __Settings_h


#include <GraphicsDefs.h>
#include <Path.h>
#include <Message.h>
#include <String.h>

// hot-keys constants...
const int32 KEY_LCTRL_SHIFT = 0x2000;
const int32 KEY_OPT_SHIFT	 = 0x2001;
const int32 KEY_ALT_SHIFT   = 0x2002;
const int32 KEY_SHIFT_SHIFT = 0x2003;
const int32 KEY_CAPS_LOCK   = 0x2004;
const int32 KEY_SCROLL_LOCK = 0x2005;


class Settings : public BMessage
{
public:
	Settings(const char *filename);
	~Settings();
	status_t InitCheck() { return status; }
	status_t Reload();
	status_t SetDefaults();
	status_t Save();
	status_t SetBool(const char *name, bool b);
	status_t SetInt8(const char *name, int8 i);
	status_t SetInt16(const char *name, int16 i);
	status_t SetInt32(const char *name, int32 i);
	status_t SetInt64(const char *name, int64 i);
	status_t SetFloat(const char *name, float f);
	status_t SetColor(const char *name, const rgb_color* color);
	status_t SetDouble(const char *name, double d);
	status_t SetString(const char *name, const char *string);
	status_t SetPoint(const char *name, BPoint p);
	status_t SetRect(const char *name, BRect r);
	status_t SetMessage(const char *name, const BMessage *message);
	status_t SetFlat(const char *name, const BFlattenable *obj);
	BString GetPath() { return path.Path(); }
	bool GetColor(const char *name, rgb_color* color);
private:
	BPath 	path;
	status_t status;
	bool dirty;
};

#endif

