/*
 * IA-64 CPU QOM definitions for qemu.
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

#ifndef QEMU_IA64_CPU_QOM_H
#define QEMU_IA64_CPU_QOM_H

#include "hw/core/cpu.h"

#define TYPE_IA64_CPU "ia64-cpu"

OBJECT_DECLARE_CPU_TYPE(IA64CPU, IA64CPUClass, IA64_CPU)

#define TYPE_IA64_CPU_GENERIC  TYPE_IA64_CPU "-generic"
#define TYPE_ITANIUM_CPU       TYPE_IA64_CPU "-itanium"
#define TYPE_ITANIUM2_CPU      TYPE_IA64_CPU "-itanium2"

#endif /* QEMU_IA64_CPU_QOM_H */