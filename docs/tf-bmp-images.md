# TF Card BMP Images

The runtime image path is intentionally simple first:

```text
TF card FAT32
  -> /sdcard/boot.bmp
  -> image_service
  -> display_service
  -> display_port
  -> ST7305 driver
```

## Supported File

Put this file on a FAT32 microSD/TF card:

```text
/boot.bmp
```

The first implementation supports:

- Uncompressed BMP.
- `300x400` pixels, matching the current RLCD panel config.
- 8-bit indexed BMP, 24-bit BMP, or 32-bit BMP.
- Runtime threshold and Floyd-Steinberg dithering.

If `/sdcard/boot.bmp` is missing or invalid, the product falls back to the existing display test loop.

## Convert An Image To BMP

Use any image tool on PC to resize/crop first:

```text
300 x 400
BMP
24-bit
No compression
```

ImageMagick example:

```powershell
magick input.jpg -resize 300x400^ -gravity center -extent 300x400 BMP3:boot.bmp
```

Then copy `boot.bmp` to the root of the TF card.

## Runtime Monochrome Config

The home app currently uses:

```c
const image_service_mono_config_t boot_image_config = {
    .threshold = 128,
    .dither = true,
    .invert = false,
};
```

Use `.dither = true` for photos or gradients. Use `.dither = false` for clean icon/text assets.

If black and white appear reversed on the panel, change `.invert = true`.

## TF Card Pins

The BSP storage config uses SDMMC 1-bit mode:

- `CLK`: GPIO38
- `CMD`: GPIO21
- `D0`: GPIO39
- Mount point: `/sdcard`

These pins are isolated in `bsp/waveshare_esp32_s3_rlcd_4_2/src/bsp_storage.c`, so the storage service does not know board wiring.

## Convert
magick 1.jpg -resize 300x400^ -gravity center -extent 300x400 BMP3:boot.bmp 

## format TF
diskpart
list disk
select disk 2
clean
convert mbr
create partition primary
list volume
select volume X
format fs=fat32 unit=32768 quick
assign
exit