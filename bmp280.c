#include "bmp280.h"
#include "i2c.h"
#include "systick.h"

/* ========== 校准数据（init 时填充） ========== */
static uint8_t  cal[26];
static int32_t  t_fine;

/* 辅助：从 cal[] 取 uint16 */
static uint16_t cal_u16(int idx) {
    return cal[idx] | (cal[idx + 1] << 8);
}

/* 辅助：从 cal[] 取 int16 */
static int16_t cal_s16(int idx) {
    return (int16_t)(cal[idx] | (cal[idx + 1] << 8));
}

/* ========== 初始化 ========== */
void bmp280_init(void) {
    // 读 ID 验证
    i2c_send_byte(I2C1, 0x76, 0xD0);
    uint8_t id;
    i2c_read_bytes(I2C1, 0x76, 1, &id);

    // 读 26 字节校准系数
    i2c_send_byte(I2C1, 0x76, 0x88);
    i2c_read_bytes(I2C1, 0x76, 26, cal);

    // 写控制寄存器开机
     i2c_write_reg(I2C1, 0x76, 0xF4,0x27);
    systick_delay_ms(15);
}

/* ========== 读温度 ========== */
float bmp280_read_temp(void) {
    i2c_send_byte(I2C1, 0x76, 0xFA);
    uint8_t tbuf[3];
    i2c_read_bytes(I2C1, 0x76, 3, tbuf);

    int32_t temp_raw = ((int32_t)tbuf[0] << 12)
                     | ((int32_t)tbuf[1] << 4)
                     | (tbuf[2] >> 4);

    int32_t var1 = ((((temp_raw >> 3) - ((int32_t)cal_u16(0) << 1)))
                  * ((int32_t)cal_s16(2))) >> 11;
    int32_t var2 = (((((temp_raw >> 4) - ((int32_t)cal_u16(0)))
                  * ((temp_raw >> 4) - ((int32_t)cal_u16(0)))) >> 12)
                  * ((int32_t)cal_s16(4))) >> 14;
    t_fine = var1 + var2;
    int32_t temp_c = (t_fine * 5 + 128) >> 8;
    return temp_c / 100.0f;
}

/* ========== 读气压 ========== */
float bmp280_read_press(void) {
    i2c_send_byte(I2C1, 0x76, 0xF7);
    uint8_t d[6];
    i2c_read_bytes(I2C1, 0x76, 6, d);

    uint32_t press_raw = (d[0] << 12) | (d[1] << 4) | (d[2] >> 4);
    int32_t  temp_raw  = (d[3] << 12) | (d[4] << 4) | (d[5] >> 4);

    // 重算 t_fine（保证气压不依赖 read_temp 调用顺序）
    int32_t var1 = ((((temp_raw >> 3) - ((int32_t)cal_u16(0) << 1)))
                  * ((int32_t)cal_s16(2))) >> 11;
    int32_t var2 = (((((temp_raw >> 4) - ((int32_t)cal_u16(0)))
                  * ((temp_raw >> 4) - ((int32_t)cal_u16(0)))) >> 12)
                  * ((int32_t)cal_s16(4))) >> 14;
    t_fine = var1 + var2;

    // 气压补偿公式
    int64_t v1 = ((int64_t)t_fine) - 128000;
    int64_t v2 = v1 * v1 * (int64_t)cal_s16(16);
    v2 = v2 + ((v1 * (int64_t)cal_s16(14)) << 17);
    v2 = v2 + (((int64_t)cal_s16(12)) << 35);
    v1 = ((v1 * v1 * (int64_t)cal_s16(10)) >> 8)
       + ((v1 * (int64_t)cal_s16(8)) << 12);
    v1 = (((((int64_t)1) << 47) + v1)) * ((int64_t)cal_u16(6)) >> 33;

    int64_t p;
    if (v1 == 0) {
        p = 0;
    } else {
        p = 1048576 - (int64_t)press_raw;
        p = (((p << 31) - v2) * 3125) / v1;
        v1 = (((int64_t)cal_s16(22)) * (p >> 13) * (p >> 13)) >> 25;
        v2 = (((int64_t)cal_s16(20)) * p) >> 19;
        p = ((p + v1 + v2) >> 8) + (((int64_t)cal_s16(18)) << 4);
    }

    return (float)((uint32_t)(p >> 8)) / 100.0f;
}
