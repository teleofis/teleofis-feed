include $(TOPDIR)/rules.mk

PKG_NAME:=iolinesd
PKG_VERSION:=1
PKG_RELEASE:=1.2.0
PKG_BUILD_DIR:=$(BUILD_DIR)/iolinesd

PKG_MAINTAINER:=TELEOFIS

include $(INCLUDE_DIR)/package.mk

define Package/iolinesd
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=I/O lines threshold monitor
  DEPENDS:=
endef

define Package/iolinesd/description
	 This package contains a daemon that polls arbitrary files, compares the
	 values against thresholds and runs a command on every threshold crossing.
endef

define Package/iolinesd/conffiles
/etc/iolinesd.conf
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/iolinesd/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/iolinesd $(1)/sbin/
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_CONF) ./files/iolinesd.conf $(1)/etc/iolinesd.conf
	$(INSTALL_DIR) $(1)/usr/share/iolinesd
	$(INSTALL_DATA) ./files/iolinesd.sh $(1)/usr/share/iolinesd/iolinesd.sh
endef

$(eval $(call BuildPackage,iolinesd))
