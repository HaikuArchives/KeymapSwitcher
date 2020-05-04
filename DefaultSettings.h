#ifndef __DefaultSettings_h
#define __DefaultSettings_h

#include <FindDirectory.h>

struct _keymap_data {
	const char *locale;
	const char *keymap;
	int32 remap;
	int32 directory;
};


_keymap_data keymapDefaults[] = {
	{"ru",	"Russian",		1L, B_SYSTEM_DATA_DIRECTORY},
	{"uk",	"Ukainian",		1L, B_SYSTEM_DATA_DIRECTORY},
	{"be",	"Belarusian",	1L, B_SYSTEM_DATA_DIRECTORY}
};

#endif

