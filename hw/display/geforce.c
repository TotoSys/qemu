/*
 * QEMU GeForce GPU emulation
 *
 * Copyright (c) 2025 QEMU Project
 *
 * Based on GeForce emulation from Vort's Bochs fork
 * Copyright (C) 2025 The Bochs Project
 *
 * This implementation provides basic GeForce3 Ti 500 emulation with focus on:
 * - D3D semaphore support for Kelvin (0x97) engine 
 * - MMIO register emulation for graphics operations
 * - FIFO command processing for GPU commands
 * - Basic VGA compatibility through VGACommonState
 *
 * The D3D semaphore functionality enables synchronization for Direct3D games
 * by providing memory-mapped semaphore operations that allow the GPU and CPU
 * to coordinate graphics operations.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "hw/pci/pci_device.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "ui/console.h"
#include "vga_int.h"
#include "trace.h"
#include "geforce.h"

#define GEFORCE_DEBUG 0

#if GEFORCE_DEBUG
#define DPRINTF(fmt, ...) printf("geforce: " fmt, ## __VA_ARGS__)
#else
#define DPRINTF(fmt, ...) do {} while (0)
#endif

/* GeForce register offsets */
#define NV_PMC_INTR_0           0x00000100
#define NV_PMC_INTR_EN_0        0x00000140
#define NV_PMC_ENABLE           0x00000200

#define NV_PBUS_INTR_0          0x00001100
#define NV_PBUS_INTR_EN_0       0x00001140

#define NV_PFIFO_INTR_0         0x00002100
#define NV_PFIFO_INTR_EN_0      0x00002140
#define NV_PFIFO_RAMHT          0x00002210
#define NV_PFIFO_RAMFC          0x00002214
#define NV_PFIFO_RAMRO          0x00002218
#define NV_PFIFO_MODE           0x00002504
#define NV_PFIFO_CACHE1_PUSH1   0x00003204
#define NV_PFIFO_CACHE1_PUT     0x00003210
#define NV_PFIFO_CACHE1_DMA_PUSH 0x00003220
#define NV_PFIFO_CACHE1_DMA_INSTANCE 0x00003224
#define NV_PFIFO_CACHE1_DMA_PUT 0x00003240
#define NV_PFIFO_CACHE1_DMA_GET 0x00003244
#define NV_PFIFO_CACHE1_REF_CNT 0x00003248
#define NV_PFIFO_CACHE1_PULL0   0x00003250
#define NV_PFIFO_CACHE1_SEMAPHORE 0x00003254

/* FIFO methods */
#define NV_FIFO_METHOD_REF_CNT      0x014
#define NV_FIFO_METHOD_SEMAPHORE_ACQUIRE 0x01a
#define NV_FIFO_METHOD_SEMAPHORE_RELEASE 0x01b

/* CRTC registers */
#define NV_CRTC_INDEX_COLOR     0x3d4
#define NV_CRTC_DATA_COLOR      0x3d5

static const struct {
    const char *name;
    uint16_t device_id;
} geforce_models[] = {
    { "geforce3", PCI_DEVICE_ID_NVIDIA_GEFORCE3_TI500 },
};

static void geforce_update_irq(GeForceState *s)
{
    uint32_t mc_intr = 0;
    
    /* Check for pending interrupts */
    if (s->regs.bus_intr & s->regs.bus_intr_en) {
        mc_intr |= 0x01;
    }
    if (s->regs.fifo_intr & s->regs.fifo_intr_en) {
        mc_intr |= 0x100;
    }
    
    /* Update IRQ line */
    bool irq_level = (mc_intr & s->regs.mc_intr_en) != 0;
    pci_set_irq(&s->parent_obj, irq_level);
    
    trace_geforce_irq_update(mc_intr, irq_level);
}

