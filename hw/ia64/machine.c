/*
 * IA-64 Itanium machine with Intel 82460GX chipset
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
#include "qemu/units.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_bus.h"
#include "hw/qdev-properties.h"
#include "hw/ide/pci.h"
#include "hw/timer/i8254.h"
#include "hw/rtc/mc146818rtc.h"
#include "hw/char/serial.h"
#include "hw/block/fdc.h"
#include "net/net.h"
#include "qemu/error-report.h"
#include "system/reset.h"
#include "system/memory.h"
#include "exec/address-spaces.h"
#include "target/ia64/cpu.h"
#include "hw/ia64/ia64_chipset.h"
#include "hw/isa/isa.h"

#define IA64_MAX_CPUS 64

/* Memory layout for IA-64 machine */
#define IA64_RAM_BASE        0x0000000000000000ULL
#define IA64_FIRMWARE_BASE   0x00000001FFE00000ULL  /* 2MB firmware space */
#define IA64_PCI_MEM_BASE    0x00000001F0000000ULL
#define IA64_PCI_IO_BASE     0x00000001E0000000ULL

static void ia64_machine_init(MachineState *machine)
{
    IA64CPU *cpu;
    MemoryRegion *system_memory = get_system_memory();
    MemoryRegion *ram = g_new(MemoryRegion, 1);
    MemoryRegion *firmware = g_new(MemoryRegion, 1);
    MemoryRegion *pci_memory = g_new(MemoryRegion, 1);
    MemoryRegion *pci_io = g_new(MemoryRegion, 1);
    PCIBus *pci_bus;
    DeviceState *i82460gx_dev;
    DeviceState *isa_dev;
    int i;

    /* Create CPUs */
    for (i = 0; i < machine->smp.cpus; i++) {
        cpu = IA64_CPU(cpu_create(machine->cpu_type));
        if (!cpu) {
            error_report("Unable to create IA64 CPU");
            exit(1);
        }
    }

    /* Allocate RAM */
    if (machine->ram_size > 64 * GiB) {
        error_report("IA-64 machine supports maximum 64GB RAM");
        exit(1);
    }

    memory_region_init_ram(ram, NULL, "ia64.ram", machine->ram_size,
                           &error_fatal);
    memory_region_add_subregion(system_memory, IA64_RAM_BASE, ram);

    /* Firmware/BIOS area */
    memory_region_init_ram(firmware, NULL, "ia64.firmware", 2 * MiB,
                           &error_fatal);
    memory_region_add_subregion(system_memory, IA64_FIRMWARE_BASE, firmware);

    /* PCI memory space */
    memory_region_init(pci_memory, NULL, "pci-memory", 256 * MiB);
    memory_region_add_subregion(system_memory, IA64_PCI_MEM_BASE, pci_memory);

    /* PCI I/O space */
    memory_region_init(pci_io, NULL, "pci-io", 16 * MiB);
    memory_region_add_subregion(system_memory, IA64_PCI_IO_BASE, pci_io);

    /* Create Intel 82460GX chipset */
    i82460gx_dev = qdev_new(TYPE_I82460GX_HOST_BRIDGE);
    qdev_prop_set_uint32(i82460gx_dev, "ram-size", machine->ram_size / MiB);
    pci_bus = IA64_I82460GX_HOST_BRIDGE(i82460gx_dev)->pci_bus;
    sysbus_realize_and_unref(SYS_BUS_DEVICE(i82460gx_dev), &error_fatal);

    /* ISA bridge and devices */
    isa_dev = DEVICE(pci_create_simple(pci_bus, PCI_DEVFN(7, 0), "piix3-ide"));

    /* Create ISA devices */
    /* Create ISA bus from PCI-ISA bridge */
    ISABus *isa_bus = ISA_BUS(qdev_get_child_bus(isa_dev, "isa.0"));

    /* Timer */
    i8254_pit_init(isa_bus, 0x40, 0, NULL);

    /* RTC */
    mc146818_rtc_init(isa_bus, 2000, NULL);

    /* Serial ports */
    if (serial_hd(0)) {
        serial_hd_init(isa_bus, 0, 0x3f8, 4, serial_hd(0));
    }
    if (serial_hd(1)) {
        serial_hd_init(isa_bus, 1, 0x2f8, 3, serial_hd(1));
    }

    /* Floppy controller */
    fdctrl_init_isa(isa_bus, NULL);

    /* Network card */
    if (nd_table[0].used) {
        pci_nic_init_nofail(&nd_table[0], pci_bus, "e1000", NULL);
    }

    /* Load firmware/BIOS if provided */
    if (machine->firmware) {
        uint64_t firmware_size;
        
        firmware_size = load_image_size(machine->firmware,
                                        memory_region_get_ram_ptr(firmware),
                                        2 * MiB);
        if (firmware_size < 0) {
            error_report("Could not load firmware '%s'", machine->firmware);
            exit(1);
        }
    }
}

static void ia64_machine_class_init(ObjectClass *oc, void *data)
{
    static const char * const valid_cpu_types[] = {
        TYPE_IA64_CPU_GENERIC,
        TYPE_ITANIUM_CPU,
        TYPE_ITANIUM2_CPU,
        NULL
    };
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->desc = "IA-64 machine with Intel 82460GX chipset";
    mc->init = ia64_machine_init;
    mc->max_cpus = IA64_MAX_CPUS;
    mc->default_cpus = 1;
    mc->default_cpu_type = TYPE_ITANIUM_CPU;
    mc->valid_cpu_types = valid_cpu_types;
    mc->default_ram_size = 512 * MiB;
    mc->default_ram_id = "ia64.ram";
    mc->block_default_type = IF_IDE;
    mc->no_floppy = false;
    mc->no_cdrom = false;
    mc->no_parallel = true;  /* No parallel port on this machine */
    mc->default_boot_order = "cda";
}

static const TypeInfo ia64_machine_type = {
    .name = MACHINE_TYPE_NAME("ia64"),
    .parent = TYPE_MACHINE,
    .class_init = ia64_machine_class_init,
};

static void ia64_machine_register_types(void)
{
    type_register_static(&ia64_machine_type);
}

type_init(ia64_machine_register_types)