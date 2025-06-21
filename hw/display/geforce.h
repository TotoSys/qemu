/*
 * QEMU GeForce GPU emulation
 *
 * Copyright (c) 2025 QEMU Project
 *
 * Based on GeForce emulation from Vort's Bochs fork
 * Copyright (C) 2025 The Bochs Project
 *
 * This header defines the GeForce GPU device structure and constants for:
 * - GeForce3 Ti 500 PCI device emulation
 * - D3D semaphore operations for Kelvin (0x97) engine
 * - FIFO command processing and channel management
 * - Memory-mapped I/O register definitions
 *
 * The implementation focuses on enabling D3D functionality for games by
 * providing essential graphics engine emulation and synchronization primitives.
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

#ifndef HW_DISPLAY_GEFORCE_H
#define HW_DISPLAY_GEFORCE_H

#include "hw/pci/pci_device.h"
#include "vga_int.h"
#include "qom/object.h"

#define TYPE_GEFORCE "geforce"
OBJECT_DECLARE_SIMPLE_TYPE(GeForceState, GEFORCE)

/* NVIDIA PCI IDs */
#define PCI_VENDOR_ID_NVIDIA 0x10de
#define PCI_DEVICE_ID_NVIDIA_GEFORCE3_TI500 0x0201

/* CRTC register range */
#define VGA_CRTC_MAX 0x18
#define GEFORCE_CRTC_MAX 0x9F

/* Channel and cache constants */
#define GEFORCE_CHANNEL_COUNT 32
#define GEFORCE_SUBCHANNEL_COUNT 8
#define GEFORCE_CACHE1_SIZE 64

/* Memory mapping constants */
#define GEFORCE_MMIO_SIZE 0x1000000

/* D3D method constants for Kelvin (0x97) */
#define NV_D3D_SEMAPHORE_OBJECT      0x069
#define NV_D3D_CLIP_HORIZONTAL       0x080
#define NV_D3D_CLIP_VERTICAL         0x081
#define NV_D3D_SURFACE_FORMAT        0x082
#define NV_D3D_SURFACE_PITCH         0x083
#define NV_D3D_SURFACE_COLOR_OFFSET  0x084
#define NV_D3D_SEMAPHORE_OFFSET      0x75b
#define NV_D3D_SEMAPHORE_ACQUIRE     0x75c
#define NV_D3D_COLOR_CLEAR_VALUE     0x764
#define NV_D3D_CLEAR_SURFACE         0x765

/* Channel state for D3D operations */
typedef struct GeForceChannelState {
    /* Subchannel engines */
    uint32_t engine[GEFORCE_SUBCHANNEL_COUNT];
    
    /* D3D state */
    uint32_t d3d_semaphore_obj;
    uint32_t d3d_semaphore_offset;
    uint32_t d3d_clip_horizontal;
    uint32_t d3d_clip_vertical;
    uint32_t d3d_surface_format;
    uint32_t d3d_surface_pitch;
    uint32_t d3d_surface_color_offset;
    uint32_t d3d_color_clear_value;
    uint32_t d3d_clear_surface;
    uint32_t d3d_color_bytes;
} GeForceChannelState;

/* GeForce registers structure */
typedef struct GeForceRegs {
    uint8_t crtc_index;
    uint8_t crtc_regs[GEFORCE_CRTC_MAX + 1];
    
    /* Interrupt handling */
    uint32_t mc_intr_en;
    uint32_t mc_enable;
    uint32_t bus_intr;
    uint32_t bus_intr_en;
    
    /* FIFO engine */
    uint32_t fifo_intr;
    uint32_t fifo_intr_en;
    uint32_t fifo_ramht;
    uint32_t fifo_ramfc;
    uint32_t fifo_ramro;
    uint32_t fifo_mode;
    uint32_t fifo_cache1_push1;
    uint32_t fifo_cache1_put;
    uint32_t fifo_cache1_dma_push;
    uint32_t fifo_cache1_dma_instance;
    uint32_t fifo_cache1_dma_put;
    uint32_t fifo_cache1_dma_get;
    uint32_t fifo_cache1_ref_cnt;
    uint32_t fifo_cache1_pull0;
    uint32_t fifo_cache1_semaphore;
    
    /* Channel states */
    GeForceChannelState channels[GEFORCE_CHANNEL_COUNT];
} GeForceRegs;

/* Main GeForce device state */
struct GeForceState {
    PCIDevice parent_obj;
    
    VGACommonState vga;
    GeForceRegs regs;
    
    /* Memory regions */
    MemoryRegion mmio;
    MemoryRegion vram;
    
    /* Device model */
    char *model;
    uint16_t device_id;
    
    /* IRQ line */
    qemu_irq irq;
};

/* Function declarations */
void geforce_init_registers(GeForceState *s);
uint64_t geforce_mmio_read(void *opaque, hwaddr addr, unsigned size);
void geforce_mmio_write(void *opaque, hwaddr addr, uint64_t val, unsigned size);
bool geforce_execute_d3d_command(GeForceState *s, uint32_t chid, uint32_t method, uint32_t param);
void geforce_d3d_clear_surface(GeForceState *s, uint32_t chid);

#endif /* HW_DISPLAY_GEFORCE_H */