/*
 * QEMU IA-64 CPU
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
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/lgpl-2.1.html>
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/qemu-print.h"
#include "qemu/timer.h"
#include "cpu.h"
#include "qemu/module.h"
#include "exec/translation-block.h"
#include "exec/target_page.h"
#include "fpu/softfloat.h"
#include "tcg/tcg.h"

static void ia64_cpu_set_pc(CPUState *cs, vaddr value)
{
    IA64CPU *cpu = IA64_CPU(cs);
    
    cpu->env.ip = value;
}

static vaddr ia64_cpu_get_pc(CPUState *cs)
{
    IA64CPUState *env = cpu_env(cs);
    
    return env->ip;
}

static void ia64_cpu_synchronize_from_tb(CPUState *cs, TranslationBlock *tb)
{
    IA64CPU *cpu = IA64_CPU(cs);
    
    cpu->env.ip = tb->pc;
}

static void ia64_restore_state_to_opc(CPUState *cs, TranslationBlock *tb,
                                      target_ulong *data)
{
    IA64CPU *cpu = IA64_CPU(cs);
    
    cpu->env.ip = data[0];
}

static bool ia64_cpu_has_work(CPUState *cs)
{
    return cs->interrupt_request & CPU_INTERRUPT_HARD;
}

void ia64_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    IA64CPUState *env = cpu_env(cs);
    int i;

    qemu_fprintf(f, "IP=%016" PRIx64 " PSR=%016" PRIx64 " CFM=%016" PRIx64 "\n",
                 env->ip, env->psr, env->cfm);
    qemu_fprintf(f, "PR=%016" PRIx64 "\n", env->pr);

    for (i = 0; i < 8; i++) {
        qemu_fprintf(f, "BR%d=%016" PRIx64 "%s", i, env->br[i],
                     (i & 3) == 3 ? "\n" : " ");
    }

    for (i = 0; i < 128; i++) {
        qemu_fprintf(f, "GR%02d=%016" PRIx64 "%s", i, env->gr[i],
                     (i & 3) == 3 ? "\n" : " ");
    }
}

static void ia64_cpu_reset_hold(Object *obj, ResetType type)
{
    CPUState *cs = CPU(obj);
    IA64CPU *cpu = IA64_CPU(cs);
    IA64CPUState *env = &cpu->env;
    
    memset(env, 0, sizeof(IA64CPUState));
    
    /* Initialize to reasonable defaults */
    env->psr = PSR_IC | PSR_I | PSR_DT | PSR_IT;  /* Enable basic functionality */
    env->ip = 0;
    env->cfm = 0;
    
    /* Initialize CPUID information for generic Itanium */
    env->cpuid[0] = 0x49656E74656C6950ULL;  /* "IntelIP" */
    env->cpuid[1] = 0x6974616E69756D20ULL;  /* "itanium " */
    env->cpuid[2] = 0x0;
    env->cpuid[3] = 0x1;  /* Family 31, Model 0, Revision 1 */
    env->cpuid[4] = 0x0;
}

static void ia64_cpu_disas_set_info(CPUState *cs, disassemble_info *info)
{
    info->mach = bfd_mach_ia64_elf64;
    /* IA-64 disassembly not implemented yet */
    info->print_insn = NULL;
}

static void ia64_cpu_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
    IA64CPU *cpu = IA64_CPU(dev);
    IA64CPUClass *iacc = IA64_CPU_GET_CLASS(dev);
    Error *local_err = NULL;

    cpu_exec_realizefn(cs, &local_err);
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }

    qemu_init_vcpu(cs);
    cpu_reset(cs);

    iacc->parent_realize(dev, errp);
}

static void ia64_cpu_initfn(Object *obj)
{
    IA64CPU *cpu = IA64_CPU(obj);
    
    cpu_set_cpustate_pointers(cpu);
}

static ObjectClass *ia64_cpu_class_by_name(const char *cpu_model)
{
    ObjectClass *oc;
    char *typename;

    typename = g_strdup_printf("%s-" TYPE_IA64_CPU, cpu_model);
    oc = object_class_by_name(typename);
    g_free(typename);
    
    if (oc != NULL && (!object_class_dynamic_cast(oc, TYPE_IA64_CPU) ||
                       object_class_is_abstract(oc))) {
        oc = NULL;
    }
    
    return oc;
}

