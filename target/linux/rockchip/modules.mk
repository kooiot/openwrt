# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2022 Dirk Chang <dirk@kooiot.com>


#define KernelPackage/pcie-dw-rockchip
#  SUBMENU:=$(OTHER_MENU)
#  TITLE:=Rockchip DesignWare PCIe controller
#  DEPENDS:=@PCI_SUPPORT @TARGET_rockchip
#  KCONFIG:= \
			CONFIG_PCI_MSI=y \
			CONFIG_APCI_MSI_IRQ_DOMAIN=y \
			CONFIG_PCIE_DW=y \
			CONFIG_PCIE_DW_HOST=y \
			CONFIG_PCIE_ROCKCHIP_DW_HOST
#  FILES:=$(LINUX_DIR)/drivers/pci/controller/dwc/pcie-dw-rockchip.ko
#  AUTOLOAD:=$(call AutoLoad,35,pcie-dw-rockchip)
#endef

#define KernelPackage/pcie-dw-rockchip/description
# Kernel modules for Rockchip DesignWare PCIe controller.
#endef

#$(eval $(call KernelPackage,pcie-dw-rockchip))

