include $(TOPDIR)/rules.mk

PKG_NAME:=supervisor
PKG_VERSION:=48.7.1.2.1
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/supervisor

include $(INCLUDE_DIR)/package.mk

define Package/supervisor
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Supervisor (gd3x) control for LT70
  DEPENDS:=+stm32flash-gd
  PROVIDES:=svcontrol svupgrade event-handler
  PKGARCH:=all
endef

define Package/supervisor/description
	 Supervisor watchdog, firmware upgrade, reboot notification and event handler
endef

define Package/supervisor/conffiles
/etc/config/supervisor
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/supervisor/install
	$(INSTALL_DIR) $(1)/usr/lib/supervisor
	$(INSTALL_DATA) ./files/supervisor.lib $(1)/usr/lib/supervisor/common.sh
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) ./files/svcontrol $(1)/usr/sbin/
	$(INSTALL_BIN) ./files/svupgrade $(1)/usr/sbin/
	$(INSTALL_BIN) ./files/svflash $(1)/usr/sbin/
	$(INSTALL_BIN) ./files/event-handler $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/supervisor.config $(1)/etc/config/supervisor
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/svcontrol.init $(1)/etc/init.d/svcontrol
	$(INSTALL_BIN) ./files/svreboot.init $(1)/etc/init.d/svreboot
	$(INSTALL_BIN) ./files/svupgrade.init $(1)/etc/init.d/svupgrade
	$(INSTALL_BIN) ./files/event-handler.init $(1)/etc/init.d/event-handler
	$(INSTALL_DIR) $(1)/etc/rc.button
	$(INSTALL_BIN) ./files/BTN_0 $(1)/etc/rc.button/
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_BIN) ./files/89-supervisor-defaults $(1)/etc/uci-defaults/
	$(INSTALL_BIN) ./files/90-supervisor-migrate $(1)/etc/uci-defaults/
	$(INSTALL_DIR) $(1)/lib/firmware/gd3x
	$(INSTALL_BIN) ./files/lt7x.gd3x.bin $(1)/lib/firmware/gd3x/lt7x.gd3x.bin.backup
	$(INSTALL_BIN) ./files/rtu1068v5.gd3x.bin $(1)/lib/firmware/gd3x/rtu1068v5.gd3x.bin.backup
endef

$(eval $(call BuildPackage,supervisor))
