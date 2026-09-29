# The 80960CA in MAME's i960 core

MAME's i960 core emulated the 80960KB only. `patches/i960-ca.patch` adds the 80960CA
the ViewStation uses and fixes five bugs of the shared core that this machine ran into.
The K series behaviour is unchanged unless noted.

## 80960CA support

- **Initialization boot record** at 0xFFFFFF00, and the CA layout of the processor
  control block: fault table at +0, control table at +4, arithmetic controls at +8,
  interrupt table at +16, system procedure table at +20, interrupt stack at +28.
- **Special function registers** sf0 (IPND), sf1 (IMSK) and sf2 (DMAC). DMAC bit 31
  reads back as 0: software sets it and the DMA unit clears it once the change is taken.
- **Dedicated mode interrupts**: IMAP0 to IMAP2 and ICON from the control table
  (control registers 4 to 7), level or edge per pin, priorities, and IPND/IMSK.
- **sysctl** types 0 to 4 (request interrupt, invalidate and configure the instruction
  cache, reinitialize, load control registers).
- **calls** into supervisor procedures, with the switch to the supervisor stack, and
  return types 1, 2, 3 and 7.
- New instructions: `modify`, `extract`, `modtc`, `mark`, `fmark`, `syncf`, and `sdma`
  and `udma`, which are logged only: the CA DMA unit is not emulated, and nothing the
  machine runs so far needs it.
- Debugger state entries for IPND, IMSK, DMAC, IMAP0 to IMAP2 and ICON.

## Bugs fixed

1. **`addc` never produced a carry.** The sum was computed in 32 bits:

       res = t2+(t1+((m_AC>>1)&1));

   with `t1` and `t2` 32-bit, so bit 32 was never set. The i960CA has no floating point
   unit; its software floating point adds 64-bit quantities with `addc`, so every
   double precision result could be wrong. It showed as the X server scaling bitmap
   fonts badly: the icon labels and Netscape's license text lost the top rows of every
   glyph, because the scaled ascent came out short.
2. **`bbc` set the condition code the wrong way round.** It set "true" when it branched
   (bit clear) and "false" when it did not.
3. **Multi-word loads and stores** (`ldl`, `ldt`, `ldq`, `stl`, `stt`, `stq`) only moved
   to the next word when the memory region was marked for burst access. On the CA they
   always do.
4. **Branch prediction bit.** Bit 1 of CTRL and COBR instructions is a hint on the CA;
   it was taken as part of the displacement.
5. **Stack frame alignment** is 16 bytes on the C series, not 64 as on the K series.
