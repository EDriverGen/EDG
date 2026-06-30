# coding: ascii
# spi_generic_slave.py -- Renode Python peripheral for SPI eval (any class).
#
# Emulates the STM32F103 SPI1 controller register block (CR1/CR2/SR/DR/...)
# AT 0x40013000 together with a pluggable slave device model selected by
# `target_proto` in the DEVS block. Three protocols:
#
#   "register"  -- classic addressed protocol (ADXL345, BMP280, MPU6500).
#                  First byte of each frame is a command: bits carry R/W,
#                  multi-byte, and register address; subsequent bytes are
#                  data read (slave -> master) or ignored on write.
#
#   "stream"    -- fixed-length playback (MAX31855, HX711, shift-register
#                  ADCs with CS-triggered conversion). The slave clocks
#                  out `rx_stream[i]` on byte index i, regardless of TX.
#
#   "command"   -- opcode-dispatched response (SPI flash R/W, MCP3008,
#                  SD card commands). First byte selects a key in
#                  `cmd_table`; subsequent bytes are clocked from the
#                  looked-up response vector, padding with 0x00.
#
# Frame boundaries (CS pulses) are signaled by writing any value to the
# custom offset SPI1+0x80 (TXN_RESET). This lets
# us run without physical NSS wiring through Renode. A bare-metal or
# adapter layer that wants to reuse this slave should assert TXN_RESET
# on each cs_lo().
#
# Optional custom offset SPI1+0x84 (DEV_SELECT) is ignored in this template
# (single slave per peripheral); keep reads non-fatal for compatibility.
#
# Optional env vars:
#   DRIVERGEN_SPI_TRACE_PATH -- JSONL path; one line per completed frame.
#   DRIVERGEN_SPI_FAULT_FIRST_N -- L5 fault injection. Force the slave to
#       clock out 0xFF on MISO for the first N complete frames (CS-bounded).
#       Simulates "device not present" / "MISO line stuck high" - a well-
#       behaved driver should detect the bogus response (wrong WHOAMI /
#       all-ones data) on init or read and return an error rather than
#       reporting RESULT: PASS. Counted per frame (one frame = one CS
#       assertion, delimited by TXN_RESET writes) so the fault persists
#       across multi-byte bursts within a single transaction.
#
# The DEVS block delimited by BEGIN/END markers is overwritten per
# stimulus by evaluation/runtime/slave_renderer.render_spi_generic_slave().

# STM32F103 SPI1 register offsets
OFF_CR1    = 0x00
OFF_CR2    = 0x04
OFF_SR     = 0x08
OFF_DR     = 0x0C
OFF_CRCPR  = 0x10
OFF_RXCRCR = 0x14
OFF_TXCRCR = 0x18

# Custom (emulator-only) offsets
TXN_RESET_OFF  = 0x80
DEV_SELECT_OFF = 0x84

# CR1 bits
CR1_SPE  = 1 << 6
CR1_MSTR = 1 << 2

# SR bits
SR_RXNE  = 1 << 0
SR_TXE   = 1 << 1
SR_BSY   = 1 << 7


def _new_phase(proto):
    """Return a fresh phase dict for the given protocol."""
    if proto == "register":
        return {"stage": "cmd", "reg": 0, "read": False, "mb": False, "idx": 0}
    if proto == "command":
        return {"stage": "opcode", "opcode": None, "resp": [], "idx": 0}
    # default: stream (stateless except idx)
    return {"stage": "stream", "idx": 0}


def _register_xfer(phase, regs, tx, rw_mask, mb_mask, addr_mask, read_when_set):
    """Register-addressed protocol transfer. Returns the slave-to-master byte."""
    if phase["stage"] == "cmd":
        rw_set = (tx & rw_mask) != 0
        phase["read"]  = rw_set if read_when_set else (not rw_set)
        phase["mb"]    = (tx & mb_mask) != 0 if mb_mask else False
        phase["reg"]   = tx & addr_mask
        phase["idx"]   = 0
        phase["stage"] = "data"
        return 0x00
    # data stage: for reads, index sequentially into regs[phase["reg"]]
    # (oracle preloads the full burst there; no auto-advance of reg address).
    if phase["read"]:
        buf = regs.get(phase["reg"])
        if buf is None:
            reply = 0x00
        elif phase["idx"] < len(buf):
            reply = buf[phase["idx"]] & 0xFF
        else:
            reply = 0x00
    else:
        reply = 0x00
    phase["idx"] = phase["idx"] + 1
    return reply


def _stream_xfer(phase, rx_stream, tx):
    """Pure playback: emit rx_stream[idx], pad with 0x00 beyond end."""
    idx = phase.get("idx", 0)
    if idx < len(rx_stream):
        reply = rx_stream[idx] & 0xFF
    else:
        reply = 0x00
    phase["idx"] = idx + 1
    return reply


def _command_xfer(phase, cmd_table, tx):
    """Opcode-dispatched playback. Byte 0 selects response; later bytes pad."""
    if phase["stage"] == "opcode":
        phase["opcode"] = tx
        resp = cmd_table.get(tx)
        if resp is None:
            phase["resp"] = []
        else:
            phase["resp"] = [b & 0xFF for b in resp]
        phase["idx"]   = 0
        phase["stage"] = "data"
        # During the opcode byte itself the master's TX is clocked into
        # the slave. Classic flash devices latch 0x00 on the MISO line
        # during the opcode byte, so we also return 0x00 here even if
        # resp[0] is defined; the actual response starts on byte 1.
        return 0x00
    # data stage: resp[0] is the first data byte AFTER the opcode.
    resp = phase.get("resp", [])
    idx  = phase.get("idx", 0)
    if idx < len(resp):
        reply = resp[idx]
    else:
        reply = 0x00
    phase["idx"] = idx + 1
    return reply