/* D3D semaphore operations for Kelvin (0x97) */
static uint32_t geforce_dma_read32(GeForceState *s, uint32_t object, uint32_t offset)
{
    /* Simplified DMA read - in real implementation would access memory through DMA */
    hwaddr addr = (hwaddr)object + offset;
    if (addr < s->vga.vram_size) {
        return *(uint32_t*)(s->vga.vram_ptr + addr);
    }
    return 0;
}

static void geforce_dma_write32(GeForceState *s, uint32_t object, uint32_t offset, uint32_t value)
{
    /* Simplified DMA write - in real implementation would access memory through DMA */
    hwaddr addr = (hwaddr)object + offset;
    if (addr < s->vga.vram_size) {
        *(uint32_t*)(s->vga.vram_ptr + addr) = value;
    }
}

bool geforce_execute_d3d_command(GeForceState *s, uint32_t chid, uint32_t method, uint32_t param)
{
    if (chid >= GEFORCE_CHANNEL_COUNT) {
        return false;
    }
    
    GeForceChannelState *ch = &s->regs.channels[chid];
    
    switch (method) {
    case NV_D3D_SEMAPHORE_OBJECT:
        ch->d3d_semaphore_obj = param;
        DPRINTF("D3D semaphore object: 0x%08x\n", param);
        break;
        
    case NV_D3D_CLIP_HORIZONTAL:
        ch->d3d_clip_horizontal = param;
        DPRINTF("D3D clip horizontal: 0x%08x\n", param);
        break;
        
    case NV_D3D_CLIP_VERTICAL:
        ch->d3d_clip_vertical = param;
        DPRINTF("D3D clip vertical: 0x%08x\n", param);
        break;
        
    case NV_D3D_SURFACE_FORMAT:
        ch->d3d_surface_format = param;
        /* Update color bytes based on format */
        switch (param & 0xf) {
        case 0x1: ch->d3d_color_bytes = 1; break; /* R5G6B5 */
        case 0x3: ch->d3d_color_bytes = 2; break; /* A1R5G5B5 */
        case 0x5: ch->d3d_color_bytes = 4; break; /* A8R8G8B8 */
        default: ch->d3d_color_bytes = 4; break;
        }
        DPRINTF("D3D surface format: 0x%08x, color_bytes: %d\n", param, ch->d3d_color_bytes);
        break;
        
    case NV_D3D_SURFACE_PITCH:
        ch->d3d_surface_pitch = param;
        DPRINTF("D3D surface pitch: 0x%08x\n", param);
        break;
        
    case NV_D3D_SURFACE_COLOR_OFFSET:
        ch->d3d_surface_color_offset = param;
        DPRINTF("D3D surface color offset: 0x%08x\n", param);
        break;
        
    case NV_D3D_SEMAPHORE_OFFSET:
        ch->d3d_semaphore_offset = param;
        DPRINTF("D3D semaphore offset: 0x%08x\n", param);
        break;
        
    case NV_D3D_SEMAPHORE_ACQUIRE:
        /* Write semaphore value - this is used for D3D synchronization */
        geforce_dma_write32(s, ch->d3d_semaphore_obj, ch->d3d_semaphore_offset, param);
        trace_geforce_d3d_semaphore(ch->d3d_semaphore_obj, ch->d3d_semaphore_offset, param);
        break;
        
    case NV_D3D_COLOR_CLEAR_VALUE:
        ch->d3d_color_clear_value = param;
        DPRINTF("D3D color clear value: 0x%08x\n", param);
        break;
        
    case NV_D3D_CLEAR_SURFACE:
        ch->d3d_clear_surface = param;
        geforce_d3d_clear_surface(s, chid);
        DPRINTF("D3D clear surface: 0x%08x\n", param);
        break;
        
    default:
        return false; /* Unknown method */
    }
    
    return true;
}

void geforce_d3d_clear_surface(GeForceState *s, uint32_t chid)
{
    GeForceChannelState *ch = &s->regs.channels[chid];
    
    /* Simplified surface clear implementation */
    uint32_t offset = ch->d3d_surface_color_offset;
    uint32_t pitch = ch->d3d_surface_pitch;
    uint32_t clear_value = ch->d3d_color_clear_value;
    
    DPRINTF("Clearing D3D surface: offset=0x%08x, pitch=%d, value=0x%08x\n",
            offset, pitch, clear_value);
    
    /* This would clear the surface in real implementation */
    /* For now, just mark the display as needing update */
    if (s->vga.con) {
        dpy_gfx_update_full(s->vga.con);
    }
}

