# SonicDbg

## Overview

SonicDbg is a hobby debugger for the AArch64 architecture. It supports both position-independent and non-position-independent executables, as well as the following features:

- Setting breakpoints at function symbols / addresses
- Reading/Writing to memory at address
- Reading/Writing to registers
- Continuing execution

### Dependencies
SonicDbg relies on libdwarf to parse the debug information from binaries compiled with the -g flag. SonicDbg also relies on libelf to determine if a binary is position-independent.


### Commands

#### Breakpoints
To set a breakpoint at a function symbol:  
`<sonicdbg> b main`

To set a breakpoint at an address:  
`<sonicdbg> b *0xAAAAFF30`

#### Register Read/Write
To write a register:  
`<sonicdbg> reg write pc`

To read from a register:  
`<sonicdbg> reg read pc`

#### Memory Read/Write
To read from a memory address:  
`<sonicdbg> mem read 0xAAAAFF30`

To write to a memory address:  
`<sonicdbg> mem write 0xAAAAFF30`

#### Single Step
To step over a single instruction:  
`<sonicdbg> si`

#### Continue
To continue execution:  
`<sonicdbg> continue`


### Notes
SonicDbg has not been thoroughly tested. It has only been tested on AArch64 targets--further development is needed to support x86.