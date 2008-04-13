##
##
##
OBJ_DIR := obj.x86
APP := KeymapSwitcher
ADDON := keymap_switcher

default: $(OBJ_DIR)/$(APP)

$(OBJ_DIR)/$(APP): $(OBJ_DIR)/$(ADDON)
	make -f app.makefile

$(OBJ_DIR)/$(ADDON):
	make -f addon.makefile

clean:
	-rm -rf $(OBJ_DIR)/*
