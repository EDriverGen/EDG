# coding: ascii
# uart_request_response_bot.py -- Renode Python peripheral for UART eval.
#
# Emulates the STM32F103 USART1 register block (SR/DR/BRR/CR1/CR2/CR3/GTPR)
# together with a request/response "bot" behind it. When the MCU writes
# bytes to DR, they accumulate in a buffer; once the buffer matches a
# known request (either by fixed length or by a terminating delimiter
# sequence), the bot looks up the response in `cmd_response_table` and
# queues it back to be read via DR reads.
#
# Framing modes (configured per-stimulus via the DEVS block):
#   "fixed":      request ends after `packet_len` bytes have arrived
#                 (for fixed-length UART request protocols)
#   "delimiter":  request ends when `delimiter` (a list of int bytes) is
#                 a suffix of the accumulator
#
# Lookup semantics: keys in `cmd_response_table` are tuples of int bytes
# matching the accumulator *up to and including* the delimiter (delimiter
# mode) or the full packet (fixed mode). If no match, `default_response`
# is used; if that's also empty, the accumulator is silently dropped and
# the MCU eventually sees a timeout on its read path.
#
# Baud rate / parity / stop bits are accepted-but-ignored -- Renode
# virtual time doesn't enforce UART line timing and our oracle is
# byte-exact, not timing-dependent.
#
# Optional env vars:
#   DRIVERGEN_UART_TRACE_PATH -- JSONL file; one line per completed frame
#                                 (request_hex, response_hex).
#   DRIVERGEN_UART_FAULT_FIRST_N -- L5 fault injection. For the first N
#       completed requests, suppress the response (empty rx_queue) so the
#       driver never sees the expected reply bytes. Simulates "sensor
#       silent" / unpowered / jammed line. A robust driver should time out
#       its read loop and return an error rather than hang forever or
#       report RESULT: PASS with empty data.
#
# The DEVS block delimited by BEGIN/END markers is overwritten per
# stimulus by evaluation/runtime/slave_renderer.render_uart_bot().

# STM32F103 USART register offsets
OFF_SR   = 0x00
OFF_DR   = 0x04
OFF_BRR  = 0x08
OFF_CR1  = 0x0C
OFF_CR2  = 0x10
OFF_CR3  = 0x14
OFF_GTPR = 0x18

# SR bits (subset we care about)
SR_RXNE = 1 << 5   # read-data register not empty
SR_TC   = 1 << 6   # transmission complete
SR_TXE  = 1 << 7   # transmit data register empty
SR_ORE  = 1 << 3   # overrun
SR_IDLE = 1 << 4   # idle line


def _matches_delimiter(buf, delim):
    """Return True iff `delim` is a suffix of `buf`."""
    if not delim:
        return False
    if len(buf) < len(delim):
        return False
    return buf[-len(delim):] == list(delim)


def _try_match_and_respond(buf, table, default_resp):
    """Given the accumulator `buf`, try to look up a response."""
    key = tuple(buf)
    if key in table:
        return (list(table[key]), len(buf))
    if default_resp:
        return (list(default_resp), len(buf))
    return ([], 0)


if request.isInit:
    sr   = SR_TC | SR_TXE
    dr_out = 0
    brr  = 0
    cr1  = 0
    cr2  = 0
    cr3  = 0
    gtpr = 0

    import os
    import json
    trace_path = os.environ.get("DRIVERGEN_UART_TRACE_PATH", "")
    if trace_path:
        try:
            f = open(trace_path, "w")
            f.close()
        except Exception:
            trace_path = ""

    # --- L5 error-injection state (read-only after init) ---
    # DRIVERGEN_UART_FAULT_FIRST_N=<int>: suppress the slave's response for
    # the first N matched request frames so the MCU sees a dead silent
    # sensor. Decremented each time a frame would have been enqueued.
    try:
        fault_frames_remaining = int(
            os.environ.get("DRIVERGEN_UART_FAULT_FIRST_N", "0") or "0"
        )
    except Exception:
        fault_frames_remaining = 0

    cmd_accum = []       # bytes MCU has TX'd so far, pending match
    rx_queue  = []       # bytes queued to be read by MCU

    # === DEVS_BLOCK_BEGIN ===
    # Per-stimulus configuration. slave_renderer overwrites this block.
    framing        = "fixed"       # "fixed" | "delimiter"
    packet_len     = 1             # used when framing == "fixed"
    delimiter      = []            # used when framing == "delimiter"; list of int
    cmd_response_table = {}
    default_response = []          # fallback if no table entry matches
    # === DEVS_BLOCK_END ===


