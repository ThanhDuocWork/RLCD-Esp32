# Board Porting Notes

Confirmed facts:
- Board: Waveshare `ESP32-S3-RLCD-4.2`
- Panel: `ST7305`
- Resolution: `300 x 400`
- Type: reflective RLCD

Still required:
- SPI GPIO mapping
- Optional power gate GPIO
- I2C mapping for SHTC3 and PCF85063
- I2S mapping for ES7210 and ES8311
