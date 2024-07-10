#
# Copyright (C) 2007-2021 OpenWrt.org
# Copyright (C) 2010 Vertical Communications

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/version.mk
include $(INCLUDE_DIR)/package.mk

PKG_NAME:=base-files-lt70
PKG_VERSION:=1
PKG_RELEASE:=0
PKG_BUILD_DIR:=$(BUILD_DIR)/base-files-lt70

define Package/base-files-lt70
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Base config for TELEOFIS LT70
  DEPENDS:=
endef

define Package/base-files-lt70/description
	 This package contains a utility for managing the interface using ping requests.
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/base-files-lt70/install
	$(CP) ./files/* $(1)/
	$(VERSION_SED_SCRIPT) $(1)/etc/banner
	$(VERSION_SED_SCRIPT) $(1)/etc/openwrt_release
endef

$(eval $(call BuildPackage,base-files-lt70))