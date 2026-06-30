# coding: utf-8
# Renode Python I2C register peripheral.
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
# Evaluation code consumes these traces and
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
    # Per-stimulus mock_preload data. slave_renderer overwrites this entire
    # region (between the BEGIN/END markers) when staging a vector. The
    # inline defaults below cover all 21 reference devices so the file
    # stays runnable for ad-hoc Renode invocations.
    devs = {}
    direct_read_bytes = {}
    port_only_devs = set()
    command_mode_devs = {0x23, 0x44}  # BH1750, SHT30: command byte is not a register pointer
    # LM75A @ 0x48
    devs[0x48] = {}
    devs[0x48][0] = [0x19, 0x00]   # Temp  = 25.0 C
    devs[0x48][1] = [0x00]         # Conf  = 0x00
    devs[0x48][2] = [0x4B, 0x00]   # Thyst = 75.0 C
    devs[0x48][3] = [0x50, 0x00]   # Tos   = 80.0 C
    # BH1750 @ 0x23
    devs[0x23] = {}
    devs[0x23][0] = [0x04, 0xB0]
    # BME280 @ 0x76
    devs[0x76] = {}
    devs[0x76][0xD0] = [0x60]
    devs[0x76][0x88] = [0x70, 0x6B, 0x43, 0x67, 0x18, 0xFC,
                        0x7D, 0x8E, 0x43, 0xD6, 0xD0, 0x0B,
                        0x27, 0x0B, 0x8C, 0x00, 0xF9, 0xFF,
                        0x8C, 0x3C, 0xF8, 0xC6, 0x70, 0x17,
                        0x00, 0x00]
    devs[0x76][0xA1] = [0x4B]
    devs[0x76][0xE1] = [0x72, 0x01, 0x00, 0x13, 0x29, 0x03, 0x1E]
    devs[0x76][0xF7] = [0x51, 0x40, 0x00, 0x7E, 0xED, 0x00,
                        0x6D, 0x60]
    devs[0x76][0xF2] = [0x00]
    devs[0x76][0xF3] = [0x00]     # Status: not measuring, not updating
    devs[0x76][0xF4] = [0x00]
    # SHT30 @ 0x44
    devs[0x44] = {}
    devs[0x44][0] = [0x66, 0x66, 0x93, 0x7F, 0xFF, 0x8F]
    # MPU6050 @ 0x68
    devs[0x68] = {}
    devs[0x68][0x75] = [0x68]
    devs[0x68][0x6B] = [0x00]
    devs[0x68][0x3B] = [0x00, 0x00, 0x00, 0x00, 0x40, 0x00]
    devs[0x68][0x43] = [0x00, 0x10, 0x00, 0x20, 0x00, 0x30]
    # TMP105 @ 0x49 (alternate address to avoid LM75A conflict)
    devs[0x49] = {}
    devs[0x49][0] = [0x19, 0x10]   # 12-bit temp: raw=(0x1910>>4)=0x191=401, temp_mc=401*625/10=25062
    devs[0x49][1] = [0x60]         # Config: 12-bit resolution

    # AT24C256 EEPROM @ 0x50
    devs[0x50] = {}
    devs[0x50][0] = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,
                     0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00]

    # EMC1413 multi-channel temperature @ 0x4C
    devs[0x4C] = {}
    devs[0x4C][0xFE] = [0x5D]     # Manufacturer ID (SMSC)
    devs[0x4C][0xFD] = [0x21]     # Product ID (EMC1413)
    devs[0x4C][0xFF] = [0x01]     # Revision
    devs[0x4C][0x02] = [0x00]     # Status
    devs[0x4C][0x03] = [0x00]     # Config
    devs[0x4C][0x04] = [0x08]     # Conversion rate
    devs[0x4C][0x00] = [0x19]     # Internal temp hi = 25
    devs[0x4C][0x29] = [0x00]     # Internal temp lo = 0  → 25000 mC
    devs[0x4C][0x01] = [0x1C]     # Ext1 temp hi = 28
    devs[0x4C][0x10] = [0x60]     # Ext1 temp lo
    devs[0x4C][0x23] = [0x17]     # Ext2 temp hi = 23
    devs[0x4C][0x24] = [0x80]     # Ext2 temp lo

    # DS3231 RTC @ 0x68 (shares with MPU6050, no register overlap)
    devs[0x68][0x00] = [0x00, 0x30, 0x12, 0x01, 0x15, 0x06, 0x25]  # BCD 00:30:12 Day1 15-Jun-25
    devs[0x68][0x11] = [0x19, 0x00]   # Temperature: raw=((0x19<<8)|0x00)>>6=100, temp_mc=100*250=25000

    # DPS310 pressure/temp @ 0x77
    devs[0x77] = {}
    devs[0x77][0x0D] = [0x10]     # PROD_ID
    devs[0x77][0x08] = [0xF0]     # MEAS_CFG (COEF_RDY|SENSOR_RDY|TMP_RDY|PRS_RDY)
    devs[0x77][0x06] = [0x01]     # PRS_CFG (1x oversampling)
    devs[0x77][0x07] = [0x81]     # TMP_CFG (EXT sensor, 2x oversampling)
    devs[0x77][0x09] = [0x00]     # CFG_REG
    devs[0x77][0x0A] = [0x00]     # INT_STS
    devs[0x77][0x0B] = [0x00]     # FIFO_STS
    devs[0x77][0x0C] = [0x00]     # RESET
    devs[0x77][0x28] = [0x80]     # COEF_SRCE (external temp sensor)
    # Calibration coefficients (18 bytes): c0=256, c1=1024, others=0/1
    devs[0x77][0x10] = [0x10, 0x04, 0x00,
                        0x00, 0x00, 0x00,
                        0x00, 0x00,
                        0x00, 0x01,
                        0x00, 0x00,
                        0x00, 0x00,
                        0x00, 0x00,
                        0x00, 0x00]
    devs[0x77][0x00] = [0x01, 0x00, 0x00]   # PSR 24-bit pressure
    devs[0x77][0x03] = [0x10, 0x00, 0x00]   # TMP 24-bit temperature

    # LSM303DLHC accelerometer @ 0x19
    devs[0x19] = {}
    devs[0x19][0x20] = [0x57]     # CTRL_REG1_A (ODR=50Hz, XYZ enabled)
    devs[0x19][0x23] = [0x08]     # CTRL_REG4_A (2g, high-res)
    devs[0x19][0x27] = [0x0F]     # STATUS_REG_A (data available)
    devs[0x19][0x28] = [0x00, 0x10, 0x00, 0x20, 0x00, 0x40]  # XYZ accel data

    # LSM303DLHC magnetometer @ 0x1E
    devs[0x1E] = {}
    devs[0x1E][0x0A] = [0x48]     # IRA_REG_M
    devs[0x1E][0x0B] = [0x34]     # IRB_REG_M
    devs[0x1E][0x0C] = [0x33]     # IRC_REG_M
    devs[0x1E][0x00] = [0x14]     # CRA_REG_M
    devs[0x1E][0x01] = [0x20]     # CRB_REG_M
    devs[0x1E][0x02] = [0x00]     # MR_REG_M (continuous)
    devs[0x1E][0x09] = [0x01]     # SR_REG_M (data ready)
    devs[0x1E][0x03] = [0x01, 0x00, 0x00, 0x50, 0x00, 0xA0]  # XZY mag data

    # TMP421 multi-channel temp @ 0x2A
    devs[0x2A] = {}
    devs[0x2A][0xFE] = [0x55]     # Manufacturer ID (TI)
    devs[0x2A][0xFF] = [0x21]     # Device ID
    devs[0x2A][0x08] = [0x00]     # Status
    devs[0x2A][0x09] = [0x00]     # Config 1
    devs[0x2A][0x0A] = [0x00]     # Config 2
    devs[0x2A][0x00] = [0x19]     # Local temp MSB = 25
    devs[0x2A][0x10] = [0x00]     # Local temp LSB → raw=0x190=400, temp_mc=25000
    devs[0x2A][0x01] = [0x1C]     # Remote temp MSB = 28
    devs[0x2A][0x11] = [0x80]     # Remote temp LSB

    # VL53L0X ToF distance @ 0x29
    devs[0x29] = {}
    devs[0x29][0xC0] = [0xEE]     # MODEL_ID
    devs[0x29][0x00] = [0x00]     # SYSRANGE_START
    devs[0x29][0x13] = [0x07]     # RESULT_RANGE_STATUS (data ready)
    devs[0x29][0x0B] = [0x00]     # INTERRUPT_CLEAR
    devs[0x29][0x14] = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x01, 0xF4]  # Range 500mm

    # SSD1306 OLED @ 0x3C (write-only display, minimal read support)
    devs[0x3C] = {}
    devs[0x3C][0] = [0x00]
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
        """Return True when the mock data at *addr* is pure port I/O (PCF8574)."""
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