_DISPATCHERS = {
    "register": _register_xfer,
    "stream":   _stream_xfer,
    "command":  _command_xfer,
}


if request.isInit:
    cr1        = 0
    cr2        = 0
    sr         = SR_TXE
    dr_latched = 0

    import os
    import json
    trace_path = os.environ.get("DRIVERGEN_SPI_TRACE_PATH", "")
    trace_frame = None
    trace_seq = 0
    if trace_path:
        try:
            f = open(trace_path, "w")
            f.close()
        except Exception:
            trace_path = ""

    # --- L5 error-injection state (read-only after init) ---
    # DRIVERGEN_SPI_FAULT_FIRST_N=<int>: force MISO=0xFF for the first N
    # complete frames (CS-bounded). Mimics a disconnected slave or stuck
    # MISO pull-up. Activated on each TXN_RESET (frame start) so long as
    # budget remains; stays latched for every byte in that frame, then
    # deactivates on the next TXN_RESET once the budget is exhausted.
    try:
        fault_frames_remaining = int(
            os.environ.get("DRIVERGEN_SPI_FAULT_FIRST_N", "0") or "0"
        )
    except Exception:
        fault_frames_remaining = 0
    fault_in_frame = False

    # === DEVS_BLOCK_BEGIN ===
    # Per-stimulus configuration. slave_renderer overwrites this block.
    # Inline defaults emulate a power-on ADXL345 at SPI1 (register proto).
    target_proto   = "register"
    # Register-mode wiring (ADXL345 defaults: bit 7 = R/W, bit 6 = MB, bits 5..0 = reg)
    rw_mask        = 0x80
    mb_mask        = 0x40
    addr_mask      = 0x3F
    read_when_set  = True
    regs = {}
    regs[0x00] = [0xE5]        # DEVID
    regs[0x2C] = [0x0A]        # BW_RATE
    regs[0x2D] = [0x00]        # POWER_CTL
    regs[0x31] = [0x00]        # DATA_FORMAT
    # Stream-mode wiring (unused for register proto).
    rx_stream = []
    # Command-mode wiring (unused for register proto).
    cmd_table = {}
    # === DEVS_BLOCK_END ===

    phase = _new_phase(target_proto)


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
        sr = sr & ~SR_RXNE                     # reading DR clears RXNE
    elif off == DEV_SELECT_OFF:
        request.value = 0
    else:
        request.value = 0


elif request.isWrite:
    off = request.offset
    val = request.value & 0xFFFFFFFF
    if off == OFF_CR1:
        cr1 = val
        if val & CR1_SPE:
            sr = sr | SR_TXE
    elif off == OFF_CR2:
        cr2 = val
    elif off == OFF_SR:
        # W0C-style: let the master clear status bits.
        sr = val & 0xFFFF
    elif off == OFF_DR:
        tx_byte = val & 0xFF
        if trace_path:
            if trace_frame is None:
                trace_frame = {
                    "seq":      trace_seq,
                    "proto":    target_proto,
                    "tx_bytes": [],
                    "rx_bytes": [],
                }
                trace_seq = trace_seq + 1
            trace_frame["tx_bytes"].append(tx_byte)

        handler = _DISPATCHERS.get(target_proto, _register_xfer)
        if target_proto == "register":
            reply = handler(phase, regs, tx_byte,
                            rw_mask, mb_mask, addr_mask, read_when_set)
        elif target_proto == "stream":
            reply = handler(phase, rx_stream, tx_byte)
        elif target_proto == "command":
            reply = handler(phase, cmd_table, tx_byte)
        else:
            reply = 0x00

        # L5 fault injection: override MISO to 0xFF for all bytes of the
        # first N frames. `fault_in_frame` is latched at frame start so the
        # whole frame is corrupted (not just byte 0).
        if fault_in_frame:
            reply = 0xFF

        if trace_path and trace_frame is not None:
            trace_frame["rx_bytes"].append(reply)

        dr_latched = reply & 0xFF
        sr = sr | SR_RXNE | SR_TXE
    elif off == TXN_RESET_OFF:
        # CS asserted (or re-asserted): close out the current trace frame
        # and rearm the phase state.
        if trace_path and trace_frame is not None:
            try:
                f = open(trace_path, "a")
                f.write(json.dumps(trace_frame) + "\n")
                f.close()
            except Exception:
                pass
            trace_frame = None
        phase = _new_phase(target_proto)
        # L5 fault injection: TXN_RESET marks the START of a new CS frame
        # (hw_spi1_cs_lo writes it). Activate fault_in_frame for this
        # frame if budget remains, then consume one unit; otherwise clear
        # fault_in_frame so subsequent frames see the real slave data.
        if fault_frames_remaining > 0:
            fault_in_frame = True
            fault_frames_remaining = fault_frames_remaining - 1
        else:
            fault_in_frame = False
    elif off == DEV_SELECT_OFF:
        # Multi-slave routing currently not supported by this template; the
        # write is accepted for ABI compatibility with hw_spi.h but ignored.
        pass
    # Unknown offsets (CRCPR/RXCRCR/TXCRCR/I2SCFGR/I2SPR): silently ignored.
