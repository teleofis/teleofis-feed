#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=soft-uart
PKG_RELEASE:=2026.03.21

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/adrianomarto/soft_uart.git
PKG_MIRROR_HASH:=db999824f7879faaacf3cdceaf9ffcd521b22979c028c0f2153da2e6d8105d5f
PKG_SOURCE_VERSION:=b2ebe9a513d5a94a5dbbde8362cb12642201f87f

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
