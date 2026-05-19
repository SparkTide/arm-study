#include "protocol.h"
#include <string.h>

/**
 * @brief  CRC8 计算 (多项式 0x07, 初值 0x00)
 * @param  data  待计算数据指针
 * @param  len   数据长度 (字节)
 * @retval CRC8 校验值
 */
uint8_t crc8_calc(const uint8_t *data, uint8_t len)
{
    uint8_t crc = CRC8_INIT;

    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (uint8_t)((crc << 1) ^ CRC8_POLY);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * @brief  帧封装 — 按协议拼装完整帧
 *
 *  帧格式: AA 55 | SensorID | TypeLen | Payload (0~15B) | CRC8
 *
 * @param  buf   输出缓冲区 (调用方保证 ≥ FRAME_MAX_SIZE)
 * @param  frame 待封装的传感器帧
 * @retval 完整帧长度 (5 + payload_len, 范围 5~20)
 */
uint8_t pack_frame(uint8_t *buf, const SensorFrame *frame)
{
    uint8_t idx = 0;
    uint8_t crc;

    /* 帧头 */
    buf[idx++] = FRAME_HEADER1;
    buf[idx++] = FRAME_HEADER2;

    /* 传感器 ID */
    buf[idx++] = frame->sensor_id;

    /* TypeLen */
    buf[idx++] = MAKE_TYPELEN(frame->dtype, frame->payload_len);

    /* 负载 */
    if (frame->payload_len > 0) {
        memcpy(&buf[idx], frame->payload, frame->payload_len);
        idx += frame->payload_len;
    }

    /* CRC8 — 对帧头1 到 负载末 的所有字节计算 */
    crc = crc8_calc(buf, idx);
    buf[idx++] = crc;

    return idx;
}
