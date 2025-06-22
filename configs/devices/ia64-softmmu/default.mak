# Default configuration for ia64-softmmu

# Enable IA-64 machine
CONFIG_IA64_MACHINE=y

# Enable standard PC devices that we want
CONFIG_SERIAL=y
CONFIG_SERIAL_ISA=y
CONFIG_I8254=y
CONFIG_FDC_ISA=y
CONFIG_MC146818RTC=y
CONFIG_ISA_BUS=y

# PCI support
CONFIG_PCI=y

# VGA
CONFIG_VGA_PCI=y
CONFIG_VGA=y

# Networking
CONFIG_E1000_PCI=y