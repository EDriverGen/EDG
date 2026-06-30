# coding: ascii
# ow_bitslot_injector.py -- Renode Python peripheral modelling a
# Dallas 1-Wire bit-slot slave (e.g. DS18B20 temperature sensor).
#
# Unlike `gpio_pulse_injector.py` (which replays a fixed pulse-width
# schedule suited to DHT22/HCSR04), this model responds PER BIT SLOT:
#
#   * Reset pulse: the master drives LOW for >= 480 us, then releases.
#     The slave must signal presence by pulling the line LOW on the
#     first subsequent master read. In this model we simply return 0
#     on the next IDR poll after `mcu_release`.
#
#   * Write slot: the master drives LOW briefly (bit=1: ~1 us, bit=0:
#     ~60 us) then releases. The slave does not read the written bit
#     (the sensor payload we play back is fixed); we just track the
#     edges for the L3 canonical-handshake trace.
#
#   * Read slot: the master drives LOW for ~1 us, releases, then polls
#     IDR once within ~15 us. The slave replies with the next bit from
#     its response stream.
#
# Because the driver only POLLS IDR during presence checks and read
# slots (never during its own write slots), a very simple "feed one
# bit per IDR poll from a pre-built response stream" model suffices.
# The stream for a standard ds18b20_read_temp flow is:
#
#   [p0, p1, p2, b0, b1, ..., b71]
#
# where:
#   - p0 is the presence pulse after ds18b20_init's ow_reset (= 0)
#   - p1 is the presence pulse after read_temp's first ow_reset (= 0)
#   - p2 is the presence pulse after read_temp's second ow_reset (= 0)
#   - b0..b71 are the 9 scratchpad bytes streamed LSB-first within each
#     byte, in byte 0..8 order (standard DS18B20 wire format).
#
# Extra polls past the end of the stream return `idle_level` so a
# driver that polls a few extra times for safety simply sees HIGH.
#
# Edge detection is state-based (not transition-based) so it tolerates
# every ChibiOS / RIOT / NuttX / XiUOS / Zephyr / OpenHarmony / HAL
# idiom we've observed (palSetPadMode + palClearPad, BSRR,
# gpio_init(GPIO_OUT) + gpio_clear, etc). An edge fires whenever
# `effective_low = driver_commanded_low AND pin_is_output` flips:
#
#   - False -> True  -> emit "mcu_low"
#   - True  -> False -> emit "mcu_release"
#
# This correctly handles:
#   (a) Driver releases via BSRR "set" (BSRR bit N): commanded_low=False
#       even if pin stays in OUTPUT mode (= open-drain HIGH).
#   (b) Driver releases via mode change to INPUT (CRL/CRH bits = 00):
#       pin_is_output=False, so effective_low flips.
#   (c) Driver writes full ODR word: commanded_low is (val>>N)&1==0.
#
# Initial register state is zero (reset default) and initial
# commanded_low is False, so the first mode-change to OUTPUT does NOT
# spuriously emit mcu_low: effective_low = False AND True = False.
#
# Optional env vars:
#   DRIVERGEN_GPIO_TRACE_PATH -- JSONL path; one line per edge / playback-done.
#   DRIVERGEN_OW_FAULT_FIRST_N -- L5 fault injection. For the first N IDR
#       polls in the `respond` state, return `idle_level` instead of the
#       corresponding `response_bits[poll]` byte. Simulates "no presence
#       pulse" / "slave line stuck high / device not powered": a robust
#       driver should detect the bogus presence (or CRC mismatch on the
#       scratchpad) and return an error rather than report RESULT: PASS.

import os
import json

OFF_CRL  = 0x00
OFF_CRH  = 0x04
OFF_IDR  = 0x08
OFF_ODR  = 0x0C
OFF_BSRR = 0x10
OFF_BRR  = 0x14
OFF_LCKR = 0x18


def _target_mode_bits(crl, crh, pin):
    """Return MODE[1:0] (STM32F1 CRL/CRH) for the target pin.

    MODE == 00 means input; non-zero means output (and output speed).
    """
    if pin < 8:
        shift = (pin & 7) * 4
        return (crl >> shift) & 0x3
    shift = ((pin - 8) & 7) * 4
    return (crh >> shift) & 0x3


