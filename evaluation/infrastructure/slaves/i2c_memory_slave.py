# coding: ascii
# i2c_memory_slave.py -- Renode Python peripheral for eval_class="memory".
# Models STM32 I2C1 register-level controller PLUS a single I2C memory
# device (EEPROM / FRAM) with configurable:
#   - 7-bit slave address
#   - memory size            (bytes)
#   - memory page size       (bytes, used for page-wrap write semantics)
#   - address width on bus   (1 or 2 bytes for <=256B vs >=512B parts)
#
# Compared to i2c_register_slave.py, this template models memory as a
# FLAT byte array with:
#   - sequential read:  bytes read with linear auto-increment,
#                       clamped to memory_size (returns 0xFF past end)
#   - page write:       data after the address bytes wraps within the
#                       current page, committed to `memory` on STOP
#
# The DEVS block delimited by BEGIN/END markers is overwritten per
# stimulus by evaluation/runtime/slave_renderer.py; the inline defaults
# below let the file run standalone for manual Renode invocations.

# STM32 I2C register offsets (same as i2c_register_slave.py)
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

    # --- Neutral trace recording (DRIVERGEN_I2C_TRACE_PATH, same convention
    #     as i2c_register_slave) ---
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

    # --- L5 error-injection state ---
    try:
        nack_remaining = int(
            os.environ.get("DRIVERGEN_I2C_NACK_FIRST_N", "0") or "0"
        )
    except Exception:
        nack_remaining = 0

    # === DEVS_BLOCK_BEGIN ===
    # Per-stimulus configuration. slave_renderer overwrites this block.
    # The inline defaults emulate AT24C256 (32KB, 64B pages, 2-byte addr).
    target_addr_7bit   = 0x50
    memory_size_bytes  = 32768
    memory_page_bytes  = 64
    address_size_bytes = 2
    memory = [0xFF] * memory_size_bytes
    # Preloaded bytes at the start of memory.
    memory[0] = 0xAA
    memory[1] = 0xBB
    memory[2] = 0xCC
    memory[3] = 0xDD
    # === DEVS_BLOCK_END ===

    # I2C transfer state
    state       = ST_IDLE
    cur_addr    = 0
    is_read     = False
    addr_phase  = 0      # 0..address_size_bytes-1 during address reception
    reg_ptr     = 0      # decoded flat byte address into `memory`
    got_ptr     = False
    tx_buf      = []     # bytes received (address bytes + data) for tracing
    rx_buf      = []
    rx_idx      = 0

    # Staged page-write buffer: key = absolute memory index, val = byte.
    # Applied to `memory` on STOP so that NACK or aborted transactions
    # don't half-corrupt the memory image.
    page_staged = {}


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
            dr = rx_buf[rx_idx]
            rx_idx += 1
            if trace_path and trace_txn is not None:
                trace_txn["rx_bytes"].append(dr & 0xFF)
            if rx_idx < len(rx_buf):
                sr1 = sr1 | SR1_RXNE | SR1_BTF
            else:
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
                # Build rx_buf as linear slice of `memory` from reg_ptr,
                # clamped to memory_size (reads past end return 0xFF).
                rp = reg_ptr if got_ptr else 0
                rx_buf = []
                for i in range(64):
                    p = rp + i
                    if 0 <= p < memory_size_bytes:
                        rx_buf.append(memory[p] & 0xFF)
                    else:
                        rx_buf.append(0xFF)
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
                got_ptr = False
                addr_phase = 0
                if trace_path:
                    trace_txn = {
                        "seq":      trace_seq,
                        "addr":     None,
                        "is_read":  None,
                        "tx_bytes": [],
                        "rx_bytes": [],
                    }
                    trace_seq = trace_seq + 1
                reg_ptr = 0
            tx_buf = []
            rx_buf = []
            rx_idx = 0
            cr1 = cr1 & ~CR1_START
        if val & CR1_STOP:
            # Apply staged page-write bytes to `memory` (if any).
            if page_staged:
                for idx, b in page_staged.items():
                    if 0 <= idx < memory_size_bytes:
                        memory[idx] = b & 0xFF
                page_staged = {}
            if trace_path and trace_txn is not None:
                try:
                    f = open(trace_path, "a")
                    f.write(json.dumps(trace_txn) + "\n")
                    f.close()
                except Exception:
                    pass
                trace_txn = None
            state = ST_IDLE
            if len(rx_buf) > rx_idx:
                sr2 = 0
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
            if not got_ptr:
                # Accumulate address bytes (big-endian).
                reg_ptr = ((reg_ptr << 8) | dr) & 0xFFFFFFFF
                addr_phase = addr_phase + 1
                if addr_phase >= address_size_bytes:
                    got_ptr = True
            else:
                # Data byte: stage into current page with wrap inside page.
                page_base = (reg_ptr // memory_page_bytes) * memory_page_bytes
                offset    = reg_ptr % memory_page_bytes
                written   = sum(
                    1 for k in page_staged if page_base <= k < page_base + memory_page_bytes
                )
                target = page_base + (offset + written) % memory_page_bytes
                page_staged[target] = dr & 0xFF
            if trace_path and trace_txn is not None:
                trace_txn["tx_bytes"].append(dr)
            sr1 = sr1 | SR1_TXE | SR1_BTF
    elif off == OFF_CCR:
        ccr = val
    elif off == OFF_TRISE:
        trise = val
