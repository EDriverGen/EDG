# coding: ascii
# gpio_pulse_injector.py -- Renode Python peripheral for pulse-timing
# devices driven over a single STM32 GPIO pin (DHT11/DHT22, DS18B20
# 1-wire, ultrasonic echo, PWM stimuli, etc).
#
# Model: emulates one GPIOx register bank (CRL/CRH/IDR/ODR/BSRR/BRR/
# LCKR). The adapter's bus_init picks the target pin via the
# `DRIVERGEN_GPIO_PIN_NUMBER` compile-time macro; the peripheral block
# is mapped at the port's base address via the platform .repl. Only one
# pin per peripheral instance is actively modeled -- the rest read as 0.
#
# Because Renode Python peripherals do not expose microsecond virtual
# time in a portable way, timing is measured in *MCU read calls*: each
# read of IDR advances an internal `playback_cursor` by `tick_us`
# microseconds, and the cursor index into `pulse_schedule` determines
# the currently-asserted level. This assumes the driver polls the pin
# in a tight loop, which is the standard 1-wire driver pattern.
# Resolution is therefore ~tick_us * (reads_per_bit), typically 1-5 us
# per read on 72 MHz STM32 Cortex-M3. D2 accepts "~100 us precision" --
# this comfortably covers DHT22 (50/70/27 us bit slots) and DS18B20
# (60+ us presence pulse, 15+ us slot).
#
# Protocol state machine:
#   "idle":      pin at idle_level. MCU may drive. For dual-pin devices
#                (e.g. HCSR04 trig != echo) the trigger pulse is a brief
#                HIGH; for single-pin devices (DHT22, DS18B20) the
#                trigger is a LOW start pulse on the data pin itself.
#   "trig_high": dual-pin only. MCU asserted trig HIGH; waiting for the
#                falling edge of the trigger pulse to open a session.
#                Arriving directly from "idle"/"done" on a SET transition
#                avoids mis-counting any LOW-state assertions the driver
#                performs as part of init / de-assert-and-hold setup.
#   "mcu_low":   single-pin: MCU drove pin LOW (start signal); or
#                dual-pin: trigger pulse completed, sensor response is
#                imminent. Waiting for the cue to start playback.
#   "playback":  sensor response is being replayed.
#   "done":      schedule exhausted -> pin returns to idle_level.
#
# "playback_done" semantics: fires when the cursor crosses into the LAST
# pulse-schedule segment, because by convention the last segment is the
# "bus release" trailing pulse (e.g. DHT22 trailing LOW) that a correctly
# written driver never polls -- the driver exits its loop as soon as it
# detects the transition INTO this trailing segment. Waiting for the
# cursor to reach `total_dur` would therefore miss the event for every
# well-behaved driver. For single-segment schedules (no trailing release
# pulse) we fall back to the old `cursor >= total_dur` rule.
#
# Optional env var DRIVERGEN_GPIO_TRACE_PATH writes a JSONL transcript
# of triggers (mcu_low/mcu_release/playback-done) for debugging.
#
# DRIVERGEN_GPIO_FAULT_FIRST_N -- L5 fault injection. For the first N
# "start pulse" sessions (each MCU LOW -> HIGH release cycle) the
# injector swallows the trigger and holds the pin at idle_level instead
# of replaying `pulse_schedule`. Simulates an unpowered / unwired sensor.
# A robust driver should time out its response-edge wait and return an
# error rather than hang or report RESULT: PASS with stale data.

import os
import json

# STM32F103 GPIO register offsets (relative to GPIOx_BASE)
OFF_CRL  = 0x00
OFF_CRH  = 0x04
OFF_IDR  = 0x08
OFF_ODR  = 0x0C
OFF_BSRR = 0x10
OFF_BRR  = 0x14
OFF_LCKR = 0x18


def _bsrr_to_pin_changes(val, pin):
    """Decode BSRR: bit N sets pin N HIGH; bit N+16 sets pin N LOW.

    Returns "set" | "reset" | None for our tracked pin.
    """
    set_mask   = 1 << pin
    reset_mask = 1 << (pin + 16)
    if val & reset_mask:
        return "reset"
    if val & set_mask:
        return "set"
    return None


def _brr_to_pin_changes(val, pin):
    """Decode BRR: bit N resets pin N LOW."""
    if val & (1 << pin):
        return "reset"
    return None


