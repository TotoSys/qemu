/*
 * IA-64 emulation cpu definitions for qemu.
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

#ifndef IA64_CPU_H
#define IA64_CPU_H

#include "cpu-qom.h"
#include "exec/cpu-common.h"
#include "exec/cpu-defs.h"
#include "exec/cpu-interrupt.h"
#include "system/memory.h"
#include "qemu/cpu-float.h"

#define NB_MMU_MODES 8

/* IA-64 Processor State */
struct IA64CPUState {
    uint64_t gr[128];   /* General registers */
    uint64_t fr[128];   /* Floating-point registers */
    uint64_t br[8];     /* Branch registers */

    /* Application registers */
    uint64_t ar_k[8];   /* Kernel registers */
    uint64_t ar_rsc;    /* Register Stack Configuration */
    uint64_t ar_bsp;    /* Backing Store Pointer */
    uint64_t ar_bspstore; /* Backing Store Pointer for memory stores */
    uint64_t ar_rnat;   /* Register Stack NaT Collection */
    uint64_t ar_ccv;    /* Compare and Exchange Compare Value */
    uint64_t ar_unat;   /* User NaT Collection */
    uint64_t ar_fpsr;   /* Floating-Point Status Register */
    uint64_t ar_itc;    /* Interval Timer Counter */
    uint64_t ar_pfs;    /* Previous Function State */
    uint64_t ar_lc;     /* Loop Count */
    uint64_t ar_ec;     /* Epilog Count */

    /* Control registers */
    uint64_t cr_dcr;    /* Default Control Register */
    uint64_t cr_itm;    /* Interval Timer Match */
    uint64_t cr_iva;    /* Interruption Vector Address */
    uint64_t cr_pta;    /* Page Table Address */
    uint64_t cr_ipsr;   /* Interruption PSR */
    uint64_t cr_isr;    /* Interruption Status Register */
    uint64_t cr_iip;    /* Interruption Instruction Pointer */
    uint64_t cr_ifa;    /* Interruption Faulting Address */
    uint64_t cr_itir;   /* Interruption TLB Insertion Register */
    uint64_t cr_iipa;   /* Interruption Instruction Previous Address */
    uint64_t cr_ifs;    /* Interruption Function State */
    uint64_t cr_iim;    /* Interruption Immediate */
    uint64_t cr_iha;    /* Interruption Hash Address */

    /* Processor Status Register */
    uint64_t psr;       

    /* Current instruction pointer and next instruction pointer */
    uint64_t ip;        /* Instruction pointer */
    uint64_t cfm;       /* Current Frame Marker */

    /* Predicate registers */
    uint64_t pr;        /* Predicate registers (64 bits) */

    /* Memory management */
    uint64_t rr[8];     /* Region registers */

    /* NaT bits for general registers */
    uint64_t nat_gr_low;  /* NaT bits for gr0-gr63 */
    uint64_t nat_gr_high; /* NaT bits for gr64-gr127 */

    /* CPU identification */
    uint64_t cpuid[5];

    /* PSR fields access */
    uint32_t interrupt_request;
};

typedef struct IA64CPUState IA64CPUState;
typedef struct IA64CPU IA64CPU;

struct ArchCPU {
    CPUState parent_obj;
    
    IA64CPUState env;
};

typedef struct IA64CPUClass IA64CPUClass;

struct IA64CPUClass {
    CPUClass parent_class;

    DeviceRealize parent_realize;
    ResettablePhases parent_phases;
};

static inline IA64CPUState *cpu_env(CPUState *cs)
{
    return &(IA64_CPU(cs)->env);
}

#include "exec/cpu-all.h"

/* IA-64 processor models */
typedef enum {
    IA64_CPU_TYPE_GENERIC,
    IA64_CPU_TYPE_ITANIUM,   /* Original Itanium (Merced) */
    IA64_CPU_TYPE_ITANIUM2,  /* Itanium 2 (McKinley/Madison) */
} IA64CPUType;

/* PSR bit definitions */
#define PSR_BE    (1ULL << 1)   /* Big Endian */
#define PSR_UP    (1ULL << 2)   /* User Performance monitor access */
#define PSR_AC    (1ULL << 3)   /* Alignment Check */
#define PSR_MFL   (1ULL << 4)   /* Lower floating-point */
#define PSR_MFH   (1ULL << 5)   /* Upper floating-point */
#define PSR_IC    (1ULL << 13)  /* Interruption Collection */
#define PSR_I     (1ULL << 14)  /* Interrupt enable */
#define PSR_PK    (1ULL << 15)  /* Protection Key enable */
#define PSR_DT    (1ULL << 17)  /* Data Translation */
#define PSR_DFL   (1ULL << 18)  /* Disabled FP Low */
#define PSR_DFH   (1ULL << 19)  /* Disabled FP High */
#define PSR_SP    (1ULL << 20)  /* Secure Performance monitors */
#define PSR_PP    (1ULL << 21)  /* Privileged Performance monitor */
#define PSR_DI    (1ULL << 22)  /* Disable Instruction set transition */
#define PSR_SI    (1ULL << 23)  /* Secure Interval timer */
#define PSR_DB    (1ULL << 24)  /* Debug Breakpoint fault */
#define PSR_LP    (1ULL << 25)  /* Lower Privilege transfer trap */
#define PSR_TB    (1ULL << 26)  /* Taken Branch trap */
#define PSR_RT    (1ULL << 27)  /* Register stack translation */
#define PSR_IS    (1ULL << 34)  /* Instruction Set */
#define PSR_IT    (1ULL << 36)  /* Instruction Translation */
#define PSR_ME    (1ULL << 37)  /* Machine Check abort mask */
#define PSR_BN    (1ULL << 44)  /* Register Bank */

/* CPU feature flags */
#define IA64_FEATURE_BREAK_INST  (1 << 0)  /* Break instruction */
#define IA64_FEATURE_SAT         (1 << 1)  /* Software Assist Trap */

void ia64_cpu_dump_state(CPUState *cs, FILE *f, int flags);
void ia64_cpu_do_interrupt(CPUState *cs);
bool ia64_cpu_exec_interrupt(CPUState *cs, int interrupt_request);
hwaddr ia64_cpu_get_phys_page_debug(CPUState *cs, vaddr addr);

#endif /* IA64_CPU_H */