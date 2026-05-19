#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdint.h>

/* ── 帧常量 ─────────────────────────────────────────────── */
#define FRAME_HEADER1       0xAA
#define FRAME_HEADER2       0x55
#define FRAME_HDR_SIZE      4      /* 帧头1 + 帧头2 + 传感器ID + TypeLen */
#define FRAME_CRC_SIZE      1
#define FRAME_MIN_SIZE      6      /* 最小帧: 4字节头 + 0字节负载 + 1字节CRC */
#define FRAME_MAX_SIZE      21     /* 最大帧: 4字节头 + 15字节负载 + 1字节CRC */
#define TX_BUF_SIZE         FRAME_MAX_SIZE

/* ── 传感器 ID ──────────────────────────────────────────── */
typedef enum {
    SENSOR_TEMP_03     = 0x03,   /* 温度传感器 */
    SENSOR_HUMIDITY_07 = 0x07,   /* 湿度传感器 */
} SensorID;

/* ── 数据类型 (TypeLen[7:5]) ─────────────────────────────── */
typedef enum {
    DTYPE_UINT8  = 0,
    DTYPE_INT16  = 1,
    DTYPE_UINT16 = 2,
    DTYPE_INT32  = 3,
    DTYPE_FLOAT  = 4,
    DTYPE_UINT32 = 5,
    DTYPE_RAW    = 6,
    DTYPE_RESERVED = 7,
} DataType;

/* ── TypeLen 编码宏 ──────────────────────────────────────── */
/* type: DataType (0-7),  len: 数据长度 (0-15) */
#define MAKE_TYPELEN(type, len)  ((((uint8_t)(type) & 0x07) << 5) | ((uint8_t)(len) & 0x0F))

/* ── CRC8 参数 ───────────────────────────────────────────── */
#define CRC8_POLY   0x07
#define CRC8_INIT   0x00

/* ── 传感器帧结构体 ──────────────────────────────────────── */
typedef struct {
    uint8_t  sensor_id;              /* 传感器 ID */
    DataType dtype;                  /* 数据类型 */
    uint8_t  payload_len;            /* 负载长度 (0~15) */
    uint8_t  payload[15];            /* 负载数据 */
} SensorFrame;

/* ── 函数声明 ────────────────────────────────────────────── */
uint8_t crc8_calc(const uint8_t *data, uint8_t len);
uint8_t pack_frame(uint8_t *buf, const SensorFrame *frame);

#endif /* __PROTOCOL_H */