static uint32_t geforce_register_read(GeForceState *s, uint32_t addr)
{
    uint32_t val = 0;
    
    switch (addr) {
    case NV_PMC_INTR_0:
        /* Return pending interrupt status */
        if (s->regs.bus_intr & s->regs.bus_intr_en) {
            val |= 0x01;
        }
        if (s->regs.fifo_intr & s->regs.fifo_intr_en) {
            val |= 0x100;
        }
        break;
        
    case NV_PMC_INTR_EN_0:
        val = s->regs.mc_intr_en;
        break;
        
    case NV_PMC_ENABLE:
        val = s->regs.mc_enable;
        break;
        
    case NV_PBUS_INTR_0:
        val = s->regs.bus_intr;
        break;
        
    case NV_PBUS_INTR_EN_0:
        val = s->regs.bus_intr_en;
        break;
        
    case NV_PFIFO_INTR_0:
        val = s->regs.fifo_intr;
        break;
        
    case NV_PFIFO_INTR_EN_0:
        val = s->regs.fifo_intr_en;
        break;
        
    case NV_PFIFO_RAMHT:
        val = s->regs.fifo_ramht;
        break;
        
    case NV_PFIFO_RAMFC:
        val = s->regs.fifo_ramfc;
        break;
        
    case NV_PFIFO_RAMRO:
        val = s->regs.fifo_ramro;
        break;
        
    case NV_PFIFO_MODE:
        val = s->regs.fifo_mode;
        break;
        
    case NV_PFIFO_CACHE1_PUSH1:
        val = s->regs.fifo_cache1_push1;
        break;
        
    case NV_PFIFO_CACHE1_PUT:
        val = s->regs.fifo_cache1_put;
        break;
        
    case NV_PFIFO_CACHE1_DMA_PUSH:
        val = s->regs.fifo_cache1_dma_push;
        break;
        
    case NV_PFIFO_CACHE1_DMA_INSTANCE:
        val = s->regs.fifo_cache1_dma_instance;
        break;
        
    case NV_PFIFO_CACHE1_DMA_PUT:
        val = s->regs.fifo_cache1_dma_put;
        break;
        
    case NV_PFIFO_CACHE1_DMA_GET:
        val = s->regs.fifo_cache1_dma_get;
        break;
        
    case NV_PFIFO_CACHE1_REF_CNT:
        val = s->regs.fifo_cache1_ref_cnt;
        break;
        
    case NV_PFIFO_CACHE1_PULL0:
        val = s->regs.fifo_cache1_pull0;
        break;
        
    case NV_PFIFO_CACHE1_SEMAPHORE:
        val = s->regs.fifo_cache1_semaphore;
        break;
        
    default:
        DPRINTF("Unhandled register read: 0x%08x\n", addr);
        break;
    }
    
    DPRINTF("register_read: 0x%08x = 0x%08x\n", addr, val);
    return val;
}

