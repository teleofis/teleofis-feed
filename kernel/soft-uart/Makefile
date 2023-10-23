#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=soft-uart
PKG_RELEASE:=1

PKG_SOURCE_URL:=https://github.com/adrianomarto/soft_uart
PKG_SOURCE_PROTO:=git
PKG_SOURCE_DATE:=2022-08-16
PKG_SOURCE_VERSION:=cf9f362f01ea6f3432a61f0822ac09378a9190a8
PKG_MIRROR_HASH:=c782bfc2db37f3798064cf25f825e9aece1a282c2390b6fffd80ccdd384903bf

include $(INCLUDE_DIR)/package.mk

define KernelPackage/soft-uart
  SUBMENU:= Other modules
  DEPENDS:=
  TITLE:= This module creates a software-based serial port using a configurable pair of GPIO pins
  FILES:=$(PKG_BUILD_DIR)/soft_uart.ko
endef

define Build/Compile
	 $(MAKE) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)" \
		EXTRA_CFLAGS="-Wno-error=incompatible-pointer-types" \
		modules
endef

$(eval $(call KernelPackage,soft-uart))