def _odr_pin_level(val, pin):
    """Decode ODR: bit N is pin N's output level."""
    return (val >> pin) & 1


def _resolve_level_at_cursor(schedule, cursor_us, idle_level):
    """Given a pulse schedule [(level, duration_us), ...] and a cursor
    position in microseconds, return the level asserted at that point.
    Once the cursor exceeds the total schedule duration, returns
    `idle_level`.
    """
    t_acc = 0
    for level, dur in schedule:
        t_acc += dur
        if cursor_us < t_acc:
            return level & 1
    return idle_level & 1


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

    state                 = "idle"
    playback_cursor       = 0   # microseconds since playback start
    schedule_index        = 0   # not strictly needed but useful for debug
    playback_done_fired   = False  # one-shot guard for the trace event

    # --- L5 error-injection state (read-only after init) ---
    # DRIVERGEN_GPIO_FAULT_FIRST_N=<int>: silently swallow the first N
    # start-pulse sessions (MCU LOW -> release -> playback transitions).
    # Instead of replaying `pulse_schedule`, we keep the line at
    # idle_level so the driver's response-edge wait times out.
    try:
        fault_sessions_remaining = int(
            os.environ.get("DRIVERGEN_GPIO_FAULT_FIRST_N", "0") or "0"
        )
    except Exception:
        fault_sessions_remaining = 0

    # === DEVS_BLOCK_BEGIN ===
    # Per-stimulus preload (rendered by slave_renderer).
    pin_number      = 1
    trig_pin_number = 0
    idle_level      = 1
    tick_us         = 1

    pulse_schedule  = [
        (1, 400),
        (0, 1),
    ]
    # === DEVS_BLOCK_END ===

    # Resolve effective trigger pin
    _eff_trig_pin = trig_pin_number if trig_pin_number >= 0 else pin_number
    # dual-pin if trig and data pins differ; controls trigger-edge semantics.
    dual_pin = (_eff_trig_pin != pin_number)


elif request.isRead:
    off = request.offset
    if off == OFF_IDR:
        # Advance virtual time on every IDR read.
        if state == "playback":
            lvl = _resolve_level_at_cursor(pulse_schedule, playback_cursor, idle_level)
            playback_cursor = playback_cursor + tick_us
            total_dur = sum(d for (_, d) in pulse_schedule)
            # The last schedule segment is conventionally the "bus release"
            # trailing pulse that a correct driver detects on the edge but
            # never polls all the way through. Fire `playback_done` for the
            # L3 trace AS SOON AS the cursor enters that last segment --
            # but DO NOT yet flip the state machine to "done", otherwise
            # subsequent reads would return idle_level instead of the
            # trailing pulse level and the driver's wait-for-edge would
            # miss the falling edge of the data phase.
            if len(pulse_schedule) >= 2:
                data_dur = total_dur - pulse_schedule[-1][1]
            else:
                data_dur = total_dur
            if (not playback_done_fired) and playback_cursor >= data_dur:
                playback_done_fired = True
                if trace_path:
                    try:
                        f = open(trace_path, "a")
                        f.write(json.dumps({
                            "event": "playback_done",
                            "total_us": data_dur,
                        }) + "\n")
                        f.close()
                    except Exception:
                        pass
            if playback_cursor >= total_dur:
                state = "done"
            request.value = (lvl & 1) << pin_number
        elif state == "mcu_low":
            # For dual-pin devices (e.g. HCSR04: trig != echo), auto-transition
            # to playback on first IDR read since MCU won't SET the trig pin again.
            if trig_pin_number >= 0 and trig_pin_number != pin_number:
                if fault_sessions_remaining > 0:
                    # L5 fault: swallow this session. Go straight to "done"
                    # so the pin reads idle_level and the driver times out.
                    fault_sessions_remaining = fault_sessions_remaining - 1
                    state = "done"
                    if trace_path:
                        try:
                            f = open(trace_path, "a")
                            f.write(json.dumps({"event": "fault_swallow"}) + "\n")
                            f.close()
                        except Exception:
                            pass
                    request.value = (idle_level & 1) << pin_number
                else:
                    state = "playback"
                    playback_cursor = 0
                    schedule_index  = 0
                    if trace_path:
                        try:
                            f = open(trace_path, "a")
                            f.write(json.dumps({"event": "mcu_release_auto"}) + "\n")
                            f.close()
                        except Exception:
                            pass
                    lvl = _resolve_level_at_cursor(pulse_schedule, playback_cursor, idle_level)
                    playback_cursor = playback_cursor + tick_us
                    request.value = (lvl & 1) << pin_number
            else:
                # MCU is holding the pin LOW from the output side; IDR sees 0.
                request.value = 0
        else:
            # idle or done -> pull-up
            request.value = (idle_level & 1) << pin_number
    elif off == OFF_CRL:
        request.value = crl
    elif off == OFF_CRH:
        request.value = crh
    elif off == OFF_ODR:
        request.value = odr
    elif off == OFF_BSRR:
        request.value = 0  # BSRR reads back 0 on real MCU
    elif off == OFF_BRR:
        request.value = 0
    elif off == OFF_LCKR:
        request.value = lckr
    else:
        request.value = 0


