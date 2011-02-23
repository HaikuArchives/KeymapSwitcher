##
##
##
OBJ_DIR := objects.x86-*
APP := KeymapSwitcher
ADDON := keymap_switcher
DIST_DIR := dist
ADDON_DEST := $(DIST_DIR)/common/add-ons/input_server/filters/ 
APP_DEST := $(DIST_DIR)/common/bin/ 
CATALOGS_DEST := $(DIST_DIR)/common/data/locale/catalogs/
APP_MIME_SIG := x-vnd.Nexus-KeymapSwitcher
VERSION := 1.2.6
CC_VER = $(word 1, $(subst -, , $(subst ., , $(shell $(CC) -dumpversion))))

default:
	make -f app.makefile
	make -f addon.makefile

$(OBJ_DIR)/$(APP): $(OBJ_DIR)/$(ADDON)
	make -f app.makefile

$(OBJ_DIR)/$(ADDON):
	make -f addon.makefile

clean:
	-rm -rf $(OBJ_DIR)/*

$(APP_DEST):
	mkdir -p $(APP_DEST)

$(ADDON_DEST):
	mkdir -p $(ADDON_DEST)

$(CATALOGS_DEST): $(OBJ_DIR)/$(APP_MIME_SIG)
	mkdir -p $(CATALOGS_DEST)

$(OBJ_DIR)/$(APP_MIME_SIG):
	make -f app.makefile catalogs

package: $(OBJ_DIR)/$(APP)  $(APP_DEST) $(ADDON_DEST) $(CATALOGS_DEST)
	-cp $(OBJ_DIR)/$(APP) $(APP_DEST)
	-cp $(OBJ_DIR)/$(ADDON) $(ADDON_DEST)
	-cp -r $(OBJ_DIR)/$(APP_MIME_SIG) $(CATALOGS_DEST)
	echo "Package: KeymapSwitcher" > $(DIST_DIR)/.OptionalPackageDescription
	echo "Version: $(VERSION)-gcc$(CC_VER)" >> $(DIST_DIR)/.OptionalPackageDescription
	echo "Copyright: Stanislav Maximov etc." >> $(DIST_DIR)/.OptionalPackageDescription
	echo "Description: Easy to use Keymap Switcher for Haiku OS." >> $(DIST_DIR)/.OptionalPackageDescription
	echo "License: BSD/MIT" >> $(DIST_DIR)/.OptionalPackageDescription
	echo "URL: http://www.sf.net/projects/switcher" >> $(DIST_DIR)/.OptionalPackageDescription
	cd $(DIST_DIR) && zip -9 -r -y KeymapSwitcher-$(VERSION)-x86-gcc$(CC_VER)-`date +%F`.zip common .OptionalPackageDescription