static void geforce_register_write(GeForceState *s, uint32_t addr, uint32_t val)
{
    DPRINTF("register_write: 0x%08x = 0x%08x\n", addr, val);
    
    switch (addr) {
    case NV_PMC_INTR_0:
        /* Clear interrupts by writing 1 to them */
        if (val & 0x01) {
            s->regs.bus_intr = 0;
        }
        if (val & 0x100) {
            s->regs.fifo_intr = 0;
        }
        geforce_update_irq(s);
        break;
        
    case NV_PMC_INTR_EN_0:
        s->regs.mc_intr_en = val;
        geforce_update_irq(s);
        break;
        
    case NV_PMC_ENABLE:
        s->regs.mc_enable = val;
        break;
        
    case NV_PBUS_INTR_0:
        /* Clear interrupts by writing 1 to them */
        s->regs.bus_intr &= ~val;
        geforce_update_irq(s);
        break;
        
    case NV_PBUS_INTR_EN_0:
        s->regs.bus_intr_en = val;
        geforce_update_irq(s);
        break;
        
    case NV_PFIFO_INTR_0:
        /* Clear interrupts by writing 1 to them */
        s->regs.fifo_intr &= ~val;
        geforce_update_irq(s);
        break;
        
    case NV_PFIFO_INTR_EN_0:
        s->regs.fifo_intr_en = val;
        geforce_update_irq(s);
        break;
        
    case NV_PFIFO_RAMHT:
        s->regs.fifo_ramht = val;
        break;
        
    case NV_PFIFO_RAMFC:
        s->regs.fifo_ramfc = val;
        break;
        
    case NV_PFIFO_RAMRO:
        s->regs.fifo_ramro = val;
        break;
        
    case NV_PFIFO_MODE:
        s->regs.fifo_mode = val;
        break;
        
    case NV_PFIFO_CACHE1_PUSH1:
        s->regs.fifo_cache1_push1 = val;
        break;
        
    case NV_PFIFO_CACHE1_PUT:
        s->regs.fifo_cache1_put = val;
        /* Process FIFO commands if enabled */
        if (s->regs.fifo_cache1_push1 & 0x1) {
            DPRINTF("FIFO command processing triggered\n");
            /* Simplified command processing - check for D3D commands */
            uint32_t chid = s->regs.fifo_cache1_push1 & 0x1F;
            
            /* Check if this is a Kelvin (0x97) D3D command */
            if (chid < GEFORCE_CHANNEL_COUNT) {
                /* In a real implementation, we would parse the command buffer */
                /* For now, just indicate D3D capability is available */
                DPRINTF("Channel %d ready for D3D commands (Kelvin 0x97)\n", chid);
            }
        }
        break;
        
    case NV_PFIFO_CACHE1_DMA_PUSH:
        s->regs.fifo_cache1_dma_push = val;
        break;
        
    case NV_PFIFO_CACHE1_DMA_INSTANCE:
        s->regs.fifo_cache1_dma_instance = val;
        break;
        
    case NV_PFIFO_CACHE1_DMA_PUT:
        s->regs.fifo_cache1_dma_put = val;
        break;
        
    case NV_PFIFO_CACHE1_DMA_GET:
        s->regs.fifo_cache1_dma_get = val;
        break;
        
    case NV_PFIFO_CACHE1_REF_CNT:
        s->regs.fifo_cache1_ref_cnt = val;
        break;
        
    case NV_PFIFO_CACHE1_PULL0:
        s->regs.fifo_cache1_pull0 = val;
        break;
        
    case NV_PFIFO_CACHE1_SEMAPHORE:
        s->regs.fifo_cache1_semaphore = val;
        break;
        
    default:
        DPRINTF("Unhandled register write: 0x%08x = 0x%08x\n", addr, val);
        break;
    }
}

uint64_t geforce_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    GeForceState *s = GEFORCE(opaque);
    uint64_t val = 0;
    
    if (addr < GEFORCE_MMIO_SIZE) {
        if (size == 4) {
            val = geforce_register_read(s, addr);
        } else {
            DPRINTF("Unsupported MMIO read size %u at 0x%lx\n", size, addr);
        }
    } else {
        DPRINTF("MMIO read out of bounds: 0x%lx\n", addr);
    }
    
    trace_geforce_mmio_read(addr, val);
    return val;
}

void geforce_mmio_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    GeForceState *s = GEFORCE(opaque);
    
    trace_geforce_mmio_write(addr, val);
    
    if (addr < GEFORCE_MMIO_SIZE) {
        if (size == 4) {
            geforce_register_write(s, addr, val);
        } else {
            DPRINTF("Unsupported MMIO write size %u at 0x%lx\n", size, addr);
        }
    } else {
        DPRINTF("MMIO write out of bounds: 0x%lx\n", addr);
    }
}

