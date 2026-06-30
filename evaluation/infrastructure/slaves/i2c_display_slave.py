# coding: ascii
# i2c_display_slave.py -- Renode Python peripheral for eval_class="display".
# Models STM32 I2C1 register-level controller PLUS a generic I2C display
# device using SSD1306-style control-byte framing:
#
#   <addr+W> <Co|D/C|0|0|0|0|0|0> <payload> [ <Co|D/C|000000> <payload> ... ] <STOP>
#
# Semantics of the control byte:
#   Co  (bit 7) : 0 = "stream"    following bytes stay in current mode until STOP
#                 1 = "single"    the ONE byte after this control byte is in
#                                 that mode, then a new control byte is
#                                 required before the next payload
#   D/C (bit 6) : 0 = command, 1 = data (GDDRAM write)
#   bits 5..0  : reserved / must be 0 (not validated here)
#
# Commands are appended to `cmd_log`, data bytes to `data_log`. Command
# decoding is minimal: we snoop SET_PAGE_ADDR / SET_COL_LOW / SET_COL_HIGH
# (the classic page addressing mode) into `cur_page` / `cur_col`. Other
# commands (contrast, display on, addressing mode, etc.) pass through
# transparently into `cmd_log` for trace inspection.
#
# Reads (I2C master read transactions) are rare on SSD1306 and return 0xFF
# by default. A test can override `rx_buf` via the DEVS block (filled from
# `mock_preload["status"]` when provided).
#
# The DEVS block delimited by BEGIN/END markers is overwritten per
# stimulus by evaluation/runtime/slave_renderer.render_i2c_display_slave().

# STM32 I2C register offsets (same as i2c_register_slave.py / memory slave)
OFF_CR1   = 0x00
OFF_CR2   = 0x04
OFF_OAR1  = 0x08
OFF_OAR2  = 0x0C
OFF_DR    = 0x10
OFF_SR1   = 0x14
OFF_SR2   = 0x18
OFF_CCR   = 0x1C
OFF_TRISE = 0x20

# CR1 bits
CR1_PE    = 1 << 0
CR1_START = 1 << 8
CR1_STOP  = 1 << 9
CR1_ACK   = 1 << 10

# SR1 bits
SR1_SB    = 1 << 0
SR1_ADDR  = 1 << 1
SR1_BTF   = 1 << 2
SR1_RXNE  = 1 << 6
SR1_TXE   = 1 << 7
SR1_AF    = 1 << 10

# SR2 bits
SR2_MSL   = 1 << 0
SR2_BUSY  = 1 << 1
SR2_TRA   = 1 << 2

# I2C state machine
ST_IDLE      = 0
ST_START     = 1
ST_ADDR_SENT = 2
ST_TX        = 3
ST_RX        = 4

# SSD1306 command opcodes we recognise (informational; most commands just
# pass through to cmd_log untouched).
CMD_SET_COL_LOW_MASK   = 0xF0    # 0x00..0x0F: lower nibble of column
CMD_SET_COL_HIGH_MASK  = 0xF0    # 0x10..0x1F: upper nibble of column
CMD_SET_PAGE_MASK      = 0xF8    # 0xB0..0xB7: page address
CMD_SET_PAGE_BASE      = 0xB0


if request.isInit:
    cr1   = 0
    cr2   = 0
    oar1  = 0
    oar2  = 0
    dr    = 0
    sr1   = SR1_TXE
    sr2   = 0
    ccr   = 0
    trise = 0

    import os
    import json
    trace_path = os.environ.get("DRIVERGEN_I2C_TRACE_PATH", "")
    trace_txn = None
    trace_seq = 0
    if trace_path:
        try:
            f = open(trace_path, "w")
            f.close()
        except Exception:
            trace_path = ""

    try:
        nack_remaining = int(
            os.environ.get("DRIVERGEN_I2C_NACK_FIRST_N", "0") or "0"
        )
    except Exception:
        nack_remaining = 0

    # === DEVS_BLOCK_BEGIN ===
    # Per-stimulus configuration. slave_renderer overwrites this block.
    # Inline defaults emulate a 128x64 SSD1306 at I2C addr 0x3C.
    target_addr_7bit = 0x3C
    display_width    = 128
    display_height   = 64
    display_pages    = display_height // 8
    # Optional read-back buffer (used by `drivergen_eval_read_status` when
    # the adapter decides to read the device). SSD1306 doesn't expose a
    # status register via I2C in most configurations, so default is empty
    # and the slave returns 0xFF on any read.
    mock_read = []
    # === DEVS_BLOCK_END ===

    # I2C transfer state
    state    = ST_IDLE
    cur_addr = 0
    is_read  = False

    # Control-byte state (per-transaction)
    expecting_ctrl  = True
    stream_mode     = None      # 0 = cmd, 1 = data, None = single-byte mode
    single_next_dc  = None      # when Co=1: bookmark for the very next byte

    # Logs (cumulative across the slave's lifetime)
    cmd_log  = []
    data_log = []

    # GDDRAM cursor (from recognised commands)
    cur_page = 0
    cur_col  = 0

    rx_buf = list(mock_read) if mock_read else []
    rx_idx = 0


