#
# Copyright (C) 2013-2016 OpenWrt.org
# Copyright (C) 2017 Yousong Zhou
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_VERSION:=2025.07

PKG_HASH:=0f933f6c5a426895bf306e93e6ac53c60870e4b54cda56d95211bec99e63bec7

PKG_MAINTAINER:=Zoltan HERPAI <wigyori@uid0.hu>

UBOOT_USE_INTREE_DTC:=1

include $(INCLUDE_DIR)/u-boot.mk
include $(INCLUDE_DIR)/package.mk

define U-Boot/Default
  BUILD_TARGET:=sunxi
  UBOOT_IMAGE:=u-boot-sunxi-with-spl.bin
  UENV:=default
  HIDDEN:=1
endef

define U-Boot/teleofis_rtu1068v5
  BUILD_SUBTARGET:=cortexa53
  NAME:=Teleofis RTU1068v5
  BUILD_DEVICES:=teleofis_rtu1068v5
  DEPENDS:=+PACKAGE_u-boot-teleofis_rtu1068v5:trusted-firmware-a-sunxi-rtu1068v5
  UENV:=h616
  ATF:=rtu1068v5
endef

UBOOT_TARGETS := \
	teleofis_rtu1068v5

UBOOT_CONFIGURE_VARS += USE_PRIVATE_LIBGCC=yes

UBOOT_CUSTOMIZE_CONFIG := \
	--disable TOOLS_MKEFICAPSULE

UBOOT_MAKE_FLAGS += \
	BL31=$(STAGING_DIR_IMAGE)/bl31_sunxi-$(ATF).bin SCP=/dev/null

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(CP) $(PKG_BUILD_DIR)/$(UBOOT_IMAGE) $(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-u-boot-with-spl.bin
	mkimage -C none -A arm -T script -d uEnv-$(UENV).txt \
		$(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-boot.scr
endef

define Package/u-boot/install/default
endef

$(eval $(call BuildPackage/U-Boot))
