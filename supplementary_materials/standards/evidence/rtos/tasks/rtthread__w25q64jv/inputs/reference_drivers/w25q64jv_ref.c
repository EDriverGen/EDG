/*
 * W25Q64JV 64M-bit SPI NOR Flash reference driver for RT-Thread.
 */
#include "w25q64jv_ref.h"

/* ------------------------------------------------------------------ */
static int w25q64jv_wait_ready(struct w25q64jv_device *dev)
{
    uint8_t cmd = W25Q64_CMD_READ_SR1;
    uint8_t sr1;

    for (int i = 0; i < 10000; i++) {
        rt_spi_send_then_recv(dev->spi, &cmd, 1, &sr1, 1);
        if (!(sr1 & W25Q64_SR1_BUSY))
            return 0;
        rt_thread_mdelay(1);
    }
    return -RT_ETIMEOUT;
}

/* ------------------------------------------------------------------ */
static rt_err_t w25q64jv_write_enable(struct w25q64jv_device *dev)
{
    uint8_t cmd = W25Q64_CMD_WRITE_ENABLE;
    return rt_spi_send(dev->spi, &cmd, 1);
}

/* ------------------------------------------------------------------ */
rt_err_t w25q64jv_init(struct w25q64jv_device *dev,
                       const char *spi_device_name)
{
    if (dev == RT_NULL || spi_device_name == RT_NULL)
        return -RT_EINVAL;

    dev->spi = (struct rt_spi_device *)rt_device_find(spi_device_name);
    if (dev->spi == RT_NULL)
        return -RT_ERROR;

    struct rt_spi_configuration cfg = {
        .mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB,
        .data_width = 8,
        .max_hz = W25Q64JV_SPI_MAX_HZ,
    };
    rt_spi_configure(dev->spi, &cfg);

    return RT_EOK;
}

/* ------------------------------------------------------------------ */
rt_err_t w25q64jv_probe(struct w25q64jv_device *dev)
{
    uint8_t cmd = W25Q64_CMD_JEDEC_ID;
    uint8_t id[3];
    if (rt_spi_send_then_recv(dev->spi, &cmd, 1, id, 3) != RT_EOK)
        return -RT_ERROR;
    if (id[0] != W25Q64_JEDEC_MFR_WINBOND)
        return -RT_ERROR;
    return RT_EOK;
}

/* ------------------------------------------------------------------ */
rt_err_t w25q64jv_read(struct w25q64jv_device *dev, uint32_t addr,
                       uint8_t *buf, uint32_t len)
{
    if (dev == RT_NULL || buf == RT_NULL || len == 0)
        return -RT_EINVAL;

    uint8_t cmd[4];
    cmd[0] = W25Q64_CMD_READ_DATA;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);

    if (rt_spi_send_then_recv(dev->spi, cmd, 4, buf, len) != RT_EOK)
        return -RT_ERROR;
    return RT_EOK;
}

/* ------------------------------------------------------------------ */
rt_err_t w25q64jv_write(struct w25q64jv_device *dev, uint32_t addr,
                        const uint8_t *buf, uint32_t len)
{
    if (dev == RT_NULL || buf == RT_NULL || len == 0)
        return -RT_EINVAL;

    uint32_t offset = 0;
    while (offset < len) {
        uint32_t page_rem = W25Q64JV_PAGE_SIZE -
                            ((addr + offset) % W25Q64JV_PAGE_SIZE);
        uint32_t chunk = (len - offset < page_rem) ? (len - offset)
                                                   : page_rem;

        w25q64jv_write_enable(dev);

        uint8_t hdr[4];
        hdr[0] = W25Q64_CMD_PAGE_PROGRAM;
        hdr[1] = (uint8_t)((addr + offset) >> 16);
        hdr[2] = (uint8_t)((addr + offset) >> 8);
        hdr[3] = (uint8_t)(addr + offset);

        rt_spi_send(dev->spi, hdr, 4);
        if (rt_spi_send(dev->spi, buf + offset, chunk) != RT_EOK)
            return -RT_ERROR;

        if (w25q64jv_wait_ready(dev) != 0)
            return -RT_ETIMEOUT;

        offset += chunk;
    }
    return RT_EOK;
}

/* ------------------------------------------------------------------ */
rt_err_t w25q64jv_erase_sector(struct w25q64jv_device *dev, uint32_t addr)
{
    w25q64jv_write_enable(dev);

    uint8_t cmd[4];
    cmd[0] = W25Q64_CMD_SECTOR_ERASE;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);
    if (rt_spi_send(dev->spi, cmd, 4) != RT_EOK)
        return -RT_ERROR;

    return w25q64jv_wait_ready(dev) == 0 ? RT_EOK : -RT_ETIMEOUT;
}
