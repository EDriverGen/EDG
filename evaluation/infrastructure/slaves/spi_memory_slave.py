# coding: ascii
# spi_memory_slave.py -- Renode Python peripheral for SPI memory devices.
#
# Emulates STM32F103 SPI1 controller register block AT 0x40013000 together
# with a SPI NOR flash memory responding to JEDEC ID (0x9F), Read SR1
# (0x05), Write Enable (0x06), Read Data (0x03), Page Program (0x02), and
# Sector Erase (0x20). The memory array is preloaded from the DEVS block's
# `memory_bytes` dict (byte-address -> byte-value).

# ---- STM32F103 SPI1 register offsets ----
OFF_CR1    = 0x00
OFF_CR2    = 0x04
OFF_SR     = 0x08
OFF_DR     = 0x0C
OFF_CRCPR  = 0x10
OFF_RXCRCR = 0x14
OFF_TXCRCR = 0x18

TXN_RESET_OFF  = 0x80
DEV_SELECT_OFF = 0x84

CR1_SPE  = 1 << 6
SR_RXNE  = 1 << 0
SR_TXE   = 1 << 1

CMD_JEDEC_ID      = 0x9F
CMD_READ_SR1      = 0x05
CMD_WRITE_ENABLE  = 0x06
CMD_READ_DATA     = 0x03
CMD_PAGE_PROGRAM  = 0x02
CMD_SECTOR_ERASE  = 0x20


if request.isInit:
    cr1   = 0
    cr2   = 0
    sr    = SR_TXE
    dr    = 0

    frame = {"state": "idle", "opcode": 0, "addr": 0, "addr_count": 0,
             "tx_buf": [], "tx_idx": 0, "rx_buf": []}
    write_enable = False
    memory_bytes = {}

    import os, json
    trace_path = os.environ.get("DRIVERGEN_SPI_TRACE_PATH", "")
    trace_frames = []
    trace_tx = []
    trace_rx = []
    if trace_path:
        try: open(trace_path, "w").close()
        except Exception: trace_path = ""

    # === DEVS_BLOCK_BEGIN ===
    jedec_id     = [0xEF, 0x40, 0x17]
    memory_size  = 8388608
    page_size    = 256
    sector_size  = 4096
    memory_bytes = {}
    # === DEVS_BLOCK_END ===

    for _base_str, _bytes_list in memory_bytes.items():
        _base = int(str(_base_str), 0)
        for _i, _b in enumerate(_bytes_list):
            memory_bytes[_base + _i] = int(_b) & 0xFF

# ---- Register-level handler (runs every request) ----
if request.offset >= TXN_RESET_OFF:
    if request.offset == TXN_RESET_OFF:
        op = frame["opcode"]
        if op == CMD_PAGE_PROGRAM and frame["rx_buf"]:
            a = frame["addr"]
            for _i, _b in enumerate(frame["rx_buf"]):
                memory_bytes[(a + _i) & 0xFFFFFF] = _b & 0xFF
        if trace_path and trace_tx:
            trace_frames.append({"tx_bytes": list(trace_tx),
                                 "rx_bytes": list(trace_rx)})
            trace_tx = []; trace_rx = []
        frame["state"] = "opcode"; frame["opcode"] = 0
        frame["addr"] = 0; frame["addr_count"] = 0
        frame["tx_buf"] = []; frame["tx_idx"] = 0
        frame["rx_buf"] = []
    request.value = 0

elif request.isRead:
    if request.offset == OFF_SR:
        request.value = sr
    elif request.offset == OFF_DR:
        request.value = dr
        sr &= ~SR_RXNE
    else:
        request.value = 0

else:
    if request.offset == OFF_CR1:
        if request.value & CR1_SPE:
            cr1 = request.value
    elif request.offset == OFF_DR:
        tx_byte = request.value & 0xFF
        trace_tx.append(tx_byte)
        reply = 0x00
        st = frame["state"]

        if st == "opcode":
            frame["opcode"] = tx_byte
            if tx_byte == CMD_JEDEC_ID:
                frame["tx_buf"] = list(jedec_id); frame["tx_idx"] = 0
                frame["state"] = "data_tx"
            elif tx_byte == CMD_READ_SR1:
                frame["tx_buf"] = [0x00]; frame["tx_idx"] = 0
                frame["state"] = "data_tx"
            elif tx_byte == CMD_WRITE_ENABLE:
                write_enable = True; frame["state"] = "idle"
            elif tx_byte in (CMD_READ_DATA, CMD_PAGE_PROGRAM, CMD_SECTOR_ERASE):
                frame["addr"] = 0; frame["addr_count"] = 0
                frame["state"] = "addr"
            else:
                frame["state"] = "idle"

        elif st == "addr":
            frame["addr"] = ((frame["addr"] << 8) | tx_byte) & 0xFFFFFF
            frame["addr_count"] += 1
            if frame["addr_count"] >= 3:
                op = frame["opcode"]
                a = frame["addr"]
                if op == CMD_READ_DATA:
                    buf = []
                    for _i in range(min(256, memory_size - a)):
                        buf.append(memory_bytes.get(a + _i, 0xFF))
                    frame["tx_buf"] = buf; frame["tx_idx"] = 0
                    frame["state"] = "data_tx"
                elif op == CMD_PAGE_PROGRAM:
                    frame["rx_buf"] = []; frame["state"] = "data_rx"
                elif op == CMD_SECTOR_ERASE:
                    _base = a & ~(sector_size - 1)
                    for _i in range(sector_size):
                        memory_bytes[_base + _i] = 0xFF
                    frame["state"] = "idle"

        elif st == "data_tx":
            buf = frame["tx_buf"]; idx = frame["tx_idx"]
            if idx < len(buf):
                reply = buf[idx]; frame["tx_idx"] = idx + 1

        elif st == "data_rx":
            frame["rx_buf"].append(tx_byte)

        dr = reply; sr |= SR_RXNE
