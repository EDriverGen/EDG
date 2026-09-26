# coding: utf-8
# i2c_register_slave.py -- Renode Python peripheral (v2)
# Emulates STM32 I2C controller at register level (CR1/CR2/SR1/SR2/DR/CCR/TRISE)
# Handles HAL_I2C_Master_Transmit/Receive/Mem_Read/Mem_Write.
#
# Device register maps live in the DEVS block delimited by the markers
# below. evaluation/runtime/slave_renderer.py replaces the block per
# stimulus before invoking Renode; the inline defaults are kept so the
# file remains a runnable peripheral when rendered "as is" (useful for
# manual debugging).
#
# Markers MUST stay verbatim — slave_renderer parses them by string match.

# STM32 I2C register offsets
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
SR1_SB    = 1 << 0   # Start Bit
SR1_ADDR  = 1 << 1   # Address Sent/Matched
SR1_BTF   = 1 << 2   # Byte Transfer Finished
SR1_RXNE  = 1 << 6   # RX Not Empty
SR1_TXE   = 1 << 7   # TX Empty
SR1_AF    = 1 << 10  # Acknowledge Failure

# SR2 bits
SR2_MSL   = 1 << 0   # Master Mode
SR2_BUSY  = 1 << 1   # Bus Busy
SR2_TRA   = 1 << 2   # Transmitter/Receiver (1=TX, 0=RX)

# I2C state machine
ST_IDLE      = 0
ST_START     = 1   # START generated, waiting for address write
ST_ADDR_SENT = 2   # Address written to DR, ADDR pending
ST_TX        = 3   # Master transmit mode
ST_RX        = 4   # Master receive mode

# ---------------------------------------------------------------------------
# Neutral I2C transaction trace recorder.
#
# Controlled by env var DRIVERGEN_I2C_TRACE_PATH. When set, every I2C
# transaction (START..STOP pair) is appended to that file as one JSON line
# (JSONL). When unset, no tracing happens (zero overhead for normal tests).
#
# This capability is neutral: it does NOT embed any reference/oracle data.
# Evaluation code in DriverGen/evaluation/ consumes these traces and
# compares them against per-device golden traces; the Renode model itself
# knows nothing about what is "correct".
# ---------------------------------------------------------------------------

