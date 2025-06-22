/*
 * IA-64 machine state for migration
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
#include "cpu.h"
#include "migration/cpu.h"

static const VMStateDescription vmstate_ia64_cpu = {
    .name = "cpu",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_UINT64_ARRAY(env.gr, IA64CPU, 128),
        VMSTATE_UINT64_ARRAY(env.fr, IA64CPU, 128),
        VMSTATE_UINT64_ARRAY(env.br, IA64CPU, 8),
        VMSTATE_UINT64(env.psr, IA64CPU),
        VMSTATE_UINT64(env.ip, IA64CPU),
        VMSTATE_UINT64(env.cfm, IA64CPU),
        VMSTATE_UINT64(env.pr, IA64CPU),
        VMSTATE_UINT64_ARRAY(env.rr, IA64CPU, 8),
        VMSTATE_UINT64(env.ar_bsp, IA64CPU),
        VMSTATE_UINT64(env.ar_rsc, IA64CPU),
        VMSTATE_UINT64(env.ar_pfs, IA64CPU),
        VMSTATE_END_OF_LIST()
    }
};