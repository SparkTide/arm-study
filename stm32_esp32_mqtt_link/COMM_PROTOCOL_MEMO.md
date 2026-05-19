# 三端两段通信协议备忘录

> STM32F407ZGT → ESP32 → PC 采集器系统通信层设计

---

## 1. 架构总览

```
┌──────────────┐     UART (115200)      ┌──────────────┐     WiFi / MQTT      ┌──────────────┐
│  STM32F407   │ ──────────────────────→ │    ESP32     │ ←──────────────────→ │     PC       │
│  传感器采集   │   自定义帧协议          │  Broker +    │   TCP 1883          │  MQTT Client  │
│  帧封装发送   │   GPIO: TX→RX          │  Client      │   subscribe         │  数据展示     │
└──────────────┘                        └──────────────┘                     └──────────────┘
```

| 段位 | 物理层 | 协议 | 方向 |
|------|--------|------|------|
| STM32 → ESP32 | UART 115200-8N1 | 自定义二进制帧 | 单向 |
| ESP32 ↔ PC | WiFi 802.11n | MQTT 3.1.1 | 双向 |

---

## 2. 串口帧协议 (STM32 → ESP32)

### 2.1 字节序：小端 (Little Endian)

### 2.2 帧结构

```
┌────────┬────────┬────────┬──────────┬──────────┬────────┐
│ 帧头1  │ 帧头2  │ 传感器 │ 类型/长度 │  负载    │ 校验   │
│ 0xAA   │ 0x55   │  ID    │  TypeLen  │ Payload  │ CRC8   │
│ 1 byte │ 1 byte │ 1 byte │  1 byte   │ 0~15 B   │ 1 byte │
└────────┴────────┴────────┴───────────┴──────────┴────────┘
                     固定最小长度: 6 bytes, 最大: 21 bytes
```

### 2.3 TypeLen 字段编码

```
Bit [7:5] — 数据类型:
  000 (0) = uint8   (1 byte)
  001 (1) = int16   (2 bytes)
  010 (2) = uint16  (2 bytes)
  011 (3) = int32   (4 bytes)
  100 (4) = float   (4 bytes)
  101 (5) = uint32  (4 bytes)
  110 (6) = raw     (原始字节)
  111 (7) = reserved

Bit [3:0] — 数据长度 (字节数):
  对 uint8:   固定 1
  对 int16/uint16: 固定 2
  对 int32/float/uint32: 固定 4
  对 raw: 可变，0~15
```

### 2.4 CRC8 校验

- 多项式: `0x07` (x^8 + x^2 + x + 1)
- 初值: `0x00`
- 校验范围: 帧头1 (含) 到 Payload 末 (含)，即除 CRC8 自身外的所有字节

### 2.5 帧示例

```
发送温度传感器 (ID=0x03, 值=23.5°C):
  AA 55 03  84  00 00 BC 41  XX
  帧头     ID  float+4B  float(23.5)  CRC8

发送湿度传感器 (ID=0x07, 值=68%):
  AA 55 07  01  44  XX
  帧头     ID  uint8+1B  68  CRC8
```

---

## 3. 硬件连接 (STM32 ↔ ESP32)

```
STM32F407ZGT              ESP32 DevKit
  PA9  (TX)  ──────────── GPIO16 (RX2)
  GND        ──────────── GND

波特率: 115200
数据位: 8, 停止位: 1, 无校验, 无硬件流控
```

> ESP32 的 USB 口单独接 PC 用于下载和日志查看 (Serial0)。GPIO 串口 (Serial2) 用于接收 STM32 数据。

---

## 4. MQTT 协议 (ESP32 ↔ PC)

### 4.1 连接参数

| 参数 | 值 |
|------|-----|
| Broker 地址 | ESP32 WiFi IP (演示时查串口日志) |
| 端口 | 1883 |
| Client ID | `pc-monitor-{random}` |
| Keep Alive | 60s |

### 4.2 Topic 规范

```
collector/{device_id}/{sensor_id}/{field}

示例:
  collector/esp32-01/temp-03/value      ← 温度值
  collector/esp32-01/humidity-07/value  ← 湿度值
  collector/esp32-01/status             ← ESP32 心跳/状态
```

### 4.3 消息格式 (JSON)

```json
{
  "device_id": "esp32-01",
  "sensor_id": "temp-03",
  "sensor_type": "ds18b20",
  "value": 23.5,
  "unit": "celsius",
  "timestamp": 1715692800123
}
```

---

## 5. 决策记录

| # | 决策 | 选择 | 原因 |
|---|------|------|------|
| 1 | Broker 位置 | ESP32 | 期末演示，设备数 ≤2，sMQTTBroker 足够 |
| 2 | 帧校验 | CRC8 | 比异或和更可靠，计算量小 |
| 3 | 时间戳 | ESP32 打时间戳 | 避免 STM32 增加 RTC 复杂度 |
| 4 | 串口速率 | 115200 | 平衡吞吐和稳定性 |
| 5 | 硬件流控 | 不用 | 帧间隔 + ACK 替代，省两根线 |
| 6 | WiFi 凭据 | 硬编码 | 期末演示简化 |
| 7 | 数据丢失 | 可接受 | 演示场景，不加 ACK 重传 |
| 8 | ESP32 离线数据 | 丢弃 | 不缓存，保持简单 |
| 9 | ESP32 调试 | USB Serial0 | GPIO Serial2 留给 STM32 |

---

## 6. 目录结构建议

```
Project/
├── stm32-sensor/              # Keil MDK 工程
│   ├── Core/
│   │   └── Src/
│   │       ├── main.c
│   │       ├── uart_tx.c      # UART DMA 发送
│   │       └── protocol.c     # 帧封装
│   └── ...
├── esp32-broker/              # PlatformIO 工程
│   ├── src/
│   │   ├── main.cpp
│   │   ├── uart_rx.cpp       # UART 中断接收 + 帧解析
│   │   ├── mqtt_broker.cpp   # sMQTTBroker 初始化
│   │   └── wifi_manager.cpp  # WiFi 连接
│   └── platformio.ini
├── pc-client/                 # Python
│   ├── mqtt_subscriber.py
│   └── requirements.txt
└── COMM_PROTOCOL_MEMO.md
```
