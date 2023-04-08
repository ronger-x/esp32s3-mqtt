# esp32s3-mqtt

## 通过 MQTT 协议控制 ESP32-S3 的 RGB 灯
- 数据结构
```json
{
  "rgb": {
    "state": 1,
    "r": 0,
    "g": 255,
    "b": 255
  }
}
```
- 字段说明
  - state: 0 关灯，1 开灯
  - r: 红色亮度
  - g: 绿色亮度
  - b: 蓝色亮度