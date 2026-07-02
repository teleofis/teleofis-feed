#
# OpenWrt package recipe for RMS IoT device management agent.
#
# Source is fetched from a git repository — set PKG_SOURCE_URL and
# PKG_SOURCE_VERSION (commit SHA or tag) below to point at your fork/release.
#
# Layout: copy this `openwrt/` directory into the OpenWrt build tree as
#   package/rms/
# and build with:
#   make package/rms/compile V=s
#

include $(TOPDIR)/rules.mk

PKG_NAME:=rms
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=git@gitlab.corp.teleofis.ru:stargate-v1/stargate-agent.git
PKG_SOURCE_VERSION:=e4aaf3d7e6bf3da763c6bf0069e89580d0d3b3fc
PKG_SOURCE_DATE:=2026-07-02
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_MIRROR_HASH:=skip

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_LICENSE:=Proprietary
PKG_MAINTAINER:=RMS

include $(INCLUDE_DIR)/package.mk

define Package/rms
  SECTION:=net
  CATEGORY:=Network
  TITLE:=RMS device management agent
  URL:=$(PKG_SOURCE_URL)
  DEPENDS:=+libmosquitto-ssl +libjson-c +libopenssl +libuuid
endef

define Package/rms/description
  RMS provides remote management of devices via MQTT/TLS
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include -I$(PKG_BUILD_DIR)/include" \
		LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib"
endef

define Package/rms/conffiles
/etc/config/rms
/etc/rms/*
endef

define Package/rms/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/rms-agent     $(1)/usr/sbin/rms-agent
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/rms-telemetry $(1)/usr/sbin/rms-telemetry

	$(INSTALL_DIR) $(1)/etc/rms
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/config/blacklist.txt $(1)/etc/rms/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/config/deferred.conf $(1)/etc/rms/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/config/payload.*.csv   $(1)/etc/rms/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/config/metrics.csv   $(1)/etc/rms/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/config/apply_config.sh  $(1)/etc/rms/
	$(INSTALL_BIN)  $(PKG_BUILD_DIR)/config/storage_classify.sh $(1)/etc/rms/

	$(INSTALL_DIR) $(1)/etc/rms/certs
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/config/bootstrap/ca.crt        $(1)/etc/rms/certs/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/config/bootstrap/bootstrap.crt $(1)/etc/rms/certs/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/config/bootstrap/bootstrap.key $(1)/etc/rms/certs/

	$(INSTALL_DIR) $(1)/etc/rms/certs/selfhosted

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/config/openwrt/rms-agent.init     $(1)/etc/init.d/rms-agent
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/config/openwrt/rms-telemetry.init $(1)/etc/init.d/rms-telemetry

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/config/rms.config $(1)/etc/config/rms
endef

$(eval $(call BuildPackage,rms))
