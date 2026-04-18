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

The home app now looks for UI images first:

```text
/images/home.bmp
/images/settings.bmp
```

Press the board `KEY` button to switch between `home.bmp` and `settings.bmp`. If these files are missing, the product falls back to `/boot.bmp`, then finally to the display test loop.

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

Recommended project helper:

```powershell
python -m pip install pillow
python tools/prepare_tf_images.py C:\Users\thanh\Downloads\cat.jpg E:\cat.bmp
```

You can also convert a whole folder of PNG/JPG/BMP files into TF-ready BMP files:

```powershell
python tools/prepare_tf_images.py C:\Users\thanh\Downloads E:\images
```

The helper auto-detects supported input extensions:

```text
.png
.jpg
.jpeg
.bmp
```

Use `--mode cover` to fill the screen and crop edges, or `--mode contain` to keep the whole image with padding:

```powershell
python tools/prepare_tf_images.py C:\Users\thanh\Downloads E:\images --mode contain
```

ImageMagick example, only if ImageMagick is installed:

```powershell
magick input.jpg -resize 300x400^ -gravity center -extent 300x400 BMP3:boot.bmp
```

Then copy `boot.bmp` to the root of the TF card.

For the two-image UI list, create:

```powershell
python -c "from PIL import Image; img=Image.open('home.jpg').convert('RGB'); w,h=img.size; scale=max(300/w,400/h); img=img.resize((round(w*scale),round(h*scale))); left=(img.width-300)//2; top=(img.height-400)//2; img.crop((left,top,left+300,top+400)).save('home.bmp','BMP')"
python -c "from PIL import Image; img=Image.open('settings.jpg').convert('RGB'); w,h=img.size; scale=max(300/w,400/h); img=img.resize((round(w*scale),round(h*scale))); left=(img.width-300)//2; top=(img.height-400)//2; img.crop((left,top,left+300,top+400)).save('settings.bmp','BMP')"
```

Copy them to:

```text
TF_CARD/
└── images/
    ├── home.bmp
    └── settings.bmp
```

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
- `KEY`: GPIO18, active low

These pins are isolated in `bsp/waveshare_esp32_s3_rlcd_4_2/src/bsp_storage.c`, so the storage service does not know board wiring.

## Convert
python -c "from PIL import Image; img=Image.open('cat.jpg').convert('RGB'); w,h=img.size; scale=max(300/w,400/h); img=img.resize((round(w*scale),round(h*scale))); left=(img.width-300)//2; top=(img.height-400)//2; img.crop((left,top,left+300,top+400)).save('cat.bmp','BMP')"


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
