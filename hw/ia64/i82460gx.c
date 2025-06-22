/*
 * Intel 82460GX Host Bridge and Memory Controller Hub
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

#include "qemu/osdep.h"
#include "hw/pci/pci_device.h"
#include "hw/pci/pci_bus.h"
#include "hw/qdev-properties.h"
#include "qemu/module.h"
#include "qapi/error.h"
#include "hw/ia64/ia64_chipset.h"
#include "exec/address-spaces.h"

/* Intel 82460GX PCI Configuration Space */
#define I82460GX_VENDOR_ID    0x8086
#define I82460GX_DEVICE_ID    0x84EA
#define I82460GX_REVISION     0x01

/* Memory Controller Registers */
#define I82460GX_DRB_REG      0x60   /* DRAM Row Boundary */
#define I82460GX_DRA_REG      0x70   /* DRAM Row Attributes */
#define I82460GX_DRT_REG      0x78   /* DRAM Timing */

struct I82460GXHostBridgeState {
    SysBusDevice parent_obj;

    PCIBus *pci_bus;
    PCIDevice *pci_dev;
    uint32_t ram_size_mb;

    /* Memory controller state */
    uint8_t drb[8];     /* DRAM Row Boundary registers */
    uint8_t dra[4];     /* DRAM Row Attribute registers */
    uint32_t drt;       /* DRAM Timing register */

    MemoryRegion pci_hole;
    MemoryRegion pci_memory;
    MemoryRegion pci_io;
};

static uint64_t i82460gx_read(void *opaque, hwaddr addr, unsigned size)
{
    /* Placeholder for memory-mapped registers */
    return 0;
}

static void i82460gx_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    /* Placeholder for memory-mapped registers */
}

static const MemoryRegionOps i82460gx_ops = {
    .read = i82460gx_read,
    .write = i82460gx_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
};

static void i82460gx_pci_config_write(PCIDevice *pci_dev, uint32_t addr,
                                      uint32_t val, int len)
{
    I82460GXHostBridgeState *s = IA64_I82460GX_HOST_BRIDGE(pci_dev);

    switch (addr) {
    case I82460GX_DRB_REG ... I82460GX_DRB_REG + 7:
        s->drb[addr - I82460GX_DRB_REG] = val;
        break;
    case I82460GX_DRA_REG ... I82460GX_DRA_REG + 3:
        s->dra[addr - I82460GX_DRA_REG] = val;
        break;
    case I82460GX_DRT_REG:
        s->drt = val;
        break;
    default:
        pci_default_write_config(pci_dev, addr, val, len);
        break;
    }
}

static uint32_t i82460gx_pci_config_read(PCIDevice *pci_dev, uint32_t addr, int len)
{
    I82460GXHostBridgeState *s = IA64_I82460GX_HOST_BRIDGE(pci_dev);

    switch (addr) {
    case I82460GX_DRB_REG ... I82460GX_DRB_REG + 7:
        return s->drb[addr - I82460GX_DRB_REG];
    case I82460GX_DRA_REG ... I82460GX_DRA_REG + 3:
        return s->dra[addr - I82460GX_DRA_REG];
    case I82460GX_DRT_REG:
        return s->drt;
    default:
        return pci_default_read_config(pci_dev, addr, len);
    }
}

static void i82460gx_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    I82460GXHostBridgeState *s = IA64_I82460GX_HOST_BRIDGE(dev);
    PCIHostState *phb = PCI_HOST_BRIDGE(dev);

    /* Create PCI bus */
    s->pci_bus = pci_bus_new(DEVICE(s), "pci.0", 
                            get_system_memory(), get_system_io(),
                            0, TYPE_PCI_BUS);
    phb->bus = s->pci_bus;

    /* Create the host bridge PCI device */
    s->pci_dev = pci_create_simple(s->pci_bus, 0, TYPE_I82460GX_PCI_DEVICE);

    /* Initialize memory controller registers with defaults */
    memset(s->drb, 0, sizeof(s->drb));
    memset(s->dra, 0, sizeof(s->dra));
    s->drt = 0;

    /* Configure memory based on RAM size */
    if (s->ram_size_mb > 0) {
        /* Set up DRAM Row Boundary registers */
        int row = 0;
        uint32_t remaining = s->ram_size_mb;
        
        while (remaining > 0 && row < 8) {
            uint32_t row_size = (remaining > 256) ? 256 : remaining;
            s->drb[row] = (row == 0) ? row_size : s->drb[row - 1] + row_size;
            remaining -= row_size;
            row++;
        }
    }
}

static Property i82460gx_properties[] = {
    DEFINE_PROP_UINT32("ram-size", I82460GXHostBridgeState, ram_size_mb, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void i82460gx_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = i82460gx_realize;
    dc->desc = "Intel 82460GX Host Bridge";
    device_class_set_props(dc, i82460gx_properties);
    dc->user_creatable = false;
}

/* PCI device part of the 82460GX */
static void i82460gx_pci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *pc = PCI_DEVICE_CLASS(klass);

    pc->vendor_id = I82460GX_VENDOR_ID;
    pc->device_id = I82460GX_DEVICE_ID;
    pc->revision = I82460GX_REVISION;
    pc->class_id = PCI_CLASS_BRIDGE_HOST;
    pc->config_read = i82460gx_pci_config_read;
    pc->config_write = i82460gx_pci_config_write;
    
    dc->desc = "Intel 82460GX PCI Host Bridge";
    dc->user_creatable = false;
}

static const TypeInfo i82460gx_host_bridge_info = {
    .name = TYPE_I82460GX_HOST_BRIDGE,
    .parent = TYPE_PCI_HOST_BRIDGE,
    .instance_size = sizeof(I82460GXHostBridgeState),
    .class_init = i82460gx_class_init,
};

static const TypeInfo i82460gx_pci_device_info = {
    .name = TYPE_I82460GX_PCI_DEVICE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(PCIDevice),
    .class_init = i82460gx_pci_class_init,
    .interfaces = (InterfaceInfo[]) {
        { INTERFACE_CONVENTIONAL_PCI_DEVICE },
        { },
    },
};

static void i82460gx_register_types(void)
{
    type_register_static(&i82460gx_host_bridge_info);
    type_register_static(&i82460gx_pci_device_info);
}

type_init(i82460gx_register_types)