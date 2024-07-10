include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/package.mk

PKG_NAME:=base-files-common
PKG_VERSION:=1
PKG_RELEASE:=0
PKG_BUILD_DIR:=$(BUILD_DIR)/base-files-common

define Package/base-files-common
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=Common files for TELEOFIS devices
	DEPENDS:=
endef

define Package/base-files-common/description
	 This package contains a base filesystem and system scripts for devices.
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/base-files-common/install
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,base-files-common))