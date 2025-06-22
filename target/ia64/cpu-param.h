/*
 * IA-64 CPU param definitions for qemu.
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

#ifndef IA64_CPU_PARAM_H
#define IA64_CPU_PARAM_H

#define TARGET_LONG_BITS 64
#define TARGET_PAGE_BITS 14  /* 16KB pages typical for IA-64 */
#define TARGET_PHYS_ADDR_SPACE_BITS 64
#define TARGET_VIRT_ADDR_SPACE_BITS 64

/* IA-64 has variable-length instructions (bundles of 3 instructions) */
#define NB_MMU_MODES 8

#endif /* IA64_CPU_PARAM_H */