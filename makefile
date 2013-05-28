##
##
##

MACHINE=$(shell uname -m)
ifeq ($(MACHINE), BePC)
	MACHINE = x86-gcc$(word 1, $(subst -, , $(subst ., , $(shell $(CC) -dumpversion))))
endif

OBJ_DIR := objects.$(MACHINE)-*
APP := KeymapSwitcher
ADDON := keymap_switcher
DIST_DIR := dist
ADDON_DEST := $(DIST_DIR)/common/add-ons/input_server/filters/ 
APP_DEST := $(DIST_DIR)/common/bin/ 
CATALOGS_DEST := $(DIST_DIR)/common/data/locale/catalogs/
VERSION := 1.2.7
DATE := `date +%F`
PACKAGE_NAME := KeymapSwitcher-$(VERSION)-$(MACHINE)-$(DATE)

# Haiku package support
APP_MIME_SIG := x-vnd.Nexus-KeymapSwitcher
HPKG_ADDON_DEST := $(BUILDHOME)/add-ons/input_server/filters/ 
HPKG_APP_DEST := $(BUILDHOME)/bin/ 
HPKG_CATALOGS_DEST := $(BUILDHOME)/data/locale/catalogs/

default:
	make -f app.makefile
	make -f addon.makefile

$(OBJ_DIR)/$(APP): $(OBJ_DIR)/$(ADDON)
	make -f app.makefile
	make -f app.makefile bindcatalogs

$(OBJ_DIR)/$(ADDON):
	make -f addon.makefile

clean:
	-rm -rf $(OBJ_DIR)/*

clean_dist:
	-rm -rf $(DIST_DIR)/common/*

$(APP_DEST):
	mkdir -p $(APP_DEST)

$(ADDON_DEST):
	mkdir -p $(ADDON_DEST)

package: clean clean_dist $(OBJ_DIR)/$(APP) $(APP_DEST) $(ADDON_DEST)
	-cp $(OBJ_DIR)/$(APP) $(APP_DEST)
	-cp $(OBJ_DIR)/$(ADDON) $(ADDON_DEST)
	echo "Package: KeymapSwitcher" > $(DIST_DIR)/.OptionalPackageDescription
	echo "Version: $(VERSION)-$(MACHINE)" >> $(DIST_DIR)/.OptionalPackageDescription
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

# Haiku package support - do not use directly! Only for HPKG building.

$(HPKG_CATALOGS_DEST): $(OBJ_DIR)/$(APP_MIME_SIG)
	mkdir -p $(HPKG_CATALOGS_DEST)

$(OBJ_DIR)/$(APP_MIME_SIG):
	make -f app.makefile catalogs

$(HPKG_APP_DEST):
	mkdir -p $(HPKG_APP_DEST)

$(HPKG_ADDON_DEST):
	mkdir -p $(HPKG_ADDON_DEST)

hpkg: clean $(OBJ_DIR)/$(APP) $(OBJ_DIR)/$(ADDON) $(HPKG_APP_DEST) \
		$(HPKG_ADDON_DEST) $(HPKG_CATALOGS_DEST)
	-cp $(OBJ_DIR)/$(APP) $(HPKG_APP_DEST)
	-cp $(OBJ_DIR)/$(ADDON) $(HPKG_ADDON_DEST)
	-cp -r $(OBJ_DIR)/$(APP_MIME_SIG) $(HPKG_CATALOGS_DEST)

