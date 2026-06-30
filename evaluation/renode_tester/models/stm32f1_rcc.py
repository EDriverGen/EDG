# STM32F1 RCC (Reset & Clock Control) simplified model for Renode
# Handles auto-setting of ready bits when clock sources are enabled.
#
# Register layout (offset from 0x40021000):
#   0x00 CR     - Clock Control Register
#   0x04 CFGR   - Clock Configuration Register
#   0x08 CIR    - Clock Interrupt Register
#   0x0C APB2RSTR
#   0x10 APB1RSTR
#   0x14 AHBENR
#   0x18 APB2ENR
#   0x1C APB1ENR
#   0x20 BDCR
#   0x24 CSR

if request.isInit:
    # Register file: dict offset -> value
    regs = {}
    # CR default: HSION=1, HSIRDY=1, HSI trim=0x80
    regs[0x00] = 0x00000083
    regs[0x04] = 0x00000000  # CFGR
    regs[0x08] = 0x00000000  # CIR
    regs[0x0C] = 0x00000000
    regs[0x10] = 0x00000000
    regs[0x14] = 0x00000014  # AHBENR: SRAM + FLITF enabled
    regs[0x18] = 0x00000000
    regs[0x1C] = 0x00000000
    regs[0x20] = 0x00000000
    regs[0x24] = 0x0C000000  # CSR: LSIRDY + LSION

elif request.isRead:
    offset = request.offset
    # Align to 4-byte boundary
    aligned = offset & ~3
    val = regs.get(aligned, 0)

    # For CR register, auto-set ready bits based on enable bits
    if aligned == 0x00:
        # If HSEON (bit 16) is set, set HSERDY (bit 17)
        if val & (1 << 16):
            val |= (1 << 17)
        else:
            val &= ~(1 << 17)
        # If PLLON (bit 24) is set, set PLLRDY (bit 25)
        if val & (1 << 24):
            val |= (1 << 25)
        else:
            val &= ~(1 << 25)
        # HSIRDY always set if HSION
        if val & (1 << 0):
            val |= (1 << 1)

    # For CFGR, reflect SWS = SW (bits 3:2 = bits 1:0)
    if aligned == 0x04:
        sw = val & 0x03
        val = (val & ~0x0C) | (sw << 2)

    # For CSR (0x24), auto-set LSIRDY (bit 1) if LSION (bit 0)
    if aligned == 0x24:
        if val & (1 << 0):
            val |= (1 << 1)

    # For BDCR (0x20), auto-set LSERDY (bit 1) if LSEON (bit 0)
    if aligned == 0x20:
        if val & (1 << 0):
            val |= (1 << 1)

    request.value = val

elif request.isWrite:
    offset = request.offset
    aligned = offset & ~3
    regs[aligned] = request.value