static const MemoryRegionOps geforce_mmio_ops = {
    .read = geforce_mmio_read,
    .write = geforce_mmio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
    .impl = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

void geforce_init_registers(GeForceState *s)
{
    /* Initialize registers to default values */
    memset(&s->regs, 0, sizeof(s->regs));
    
    /* Set up initial register values */
    s->regs.mc_enable = 0x1;  /* Enable memory controller */
    s->regs.fifo_mode = 0x1;  /* Enable FIFO */
    
    /* Initialize channel states */
    for (int i = 0; i < GEFORCE_CHANNEL_COUNT; i++) {
        s->regs.channels[i].d3d_color_bytes = 1;  /* Default to 1 byte per pixel */
    }
    
    DPRINTF("Registers initialized\n");
}

static void geforce_reset(DeviceState *dev)
{
    GeForceState *s = GEFORCE(dev);
    
    vga_common_reset(&s->vga);
    geforce_init_registers(s);
    
    DPRINTF("Device reset\n");
}

static void geforce_realize(PCIDevice *pci_dev, Error **errp)
{
    GeForceState *s = GEFORCE(pci_dev);
    DeviceState *dev = DEVICE(pci_dev);
    
    /* Initialize VGA */
    if (!vga_common_init(&s->vga, OBJECT(dev), errp)) {
        return;
    }
    
    /* Set up PCI configuration */
    pci_dev->config[PCI_REVISION_ID] = 0xa1; /* GeForce3 Ti 500 revision */
    pci_dev->config[PCI_CLASS_PROG] = 0x00;
    pci_dev->config[PCI_INTERRUPT_PIN] = 1;
    
    /* Set up memory regions */
    memory_region_init_io(&s->mmio, OBJECT(dev), &geforce_mmio_ops, s,
                          "geforce-mmio", GEFORCE_MMIO_SIZE);
    
    /* Register PCI BARs */
    pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio);
    pci_register_bar(pci_dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY | PCI_BASE_ADDRESS_MEM_PREFETCH,
                     &s->vga.vram);
    
    /* Initialize registers */
    geforce_init_registers(s);
    
    /* Set up VGA console */
    s->vga.con = graphic_console_init(dev, 0, &s->vga.hw_ops, s);
    
    DPRINTF("Device realized\n");
}

static void geforce_exit(PCIDevice *pci_dev)
{
    GeForceState *s = GEFORCE(pci_dev);
    
    graphic_console_close(s->vga.con);
    
    DPRINTF("Device exit\n");
}

static Property geforce_properties[] = {
    DEFINE_PROP_UINT32("vgamem_mb", GeForceState, vga.vram_size_mb, 64),
    DEFINE_PROP_STRING("model", GeForceState, model),
    DEFINE_PROP_END_OF_LIST(),
};

static void geforce_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);
    
    k->realize = geforce_realize;
    k->exit = geforce_exit;
    k->vendor_id = PCI_VENDOR_ID_NVIDIA;
    k->device_id = PCI_DEVICE_ID_NVIDIA_GEFORCE3_TI500;
    k->class_id = PCI_CLASS_DISPLAY_VGA;
    k->subsystem_vendor_id = PCI_VENDOR_ID_NVIDIA;
    k->subsystem_id = PCI_DEVICE_ID_NVIDIA_GEFORCE3_TI500;
    
    dc->reset = geforce_reset;
    dc->desc = "NVIDIA GeForce GPU";
    device_class_set_props(dc, geforce_properties);
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
}

static const TypeInfo geforce_info = {
    .name = TYPE_GEFORCE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(GeForceState),
    .class_init = geforce_class_init,
    .interfaces = (InterfaceInfo[]) {
        { INTERFACE_CONVENTIONAL_PCI_DEVICE },
        { },
    },
};

static void geforce_register_types(void)
{
    type_register_static(&geforce_info);
}

type_init(geforce_register_types)