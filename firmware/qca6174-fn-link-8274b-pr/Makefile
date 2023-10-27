
include $(TOPDIR)/rules.mk

PKG_NAME:=qca6174-fn-link-8274b-pr
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/qca6174-fn-link-8274b-pr

include $(INCLUDE_DIR)/package.mk

define Package/ath10k-firmware-qca6174-fn-link-8274b-pr
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=ath10 qca6174 FN-LINK 8274B-PR
  PKGARCH:=all
endef

define Package/ath10k-firmware-qca6174-fn-link-8274b-pr/description
	 This package contains board file and firmware for the FN-LINK 8274B-PR module
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/ath10k-firmware-qca6174-fn-link-8274b-pr/install
	$(INSTALL_DIR) $(1)/lib/firmware/ath10k/QCA6174/hw3.0
	$(INSTALL_DATA) \
		./files/board-2.bin \
		$(1)/lib/firmware/ath10k/QCA6174/hw3.0/
	$(INSTALL_DATA) \
		./files/firmware-6.bin \
		$(1)/lib/firmware/ath10k/QCA6174/hw3.0/firmware-6.bin
endef

$(eval $(call BuildPackage,ath10k-firmware-qca6174-fn-link-8274b-pr))
