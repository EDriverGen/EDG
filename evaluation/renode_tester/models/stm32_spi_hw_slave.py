# coding: ascii
# stm32_spi_hw_slave.py -- Renode Python peripheral
# Emulates STM32F103 SPI1 controller at register level (CR1/CR2/SR/DR/CRCPR)
# plus an ADXL345/MAX31855/MCP3008-compatible slave behind a single virtual CS.
#
# Design parallels stm32_i2c_hw_slave.py: a `devs` registry parameterized by
# per-vector template injection provides register-map data per chip. The
# controller tracks transaction framing via a sentinel write at offset
# TXN_RESET_OFF from the bare-metal driver.
#
# Registers (STM32F103 SPI1 @ 0x40013000):
#   0x00 CR1    - MSTR/SPE/BR/CPOL/CPHA/DFF/SSM/SSI
#   0x04 CR2
#   0x08 SR     - RXNE/TXE/CHSIDE/UDR/CRCERR/MODF/OVR/BSY
#   0x0C DR     - data register (RX/TX)
#   0x10 CRCPR
#   0x14 RXCRCR
#   0x18 TXCRCR
#   0x1C I2SCFGR
#   0x20 I2SPR
#
# Custom control (outside real hardware, internal to bare-metal hw_spi driver):
#   0x80 TXN_RESET - write any value to signal "start of new SPI frame"
#                    (resets per-transaction state inside the slave device)
#   0x84 DEV_SELECT - write device slot id (0..3) to pick which slave responds
#                     (for multi-chip platforms; defaults to 0)

# SPI1 register offsets
OFF_CR1    = 0x00
OFF_CR2    = 0x04
OFF_SR     = 0x08
OFF_DR     = 0x0C
OFF_CRCPR  = 0x10
OFF_RXCRCR = 0x14
OFF_TXCRCR = 0x18

# Custom control offsets (not on real hardware)
TXN_RESET_OFF   = 0x80
DEV_SELECT_OFF  = 0x84

# CR1 bits
CR1_SPE  = 1 << 6
CR1_MSTR = 1 << 2

# SR bits
SR_RXNE  = 1 << 0
SR_TXE   = 1 << 1
SR_BSY   = 1 << 7


def _adxl345_xfer(dev_regs, phase, byte_val):
    """ADXL345-style register protocol."""
    if phase["stage"] == "cmd":
        phase["read"] = bool(byte_val & 0x80)
        phase["mb"]   = bool(byte_val & 0x40)
        phase["reg"]  = byte_val & 0x3F
        phase["idx"]  = 0
        phase["stage"] = "data"
        return 0x00, phase
    # data stage
    if phase["read"]:
        reg_data = dev_regs.get(phase["reg"], [0]) if isinstance(dev_regs, dict) else []
        if phase["idx"] < len(reg_data):
            reply = reg_data[phase["idx"]] & 0xFF
        else:
            reply = 0x00
        phase["idx"] += 1
        if phase["mb"]:
            # In multi-byte mode the register auto-advances after each byte
            # handled via idx growth within the same reg buffer.
            pass
        return reply, phase
    # write ignored in this minimal model (real chip would persist the value)
    return 0x00, phase


def _stream_xfer(dev_regs, phase, byte_val):
    """Stream-style protocol (MAX31855 / MCP3008)."""
    stream = dev_regs.get(0, []) if isinstance(dev_regs, dict) else []
    idx = phase.get("idx", 0)
    reply = stream[idx] & 0xFF if idx < len(stream) else 0x00
    phase["idx"] = idx + 1
    return reply, phase