elif request.isRead:
    off = request.offset
    if off == OFF_CR1:
        request.value = cr1
    elif off == OFF_CR2:
        request.value = cr2
    elif off == OFF_OAR1:
        request.value = oar1
    elif off == OFF_OAR2:
        request.value = oar2
    elif off == OFF_DR:
        if len(rx_buf) > rx_idx:
            dr = rx_buf[rx_idx] & 0xFF
            rx_idx += 1
            if trace_path and trace_txn is not None:
                trace_txn["rx_bytes"].append(dr)
            if rx_idx < len(rx_buf):
                sr1 = sr1 | SR1_RXNE | SR1_BTF
            else:
                sr1 = (sr1 & ~(SR1_RXNE | SR1_BTF)) | SR1_TXE
        else:
            dr = 0xFF
            if trace_path and trace_txn is not None:
                trace_txn["rx_bytes"].append(dr)
            sr1 = (sr1 & ~(SR1_RXNE | SR1_BTF)) | SR1_TXE
        request.value = dr & 0xFF
    elif off == OFF_SR1:
        request.value = sr1
        if sr1 & SR1_SB:
            sr1 = sr1 & ~SR1_SB
    elif off == OFF_SR2:
        request.value = sr2
        if sr1 & SR1_ADDR:
            sr1 = sr1 & ~SR1_ADDR
            if is_read:
                state = ST_RX
                if not rx_buf:
                    rx_buf = [0xFF] * 8
                rx_idx = 0
                sr1 = sr1 | SR1_RXNE | SR1_BTF
            else:
                state = ST_TX
                sr1 = sr1 | SR1_TXE | SR1_BTF
    elif off == OFF_CCR:
        request.value = ccr
    elif off == OFF_TRISE:
        request.value = trise
    else:
        request.value = 0


elif request.isWrite:
    off = request.offset
    val = request.value
    if off == OFF_CR1:
        cr1 = val
        if val & CR1_START:
            prev_state = state
            state = ST_START
            sr1 = SR1_SB | SR1_TXE
            sr2 = SR2_MSL | SR2_BUSY
            if prev_state == ST_IDLE:
                if trace_path:
                    trace_txn = {
                        "seq":      trace_seq,
                        "addr":     None,
                        "is_read":  None,
                        "tx_bytes": [],
                        "rx_bytes": [],
                    }
                    trace_seq = trace_seq + 1
                expecting_ctrl = True
                stream_mode    = None
                single_next_dc = None
            rx_idx = 0
            cr1 = cr1 & ~CR1_START
        if val & CR1_STOP:
            if trace_path and trace_txn is not None:
                try:
                    f = open(trace_path, "a")
                    f.write(json.dumps(trace_txn) + "\n")
                    f.close()
                except Exception:
                    pass
                trace_txn = None
            state = ST_IDLE
            sr1 = SR1_TXE
            sr2 = 0
            cr1 = cr1 & ~CR1_STOP
    elif off == OFF_CR2:
        cr2 = val
    elif off == OFF_OAR1:
        oar1 = val
    elif off == OFF_OAR2:
        oar2 = val
    elif off == OFF_DR:
        dr = val & 0xFF
        if state == ST_START:
            a7 = (dr >> 1) & 0x7F
            rw = dr & 1
            is_read = (rw == 1)
            if trace_path and trace_txn is not None:
                trace_txn["addr"] = a7
                trace_txn["is_read"] = bool(is_read)
            if a7 == target_addr_7bit:
                if nack_remaining > 0:
                    nack_remaining = nack_remaining - 1
                    sr1 = SR1_AF
                    state = ST_IDLE
                    trace_txn = None
                else:
                    cur_addr = a7
                    state = ST_ADDR_SENT
                    sr1 = SR1_ADDR | SR1_TXE
                    if is_read:
                        sr2 = SR2_MSL | SR2_BUSY
                    else:
                        sr2 = SR2_MSL | SR2_BUSY | SR2_TRA
            else:
                sr1 = SR1_AF
                state = ST_IDLE
        elif state == ST_TX:
            if trace_path and trace_txn is not None:
                trace_txn["tx_bytes"].append(dr)
            if expecting_ctrl:
                # Parse control byte.
                co  = (dr >> 7) & 1
                dc  = (dr >> 6) & 1
                if co == 1:
                    # Single-byte mode: only the NEXT byte is in this mode.
                    single_next_dc = dc
                    stream_mode    = None
                else:
                    # Stream mode: this + all remaining bytes until STOP.
                    stream_mode    = dc
                    single_next_dc = None
                expecting_ctrl = False
            else:
                # Payload byte.
                if single_next_dc is not None:
                    dc = single_next_dc
                    single_next_dc = None
                    expecting_ctrl = True       # need another control byte
                else:
                    dc = stream_mode if stream_mode is not None else 0
                    # stays in stream, expecting_ctrl remains False

                if dc == 0:
                    cmd_log.append(dr)
                    # Snoop page / column-set commands.
                    if (dr & CMD_SET_PAGE_MASK) == CMD_SET_PAGE_BASE:
                        cur_page = dr & 0x07
                    elif 0x00 <= dr <= 0x0F:
                        cur_col = (cur_col & 0xF0) | (dr & 0x0F)
                    elif 0x10 <= dr <= 0x1F:
                        cur_col = (cur_col & 0x0F) | ((dr & 0x0F) << 4)
                else:
                    data_log.append(dr)
                    # Page addressing mode advance: col++ then wrap.
                    cur_col = cur_col + 1
                    if cur_col >= display_width:
                        cur_col = 0
            sr1 = sr1 | SR1_TXE | SR1_BTF
    elif off == OFF_CCR:
        ccr = val
    elif off == OFF_TRISE:
        trise = val
