##
##
##
CC_VER = $(word 1, $(subst -, , $(subst ., , $(shell $(CC) -dumpversion))))
OBJ_DIR := objects.x86-gcc$(CC_VER)-*
APP := KeymapSwitcher
ADDON := keymap_switcher
DIST_DIR := dist
ADDON_DEST := $(DIST_DIR)/common/add-ons/input_server/filters/ 
APP_DEST := $(DIST_DIR)/common/bin/ 
CATALOGS_DEST := $(DIST_DIR)/common/data/locale/catalogs/
APP_MIME_SIG := x-vnd.Nexus-KeymapSwitcher
VERSION := 1.2.7
DATE := `date +%F`
PACKAGE_NAME := KeymapSwitcher-$(VERSION)-x86-gcc$(CC_VER)-$(DATE)


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

package: clean $(OBJ_DIR)/$(APP) $(APP_DEST) $(ADDON_DEST) $(CATALOGS_DEST)
	-cp $(OBJ_DIR)/$(APP) $(APP_DEST)
	-cp $(OBJ_DIR)/$(ADDON) $(ADDON_DEST)
	-cp -r $(OBJ_DIR)/$(APP_MIME_SIG) $(CATALOGS_DEST)
	echo "Package: KeymapSwitcher" > $(DIST_DIR)/.OptionalPackageDescription
	echo "Version: $(VERSION)-gcc$(CC_VER)" >> $(DIST_DIR)/.OptionalPackageDescription
	echo "Copyright: Stanislav Maximov etc." >> $(DIST_DIR)/.OptionalPackageDescription
	echo "Description: Easy to use Keymap Switcher for Haiku OS." >> $(DIST_DIR)/.OptionalPackageDescription
	echo "License: BSD/MIT" >> $(DIST_DIR)/.OptionalPackageDescription
	echo "URL: http://www.sf.net/projects/switcher" >> $(DIST_DIR)/.OptionalPackageDescription
	cd $(DIST_DIR) && zip -9 -r -z -y $(PACKAGE_NAME).zip common .OptionalPackageDescription < .OptionalPackageDescription
	echo "#!/bin/sh" > $(DIST_DIR)/update-$(PACKAGE_NAME).sh
	echo "cd \`dirname \$$""0\`" >> $(DIST_DIR)/update-$(PACKAGE_NAME).sh
	echo "hey input_server quit" >> $(DIST_DIR)/update-$(PACKAGE_NAME).sh
	echo "hey Deskbar quit" >> $(DIST_DIR)/update-$(PACKAGE_NAME).sh
	echo "unzip -o $(PACKAGE_NAME).zip -d /boot" >> $(DIST_DIR)/update-$(PACKAGE_NAME).sh
	echo "/boot/system/Deskbar &" >> $(DIST_DIR)/update-$(PACKAGE_NAME).sh
	echo "/boot/system/servers/input_server &" >> $(DIST_DIR)/update-$(PACKAGE_NAME).sh
	echo "/boot/common/bin/KeymapSwitcher --deskbar" >> $(DIST_DIR)/update-$(PACKAGE_NAME).sh
	chmod 700 $(DIST_DIR)/update-$(PACKAGE_NAME).sh
