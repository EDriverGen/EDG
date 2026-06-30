# coding: ascii
# stm32_usart_hw_slave.py -- Renode Python peripheral
# Emulates STM32F103 USART1 @ 0x40013800 at register level with a
# fixed-length command/response table. When the MCU TX's a complete
# request frame that matches a known command, the corresponding response
# bytes are enqueued for the MCU to RX.
#
# This is a "stream" oracle rather than register-map oracle: the
# per-vector template injects `cmd_response_table` (a dict keyed by
# a tuple of frame bytes). Unknown commands emit the empty response
# (MCU will see read timeouts, useful for negative tests later).
#
# STM32F103 USART register offsets
OFF_SR   = 0x00   # Status (RXNE, TC, TXE, ORE, ...)
OFF_DR   = 0x04   # Data register
OFF_BRR  = 0x08   # Baud rate
OFF_CR1  = 0x0C
OFF_CR2  = 0x10
OFF_CR3  = 0x14
OFF_GTPR = 0x18

# SR bits
SR_RXNE = 1 << 5   # Read-data register not empty
SR_TC   = 1 << 6   # Transmission complete
SR_TXE  = 1 << 7   # Transmit data register empty

UART_FRAME_LEN = 1

if request.isInit:
    sr = SR_TC | SR_TXE
    dr_out = 0
    brr = 0
    cr1 = 0
    cr2 = 0
    cr3 = 0
    gtpr = 0

    # Accumulator for inbound bytes from MCU (i.e. what MCU is TXing).
    cmd_accum = []
    # Queue of bytes pending to be read by MCU via DR (our response).
    rx_queue = []

    # Command/response table. Populated by per-vector template injection
    # (the runner replaces the table declaration below). Default is empty
    # (unmatched commands produce no response).
    # Shape: table[tuple(command bytes)] = [response bytes]
    cmd_response_table = {}

elif request.isRead:
    off = request.offset
    if off == OFF_SR:
        request.value = sr
    elif off == OFF_DR:
        if rx_queue:
            b = rx_queue[0]
            rx_queue = rx_queue[1:]
            dr_out = b & 0xFF
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
        sr = val & 0xFFFF
    elif off == OFF_DR:
        b = val & 0xFF
        cmd_accum = cmd_accum + [b]
        if len(cmd_accum) >= UART_FRAME_LEN:
            key = tuple(cmd_accum[:UART_FRAME_LEN])
            cmd_accum = cmd_accum[UART_FRAME_LEN:]
            resp = cmd_response_table.get(key, [])
            rx_queue = rx_queue + list(resp)
            if rx_queue:
                sr = sr | SR_RXNE
        # Transmit-side flags always ready for next byte
        sr = sr | SR_TC | SR_TXE
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
    # else: ignore
