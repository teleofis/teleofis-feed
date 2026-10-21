include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-zabbix

LUCI_TITLE:=zabbix agentd configuration
LUCI_DEPENDS:=+luci-base +zabbix-agentd-openssl +zabbix-config
LUCI_PKGARCH:=all

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature
