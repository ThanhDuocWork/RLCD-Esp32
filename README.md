# RLCD-Esp32-Brookesia

Project skeleton theo huong product-oriented mo rong tot cho ESP32-S3 RLCD speaker.

## Target structure

```text
.
|-- main/
|-- core/
|   |-- system_manager/
|   |-- app_framework/
|   `-- event_hub/
|-- products/
|   `-- speaker/
|       |-- apps/
|       |-- resources/
|       `-- config/
|-- services/
|-- ports/
|-- drivers/
|-- bsp/
|   `-- waveshare_esp32_s3_rlcd_4_2/
|-- assets/
|-- docs/
`-- third_party/
```

## Event-driven flow

- `system_manager` khoi tao `event_hub`
- `services` publish cac event domain-level
- `products/speaker/apps/*` subscribe event de cap nhat UI/state
- `ports` va `drivers` giu vai tro adapter/phu tro, khong chua business flow

## Current bring-up status

- `bsp/waveshare_esp32_s3_rlcd_4_2`: board inventory + display config placeholder
- `drivers/bus/spi_master`: SPI host skeleton
- `drivers/display/st7305`: panel skeleton
- `ports/display_port`: panel adapter skeleton
- `services/display`: display orchestration + event post
- `core/event_hub`: default event loop wrapper

## Next implementation steps

1. Fill real GPIO map in `bsp/waveshare_esp32_s3_rlcd_4_2/src/board_config.c`
2. Add SPI device attach + init command table in `drivers/display/st7305`
3. Add `lvgl_port`
4. Add `i2c_master`, `i2s_master`, and the sensor/audio chip drivers