if request.isInit:
    cr1   = 0
    cr2   = 0
    oar1  = 0
    oar2  = 0
    dr    = 0
    sr1   = SR1_TXE   # TXE = 1 when idle
    sr2   = 0
    ccr   = 0
    trise = 0

    # --- Trace recording state (all no-ops if trace_path is empty) ---
    import os
    import json
    trace_path = os.environ.get("DRIVERGEN_I2C_TRACE_PATH", "")
    trace_txn = None      # dict(seq, addr, is_read, tx_bytes, rx_bytes)
    trace_seq = 0         # transaction counter since boot
    # Truncate file at boot so each run starts fresh.
    if trace_path:
        try:
            f = open(trace_path, "w")
            f.close()
        except Exception:
            trace_path = ""

    # --- L5 error-injection state (read-only after init) ---
    # DRIVERGEN_I2C_NACK_FIRST_N=<int>: force NACK on the first N address-match
    # events that would otherwise have been ACKed. 0 = no injection.
    try:
        nack_remaining = int(os.environ.get("DRIVERGEN_I2C_NACK_FIRST_N", "0") or "0")
    except Exception:
        nack_remaining = 0

    state = ST_IDLE
    cur_addr = 0      # current 7-bit slave address
    is_read  = False   # read or write direction
    tx_buf   = []      # bytes received from master (writes)
    rx_buf   = []      # bytes to send to master (reads)
    rx_idx   = 0       # current read index in rx_buf
    reg_ptr  = 0       # register pointer set by first write byte
    got_ptr  = False   # whether register pointer has been set
    mem_size = 0       # memory address size (for Mem_Read/Write)

    # === DEVS_BLOCK_BEGIN ===
    # Per-stimulus preload (rendered by slave_renderer).
    devs = {}
    direct_read_bytes = {}
    port_only_devs = set()
    command_mode_devs = set()
    devs[0x76] = {}
    devs[0x76][136] = [0x70, 0x6B, 0x43, 0x67, 0x18, 0xFC, 0x7D, 0x8E, 0x43, 0xD6, 0xD0, 0x0B, 0x27, 0x0B, 0x8C, 0x00, 0xF9, 0xFF, 0x8C, 0x3C, 0xF8, 0xC6, 0x70, 0x17, 0x00, 0x00]
    devs[0x76][161] = [0x4B]
    devs[0x76][208] = [0x60]
    devs[0x76][225] = [0x72, 0x01, 0x00, 0x13, 0x29, 0x03, 0x1E]
    devs[0x76][242] = [0x00]
    devs[0x76][243] = [0x00]
    devs[0x76][244] = [0x00]
    devs[0x76][247] = [0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF]
    # === DEVS_BLOCK_END ===

    # ----- Status register overrides -----
    # When reading a register after a write, OR the stored value with this mask
    # Needed for devices where writing a command doesn't set status/ready bits
    status_overrides = {}
    status_overrides[(0x77, 0x08)] = 0xF0  # DPS310 MEAS_CFG: force all ready bits

    # ----- Per-device reg_ptr persistence across separate transactions -----
    # Register-addressed devices keep reg_ptr across STOP→START sequences,
    # mimicking real I2C slaves that remember the internal register pointer.
    # Command-based devices (BH1750, SHT30, SSD1306) reset to 0.
    reg_ptr_persist = {
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
        0x40,
        0x48, 0x49, 0x4C, 0x50, 0x68, 0x77, 0x76,
        0x19, 0x1E, 0x29, 0x2A,
    }  # MCP23017 (0x20-0x27), PCA9685 (0x40), LM75A, TMP105, EMC1413, AT24C256,
       # DS3231/MPU6050, DPS310, BME280, LSM303DLHC accel/mag, VL53L0X, TMP421
    saved_reg_ptr = {}  # device_addr → last reg_ptr
    def _is_port_only(addr):
        """Return True when the mock data at *addr* is pure port I/O (PCF8574).

        The renderer can mark known port-only devices explicitly. Otherwise,
        if the device mock has register-address keys (integers or "0xNN" hex
        strings), treat it as a register-pointer device even at addresses
        that overlap PCF8574 (0x20-0x27, e.g. MCP23017).
        """
        if addr in port_only_devs:
            return True
        if addr not in {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27}:
            return False
        regs = devs.get(addr, {})
        if not isinstance(regs, dict) or not regs:
            return True
        for k in regs:
            try:
                int(str(k), 0)
                return False
            except (ValueError, TypeError):
                pass
        return True

    def _is_command_mode(addr):
        return addr in command_mode_devs

    # ----- 2-byte addressing for EEPROM devices -----
    two_byte_addr_devs = {0x50}  # AT24C256 uses 16-bit memory addressing
    addr_phase = 0  # 0=expecting first address byte, 1=got first byte

    # ----- Auto-increment bit mask for LSM303DLHC -----
    auto_incr_mask_devs = {0x19, 0x1E}  # Strip bit 7 from reg_ptr

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
        # Reading DR: return next byte from slave
        # Works in ST_RX or after STOP (HAL reads DR after generating STOP)
        if len(rx_buf) > rx_idx:
            dr = rx_buf[rx_idx]
            rx_idx += 1
            # --- trace: record every RX byte delivered to master ---
            if trace_path and trace_txn is not None:
                trace_txn["rx_bytes"].append(dr & 0xFF)
            if rx_idx < len(rx_buf):
                sr1 = sr1 | SR1_RXNE | SR1_BTF
            else:
                # All data consumed: clear RXNE/BTF
                sr1 = (sr1 & ~(SR1_RXNE | SR1_BTF)) | SR1_TXE
        request.value = dr & 0xFF
    elif off == OFF_SR1:
        request.value = sr1
        # Reading SR1 clears SB and ADDR (part of clear sequence)
        if sr1 & SR1_SB:
            sr1 = sr1 & ~SR1_SB
        # ADDR is cleared by reading SR1 then SR2
    elif off == OFF_SR2:
        request.value = sr2
        # Reading SR2 after SR1 clears ADDR
        if sr1 & SR1_ADDR:
            sr1 = sr1 & ~SR1_ADDR
            # Now transition to TX or RX mode
            if is_read:
                state = ST_RX
                # Prepare read buffer from device registers
                rp = 0 if (_is_port_only(cur_addr) or _is_command_mode(cur_addr)) else (reg_ptr if got_ptr else 0)
                # Strip auto-increment bit for LSM303DLHC
                if cur_addr in auto_incr_mask_devs and rp >= 0x80:
                    rp = rp & 0x7F
                # Build rx_buf with auto-increment across registers
                dev_regs = devs.get(cur_addr, {})
                direct = direct_read_bytes.get(cur_addr, None)
                if direct is not None:
                    rx_buf = list(direct)
                    while len(rx_buf) < 32:
                        rx_buf.append(0xFF)
                else:
                    rx_buf = []
                    p = rp
                    for _ in range(32):
                        chunk = dev_regs.get(p, None)
                        if chunk is not None:
                            rx_buf.extend(chunk)
                            p = p + len(chunk)
                        else:
                            # Multi-byte register data (e.g. MCP23017
                            # GPIOA at 0x12 returns 2 bytes covering
                            # GPIOA+GPIOB).  When the MCU reads them
                            # separately (write 0x12→read, write
                            # 0x13→read), resolve the second read
                            # from the first register's extra bytes.
                            derived = None
                            for rp_cand, rp_data in dev_regs.items():
                                if not (isinstance(rp_cand, int) and isinstance(rp_data, list)):
                                    continue
                                off = p - rp_cand
                                if 0 <= off < len(rp_data):
                                    derived = rp_data[off]
                                    break
                            if derived is not None:
                                rx_buf.append(derived)
                                p = p + 1
                            else:
                                rx_buf.append(0xFF)
                                p = p + 1
                # Apply status register overrides (e.g., DPS310 ready bits)
                key = (cur_addr, rp)
                if key in status_overrides and len(rx_buf) > 0:
                    rx_buf[0] = rx_buf[0] | status_overrides[key]
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
            # START condition (or repeated START for Mem_Read)
            prev_state = state
            state = ST_START
            sr1 = SR1_SB | SR1_TXE   # Set Start Bit flag
            sr2 = SR2_MSL | SR2_BUSY  # Master mode, bus busy
            # On repeated START (Mem_Read), preserve reg_ptr set during write phase
            if prev_state == ST_IDLE:
                got_ptr = False
                addr_phase = 0
                # --- trace: begin new transaction on fresh START ---
                if trace_path:
                    trace_txn = {
                        "seq": trace_seq,
                        "addr": None,
                        "is_read": None,
                        "tx_bytes": [],
                        "rx_bytes": [],
                    }
                    trace_seq = trace_seq + 1
                # On fresh START, save reg_ptr for last-talked device.
                # Save reg_ptr for the device we just communicated with
                if cur_addr in devs and not _is_port_only(cur_addr) and not _is_command_mode(cur_addr):
                    saved_reg_ptr[cur_addr] = reg_ptr
                reg_ptr = 0  # default; may be overridden by address byte handler
            # else: keep got_ptr and reg_ptr from write phase
            tx_buf = []
            rx_buf = []
            rx_idx = 0
            cr1 = cr1 & ~CR1_START    # Clear START bit after processing
        if val & CR1_STOP:
            # STOP condition
            # Save reg_ptr for potential future reads from same device
            if _is_port_only(cur_addr) or _is_command_mode(cur_addr):
                saved_reg_ptr.pop(cur_addr, None)
            elif cur_addr in devs:
                saved_reg_ptr[cur_addr] = reg_ptr
            # --- trace: finalize current transaction ---
            if trace_path and trace_txn is not None:
                try:
                    f = open(trace_path, "a")
                    f.write(json.dumps(trace_txn) + "\n")
                    f.close()
                except Exception:
                    pass
                trace_txn = None
            state = ST_IDLE
            # Preserve RXNE if unread data exists (HAL reads DR after STOP)
            if len(rx_buf) > rx_idx:
                sr2 = 0   # clear BUSY, keep sr1 flags (RXNE/BTF)
            else:
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
            # Address byte written: extract 7-bit addr and R/W
            a7 = (dr >> 1) & 0x7F
            rw = dr & 1
            is_read = (rw == 1)
            # --- trace: record address and direction ---
            if trace_path and trace_txn is not None:
                trace_txn["addr"] = a7
                trace_txn["is_read"] = bool(is_read)
            if a7 in devs:
                # --- L5 error injection: force NACK for first N address matches ---
                if nack_remaining > 0:
                    nack_remaining = nack_remaining - 1
                    sr1 = SR1_AF
                    state = ST_IDLE
                    # Abort the in-progress trace transaction so we don't
                    # emit a half-captured line.
                    if trace_path and trace_txn is not None:
                        trace_txn = None
                else:
                    cur_addr = a7
                    state = ST_ADDR_SENT
                    sr1 = SR1_ADDR | SR1_TXE
                    # For register-addressed devices, restore reg_ptr from last
                    # transaction so that separate write(reg)+read(data) works.
                    # Only apply when got_ptr is False (no reg set in current
                    # transaction's write phase), so repeated-START is not broken.
                    if is_read and not got_ptr and a7 in reg_ptr_persist and a7 in saved_reg_ptr \
                       and not _is_port_only(a7) and not _is_command_mode(a7):
                        reg_ptr = saved_reg_ptr[a7]
                        got_ptr = True
                    # Set TRA based on direction
                    if is_read:
                        sr2 = SR2_MSL | SR2_BUSY          # receiver: TRA=0
                    else:
                        sr2 = SR2_MSL | SR2_BUSY | SR2_TRA  # transmitter: TRA=1
            elif a7 == 0x00 and not is_read:
                # I2C General Call broadcast (address 0x00, write only).
                # Used by datasheet-mandated commands such as SHT3x General
                # Call Reset (0x06). The bus must ACK and absorb the bytes,
                # but no per-device register state is mutated:
                # `devs.get(0x00, {})` is empty, so the ST_TX data-byte path
                # (`d = devs.get(cur_addr, {}).get(reg_ptr, None)`) is a
                # no-op for cur_addr = 0x00. STOP also won't persist
                # reg_ptr (cur_addr not in devs => skipped).
                cur_addr = a7
                state = ST_ADDR_SENT
                sr1 = SR1_ADDR | SR1_TXE
                sr2 = SR2_MSL | SR2_BUSY | SR2_TRA  # broadcast = transmitter
            else:
                # NACK -- device not present (also covers reads to the
                # broadcast address 0x00, which the I2C spec forbids).
                sr1 = SR1_AF
                state = ST_IDLE
        elif state == ST_TX:
            # Data byte from master
            if not got_ptr:
                if cur_addr in two_byte_addr_devs:
                    # 2-byte addressing (e.g., AT24C256 EEPROM)
                    if addr_phase == 0:
                        reg_ptr = dr << 8   # High byte of 16-bit address
                        addr_phase = 1
                    else:
                        reg_ptr = reg_ptr | dr  # Low byte
                        got_ptr = True
                        addr_phase = 0
                elif _is_port_only(cur_addr):
                    # Port-only device: first byte IS data (port output), not register addr.
                    # Accept the byte but do NOT overwrite preloaded mock data;
                    # the preload represents the external port state for reads.
                    reg_ptr = 0
                    got_ptr = True
                    tx_buf.append(dr)
                else:
                    reg_ptr = dr
                    got_ptr = True
            else:
                if not _is_port_only(cur_addr) and not _is_command_mode(cur_addr):
                    d = devs.get(cur_addr, {}).get(reg_ptr, None)
                    if d is not None:
                        idx = len(tx_buf)
                        if idx < len(d):
                            d[idx] = dr
                tx_buf.append(dr)
            # --- trace: record every TX byte from master (incl. reg ptr) ---
            if trace_path and trace_txn is not None:
                trace_txn["tx_bytes"].append(dr)
            sr1 = sr1 | SR1_TXE | SR1_BTF
    elif off == OFF_CCR:
        ccr = val
    elif off == OFF_TRISE:
        trise = val