static void ia64_cpu_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    IA64CPUClass *iacc = IA64_CPU_CLASS(oc);
    ResettableClass *rc = RESETTABLE_CLASS(oc);

    device_class_set_parent_realize(dc, ia64_cpu_realizefn,
                                    &iacc->parent_realize);

    resettable_class_set_parent_phases(rc, NULL, ia64_cpu_reset_hold, NULL,
                                       &iacc->parent_phases);

    cc->class_by_name = ia64_cpu_class_by_name;
    cc->has_work = ia64_cpu_has_work;
    cc->dump_state = ia64_cpu_dump_state;
    cc->set_pc = ia64_cpu_set_pc;
    cc->get_pc = ia64_cpu_get_pc;
    cc->synchronize_from_tb = ia64_cpu_synchronize_from_tb;
    cc->restore_state_to_opc = ia64_restore_state_to_opc;
    cc->do_interrupt = ia64_cpu_do_interrupt;
    cc->cpu_exec_interrupt = ia64_cpu_exec_interrupt;
    cc->get_phys_page_debug = ia64_cpu_get_phys_page_debug;

    cc->gdb_num_core_regs = 128 + 8 + 8; /* GR + BR + FR subset */
    cc->gdb_core_xml_file = "ia64-core.xml";

    dc->user_creatable = true;
}

/* CPU model definitions */
static void ia64_cpu_generic_initfn(Object *obj)
{
    IA64CPU *cpu = IA64_CPU(obj);
    IA64CPUState *env = &cpu->env;
    
    /* Set generic Itanium CPUID */
    env->cpuid[0] = 0x49656E74656C6950ULL;  /* "IntelIP" */
    env->cpuid[1] = 0x6974616E69756D20ULL;  /* "itanium " */
    env->cpuid[3] = 0x1F00;  /* Family 31 (Itanium), Model 0 */
}

static void ia64_cpu_itanium_initfn(Object *obj)
{
    IA64CPU *cpu = IA64_CPU(obj);
    IA64CPUState *env = &cpu->env;
    
    /* Itanium (Merced) specific CPUID */
    env->cpuid[0] = 0x49656E74656C6950ULL;  /* "IntelIP" */
    env->cpuid[1] = 0x4974616E69756D31ULL;  /* "Itanium1" */
    env->cpuid[3] = 0x1F00;  /* Family 31, Model 0 */
}

static void ia64_cpu_itanium2_initfn(Object *obj)
{
    IA64CPU *cpu = IA64_CPU(obj);
    IA64CPUState *env = &cpu->env;
    
    /* Itanium 2 (McKinley) specific CPUID */
    env->cpuid[0] = 0x49656E74656C6950ULL;  /* "IntelIP" */
    env->cpuid[1] = 0x4974616E69756D32ULL;  /* "Itanium2" */
    env->cpuid[3] = 0x1F01;  /* Family 31, Model 1 */
}

static const TypeInfo ia64_cpu_type_infos[] = {
    {
        .name = TYPE_IA64_CPU,
        .parent = TYPE_CPU,
        .instance_size = sizeof(IA64CPU),
        .instance_init = ia64_cpu_initfn,
        .abstract = true,
        .class_size = sizeof(IA64CPUClass),
        .class_init = ia64_cpu_class_init,
    },
    {
        .name = TYPE_IA64_CPU_GENERIC,
        .parent = TYPE_IA64_CPU,
        .instance_init = ia64_cpu_generic_initfn,
    },
    {
        .name = TYPE_ITANIUM_CPU,
        .parent = TYPE_IA64_CPU,
        .instance_init = ia64_cpu_itanium_initfn,
    },
    {
        .name = TYPE_ITANIUM2_CPU,
        .parent = TYPE_IA64_CPU,
        .instance_init = ia64_cpu_itanium2_initfn,
    }
};

DEFINE_TYPES(ia64_cpu_type_infos)