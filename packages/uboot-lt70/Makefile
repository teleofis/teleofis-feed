include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_VERSION:=2023.04
PKG_HASH:=e31cac91545ff41b71cec5d8c22afd695645cd6e2a442ccdacacd60534069341
PKG_BUILD_DEPENDS:=!(TARGET_ramips||TARGET_mediatek_mt7623):arm-trusted-firmware-tools/host

UBOOT_USE_INTREE_DTC:=1

include $(INCLUDE_DIR)/u-boot.mk
include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

MT7621_LOWLEVEL_PRELOADER_URL:=https://raw.githubusercontent.com/mtk-openwrt/mt7621-lowlevel-preloader/a03b07c60bf1ba4add9b671d32caa102fe948180/

define Download/mt7621-stage-sram
  FILE:=mt7621_stage_sram.bin
  URL:=$(MT7621_LOWLEVEL_PRELOADER_URL)
  HASH:=1dda68aa089f0ff262e01539b990dea478952e9fb68bcc0a8cd6f76f0135c62e
endef

define Download/mt7621-stage-sram-noprint
  FILE:=mt7621_stage_sram_noprint.bin
  URL:=$(MT7621_LOWLEVEL_PRELOADER_URL)
  HASH:=8ee419275144fc298e9444d413d98e965a55d283152a74ea6a1f8de79eb516b6
endef

ifdef CONFIG_TARGET_ramips_mt7621
ifdef CONFIG_DEBUG
$(eval $(call Download,mt7621-stage-sram))
else
$(eval $(call Download,mt7621-stage-sram-noprint))
endif
endif

define U-Boot/Default
  BUILD_TARGET:=mediatek
  UBOOT_IMAGE:=u-boot-mtk.bin
  HIDDEN:=1
endef

define U-Boot/teleofis_lt70
  NAME:=TELEOFIS LT70
  UBOOT_CONFIG:=mt7621_teleofis_lt70
  BUILD_DEVICES:=teleofis_lt70
  BUILD_TARGET:=ramips
  BUILD_SUBTARGET:=mt7621
  UBOOT_IMAGE:=u-boot-mt7621.bin
endef

UBOOT_TARGETS := \
	teleofis_lt70

ifdef CONFIG_TARGET_ramips_mt7621
define Build/Prepare
	$(call Build/Prepare/Default)
ifdef CONFIG_DEBUG
	$(CP) $(DL_DIR)/mt7621_stage_sram.bin $(PKG_BUILD_DIR)/
else
	$(CP) $(DL_DIR)/mt7621_stage_sram_noprint.bin $(PKG_BUILD_DIR)/mt7621_stage_sram.bin
endif
endef
endif

define Build/Configure
	$(call Build/Configure/U-Boot)
	sed -i 's/CONFIG_TOOLS_LIBCRYPTO=y/# CONFIG_TOOLS_LIBCRYPTO is not set/' $(PKG_BUILD_DIR)/.config
endef

define Build/Compile
	$(call Build/Compile/U-Boot)
endef

# don't stage files to bindir, let target/linux/mediatek/image/*.mk do that
ifdef CONFIG_TARGET_mediatek
define Package/u-boot/install
endef
endif

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(UBOOT_IMAGE) $(STAGING_DIR_IMAGE)/$(BUILD_VARIANT)-$(UBOOT_IMAGE)
endef

$(eval $(call BuildPackage/U-Boot))
