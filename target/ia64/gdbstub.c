/*
 * IA-64 gdbstub
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
#include "exec/gdbstub.h"

int ia64_cpu_gdb_read_register(CPUState *cs, GByteArray *mem_buf, int n)
{
    IA64CPUState *env = cpu_env(cs);

    if (n < 128) {
        /* General registers */
        return gdb_get_reg64(mem_buf, env->gr[n]);
    } else if (n < 136) {
        /* Branch registers */
        return gdb_get_reg64(mem_buf, env->br[n - 128]);
    } else if (n < 144) {
        /* Floating-point registers (subset) */
        return gdb_get_reg64(mem_buf, env->fr[n - 136]);
    }

    return 0;
}

int ia64_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    IA64CPUState *env = cpu_env(cs);
    uint64_t tmp = ldq_p(mem_buf);

    if (n < 128) {
        /* General registers */
        env->gr[n] = tmp;
        return 8;
    } else if (n < 136) {
        /* Branch registers */
        env->br[n - 128] = tmp;
        return 8;
    } else if (n < 144) {
        /* Floating-point registers (subset) */
        env->fr[n - 136] = tmp;
        return 8;
    }

    return 0;
}