elif request.isWrite:
    off = request.offset
    val = request.value & 0xFFFFFFFF

    pin_transition = None
    if off == OFF_ODR:
        odr = val
        lvl = _odr_pin_level(val, _eff_trig_pin)
        pin_transition = "reset" if lvl == 0 else "set"
    elif off == OFF_BSRR:
        pin_transition = _bsrr_to_pin_changes(val, _eff_trig_pin)
    elif off == OFF_BRR:
        pin_transition = _brr_to_pin_changes(val, _eff_trig_pin)
    elif off == OFF_CRL:
        crl = val
    elif off == OFF_CRH:
        crh = val
    elif off == OFF_LCKR:
        lckr = val
    # Ignore unknown offsets.

    if dual_pin:
        # Dual-pin devices (HCSR04, etc.) use a HIGH trigger pulse
        # (SET -> delay -> RESET) to request a sensor response. We must
        # require BOTH the SET (rising edge) and the subsequent RESET
        # (falling edge) before counting a session, otherwise any
        # init-time trig=LOW assertion would be mis-counted as a session
        # start and consume the L5 fault budget before the driver gets
        # to issue its first real trigger.
        if pin_transition == "set":
            if state in ("idle", "done"):
                state = "trig_high"
                if trace_path:
                    try:
                        f = open(trace_path, "a")
                        f.write(json.dumps({"event": "trig_high"}) + "\n")
                        f.close()
                    except Exception:
                        pass
        elif pin_transition == "reset":
            if state == "trig_high":
                # Trigger pulse completed: session starts. The read-side
                # fast-path (dual_pin + state=="mcu_low") either enters
                # playback on the next IDR read or swallows the session
                # when L5 fault budget remains.
                state = "mcu_low"
                if trace_path:
                    try:
                        f = open(trace_path, "a")
                        f.write(json.dumps({"event": "mcu_low"}) + "\n")
                        f.close()
                    except Exception:
                        pass
            # else: ignore stray RESETs (init-time setup, etc.).
    else:
        # Single-pin devices (DHT22, DS18B20) signal a request with a
        # LOW pulse on the data pin; the subsequent release (SET) is the
        # cue for the sensor response.
        if pin_transition == "reset":
            if state in ("idle", "done"):
                state = "mcu_low"
                if trace_path:
                    try:
                        f = open(trace_path, "a")
                        f.write(json.dumps({"event": "mcu_low"}) + "\n")
                        f.close()
                    except Exception:
                        pass
        elif pin_transition == "set":
            if state == "mcu_low":
                if fault_sessions_remaining > 0:
                    # L5 fault: swallow this session. Skip the playback
                    # phase and return to idle so the pin stays at
                    # idle_level and the driver times out waiting for the
                    # response edge.
                    fault_sessions_remaining = fault_sessions_remaining - 1
                    state = "done"
                    if trace_path:
                        try:
                            f = open(trace_path, "a")
                            f.write(json.dumps({"event": "fault_swallow"}) + "\n")
                            f.close()
                        except Exception:
                            pass
                else:
                    state = "playback"
                    playback_cursor = 0
                    schedule_index  = 0
                    if trace_path:
                        try:
                            f = open(trace_path, "a")
                            f.write(json.dumps({"event": "mcu_release"}) + "\n")
                            f.close()
                        except Exception:
                            pass
