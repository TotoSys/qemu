# GeForce GPU Emulation in QEMU

This implementation adds GeForce3 Ti 500 emulation to QEMU with focus on D3D compatibility for games.

## Features Implemented

### Core GPU Infrastructure
- PCI device emulation following QEMU patterns
- Memory-mapped I/O (MMIO) register access
- RAMIN (instance memory) support for GPU objects
- VGA compatibility through VGACommonState integration
- Interrupt handling for GPU events

### D3D Semaphore Support
The key feature enabling D3D games to function correctly:
- Semaphore object and offset configuration (methods 0x069, 0x75b)
- Semaphore acquire/release operations (method 0x75c)
- Memory-mapped semaphore access for GPU/CPU synchronization
- Support for Kelvin engine (0x97) D3D commands

### GPU Engines Supported
- **Kelvin (0x97)**: D3D/3D graphics engine for Direct3D games
- **SURF2D (0x62)**: 2D surface operations
- **GDI (0x4a)**: Rectangle drawing operations  
- **M2MF (0x39)**: Memory-to-memory copy operations
- **IFC (0x61)**: Image from CPU transfer

### FIFO Command Processing
- Channel-based command routing (32 channels, 8 subchannels each)
- Command queue management with push/pull mechanisms
- Engine dispatch based on object class
- Reference counting for synchronization

## Usage

The GeForce device can be added to a QEMU virtual machine:

```bash
qemu-system-x86_64 -device geforce,vgamem_mb=64
```

## Technical Details

### Memory Layout
- **BAR 0**: MMIO registers (16MB)
- **BAR 1**: Video RAM (configurable, default 64MB)  
- **BAR 2**: RAMIN instance memory (64KB)

### Key Registers
- PMC: Master control and interrupts
- PFIFO: Command FIFO management
- PBUS: Bus interface
- PGRAPH: Graphics engine (future expansion)

### D3D Synchronization
The semaphore mechanism allows D3D applications to:
1. Set up semaphore objects in GPU memory
2. Signal completion of GPU operations
3. Wait for GPU operations to complete
4. Coordinate between CPU and GPU threads

This enables proper D3D game functionality that requires hardware-accelerated graphics synchronization.

## Development Notes

This implementation is based on the GeForce emulation from Vort's Bochs fork, adapted to QEMU's architecture and coding standards. The focus is on providing essential D3D functionality rather than complete hardware emulation.

Future enhancements could include:
- Full 3D pipeline emulation
- Texture management
- Vertex/pixel shader support
- Additional GPU engines
- Performance optimizations