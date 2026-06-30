static int dps310_wait_ready(struct dps310_device *dev, uint8_t mask, uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    uint8_t meas_cfg = 0;
    while (elapsed < timeout_ms) {
        if (dps310_read_registers(dev, DPS310_REG_MEAS_CFG, &meas_cfg, 1) != 0) return -1;
        if ((meas_cfg & mask) != 0) return 0;
        dps310_delay_ms(10);
        elapsed += 10;
    }
    return -1;
}

static int32_t dps310_twos_complement_24(uint32_t raw)
{
    return (raw & 0x800000U) ? (int32_t)(raw | 0xFF000000U) : (int32_t)raw;
}

static void dps310_parse_coefficients(const uint8_t raw[18], struct dps310_calib_coeff *coeff)
{
    coeff->c0 = (int32_t)(((uint16_t)raw[0] << 4) | ((uint16_t)raw[1] >> 4));
    if (coeff->c0 & 0x0800) coeff->c0 |= 0xFFFFF000;
    coeff->c1 = (int32_t)((((uint16_t)raw[1] & 0x0F) << 8) | (uint16_t)raw[2]);
    if (coeff->c1 & 0x0800) coeff->c1 |= 0xFFFFF000;
    coeff->c00 = (int32_t)(((uint32_t)raw[3] << 12) | ((uint32_t)raw[4] << 4) | ((uint32_t)raw[5] >> 4));
    if (coeff->c00 & 0x80000) coeff->c00 |= 0xFFF00000;
    coeff->c10 = (int32_t)((((uint32_t)raw[5] & 0x0F) << 16) | ((uint32_t)raw[6] << 8) | (uint32_t)raw[7]);
    if (coeff->c10 & 0x80000) coeff->c10 |= 0xFFF00000;
    coeff->c01 = (int16_t)(((uint16_t)raw[8] << 8) | raw[9]);
    coeff->c11 = (int16_t)(((uint16_t)raw[10] << 8) | raw[11]);
    coeff->c20 = (int16_t)(((uint16_t)raw[12] << 8) | raw[13]);
    coeff->c21 = (int16_t)(((uint16_t)raw[14] << 8) | raw[15]);
    coeff->c30 = (int16_t)(((uint16_t)raw[16] << 8) | raw[17]);
}

int dps310_probe(struct dps310_device *dev)
{
    uint8_t id = 0;
    if (dps310_read_registers(dev, DPS310_REG_PRODUCT_ID, &id, 1) != 0) return -1;
    return ((id & 0xF0U) == DPS310_PRODUCT_ID) ? 0 : -1;
}

int dps310_reset(struct dps310_device *dev)
{
    if (dps310_write_register(dev, DPS310_REG_RESET, DPS310_RESET_SOFT) != 0) return -1;
    dps310_delay_ms(40);
    return dps310_wait_ready(dev, DPS310_MEAS_CFG_SENSOR_RDY | DPS310_MEAS_CFG_COEF_RDY, 500);
}

int dps310_read_calibration(struct dps310_device *dev)
{
    uint8_t raw[18];
    uint8_t coef_srce = 0;
    if (dev == 0) return -1;
    if (dps310_read_registers(dev, DPS310_REG_COEF, raw, 18) != 0) return -1;
    dps310_parse_coefficients(raw, &dev->coeff);
    if (dps310_read_registers(dev, DPS310_REG_COEF_SRCE, &coef_srce, 1) != 0) return -1;
    if ((coef_srce & 0x80U) != 0) {
        uint8_t tmp_cfg = 0;
        if (dps310_read_registers(dev, DPS310_REG_TMP_CFG, &tmp_cfg, 1) != 0) return -1;
        tmp_cfg |= 0x80U;
        if (dps310_write_register(dev, DPS310_REG_TMP_CFG, tmp_cfg) != 0) return -1;
    }
    return 0;
}

int dps310_read_temperature(struct dps310_device *dev, int32_t *temp_c100)
{
    uint8_t data[3];
    int32_t raw_temp;
    double t_raw_sc;
    double t_comp;
    if (temp_c100 == 0) return -1;
    if (dps310_write_register(dev, DPS310_REG_MEAS_CFG, DPS310_MODE_TMP_SINGLE) != 0) return -1;
    if (dps310_wait_ready(dev, DPS310_MEAS_CFG_TMP_RDY, 200) != 0) return -1;
    if (dps310_read_registers(dev, DPS310_REG_TMP_B2, data, 3) != 0) return -1;
    raw_temp = dps310_twos_complement_24(((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2]);
    t_raw_sc = (double)raw_temp / (double)dev->kT;
    t_comp = (double)dev->coeff.c0 * 0.5 + (double)dev->coeff.c1 * t_raw_sc;
    *temp_c100 = (int32_t)(t_comp * 100.0);
    return 0;
}

int dps310_read_pressure(struct dps310_device *dev, int32_t *pressure_pa100)
{
    uint8_t data[3];
    uint8_t tdata[3];
    int32_t raw_prs;
    int32_t raw_temp;
    int32_t temp_c100 = 0;
    double p_raw_sc;
    double t_raw_sc;
    double p_comp;
    if (pressure_pa100 == 0) return -1;
    if (dps310_read_temperature(dev, &temp_c100) != 0) return -1;
    if (dps310_write_register(dev, DPS310_REG_MEAS_CFG, DPS310_MODE_PRS_SINGLE) != 0) return -1;
    if (dps310_wait_ready(dev, DPS310_MEAS_CFG_PRS_RDY, 200) != 0) return -1;
    if (dps310_read_registers(dev, DPS310_REG_PSR_B2, data, 3) != 0) return -1;
    raw_prs = dps310_twos_complement_24(((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2]);
    p_raw_sc = (double)raw_prs / (double)dev->kP;
    if (dps310_read_registers(dev, DPS310_REG_TMP_B2, tdata, 3) != 0) return -1;
    raw_temp = dps310_twos_complement_24(((uint32_t)tdata[0] << 16) | ((uint32_t)tdata[1] << 8) | tdata[2]);
    t_raw_sc = (double)raw_temp / (double)dev->kT;
    p_comp = (double)dev->coeff.c00 +
             p_raw_sc * ((double)dev->coeff.c10 +
                         p_raw_sc * ((double)dev->coeff.c20 +
                                     p_raw_sc * (double)dev->coeff.c30)) +
             t_raw_sc * (double)dev->coeff.c01 +
             t_raw_sc * p_raw_sc * ((double)dev->coeff.c11 +
                                    p_raw_sc * (double)dev->coeff.c21);
    *pressure_pa100 = (int32_t)(p_comp * 100.0);
    return 0;
}