elif request.isRead:
    off = request.offset
    if off == OFF_SR:
        request.value = sr
    elif off == OFF_DR:
        if rx_queue:
            b = rx_queue[0] & 0xFF
            rx_queue = rx_queue[1:]
            dr_out = b
            if not rx_queue:
                sr = sr & ~SR_RXNE
        else:
            dr_out = 0
        request.value = dr_out
    elif off == OFF_BRR:
        request.value = brr
    elif off == OFF_CR1:
        request.value = cr1
    elif off == OFF_CR2:
        request.value = cr2
    elif off == OFF_CR3:
        request.value = cr3
    elif off == OFF_GTPR:
        request.value = gtpr
    else:
        request.value = 0


elif request.isWrite:
    off = request.offset
    val = request.value & 0xFFFFFFFF

    if off == OFF_SR:
        # W0C-style: let CPU clear ORE/IDLE/RXNE etc.
        sr = val & 0xFFFF
    elif off == OFF_DR:
        tx_byte = val & 0xFF
        cmd_accum = cmd_accum + [tx_byte]

        matched = False
        if framing == "fixed":
            while len(cmd_accum) >= packet_len > 0:
                window = cmd_accum[:packet_len]
                resp, consumed = _try_match_and_respond(
                    window, cmd_response_table, default_response,
                )
                cmd_accum = cmd_accum[packet_len:]
                # L5 fault injection: drop the response for the first N frames.
                dropped_by_fault = False
                if fault_frames_remaining > 0:
                    fault_frames_remaining = fault_frames_remaining - 1
                    resp = []
                    dropped_by_fault = True
                if resp:
                    rx_queue = rx_queue + list(resp)
                    matched = True
                if trace_path:
                    try:
                        f = open(trace_path, "a")
                        f.write(json.dumps({
                            "req_hex":  "".join("%02X" % b for b in window),
                            "resp_hex": "".join("%02X" % b for b in resp),
                            "fault":    dropped_by_fault,
                        }) + "\n")
                        f.close()
                    except Exception:
                        pass
        elif framing == "delimiter":
            if _matches_delimiter(cmd_accum, delimiter):
                window = list(cmd_accum)
                resp, consumed = _try_match_and_respond(
                    window, cmd_response_table, default_response,
                )
                cmd_accum = []
                dropped_by_fault = False
                if fault_frames_remaining > 0:
                    fault_frames_remaining = fault_frames_remaining - 1
                    resp = []
                    dropped_by_fault = True
                if resp:
                    rx_queue = rx_queue + list(resp)
                    matched = True
                if trace_path:
                    try:
                        f = open(trace_path, "a")
                        f.write(json.dumps({
                            "req_hex":  "".join("%02X" % b for b in window),
                            "resp_hex": "".join("%02X" % b for b in resp),
                            "fault":    dropped_by_fault,
                        }) + "\n")
                        f.close()
                    except Exception:
                        pass
        # else: unknown framing -> buffer grows unbounded; rely on renderer
        # to reject bad framing at build time.

        if rx_queue:
            sr = sr | SR_RXNE
        sr = sr | SR_TC | SR_TXE
        _ = matched  # quiet linters; matched is informational
    elif off == OFF_BRR:
        brr = val
    elif off == OFF_CR1:
        cr1 = val
    elif off == OFF_CR2:
        cr2 = val
    elif off == OFF_CR3:
        cr3 = val
    elif off == OFF_GTPR:
        gtpr = val
    # else: ignore unknown offsets for robustness.
