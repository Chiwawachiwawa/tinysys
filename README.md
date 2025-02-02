# tinysys

![tinysys](./tinysys.jpg "tinysys")

# What is it?

Tinysys started out as a hobby project. It now has two RISC-V cores, and several other facilities listed below, and can happily run most software with minimal tweaks.

Of course, before you ask, it does run DOOM, and with sound and keyboard input!

# System specifications

- 2x RISC-V based CPUs (architecture: rv32im_zicsr_zifencei_zfinx)
- 150MHz bus and CPU clock, however we're not pipelined yet (TBD)
- Supports instruction fence and data cache flush/invalidate operations
- Single precision FPU (no float control CRSs)
- Float and integer GPRs share the same register space (zfinx extension)
- 32 GRPs x 32 bits
- 4096 CSRs x 32 bits (some registers reserved for CPU and some are immutable)
- DMA with optional zero-masking for 16 byte aligned memory copies, coherency via CPU instructions
- Integer multiply / divide
- Software, hardware and timer interrupts
- 16Kbytes of direct mapped instruction cache (256 lines x 64 bytes)
- 32Kbytes of direct mapped data cache (512 lines x 64 bytes)
- 128 bit AXI4 bus with 32 bit address line
- Memory arbiter for on-board devices
- Memory mapped external hardware
- USB host controller chip for HID access (WiP, some keyboards and mice work)
- 16 bit stereo audio output chip (24 bit native in reality, might extend later)
- DVI 1.0 compatible video output via external chip (12 bits per pixel, RGB or paletted modes)
- SDCard for file I/O
- 4 debug LEDs (also shared by OS as indicators)
- Custom preemptive multitasking OS, with file I/O and basic memory allocator via syscalls
- Optionally, a rom.bin image can be loaded from SDCard to replace the OS in ROM
- ESP32C6 on board for I/O handling (UART serial for CLI)
- The ESP chip also gives USB serial access and BLE serial terminal access to the board

# Overview of the processing units

## CPU
Based on 32 bit RISC-V ISA. Implements base integer instruction set, required cache operations (I$ and D$) and a large CSR file.
The core currently has an average instruction retirement rate of 6 CPI (clocks per instruction) and runs at 150.000MHZ, which
is also the speed of the AXI4 bus, where all peripherals and memory reside.

### Fetch/Decode/IRQ
This unit reads an instruction at PC, decodes it and outputs it (together with its PC) to instruction output FIFO (IFIFO). If it's an interrupt entry/exit or some other special instruction (for instance I$ flush) then it is handled entirely within the fetch unit. This unit is also responsible for inserting pre/post interrupt code from an internal ROM at interrupt or exception time. Conditional branches will place fetch unit into an idle state, where it will wait for the execute unit to resolve the target branch address. This means there's no branch prediction on the CPU just yet.

### Execute/Load/Store
This unit will read an instruction from the IFIFO if available, load registers with values, execute (ALU/BLU/CSR/SYS) and decide on new branch target if there's a branch involved. After deciding on the branch address, fetch unit is notified so it can resume instruction fetches. Where possible, load or store operations will attempt to overlap with fetch and execution.

## VPU
Video processing unit. Handles scan-out of various video sizes (320x240 and 640x480) and bit depths (8bpp index color or 16bpp RGB color)

## APU
Audio processing unit. Handles RAW audio outputs, and also manages 44/22/11KHz stereo playback and double-buffer handling of RAW audio.

## DMA
Direct memory access unit. Used to copy blocks of memory within memory address space, and won't DMA between or from other devices. It can optionally ignore zeros on input data, and won't write them to the output location, therefore it can be used to overlay UI onto other graphics by automatically masking transparent parts. It will be expanded to support misaligned DMA in the future.

# Overview of the bus

## AXI4 bus
The AXI4 bus, running at 150.000MHz, connects all of the processing units to memory or devices. In the case of memory, access is cached per perhipheral as needed. Memory mapped device access is always uncached.

# Custom instructions

## Convert from float to 4 bit integer, saturated (FCVTSWU5SAT)

This instuction has been contributed by Wade Brainerd. It's very useful in converting floating point values to device specific 4 bit color values.

The following python script helps encode a hex representation of this instruction, for convenience:

```
op = 0b1010011		# OP-FP (floating point operation)
rd = 0b01010		# destination register - a0 in this sample (a0 == x10)
rs1 = 0b01011		# source register - a1 in this sample (a1 == x11)
funct7 = 0b1100001	# funct5 F7_FCVTSWU5SAT - this is our new sub-instruction
inst = op | (rd << 7) | (rs1 << 15) | (funct7 << 25)

print(f"{inst:x}")
```

You can then use it in code as follows:

```
inline uint32_t ftoui4sat(float value)
{
  uint32_t retval;
  asm (
    "mv a1, %1;"
    ".word 0xc2058553;" // fcvtswu4sat a0, a1 // note A0==cpu.x10, A1==cpu.x11
    "mv %0, a0; "
    : "=r" (retval)
    : "r" (value)
    : "a0", "a1"
  );
  return retval;
}
```

P.S. For the future we might be able to do this natively on a RISC-V or propose an extension for arbitrary saturation for custom formats.

# Software

Please see [README.md](./software/README.md) for details on software samples, ROM and prerequisites for building ELF files.
