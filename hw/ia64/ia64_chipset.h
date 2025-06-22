/*
 * IA-64 chipset definitions
 *
 * Copyright (c) 2024 QEMU IA-64 Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HW_IA64_CHIPSET_H
#define HW_IA64_CHIPSET_H

#include "hw/pci/pci_host.h"
#include "hw/sysbus.h"
#include "qom/object.h"

/* Intel 82460GX Host Bridge */
#define TYPE_I82460GX_HOST_BRIDGE "i82460gx-host-bridge"
#define TYPE_I82460GX_PCI_DEVICE  "i82460gx-pci-device"

typedef struct I82460GXHostBridgeState I82460GXHostBridgeState;

DECLARE_INSTANCE_CHECKER(I82460GXHostBridgeState, IA64_I82460GX_HOST_BRIDGE,
                         TYPE_I82460GX_HOST_BRIDGE)

#endif /* HW_IA64_CHIPSET_H */