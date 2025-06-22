ia64_ss = ss.source_set()
ia64_ss.add(when: 'CONFIG_IA64_MACHINE', if_true: files(
  'machine.c',
  'i82460gx.c',
))

hw_arch += {'ia64': ia64_ss}