def _mcp3008_xfer(dev_regs, phase, byte_val):
    """MCP3008-specific: TX byte 1 selects channel -> response from channels map."""
    idx = phase.get("idx", 0)
    if idx == 0:
        phase["rx_buf"] = [0x00, 0x00, 0x00]
        phase["idx"] = 1
        return 0x00, phase
    if idx == 1:
        channel = (byte_val >> 4) & 0x07
        ch_data = dev_regs.get(channel, [0, 0]) if isinstance(dev_regs, dict) else [0, 0]
        adc_val = ((ch_data[0] & 0xFF) << 8) | (ch_data[1] & 0xFF)
        adc_val &= 0x3FF
        phase["rx_buf"][1] = (adc_val >> 8) & 0x03
        phase["rx_buf"][2] = adc_val & 0xFF
        phase["idx"] = 2
        return phase["rx_buf"][1], phase
    if idx == 2:
        phase["idx"] = 3
        return phase["rx_buf"][2], phase
    return 0x00, phase


# Dispatch table keyed by proto name
_PROTO_HANDLERS = {
    "adxl345": _adxl345_xfer,
    "stream":  _stream_xfer,
    "mcp3008": _mcp3008_xfer,
}


if request.isInit:
    cr1 = 0
    cr2 = 0
    sr  = SR_TXE          # TX empty at boot
    dr_latched = 0        # last byte clocked in from slave (visible via DR read)

    dev_select = 0        # which device slot is currently selected

    # Per-device phase state: {slot: {"stage":..., "reg":..., "idx":..., ...}}
    dev_phase = {0: {"stage": "cmd", "reg": 0, "read": False, "mb": False, "idx": 0}}

    # Device registry -- parameterized by per-vector template injection.
    # Shape: devs[slot] = {"proto": "adxl345"|"stream"|"mcp3008",
    #                     "regs": {reg_addr: [bytes]}}
    # Default (no-op slave) lets the template provide real contents.
    devs = {}
    devs[0] = {"proto": "adxl345", "regs": {}}
    # -- ADXL345 power-on defaults (overridden by vector regs if provided) --
    devs[0]["regs"][0x00] = [0xE5]  # DEVID
    devs[0]["regs"][0x2C] = [0x0A]  # BW_RATE
    devs[0]["regs"][0x2D] = [0x00]  # POWER_CTL
    devs[0]["regs"][0x31] = [0x00]  # DATA_FORMAT

elif request.isRead:
    off = request.offset
    if off == OFF_CR1:
        request.value = cr1
    elif off == OFF_CR2:
        request.value = cr2
    elif off == OFF_SR:
        request.value = sr
    elif off == OFF_DR:
        request.value = dr_latched & 0xFF
        # Reading DR clears RXNE
        sr = sr & ~SR_RXNE
    else:
        request.value = 0

elif request.isWrite:
    off = request.offset
    val = request.value & 0xFFFFFFFF

    if off == OFF_CR1:
        cr1 = val
        if val & CR1_SPE:
            sr = sr | SR_TXE  # peripheral enabled
    elif off == OFF_CR2:
        cr2 = val
    elif off == OFF_SR:
        sr = val & 0xFFFF
    elif off == OFF_DR:
        # Master clocks out one byte; slave simultaneously clocks one back.
        tx_byte = val & 0xFF
        dev = devs.get(dev_select, devs.get(0, {"proto": "adxl345", "regs": {}}))
        proto = dev.get("proto", "adxl345")
        handler = _PROTO_HANDLERS.get(proto, _adxl345_xfer)
        phase = dev_phase.setdefault(dev_select,
                                     {"stage": "cmd", "reg": 0, "read": False,
                                      "mb": False, "idx": 0})
        reply, phase = handler(dev.get("regs", {}), phase, tx_byte)
        dev_phase[dev_select] = phase
        dr_latched = reply & 0xFF
        sr = sr | SR_RXNE | SR_TXE   # data available to read, TX slot open
    elif off == TXN_RESET_OFF:
        # bare-metal driver signals "start of new SPI frame" (CS asserted)
        dev_phase[dev_select] = {"stage": "cmd", "reg": 0, "read": False,
                                 "mb": False, "idx": 0}
    elif off == DEV_SELECT_OFF:
        dev_select = int(val) & 0xFF
        if dev_select not in dev_phase:
            dev_phase[dev_select] = {"stage": "cmd", "reg": 0, "read": False,
                                     "mb": False, "idx": 0}
    # else: ignore writes to CRCPR/I2SCFGR/I2SPR
