
include $(TOPDIR)/rules.mk

PKG_NAME:=iolines-rtu1068v5
PKG_VERSION:=1
PKG_RELEASE:=2
PKG_BUILD_DIR:=$(BUILD_DIR)/iolines-rtu1068v5

include $(INCLUDE_DIR)/package.mk

define Package/iolines-rtu1068v5
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Network interface watchdog
  DEPENDS:=+iolinesd
endef

define Package/iolines-rtu1068v5/description
	 This package contains a utility for managing the interface using ping requests.
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/iolines-rtu1068v5/conffiles
/etc/config/iolines
endef

define Package/iolines-rtu1068v5/install
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./files/iolines.config $(1)/etc/config/iolines
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/iolines.init $(1)/etc/init.d/iolines
endef

$(eval $(call BuildPackage,iolines-rtu1068v5))
