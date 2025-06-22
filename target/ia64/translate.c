/*
 * IA-64 virtual memory translation
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
#include "exec/translation-block.h"

/* Placeholder TCG translation - minimal implementation */
void gen_intermediate_code(CPUState *cs, TranslationBlock *tb, int *max_insns,
                          vaddr pc, void *host_pc)
{
    /* Minimal placeholder - just set the tb to end immediately */
    tb->size = 16;  /* IA-64 bundle size */
    tb->icount = 1;
    *max_insns = 1;
}