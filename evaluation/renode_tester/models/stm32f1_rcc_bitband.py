# STM32F1 RCC Bit-Band Alias Handler for Renode
# Maps bit-band writes (0x42420000) to the actual RCC registers (0x40021000)
#
# Cortex-M3 bit-band: each bit of peripheral region (0x40000000)
# gets a word-aligned alias at 0x42000000.
# Formula: alias = 0x42000000 + (byte_offset * 32) + (bit_number * 4)
# Reverse: byte_offset = (alias - 0x42000000) / 32
#          bit_number  = ((alias - 0x42000000) % 32) / 4

if request.isInit:
    pass
elif request.isWrite:
    # Calculate which peripheral byte & bit this bit-band write targets
    alias_offset = request.offset  # offset within this peripheral (0x42420000 base)
    byte_off = alias_offset // 32   # byte offset in RCC register space (from 0x40021000)
    bit_num  = (alias_offset % 32) // 4  # which bit

    # Read current RCC register value (word-aligned)
    reg_off = byte_off & ~3  # align to 4-byte register
    bit_in_reg = (byte_off & 3) * 8 + bit_num  # bit position in 32-bit register

    # Get current value from RCC via sysbus
    import clr
    rcc_base = 0x40021000
    current = self.GetMachine().SystemBus.ReadDoubleWord(rcc_base + reg_off)

    if request.value & 1:
        current |= (1 << bit_in_reg)
    else:
        current &= ~(1 << bit_in_reg)

    self.GetMachine().SystemBus.WriteDoubleWord(rcc_base + reg_off, current)

elif request.isRead:
    alias_offset = request.offset
    byte_off = alias_offset // 32
    bit_num  = (alias_offset % 32) // 4
    reg_off = byte_off & ~3
    bit_in_reg = (byte_off & 3) * 8 + bit_num

    rcc_base = 0x40021000
    current = self.GetMachine().SystemBus.ReadDoubleWord(rcc_base + reg_off)
    request.value = 1 if (current & (1 << bit_in_reg)) else 0