if request.isInit:
    crl  = 0
    crh  = 0
    odr  = 0
    lckr = 0

    trace_path = os.environ.get("DRIVERGEN_GPIO_TRACE_PATH", "")
    if trace_path:
        try:
            f = open(trace_path, "w")
            f.close()
        except Exception:
            trace_path = ""

    # Edge-detection state for mcu_low / mcu_release emission.
    prev_effective_low   = False
    driver_commanded_low = False  # last driver write intended LOW on target pin
    pin_is_output        = False  # target pin currently in an output MODE

    # Bus-state used on IDR reads.
    #   "idle"    : master not driving; pull-up => idle_level on IDR.
    #   "low"     : master pulled LOW; IDR reads back 0 regardless.
    #   "respond" : master released; next IDR poll consumes one response bit.
    state = "idle"

    # Response-stream cursor and one-shot done flag.
    poll_count = 0
    playback_done_fired = False

    # --- L5 error-injection state (read-only after init) ---
    # DRIVERGEN_OW_FAULT_FIRST_N=<int>: force the first N `respond` polls
    # to return idle_level instead of response_bits[poll]. Covers both
    # "no presence pulse" (first few polls are presence bits) and
    # "scratchpad garbled" (all-ones returns cause CRC mismatch).
    try:
        fault_polls_remaining = int(
            os.environ.get("DRIVERGEN_OW_FAULT_FIRST_N", "0") or "0"
        )
    except Exception:
        fault_polls_remaining = 0

    # === DEVS_BLOCK_BEGIN ===
    # Per-stimulus configuration. slave_renderer overwrites this block.
    # Defaults emulate DS18B20 at 25.0625 C (datasheet example scratchpad).
    pin_number       = 5
    idle_level       = 1
    n_presence_polls = 3
    # 3 presence zeros + 72 scratchpad bits (LSB-first within each byte).
    # Scratchpad bytes (hex) for 25.0625 C:
    #   0x91 0x01 0x4B 0x46 0x7F 0xFF 0x00 0x10 0x3D
    response_bits    = [
        0, 0, 0,
        1, 0, 0, 0, 1, 0, 0, 1,  # 0x91
        1, 0, 0, 0, 0, 0, 0, 0,  # 0x01
        1, 1, 0, 1, 0, 0, 1, 0,  # 0x4B
        0, 1, 1, 0, 0, 0, 1, 0,  # 0x46
        1, 1, 1, 1, 1, 1, 1, 0,  # 0x7F
        1, 1, 1, 1, 1, 1, 1, 1,  # 0xFF
        0, 0, 0, 0, 0, 0, 0, 0,  # 0x00
        0, 0, 0, 0, 1, 0, 0, 0,  # 0x10
        1, 0, 1, 1, 1, 1, 0, 0,  # 0x3D
    ]
    # === DEVS_BLOCK_END ===

    total_response_polls = len(response_bits)


elif request.isRead:
    off = request.offset

    if off == OFF_IDR:
        if state == "low":
            # Master is actively pulling the line LOW from its own output
            # side; on real HW IDR reads 0 through the pin's input buffer.
            request.value = 0
        elif state == "respond":
            if fault_polls_remaining > 0:
                # L5 fault: pretend the slave is unpowered / line floating
                # high. Still advance poll_count so the playback_done bound
                # eventually fires and we don't loop forever if the driver
                # keeps polling.
                lvl = idle_level & 1
                fault_polls_remaining = fault_polls_remaining - 1
            elif poll_count < total_response_polls:
                lvl = response_bits[poll_count] & 1
            else:
                lvl = idle_level & 1
            poll_count = poll_count + 1
            # Emit playback_done once the final data bit has been consumed.
            if (not playback_done_fired) and poll_count >= total_response_polls:
                playback_done_fired = True
                if trace_path:
                    try:
                        f = open(trace_path, "a")
                        f.write(json.dumps({
                            "event": "playback_done",
                            "total_us": poll_count,
                        }) + "\n")
                        f.close()
                    except Exception:
                        pass
            request.value = (lvl & 1) << pin_number
        else:  # "idle"
            request.value = (idle_level & 1) << pin_number
    elif off == OFF_CRL:
        request.value = crl
    elif off == OFF_CRH:
        request.value = crh
    elif off == OFF_ODR:
        request.value = odr
    elif off == OFF_BSRR:
        request.value = 0
    elif off == OFF_BRR:
        request.value = 0
    elif off == OFF_LCKR:
        request.value = lckr
    else:
        request.value = 0


elif request.isWrite:
    off = request.offset
    val = request.value & 0xFFFFFFFF

    if off == OFF_CRL:
        crl = val
    elif off == OFF_CRH:
        crh = val
    elif off == OFF_ODR:
        odr = val
        bit = (val >> pin_number) & 1
        if bit == 0:
            driver_commanded_low = True
        else:
            driver_commanded_low = False
    elif off == OFF_BSRR:
        set_mask   = 1 << pin_number
        reset_mask = 1 << (pin_number + 16)
        # BSRR "reset bits" take precedence over "set bits" per ST RM0008.
        if val & reset_mask:
            driver_commanded_low = True
        elif val & set_mask:
            driver_commanded_low = False
        # Neither bit touched for target pin -> keep current intent.
    elif off == OFF_BRR:
        if val & (1 << pin_number):
            driver_commanded_low = True
    elif off == OFF_LCKR:
        lckr = val
    # Ignore unknown offsets.

    # Recompute target pin's direction after any CRL/CRH touch (and
    # re-evaluate effective_low after any register write).
    mode_bits = _target_mode_bits(crl, crh, pin_number)
    pin_is_output = (mode_bits != 0)

    effective_low = driver_commanded_low and pin_is_output

    if effective_low and not prev_effective_low:
        state = "low"
        if trace_path:
            try:
                f = open(trace_path, "a")
                f.write(json.dumps({"event": "mcu_low"}) + "\n")
                f.close()
            except Exception:
                pass
    elif (not effective_low) and prev_effective_low:
        state = "respond"
        if trace_path:
            try:
                f = open(trace_path, "a")
                f.write(json.dumps({"event": "mcu_release"}) + "\n")
                f.close()
            except Exception:
                pass

    prev_effective_low = effective_low
